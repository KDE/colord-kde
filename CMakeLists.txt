cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(colord-kde)

set(QT_MIN_VERSION "5.3.0")

find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

set(COLORD_KDE_VERSION "0.5.0")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(FindPkgConfig)
include(ECMOptionalAddSubdirectory)
include(FeatureSummary)
include(ECMSetupVersion)

ecm_setup_version("${COLORD_KDE_VERSION}"
    VARIABLE_PREFIX "COLORD_KDE"
    VERSION_HEADER "version.h"
)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    DBus
    Widgets
)

find_package(X11)
set_package_properties(X11 PROPERTIES
    TYPE OPTIONAL
    PURPOSE "For colord integration on X11"
)

if (X11_FOUND)
    find_package(XCB REQUIRED COMPONENTS XCB RANDR)
    find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS X11Extras)
endif()

find_package(KF5 REQUIRED ConfigWidgets CoreAddons DBusAddons I18n KCMUtils
             WidgetsAddons WindowSystem)

add_definitions(-DTRANSLATION_DOMAIN=\"colord-kde\")

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

pkg_check_modules(LCMS2 REQUIRED lcms2 IMPORTED_TARGET)

include_directories(${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

if (X11_FOUND)
    add_subdirectory(colord-kded)
endif()
add_subdirectory(colord-kcm)
add_subdirectory(icc-importer)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
