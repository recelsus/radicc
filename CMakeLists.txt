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
  add_library(radicc_ffmpeg INTERFACE)

  function(radicc_append_existing_dirs out_var)
    set(_existing_dirs "")
    foreach(_dir IN LISTS ARGN)
      if (EXISTS "${_dir}")
        list(APPEND _existing_dirs "${_dir}")
      endif()
    endforeach()
    set(${out_var} "${_existing_dirs}" PARENT_SCOPE)
  endfunction()

  set(_radicc_ffmpeg_found OFF)
  set(_radicc_pkgconfig_hint OFF)

  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(AVFORMAT QUIET libavformat)
    pkg_check_modules(AVCODEC QUIET libavcodec)
    pkg_check_modules(AVUTIL QUIET libavutil)

    radicc_append_existing_dirs(
      _radicc_ffmpeg_include_dirs
      ${AVFORMAT_INCLUDE_DIRS}
      ${AVCODEC_INCLUDE_DIRS}
      ${AVUTIL_INCLUDE_DIRS}
    )

    if (AVFORMAT_FOUND AND AVCODEC_FOUND AND AVUTIL_FOUND)
      if (_radicc_ffmpeg_include_dirs)
        target_include_directories(radicc_ffmpeg INTERFACE ${_radicc_ffmpeg_include_dirs})
        target_link_directories(radicc_ffmpeg INTERFACE
          ${AVFORMAT_LIBRARY_DIRS}
          ${AVCODEC_LIBRARY_DIRS}
          ${AVUTIL_LIBRARY_DIRS}
        )
        target_link_libraries(radicc_ffmpeg INTERFACE
          ${AVFORMAT_LINK_LIBRARIES}
          ${AVCODEC_LINK_LIBRARIES}
          ${AVUTIL_LINK_LIBRARIES}
        )
        set(_radicc_ffmpeg_found ON)
      else()
        set(_radicc_pkgconfig_hint ON)
      endif()
    endif()
  endif()

  if (NOT _radicc_ffmpeg_found)
    if (APPLE)
      set(_radicc_ffmpeg_roots /opt/homebrew /usr/local /usr)
    else()
      set(_radicc_ffmpeg_roots /usr /usr/local)
    endif()

    find_path(RADICC_FFMPEG_INCLUDE_DIR
      NAMES libavformat/avformat.h
      HINTS ${_radicc_ffmpeg_roots}
      PATH_SUFFIXES include
    )
    find_library(RADICC_AVFORMAT_LIBRARY
      NAMES avformat
      HINTS ${_radicc_ffmpeg_roots}
      PATH_SUFFIXES lib
    )
    find_library(RADICC_AVCODEC_LIBRARY
      NAMES avcodec
      HINTS ${_radicc_ffmpeg_roots}
      PATH_SUFFIXES lib
    )
    find_library(RADICC_AVUTIL_LIBRARY
      NAMES avutil
      HINTS ${_radicc_ffmpeg_roots}
      PATH_SUFFIXES lib
    )

    if (RADICC_FFMPEG_INCLUDE_DIR AND RADICC_AVFORMAT_LIBRARY AND RADICC_AVCODEC_LIBRARY AND RADICC_AVUTIL_LIBRARY)
      target_include_directories(radicc_ffmpeg INTERFACE ${RADICC_FFMPEG_INCLUDE_DIR})
      target_link_libraries(radicc_ffmpeg INTERFACE
        ${RADICC_AVFORMAT_LIBRARY}
        ${RADICC_AVCODEC_LIBRARY}
        ${RADICC_AVUTIL_LIBRARY}
      )
      set(_radicc_ffmpeg_found ON)
    endif()
  endif()

  if (NOT _radicc_ffmpeg_found)
    if (APPLE AND _radicc_pkgconfig_hint)
      message(FATAL_ERROR
        "FFmpeg headers/libs were reported by pkg-config, but the include paths do not exist. "
        "This usually means Homebrew upgraded or removed an old ffmpeg keg. "
        "Try reinstalling ffmpeg/pkg-config or fixing PKG_CONFIG_PATH, then re-run CMake."
      )
    endif()
    message(FATAL_ERROR
      "libavformat/libavcodec/libavutil not found. Install FFmpeg dev packages or set "
      "-DRADICC_USE_LIBAV=OFF explicitly (not recommended)."
    )
  endif()

  message(STATUS "libav* found; enabling library-based recorder (default)")
  target_compile_definitions(radicc_recording PRIVATE USE_LIBAV)
  target_link_libraries(radicc_recording PRIVATE radicc_ffmpeg)
else()
  message(FATAL_ERROR "CLI recorder has been deprecated. Build with RADICC_USE_LIBAV=ON (default).")
endif()
