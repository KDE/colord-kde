set(colord_kded_SRCS
    DmiUtils.cpp
    Edid.cpp
    Output.cpp
    ProfileUtils.cpp
    XEventHandler.cpp
    ProfilesWatcher.cpp
    ColorD.cpp
)

#find_file(COLORD_INTERFACE_XML org.freedesktop.ColorManager.xml
#        PATHS ${CMAKE_INSTALL_PREFIX}/share/dbus-1/interfaces/
#)
set(COLORD_INTERFACE_XML
    ../colord-tmp-interfaces/org.freedesktop.ColorManager.xml
)
set(COLORD_DEVICE_INTERFACE_XML
    ../colord-tmp-interfaces/org.freedesktop.ColorManager.Device.xml
)
set(COLORD_PROFILE_INTERFACE_XML
    ../colord-tmp-interfaces/org.freedesktop.ColorManager.Profile.xml
)
#if (COLORD_INTERFACE_XML STREQUAL "COLORD_INTERFACE_XML-NOTFOUND" OR
#    COLORD_DEVICE_INTERFACE_XML STREQUAL "COLORD_DEVICE_INTERFACE_XML-NOTFOUND")
#        message (FATAL_ERROR "Unable to find colord DBus specifications! Please install colord to continue!")
#endif ()

set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES INCLUDE "../dbus-types.h")
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES CLASSNAME CdInterface)
qt4_add_dbus_interface(colord_kded_SRCS ${COLORD_INTERFACE_XML} CdInterface)

set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES INCLUDE "../dbus-types.h")
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES CLASSNAME CdDeviceInterface)
qt4_add_dbus_interface(colord_kded_SRCS ${COLORD_DEVICE_INTERFACE_XML} CdDeviceInterface)

set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES INCLUDE "../dbus-types.h")
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES CLASSNAME CdProfileInterface)
qt4_add_dbus_interface(colord_kded_SRCS ${COLORD_PROFILE_INTERFACE_XML} CdProfileInterface)

kde4_add_plugin(kded_colord ${colord_kded_SRCS})
target_link_libraries(kded_colord
    ${KDE4_KDEUI_LIBS}
    ${X11_Xrandr_LIB}
    ${X11_X11_LIB}
    ${LCMS2_LIBRARIES}
)

install(TARGETS kded_colord DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES colord.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
