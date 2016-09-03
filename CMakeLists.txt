project(colord-kde)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.3.0")

find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

set(COLORD_KDE_VERSION "0.5.0")
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

include(FindPkgConfig)
include(ECMPackageConfigHelpers)
include(ECMOptionalAddSubdirectory)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    DBus
    Widgets
    X11Extras
)

find_package(X11)
find_package(XCB COMPONENTS XCB RANDR)

find_package(KF5 REQUIRED Config ConfigWidgets CoreAddons DBusAddons IconThemes I18n KCMUtils KIO
             Notifications Plasma WidgetsAddons WindowSystem ItemViews)

add_definitions(-DTRANSLATION_DOMAIN=\"colord-kde\")

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

message(STATUS "X randr is required, found: " ${XRANDR_1_3_FOUND})

pkg_check_modules(LCMS2 REQUIRED lcms2)

include_directories(${CMAKE_SOURCE_DIR}
    ${X11_Xrandr_INCLUDE_PATH}
    ${X11_X11_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(colord-kded)
add_subdirectory(colord-kcm)
add_subdirectory(icc-importer)
#add_subdirectory(dataengine)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
