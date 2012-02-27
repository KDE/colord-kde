project(colord-kde)

configure_file (config-randr.h.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/config-randr.h )

find_package(KDE4 REQUIRED)

include(FindPkgConfig)
include(KDE4Defaults)
include(ConfigureChecks.cmake)

message(STATUS "X randr is required, found: " ${XRANDR_1_3_FOUND})

add_definitions(${QT_DEFINITIONS} ${KDE_DEFINITIONS})

include_directories(${CMAKE_SOURCE_DIR}
    ${X11_Xrandr_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${KDE4_INCLUDES}
)

add_subdirectory(colord-kded)
# add_subdirectory(colord-kcm)
