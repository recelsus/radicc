#include "app/common.h"
#include "app/output_path.h"

#include <array>
#include <cassert>
#include <string>

namespace {

void test_decode_xml_entities() {
  assert(radicc::decode_xml_entities("モーニング娘。&apos;26") == "モーニング娘。'26");
  assert(radicc::decode_xml_entities("&quot;A&amp;B&quot;") == "\"A&B\"");
  assert(radicc::decode_xml_entities("&#39;&#x27;") == "''");
}

void test_output_path_sanitizes_generated_names() {
  const std::array<std::string, 3> datetime = {"2026", "0710", "210000"};
  const auto paths = radicc::resolve_output_paths(
      "/recordings/",
      "",
      "",
      "A/B:C*?\"<>|.",
      "Dir/Name:",
      datetime,
      0);

  assert(paths.filename == "ABC-20260710.m4a");
  assert(paths.dir_name == "DirName");
  assert(paths.output_path == "/recordings/DirName/ABC-20260710.m4a");
}

void test_output_path_keeps_apostrophe() {
  const std::array<std::string, 3> datetime = {"2026", "0710", "210000"};
  const auto paths = radicc::resolve_output_paths(
      "/recordings/",
      "",
      "",
      "モーニング娘。'26井上春華のはるさんち",
      "井上春華のはるさんち",
      datetime,
      0);

  assert(paths.filename == "モーニング娘。'26井上春華のはるさんち-20260710.m4a");
}

void test_output_path_date_offset() {
  const std::array<std::string, 3> datetime = {"2026", "0711", "003000"};
  const auto paths = radicc::resolve_output_paths(
      "/recordings/",
      "",
      "",
      "宮本佳林の雑談ラジオ",
      "",
      datetime,
      1);

  assert(paths.filename == "宮本佳林の雑談ラジオ-20260710.m4a");
}

}  // namespace

int main() {
  test_decode_xml_entities();
  test_output_path_sanitizes_generated_names();
  test_output_path_keeps_apostrophe();
  test_output_path_date_offset();
  return 0;
}
