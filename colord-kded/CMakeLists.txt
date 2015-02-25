include_directories(${CMAKE_SOURCE_DIR}/colord-kded
                    ${CMAKE_BUILD_DIR}
                    ${X11_Xrandr_INCLUDE_PATH}
)

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
    ${X11_Xrandr_LIB}
    ${X11_LIBRARIES}
    ${XCB_LIBRARIES}
    ${LCMS2_LIBRARIES}
)

install(TARGETS kded_colord DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES colord.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
