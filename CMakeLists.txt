cmake_minimum_required(VERSION 3.11)

project(tuisic LANGUAGES C CXX VERSION 1.0.0)
add_executable(tuisic src/main.cpp)

find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# include(FetchContent)
# set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
# FetchContent_Declare(ftxui
#   GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
#   GIT_TAG main # Important: Specify a version or a commit hash here.
# )
# FetchContent_MakeAvailable(ftxui)

# Try to find FTXUI installed on system
find_package(ftxui CONFIG)

if(NOT ftxui_FOUND)
    message(STATUS "FTXUI not found, fetching it...")
    include(FetchContent)
    set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
    FetchContent_Declare(ftxui
      GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
      GIT_TAG main
    )
    FetchContent_MakeAvailable(ftxui)
endif()

 

# Set installation directories
include(GNUInstallDirs)

# Configure CAVA build options
set(CAVA_BUILD_OPTIONS
    -DCMAKE_C_STANDARD=11
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

# Build CAVA as an external project
include(ExternalProject)
ExternalProject_Add(cava_external
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cava
    CMAKE_ARGS ${CAVA_BUILD_OPTIONS}
    INSTALL_COMMAND ""  # Skip installation of CAVA itself
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/cava-prefix/src/cava-build/libcavacore.a
)


# Cava core
add_library(cavacore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cava/cavacore.c
)



set_target_properties(cavacore PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/cava-prefix/src/cava-build/libcavacore.a
)

add_dependencies(cavacore cava_external)

# Link against FFTW3
find_package(FFTW3 REQUIRED)
target_include_directories(cavacore PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/cava
    ${FFTW3_INCLUDE_DIRS}
)
target_link_libraries(cavacore PUBLIC 
    ${FFTW3_LIBRARIES}
    m  # Math library
)

# Find CURL package
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(fmt REQUIRED)

# Use pkg-config to find MPV
pkg_check_modules(MPV REQUIRED mpv)

target_include_directories(tuisic 
    PRIVATE 
    ${MPV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/cava
)

target_link_libraries(tuisic
    PRIVATE 
    ${MPV_LIBRARIES}
    PRIVATE fmt::fmt
    PRIVATE CURL::libcurl
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
    PRIVATE cavacore
)

# Installation configuration
install(TARGETS tuisic
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT tuisic_runtime
)

# Additional files like configuration files, desktop entries, or man pages:
# install(FILES config/tuisic.conf
#     DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/tuisic
#     COMPONENT tuisic_config
# )

# Optional: Desktop entry installation
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/desktop/tuisic.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    OPTIONAL
)

# Set version for package creation
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_CONTACT "Dark-Kernel <sumitpatel24389@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A TUI Music Player")
set(CPACK_PACKAGE_DESCRIPTION "A terminal-based online music player.")

# Enable packaging
include(CPack)
