cmake_minimum_required(VERSION 3.11)

project(tuisic LANGUAGES C CXX VERSION 1.0.0)

option(WITH_MPRIS "Enable MPRIS support (via sdbus‑c++)" ON)
option(WITH_CAVA  "Build CAVA visualiser"               OFF)

add_executable(tuisic src/core/main.cpp)

# git submodules
find_package(Git REQUIRED)
execute_process(
  COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# ─── Platform Definitions ──────────────────────────────────────────────────────
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_compile_definitions(PLATFORM_LINUX)
  message("Platform: Linux")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  add_compile_definitions(PLATFORM_MACOS)
  message("Platform: MacOS")
elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  add_compile_definitions(PLATFORM_FREEBSD)
  message("Platform: FreeBSD")
elseif (WIN32)
  add_compile_definitions(PLATFORM_WINDOWS)
  message("Platform: Windows")
endif()

# ─── MPRIS ──────────────────────────────────────────────────────────────────────
if (WITH_MPRIS AND UNIX AND NOT APPLE AND NOT WIN32)
  find_package(sdbus-c++ REQUIRED)
  target_link_libraries(tuisic PRIVATE SDBusCpp::sdbus-c++)
  target_compile_definitions(tuisic PRIVATE -DWITH_MPRIS)
endif()

# ─── FTXUI ─────────────────────────────────────────────────────────────────────
find_package(ftxui CONFIG)
if (NOT ftxui_FOUND)
  include(FetchContent)
  set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
  FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG main)
  FetchContent_MakeAvailable(ftxui)
endif()

# ─── Optional CAVA ‑ currently OFF by default ─────────────────────────────────
if (WITH_CAVA)
  include(ExternalProject)
  ExternalProject_Add(cava_external
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cava
    CMAKE_ARGS
      -DCMAKE_C_STANDARD=11
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    INSTALL_COMMAND "")

  add_library(cavacore STATIC IMPORTED)
  add_dependencies(cavacore cava_external)
  set_target_properties(cavacore PROPERTIES
    IMPORTED_LOCATION
      ${CMAKE_CURRENT_BINARY_DIR}/cava-prefix/src/cava-build/libcavacore.a)

  find_package(FFTW3 REQUIRED)
  target_include_directories(cavacore INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/cava
    ${FFTW3_INCLUDE_DIRS})
  target_link_libraries(cavacore INTERFACE ${FFTW3_LIBRARIES} m)
endif()

# ─── Core dependencies ─────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)
find_package(CURL REQUIRED)
find_package(fmt REQUIRED)

target_include_directories(tuisic PRIVATE ${MPV_INCLUDE_DIRS})
target_link_directories(tuisic PRIVATE ${MPV_LIBRARY_DIRS})
target_link_libraries(tuisic
  PRIVATE ${MPV_LIBRARIES}
  PRIVATE fmt::fmt
  PRIVATE CURL::libcurl
  PRIVATE ftxui::screen ftxui::dom ftxui::component
)

if (WITH_CAVA)
  target_link_libraries(tuisic PRIVATE cavacore)
endif()

# ─── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS tuisic RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# install desktop file
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/desktop/tuisic.desktop
          DESTINATION ${CMAKE_INSTALL_DATADIR}/applications OPTIONAL)
endif()

# ─── Packaging ─────────────────────────────────────────────────────────────────
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_CONTACT "Dark-Kernel <sumitpatel24389@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A TUI Music Streaming Application")
set(CPACK_PACKAGE_DESCRIPTION "First of its kind, A simple TUI online music streaming application written in c++ with easy vim motions. ")

include(CPack)

