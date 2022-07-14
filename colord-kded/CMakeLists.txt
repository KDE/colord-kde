include_directories(${CMAKE_SOURCE_DIR}/colord-kded
                    ${CMAKE_BUILD_DIR}
)

add_definitions(-DPROJECT_NAME=\"${PROJECT_NAME}\")

set(colord_kded_SRCS
    DmiUtils.cpp
    Edid.cpp
    Output.cpp
    ProfileUtils.cpp
    XEventHandler.cpp
    ProfilesWatcher.cpp
    ColorD.cpp
)

set(COLORD_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.xml)
set(COLORD_DEVICE_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Device.xml)
set(COLORD_PROFILE_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Profile.xml)

set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES CLASSNAME CdInterface)
qt5_add_dbus_interface(colord_kded_SRCS ${COLORD_INTERFACE_XML} CdInterface)

set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES CLASSNAME CdDeviceInterface)
qt5_add_dbus_interface(colord_kded_SRCS ${COLORD_DEVICE_INTERFACE_XML} CdDeviceInterface)

set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES CLASSNAME CdProfileInterface)
qt5_add_dbus_interface(colord_kded_SRCS ${COLORD_PROFILE_INTERFACE_XML} CdProfileInterface)

add_library(kded_colord MODULE ${colord_kded_SRCS})
target_link_libraries(kded_colord
    Qt5::Core
    Qt5::Widgets
    Qt5::DBus
    Qt5::X11Extras
    KF5::CoreAddons
    KF5::DBusAddons
    KF5::WindowSystem
    X11::X11
    X11::Xrandr
    PkgConfig::LCMS2
)

set_target_properties(kded_colord PROPERTIES OUTPUT_NAME colord)
kcoreaddons_desktop_to_json(kded_colord colord.desktop)
install(TARGETS kded_colord DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)
