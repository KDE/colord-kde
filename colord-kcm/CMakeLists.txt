set(kcm_colord_SRCS
    NoSelectionRectDelegate.cpp
    DeviceModel.cpp
    ProfileModel.cpp
    Profile.cpp
    ProfileNamedColors.cpp
    ProfileMetaData.cpp
    Description.cpp
    ColordKCM.cpp
)

kde4_add_ui_files(kcm_colord_SRCS
    ProfileNamedColors.ui
    ProfileMetaData.ui
    Description.ui
    ColordKCM.ui
)

#find_file(COLOR_HELPER_INTERFACE_XML org.freedesktop.ColorHelper.xml
#        PATHS ${CMAKE_INSTALL_PREFIX}/share/dbus-1/interfaces/
#)
#set(COLOR_HELPER_INTERFACE_XML
#    ../colord-tmp-interfaces/org.freedesktop.ColorHelper.xml
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
set(COLORD_SENSOR_INTERFACE_XML
    ../colord-tmp-interfaces/org.freedesktop.ColorManager.Sensor.xml
)
#if (COLORD_INTERFACE_XML STREQUAL "COLORD_INTERFACE_XML-NOTFOUND" OR
#    COLORD_DEVICE_INTERFACE_XML STREQUAL "COLORD_DEVICE_INTERFACE_XML-NOTFOUND")
#        message (FATAL_ERROR "Unable to find colord DBus specifications! Please install colord to continue!")
#endif ()

#set_source_files_properties(${COLOR_HELPER_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
#set_source_files_properties(${COLOR_HELPER_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
#qt4_add_dbus_interfaces(kcm_colord_SRCS ${COLOR_HELPER_INTERFACE_XML})

set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES CLASSNAME CdInterface)
qt4_add_dbus_interface(kcm_colord_SRCS ${COLORD_INTERFACE_XML} CdInterface)

set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES CLASSNAME CdDeviceInterface)
qt4_add_dbus_interface(kcm_colord_SRCS ${COLORD_DEVICE_INTERFACE_XML} CdDeviceInterface)

set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES CLASSNAME CdProfileInterface)
qt4_add_dbus_interface(kcm_colord_SRCS ${COLORD_PROFILE_INTERFACE_XML} CdProfileInterface)

set_source_files_properties(${COLORD_SENSOR_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_SENSOR_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_SENSOR_INTERFACE_XML} PROPERTIES CLASSNAME CdSensorInterface)
qt4_add_dbus_interface(kcm_colord_SRCS ${COLORD_SENSOR_INTERFACE_XML} CdSensorInterface)

kde4_add_plugin(kcm_colord ${kcm_colord_SRCS})
target_link_libraries(kcm_colord
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${LCMS2_LIBRARIES}
)

install(TARGETS kcm_colord DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_colord.desktop DESTINATION ${SERVICES_INSTALL_DIR})
