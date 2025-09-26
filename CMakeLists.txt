cmake_minimum_required(VERSION 3.22)
project(radicc VERSION 1.0 LANGUAGES CXX)

# Export compile_commands.json for LSP/clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(radicc
  src/main.cpp
  src/cli/arguments.cpp
  src/core/url_parser.cpp
  src/core/radiko_recorder.cpp
  src/core/radiko_programs.cpp
  src/core/toml_parser.cpp
  src/utils/date.cpp
  src/utils/env_loader.cpp
)

target_include_directories(radicc PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
)

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

# Create/refresh a top-level compile_commands.json symlink (or copy on Windows)
if (EXISTS ${CMAKE_BINARY_DIR}/compile_commands.json)
  if (WIN32)
    add_custom_target(link_compile_commands ALL
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${CMAKE_BINARY_DIR}/compile_commands.json
              ${CMAKE_SOURCE_DIR}/compile_commands.json
      COMMENT "Copy compile_commands.json to project root for clangd")
  else()
    add_custom_target(link_compile_commands ALL
      COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/compile_commands.json
      COMMAND ${CMAKE_COMMAND} -E create_symlink
              ${CMAKE_BINARY_DIR}/compile_commands.json
              ${CMAKE_SOURCE_DIR}/compile_commands.json
      COMMENT "Symlink compile_commands.json to project root for clangd")
  endif()
endif()

# Install rules
include(GNUInstallDirs)
install(TARGETS radicc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/radicc.toml.example config/env.example DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/radicc)
add_executable(radicc_fetch
  src/tools/fetch.cpp
)
target_include_directories(radicc_fetch PRIVATE ${CMAKE_SOURCE_DIR})
option(RADICC_USE_LIBAV "Use FFmpeg libav* libraries (default ON)" ON)

if (RADICC_USE_LIBAV)
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(AVFORMAT QUIET libavformat)
    pkg_check_modules(AVCODEC QUIET libavcodec)
    pkg_check_modules(AVUTIL QUIET libavutil)
  endif()
  if (NOT (AVFORMAT_FOUND AND AVCODEC_FOUND AND AVUTIL_FOUND))
    message(FATAL_ERROR "libavformat/libavcodec/libavutil not found. Install FFmpeg dev packages or set -DRADICC_USE_LIBAV=OFF explicitly (not recommended).")
  endif()
  message(STATUS "libav* found; enabling library-based recorder (default)")
  target_compile_definitions(radicc PRIVATE USE_LIBAV)
  target_include_directories(radicc PRIVATE ${AVFORMAT_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS})
  target_link_libraries(radicc PRIVATE ${AVFORMAT_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVUTIL_LIBRARIES})
else()
  message(FATAL_ERROR "CLI recorder has been deprecated. Build with RADICC_USE_LIBAV=ON (default).")
endif()
