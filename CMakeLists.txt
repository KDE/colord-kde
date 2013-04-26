project(colord-kde)

set(COLORD_KDE_VERSION "0.3.0")
configure_file(config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h )

find_package(KDE4 REQUIRED)

include(FindPkgConfig)
include(KDE4Defaults)
include(ConfigureChecks.cmake)

message(STATUS "X randr is required, found: " ${XRANDR_1_3_FOUND})

pkg_check_modules(LCMS2 REQUIRED lcms2)

add_definitions(${QT_DEFINITIONS} ${KDE_DEFINITIONS})

include_directories(${CMAKE_SOURCE_DIR}
    ${X11_Xrandr_INCLUDE_PATH}
    ${X11_X11_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${KDE4_INCLUDES}
)

add_subdirectory(colord-kded)
add_subdirectory(colord-kcm)
add_subdirectory(icc-importer)
#add_subdirectory(dataengine)
