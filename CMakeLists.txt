cmake_minimum_required(VERSION 3.10)
project(Mocha VERSION 1.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(X11 REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11 xft xext xfixes xrender xinerama)
pkg_check_modules(CAIRO REQUIRED cairo)

# Check for X extensions
pkg_check_modules(XFIXES REQUIRED xfixes)
pkg_check_modules(XEXT REQUIRED xext)
pkg_check_modules(XRENDER REQUIRED xrender)

# Include directories
include_directories(
    include
    ${X11_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
)

# Source files
set(SOURCE_FILES
    src/main.c
    src/event/event.c
    src/features/launcher.c
    src/ui/toast.c
    src/util/client.c
    src/util/config.c
    src/util/mocha_util.c
    src/mocha_launcher.c
    src/util/app.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")

# Create executable
add_executable(mocha-shell ${SOURCE_FILES})

target_include_directories(mocha-shell
    PRIVATE
    ${X11_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    include
)

# Link libraries
target_link_libraries(mocha-shell
    PRIVATE
    ${X11_LIBRARIES}
    ${CAIRO_LIBRARIES}
    m
)

install(TARGETS mocha-shell DESTINATION bin)
install(FILES mocha.desktop DESTINATION share/applications)
install(FILES config/config.mconf config/features.mconf config/keybinds.mconf config/theme.mconf
        DESTINATION share/mocha)
install(CODE "
    execute_process(
        COMMAND mkdir -p \$ENV{HOME}/.config/mocha
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/config/*.mconf \$ENV{HOME}/.config/mocha/
    )
")

