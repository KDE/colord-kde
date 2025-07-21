include_directories(${CMAKE_SOURCE_DIR}/colord-kded
                    ${CMAKE_BUILD_DIR}
)

add_definitions(-DPROJECT_NAME=\"${PROJECT_NAME}\")

kcoreaddons_add_plugin(kded_colord INSTALL_NAMESPACE kf6/kded)

target_sources(kded_colord PRIVATE
    DmiUtils.cpp
    Edid.cpp
    Output.cpp
    ProfileUtils.cpp
    XEventHandler.cpp
    ProfilesWatcher.cpp
    ColorD.cpp

    DmiUtils.h
    Edid.h
    Output.h
    ProfileUtils.h
    XEventHandler.h
    ProfilesWatcher.h
    ColorD.h
)

set(COLORD_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.xml)
set(COLORD_DEVICE_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Device.xml)
set(COLORD_PROFILE_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Profile.xml)

set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES CLASSNAME CdInterface)
qt_add_dbus_interface(dbus_SRCS ${COLORD_INTERFACE_XML} CdInterface)

set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES CLASSNAME CdDeviceInterface)
qt_add_dbus_interface(dbus_SRCS ${COLORD_DEVICE_INTERFACE_XML} CdDeviceInterface)

set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES CLASSNAME CdProfileInterface)
qt_add_dbus_interface(dbus_SRCS ${COLORD_PROFILE_INTERFACE_XML} CdProfileInterface)

target_sources(kded_colord PRIVATE ${dbus_SRCS})

target_link_libraries(kded_colord
    Qt::Core
    Qt::Widgets
    Qt::DBus
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::WindowSystem
    X11::X11
    X11::Xrandr
    PkgConfig::LCMS2
)

set_target_properties(kded_colord PROPERTIES OUTPUT_NAME colord)
