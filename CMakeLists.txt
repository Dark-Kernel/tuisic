cmake_minimum_required(VERSION 3.11)
 
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG main # Important: Specify a version or a commit hash here.
)
FetchContent_MakeAvailable(ftxui)
 
project(tuisic LANGUAGES CXX VERSION 1.0.0)
add_executable(tuisic src/main.cpp)

# Find CURL package
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(fmt REQUIRED)

# Use pkg-config to find MPV
pkg_check_modules(MPV REQUIRED mpv)

target_include_directories(tuisic 
    PRIVATE 
    ${MPV_INCLUDE_DIRS}
)

target_link_libraries(tuisic
    PRIVATE 
    ${MPV_LIBRARIES}
    PRIVATE fmt::fmt
    PRIVATE CURL::libcurl
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
)
