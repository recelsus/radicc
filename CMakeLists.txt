cmake_minimum_required(VERSION 3.22)
project(radicc VERSION 1.0 LANGUAGES CXX)

# Export compile_commands.json for LSP/clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(radicc_utils
  src/app/common.cpp
  src/cli/arguments.cpp
  src/core/radiko_http.cpp
  src/utils/base64.cpp
  src/utils/date.cpp
  src/utils/env_loader.cpp
)
target_include_directories(radicc_utils PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
)

add_library(radicc_radiko
  src/app/list_command.cpp
  src/app/output_path.cpp
  src/app/record_resolver.cpp
  src/core/radiko_auth.cpp
  src/core/radiko_stream.cpp
  src/core/url_parser.cpp
  src/core/radiko_programs_date.cpp
  src/core/radiko_programs.cpp
  src/core/radiko_programs_xml.cpp
  src/core/toml_parser.cpp
)
target_include_directories(radicc_radiko PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(radicc_radiko PUBLIC radicc_utils)

add_library(radicc_recording
  src/service/record_service.cpp
  src/core/radiko_recorder.cpp
)
target_include_directories(radicc_recording PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(radicc_recording PUBLIC radicc_radiko)

add_library(radicc_cli
  src/app/record_command.cpp
)
target_include_directories(radicc_cli PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(radicc_cli PUBLIC radicc_recording)

add_executable(radicc
  src/main.cpp
)
target_include_directories(radicc PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(radicc PRIVATE radicc_cli)

add_executable(radicc-server
  src/server/main.cpp
)
target_include_directories(radicc-server PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(radicc-server PRIVATE radicc_recording)

# Build metadata (similar to karing)
execute_process(
  COMMAND git -C ${CMAKE_SOURCE_DIR} describe --tags --always --dirty
  OUTPUT_VARIABLE RADICC_GIT_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(NOT RADICC_GIT_DESCRIBE)
  execute_process(
    COMMAND git -C ${CMAKE_SOURCE_DIR} rev-parse --short=12 HEAD
    OUTPUT_VARIABLE RADICC_GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
endif()
if(NOT RADICC_GIT_DESCRIBE)
  set(RADICC_GIT_DESCRIBE "unknown")
endif()
string(TIMESTAMP RADICC_BUILD_TIME_UTC "%Y-%m-%dT%H:%M:%SZ" UTC)
target_compile_definitions(radicc PRIVATE
  RADICC_GIT_REV=\"${RADICC_GIT_DESCRIBE}\"
  RADICC_BUILD_TIME=\"${RADICC_BUILD_TIME_UTC}\"
  RADICC_CXX_STANDARD=${CMAKE_CXX_STANDARD}
  RADICC_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\"
)


# Install rules
include(GNUInstallDirs)
install(TARGETS radicc radicc-server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/radicc.toml.example config/env.example DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/radicc)
add_executable(radicc_fetch
  src/tools/fetch.cpp
)
target_include_directories(radicc_fetch PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(radicc_fetch PRIVATE radicc_utils)
option(RADICC_USE_LIBAV "Use FFmpeg libav* libraries (default ON)" ON)

if (RADICC_USE_LIBAV)
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(AVFORMAT QUIET IMPORTED_TARGET libavformat)
    pkg_check_modules(AVCODEC QUIET IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVUTIL QUIET IMPORTED_TARGET libavutil)
  endif()
  if (NOT (TARGET PkgConfig::AVFORMAT AND TARGET PkgConfig::AVCODEC AND TARGET PkgConfig::AVUTIL))
    message(FATAL_ERROR "libavformat/libavcodec/libavutil not found. Install FFmpeg dev packages or set -DRADICC_USE_LIBAV=OFF explicitly (not recommended).")
  endif()
  message(STATUS "libav* found; enabling library-based recorder (default)")
  target_compile_definitions(radicc_recording PRIVATE USE_LIBAV)
  target_link_libraries(radicc_recording PRIVATE PkgConfig::AVFORMAT PkgConfig::AVCODEC PkgConfig::AVUTIL)
else()
  message(FATAL_ERROR "CLI recorder has been deprecated. Build with RADICC_USE_LIBAV=ON (default).")
endif()
