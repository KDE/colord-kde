project(colord-kde)

cmake_minimum_required(VERSION 3.16)
set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.89")

set(PROJECT_VERSION "5.24.80")
set(PROJECT_VERSION_MAJOR 5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMMarkNonGuiExecutable)
include(CMakePackageConfigHelpers)
include(WriteBasicConfigVersionFile)
include(CheckIncludeFiles)
include(FeatureSummary)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)
include(ECMQueryQmake)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMConfiguredInstall)
include(ECMGenerateDBusServiceFile)
include(ECMQMLModules)
include(ECMGenerateExportHeader)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    DBus
    Widgets
)
ecm_setup_version("${PROJECT_VERSION}"
    VARIABLE_PREFIX "COLORD_KDE"
    VERSION_HEADER "version.h"
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
             WidgetsAddons WindowSystem Declarative)

add_definitions(-DTRANSLATION_DOMAIN=\"colord-kde\")

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

pkg_check_modules(LCMS2 REQUIRED lcms2)

include_directories(${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

if (X11_FOUND)
    add_subdirectory(colord-kded)
endif()
#add_subdirectory(colord-kcm)
add_subdirectory(icc-importer)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
