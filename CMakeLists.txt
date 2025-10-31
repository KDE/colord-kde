cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set (CMAKE_CXX_STANDARD 17)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "08")
set (RELEASE_SERVICE_VERSION_MICRO "3")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(colord-kde VERSION ${RELEASE_SERVICE_VERSION})

set(KF_MIN_VERSION "5.240.0")
set(REQUIRED_QT_VERSION "6.5.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
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

find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED COMPONENTS
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
endif()

find_package(KF6 ${KF_MIN_VERSION} REQUIRED CoreAddons DBusAddons I18n KCMUtils
             WidgetsAddons WindowSystem ItemModels)

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
