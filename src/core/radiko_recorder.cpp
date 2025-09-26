#include "core/radiko_recorder.h"
#include "base64.hpp"
#ifdef USE_LIBAV
extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/dict.h>
}
#endif
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

namespace radicc {

static std::string generate_partial_key(const std::string& authkey, int keyoffset, int keylength) {
  std::string keySegment = authkey.substr(keyoffset, keylength);
  return base64::to_base64(keySegment);
}

bool login_to_radiko(const std::string& mail, const std::string& password, std::string& session_id) {
  if (mail.empty() || password.empty()) {
    std::cerr << "Warning: No Radiko credentials provided. Skipping login.\n";
    return false;
  }
  std::ostringstream curl_command;
  curl_command << "curl --silent --request POST "
               << "--data-urlencode \"mail=" << mail << "\" "
               << "--data-urlencode \"pass=" << password << "\" "
               << "\"https://radiko.jp/v4/api/member/login\"";
  FILE* pipe = popen(curl_command.str().c_str(), "r");
  if (!pipe) { std::cerr << "Error: Failed to execute login command.\n"; return false; }
  char buffer[128]; std::string result;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
  pclose(pipe);
  std::size_t pos = result.find("\"radiko_session\":\"");
  if (pos == std::string::npos) { std::cerr << "Login failed: Radiko session not found.\n"; return false; }
  session_id = result.substr(pos + 18, result.find("\"", pos + 18) - (pos + 18));
  return true;
}

bool authorize_radiko(std::string& authtoken, const std::string& session_id) {
  std::string auth1_command =
    "curl --silent "
     "--header \"X-Radiko-App: pc_html5\" "
     "--header \"X-Radiko-App-Version: 0.0.1\" "
     "--header \"X-Radiko-Device: pc\" "
     "--header \"X-Radiko-User: dummy_user\" "
     "--dump-header - "
     "--output /dev/null "
     "\"https://radiko.jp/v2/api/auth1\"";
  FILE* pipe = popen(auth1_command.c_str(), "r");
  if (!pipe) { std::cerr << "Error: Failed to execute auth1 command.\n"; return false; }
  char buffer[128]; std::string result;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
  pclose(pipe);
  size_t token_pos = result.find("x-radiko-authtoken: ");
  size_t offset_pos = result.find("x-radiko-keyoffset: ");
  size_t length_pos = result.find("x-radiko-keylength: ");
  if (token_pos == std::string::npos || offset_pos == std::string::npos || length_pos == std::string::npos) {
    std::cerr << "Authorization failed: Required fields not found.\n"; return false; }
  authtoken = result.substr(token_pos + 20, result.find("\n", token_pos) - (token_pos + 20));
  std::string keyOffsetStr = result.substr(offset_pos + 20, result.find("\n", offset_pos) - (offset_pos + 20));
  std::string keyLengthStr = result.substr(length_pos + 20, result.find("\n", length_pos) - (length_pos + 20));
  int keyoffset = std::stoi(keyOffsetStr); int keylength = std::stoi(keyLengthStr);
  std::string partialkey = generate_partial_key("bcd151073c03b352e1ef2fd66c32209da9ca0afa", keyoffset, keylength);
  std::string auth2_command =
     "curl --silent --header \"X-Radiko-Device: pc\" "
     "--header \"X-Radiko-User: dummy_user\" "
     "--header \"X-Radiko-AuthToken: " + authtoken + "\" "
     "--header \"X-Radiko-PartialKey: " + partialkey + "\" "
     "--output /dev/null \"https://radiko.jp/v2/api/auth2?radiko_session=" + session_id + "\"";
  int auth2_result = std::system(auth2_command.c_str());
  return auth2_result == 0;
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

bool record_radiko(const std::string& station_id, const std::string& fromtime,
                  const std::string& totime, const std::string& filename,
                  const std::string& authtoken,
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
  auto do_libav = [&]() -> bool {
    // Suppress libav info logs (e.g., segment opening spam)
    av_log_set_level(AV_LOG_ERROR);
    AVFormatContext* in_fmt = nullptr;   // HLS input
    AVFormatContext* out_fmt = nullptr;  // MP4/M4A output
    AVFormatContext* img_fmt = nullptr;  // optional image input
    AVBSFContext* bsf = nullptr;
    AVDictionary* opts = nullptr;
    int audio_stream = -1;
    int img_stream = -1;

    std::string url = std::string("https://radiko.jp/v2/api/ts/playlist.m3u8?station_id=") + station_id + "&ft=" + fromtime + "&to=" + totime;
    std::string headers = std::string("X-Radiko-Authtoken: ") + authtoken + "\r\n";
    av_dict_set(&opts, "headers", headers.c_str(), 0);
    if (avformat_open_input(&in_fmt, url.c_str(), nullptr, &opts) < 0) { av_dict_free(&opts); return false; }
    av_dict_free(&opts);
    if (avformat_find_stream_info(in_fmt, nullptr) < 0) { avformat_close_input(&in_fmt); return false; }
    for (unsigned i = 0; i < in_fmt->nb_streams; ++i) {
      if (in_fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { audio_stream = (int)i; break; }
    }
    if (audio_stream < 0) { avformat_close_input(&in_fmt); return false; }

    if (avformat_alloc_output_context2(&out_fmt, nullptr, "mp4", outputPath.c_str()) < 0) {
      avformat_close_input(&in_fmt); return false; }

    AVStream* in_a = in_fmt->streams[audio_stream];
    AVStream* out_a = avformat_new_stream(out_fmt, nullptr);
    if (!out_a) { avformat_close_input(&in_fmt); avformat_free_context(out_fmt); return false; }
    if (avcodec_parameters_copy(out_a->codecpar, in_a->codecpar) < 0) { avformat_close_input(&in_fmt); avformat_free_context(out_fmt); return false; }
    out_a->codecpar->codec_tag = 0; // let muxer decide

    // Optional: open image input and prepare attached_pic stream
    AVStream* out_img = nullptr;
    if (!image_url.empty()) {
      if (avformat_open_input(&img_fmt, image_url.c_str(), nullptr, nullptr) == 0) {
        if (avformat_find_stream_info(img_fmt, nullptr) >= 0) {
          for (unsigned i = 0; i < img_fmt->nb_streams; ++i) {
            if (img_fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { img_stream = (int)i; break; }
          }
          if (img_stream >= 0) {
            AVStream* in_v = img_fmt->streams[img_stream];
            out_img = avformat_new_stream(out_fmt, nullptr);
            if (out_img && avcodec_parameters_copy(out_img->codecpar, in_v->codecpar) >= 0) {
              out_img->codecpar->codec_tag = 0;
              out_img->disposition |= AV_DISPOSITION_ATTACHED_PIC;
            } else {
              out_img = nullptr;
            }
          }
        }
      }
      // If any of above steps failed, we silently skip image embedding
    }

    // aac_adtstoasc bitstream filter
    if (in_a->codecpar->codec_id == AV_CODEC_ID_AAC) {
      const AVBitStreamFilter* f = av_bsf_get_by_name("aac_adtstoasc");
      if (f && av_bsf_alloc(f, &bsf) == 0) {
        avcodec_parameters_copy(bsf->par_in, in_a->codecpar);
        bsf->time_base_in = in_a->time_base;
        if (av_bsf_init(bsf) < 0) { av_bsf_free(&bsf); bsf = nullptr; }
      }
    }

    // metadata
    if (!pfm.empty()) av_dict_set(&out_fmt->metadata, "artist", pfm.c_str(), 0);
    if (!album_title.empty()) av_dict_set(&out_fmt->metadata, "album", album_title.c_str(), 0);

    // open io
    if (!(out_fmt->oformat->flags & AVFMT_NOFILE)) {
      if (avio_open(&out_fmt->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
        if (bsf) av_bsf_free(&bsf);
        avformat_close_input(&in_fmt); avformat_free_context(out_fmt); return false; }
    }

    if (avformat_write_header(out_fmt, nullptr) < 0) {
      if (!(out_fmt->oformat->flags & AVFMT_NOFILE) && out_fmt->pb) avio_closep(&out_fmt->pb);
      if (bsf) av_bsf_free(&bsf);
      if (img_fmt) avformat_close_input(&img_fmt);
      avformat_close_input(&in_fmt); avformat_free_context(out_fmt); return false;
    }

    // If we have an image stream, write a single packet as attached_pic
    if (img_fmt && img_stream >= 0 && out_img) {
      AVPacket* ipkt = av_packet_alloc();
      if (ipkt) {
        // Read until we get a packet from the image stream (best-effort)
        while (av_read_frame(img_fmt, ipkt) >= 0) {
          if (ipkt->stream_index == img_stream) {
            ipkt->stream_index = out_img->index;
            // For attached_pic, timestamps are not critical; write once
            av_interleaved_write_frame(out_fmt, ipkt);
            av_packet_unref(ipkt);
            break;
          }
          av_packet_unref(ipkt);
        }
        av_packet_free(&ipkt);
      }
    }

    AVPacket* pkt = av_packet_alloc();
    AVPacket* filt = bsf ? av_packet_alloc() : nullptr;
    if (!pkt || (bsf && !filt)) {
      if (filt) av_packet_free(&filt);
      if (pkt) av_packet_free(&pkt);
      if (!(out_fmt->oformat->flags & AVFMT_NOFILE) && out_fmt->pb) avio_closep(&out_fmt->pb);
      if (bsf) av_bsf_free(&bsf);
      if (img_fmt) avformat_close_input(&img_fmt);
      avformat_close_input(&in_fmt); avformat_free_context(out_fmt);
      return false;
    }
    int rc = 0;
    while ((rc = av_read_frame(in_fmt, pkt)) >= 0) {
      if (pkt->stream_index != audio_stream) { av_packet_unref(pkt); continue; }
      if (bsf) {
        if (av_bsf_send_packet(bsf, pkt) == 0) {
          while (av_bsf_receive_packet(bsf, filt) == 0) {
            filt->stream_index = out_a->index;
            av_packet_rescale_ts(filt, in_a->time_base, out_a->time_base);
            av_interleaved_write_frame(out_fmt, filt);
            av_packet_unref(filt);
          }
        }
        av_packet_unref(pkt);
      } else {
        pkt->stream_index = out_a->index;
        av_packet_rescale_ts(pkt, in_a->time_base, out_a->time_base);
        av_interleaved_write_frame(out_fmt, pkt);
        av_packet_unref(pkt);
      }
    }

    av_write_trailer(out_fmt);
    if (!(out_fmt->oformat->flags & AVFMT_NOFILE) && out_fmt->pb) avio_closep(&out_fmt->pb);
    if (filt) av_packet_free(&filt);
    if (pkt) av_packet_free(&pkt);
    if (bsf) av_bsf_free(&bsf);
    if (img_fmt) avformat_close_input(&img_fmt);
    avformat_close_input(&in_fmt);
    avformat_free_context(out_fmt);
    return true;
  };
  if (do_libav()) return true;
#endif
  std::cerr << "Recording failed: libav remux path only (CLI disabled)\n";
  return false;
}

void logout_from_radiko(const std::string& radiko_session) {
  if (radiko_session.empty()) return;
  std::ostringstream curl_command;
  curl_command << "curl --silent --request POST "
               << "--data-urlencode \"radiko_session=" << radiko_session << "\" "
               << "\"https://radiko.jp/v4/api/member/logout\"";
  std::system(curl_command.str().c_str());
}

} // namespace radicc
