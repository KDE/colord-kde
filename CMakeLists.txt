cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set (CMAKE_CXX_STANDARD 17)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "24")
set (RELEASE_SERVICE_VERSION_MINOR "01")
set (RELEASE_SERVICE_VERSION_MICRO "85")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(colord-kde VERSION ${RELEASE_SERVICE_VERSION})

set(KF5_MIN_VERSION "5.91.0")
set(REQUIRED_QT_VERSION "5.15.2")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})


include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(FindPkgConfig)
include(ECMOptionalAddSubdirectory)
include(FeatureSummary)
include(ECMSetupVersion)
include(ECMFindQmlModule)
include(ECMPoQmTools)
include(KDEGitCommitHooks)
include(KDEClangFormat)
include(ECMDeprecationSettings)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

ecm_setup_version("${PROJECT_VERSION}"
    VARIABLE_PREFIX "COLORD_KDE"
    VERSION_HEADER "version.h"
)
ecm_find_qmlmodule(org.kde.kirigamiaddons.treeview 1.0)

find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    DBus
    Widgets
)

option(WITHOUT_X11 "Build without colord-kded for X11 integration (skips finding X11)" OFF)
if(NOT WITHOUT_X11)
    find_package(X11)
    set_package_properties(X11 PROPERTIES
        TYPE OPTIONAL
        PURPOSE "For colord integration on X11"
    )
    set(HAVE_X11 ${X11_FOUND})
endif()

if (HAVE_X11)
    find_package(XCB REQUIRED COMPONENTS XCB RANDR)
    if (QT_MAJOR_VERSION STREQUAL "5")
        find_package(Qt5X11Extras ${REQUIRED_QT_VERSION} NO_MODULE)
    elseif (QT_MAJOR_VERSION STREQUAL "6")
        find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED Gui) # qtx11extras_p.h
    endif()
endif()

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED ConfigWidgets CoreAddons DBusAddons I18n KCMUtils
             WidgetsAddons WindowSystem Package Declarative ItemModels)

add_definitions(-DTRANSLATION_DOMAIN=\"colord-kde\")

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

pkg_check_modules(LCMS2 REQUIRED lcms2 IMPORTED_TARGET)

include_directories(${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

ecm_set_disabled_deprecation_versions(
	KF 5.100.0)


if (HAVE_X11)
    add_subdirectory(colord-kded)
endif()
add_subdirectory(colord-kcm)
add_subdirectory(icc-importer)

ki18n_install(po)
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
