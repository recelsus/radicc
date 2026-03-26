#include "core/radiko_recorder.h"
#ifdef USE_LIBAV
extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}
#endif
#include <iostream>
#include <sys/stat.h>

namespace radicc {

static std::string av_error_to_string(int errnum) {
  char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
  return av_make_error_string(buf, sizeof(buf), errnum);
}

static int find_audio_stream_index(AVFormatContext* input) {
  for (unsigned i = 0; i < input->nb_streams; ++i) {
    if (input->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) return static_cast<int>(i);
  }
  return -1;
}

static bool mkdir_p(const std::string& path, mode_t mode = 0755) {
  struct stat st; if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) return true;
  size_t pos = path.find_last_of('/');
  if (pos != std::string::npos) {
    std::string parent = path.substr(0, pos);
    if (!parent.empty() && !mkdir_p(parent, mode)) return false;
  }
  if (mkdir(path.c_str(), mode) != 0) {
    if (errno != EEXIST) { std::cerr << "Error: Failed to create directory " << path << ": " << strerror(errno) << std::endl; return false; }
  }
  return true;
}

bool record_radiko(const RadikoStreamPlan& stream_plan, const std::string& filename,
                  const std::string& pfm, const std::string& album_title,
                  const std::string& dir_name, const std::string& outputDir,
                  const std::string& image_url) {
  // Build final output path and ensure parent directory exists
  std::string outputPath = dir_name.empty() ? (outputDir + filename)
                                            : (outputDir + dir_name + "/" + filename);
  {
    size_t pos = outputPath.find_last_of('/');
    if (pos != std::string::npos) {
      std::string parent = outputPath.substr(0, pos);
      if (!mkdir_p(parent, 0755)) {
        std::cerr << "Error: Failed to create directory " << parent << std::endl;
        return false;
      }
    }
  }

#ifdef USE_LIBAV
  // Library-based remux with optional attached_pic (cover image).
  auto do_libav = [&](const RadikoStreamSource& source) -> bool {
    // Suppress libav info logs (e.g., segment opening spam)
    av_log_set_level(AV_LOG_ERROR);
    AVFormatContext* out_fmt = nullptr;  // MP4/M4A output
    AVFormatContext* img_fmt = nullptr;  // optional image input
    AVBSFContext* bsf = nullptr;
    int img_stream = -1;
    AVStream* out_a = nullptr;
    int64_t next_audio_ts = 0;
    bool wrote_packets = false;

    if (source.chunks.empty()) {
      std::cerr << "libav: stream source is empty\n";
      return false;
    }

    auto cleanup = [&]() {
      if (!(out_fmt && (out_fmt->oformat->flags & AVFMT_NOFILE)) && out_fmt && out_fmt->pb) avio_closep(&out_fmt->pb);
      if (bsf) av_bsf_free(&bsf);
      if (img_fmt) avformat_close_input(&img_fmt);
      if (out_fmt) avformat_free_context(out_fmt);
    };

    for (size_t chunk_index = 0; chunk_index < source.chunks.size(); ++chunk_index) {
      AVFormatContext* in_fmt = nullptr;
      AVDictionary* opts = nullptr;
      av_dict_set(&opts, "headers", stream_plan.request_headers.c_str(), 0);
      av_dict_set(&opts, "http_seekable", "0", 0);
      av_dict_set(&opts, "seekable", "0", 0);
      int rc = avformat_open_input(&in_fmt, source.chunks[chunk_index].url.c_str(), nullptr, &opts);
      av_dict_free(&opts);
      if (rc < 0) {
        std::cerr << "libav: avformat_open_input failed on chunk " << chunk_index
                  << ": " << av_error_to_string(rc) << "\n";
        if (in_fmt) avformat_close_input(&in_fmt);
        cleanup();
        return false;
      }

      rc = avformat_find_stream_info(in_fmt, nullptr);
      if (rc < 0) {
        std::cerr << "libav: avformat_find_stream_info failed on chunk " << chunk_index
                  << ": " << av_error_to_string(rc) << "\n";
        avformat_close_input(&in_fmt);
        cleanup();
        return false;
      }

      const int audio_stream = find_audio_stream_index(in_fmt);
      if (audio_stream < 0) {
        std::cerr << "libav: audio stream not found on chunk " << chunk_index << "\n";
        avformat_close_input(&in_fmt);
        cleanup();
        return false;
      }

      AVStream* in_a = in_fmt->streams[audio_stream];
      if (!out_fmt) {
        rc = avformat_alloc_output_context2(&out_fmt, nullptr, "mp4", outputPath.c_str());
        if (rc < 0) {
          std::cerr << "libav: avformat_alloc_output_context2 failed: " << av_error_to_string(rc) << "\n";
          avformat_close_input(&in_fmt);
          cleanup();
          return false;
        }

        out_a = avformat_new_stream(out_fmt, nullptr);
        if (!out_a) {
          std::cerr << "libav: avformat_new_stream (audio) returned null\n";
          avformat_close_input(&in_fmt);
          cleanup();
          return false;
        }
        rc = avcodec_parameters_copy(out_a->codecpar, in_a->codecpar);
        if (rc < 0) {
          std::cerr << "libav: avcodec_parameters_copy failed: " << av_error_to_string(rc) << "\n";
          avformat_close_input(&in_fmt);
          cleanup();
          return false;
        }
        out_a->codecpar->codec_tag = 0;
        out_a->time_base = in_a->time_base;

        if (!image_url.empty()) {
          if ((rc = avformat_open_input(&img_fmt, image_url.c_str(), nullptr, nullptr)) == 0) {
            if (avformat_find_stream_info(img_fmt, nullptr) >= 0) {
              for (unsigned i = 0; i < img_fmt->nb_streams; ++i) {
                if (img_fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                  img_stream = static_cast<int>(i);
                  break;
                }
              }
              if (img_stream >= 0) {
                AVStream* out_img = avformat_new_stream(out_fmt, nullptr);
                AVStream* in_v = img_fmt->streams[img_stream];
                if (out_img && avcodec_parameters_copy(out_img->codecpar, in_v->codecpar) >= 0) {
                  out_img->codecpar->codec_tag = 0;
                  out_img->disposition |= AV_DISPOSITION_ATTACHED_PIC;
                }
              }
            }
          } else {
            std::cerr << "libav: avformat_open_input(image) failed: " << av_error_to_string(rc) << " (non-fatal)\n";
          }
        }
        if (in_a->codecpar->codec_id == AV_CODEC_ID_AAC) {
          const AVBitStreamFilter* f = av_bsf_get_by_name("aac_adtstoasc");
          if (f && av_bsf_alloc(f, &bsf) == 0) {
            avcodec_parameters_copy(bsf->par_in, in_a->codecpar);
            bsf->time_base_in = in_a->time_base;
            if (av_bsf_init(bsf) < 0) {
              av_bsf_free(&bsf);
              bsf = nullptr;
            }
          }
        }

        if (!pfm.empty()) av_dict_set(&out_fmt->metadata, "artist", pfm.c_str(), 0);
        if (!album_title.empty()) av_dict_set(&out_fmt->metadata, "album", album_title.c_str(), 0);

        if (!(out_fmt->oformat->flags & AVFMT_NOFILE)) {
          if (avio_open(&out_fmt->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
            avformat_close_input(&in_fmt);
            cleanup();
            return false;
          }
        }

        rc = avformat_write_header(out_fmt, nullptr);
        if (rc < 0) {
          std::cerr << "libav: avformat_write_header failed: " << av_error_to_string(rc) << "\n";
          avformat_close_input(&in_fmt);
          cleanup();
          return false;
        }

        if (img_fmt && img_stream >= 0 && out_fmt->nb_streams > 1) {
          AVPacket* ipkt = av_packet_alloc();
          if (ipkt) {
            while (av_read_frame(img_fmt, ipkt) >= 0) {
              if (ipkt->stream_index == img_stream) {
                ipkt->stream_index = 1;
                av_interleaved_write_frame(out_fmt, ipkt);
                av_packet_unref(ipkt);
                break;
              }
              av_packet_unref(ipkt);
            }
            av_packet_free(&ipkt);
          }
        }
      }

      AVPacket* pkt = av_packet_alloc();
      AVPacket* filt = bsf ? av_packet_alloc() : nullptr;
      if (!pkt || (bsf && !filt)) {
        if (filt) av_packet_free(&filt);
        if (pkt) av_packet_free(&pkt);
        avformat_close_input(&in_fmt);
        cleanup();
        return false;
      }

      auto write_packet = [&](AVPacket* packet) {
        packet->stream_index = out_a->index;
        int64_t duration = packet->duration > 0
            ? av_rescale_q(packet->duration, in_a->time_base, out_a->time_base)
            : 0;
        if (duration <= 0 && in_a->codecpar->frame_size > 0 && in_a->codecpar->sample_rate > 0) {
          duration = av_rescale_q(
              in_a->codecpar->frame_size,
              AVRational{1, in_a->codecpar->sample_rate},
              out_a->time_base);
        }
        if (duration <= 0) duration = 1;

        packet->pts = next_audio_ts;
        packet->dts = next_audio_ts;
        packet->duration = duration;
        const int rc_write = av_interleaved_write_frame(out_fmt, packet);
        if (rc_write == 0) {
          wrote_packets = true;
          next_audio_ts += duration;
        }
        return rc_write;
      };

      int read_rc = 0;
      while ((read_rc = av_read_frame(in_fmt, pkt)) >= 0) {
        if (pkt->stream_index != audio_stream) {
          av_packet_unref(pkt);
          continue;
        }
        if (bsf) {
          if (av_bsf_send_packet(bsf, pkt) == 0) {
            while (av_bsf_receive_packet(bsf, filt) == 0) {
              if (write_packet(filt) < 0) {
                av_packet_unref(filt);
                av_packet_unref(pkt);
                av_packet_free(&filt);
                av_packet_free(&pkt);
                avformat_close_input(&in_fmt);
                cleanup();
                return false;
              }
              av_packet_unref(filt);
            }
          }
          av_packet_unref(pkt);
        } else {
          if (write_packet(pkt) < 0) {
            av_packet_unref(pkt);
            if (filt) av_packet_free(&filt);
            av_packet_free(&pkt);
            avformat_close_input(&in_fmt);
            cleanup();
            return false;
          }
          av_packet_unref(pkt);
        }
      }

      if (read_rc < 0 && read_rc != AVERROR_EOF) {
        std::cerr << "libav: av_read_frame failed on chunk " << chunk_index
                  << ": " << av_error_to_string(read_rc) << "\n";
        if (filt) av_packet_free(&filt);
        av_packet_free(&pkt);
        avformat_close_input(&in_fmt);
        cleanup();
        return false;
      }
      if (filt) av_packet_free(&filt);
      av_packet_free(&pkt);
      avformat_close_input(&in_fmt);
    }

    if (!out_fmt || !wrote_packets) {
      cleanup();
      return false;
    }

    av_write_trailer(out_fmt);
    cleanup();
    return true;
  };
  for (const auto& source : stream_plan.sources) {
    if (do_libav(source)) return true;
  }
#endif
  std::cerr << "Recording failed: libav remux path only (CLI disabled)\n";
  return false;
}

} // namespace radicc
