include_directories(${CMAKE_SOURCE_DIR}/colord-kcm
                    ${CMAKE_BUILD_DIR}
)

set(kcm_colord_SRCS
    DeviceModel.cpp DeviceModel.h
    ProfileModel.cpp ProfileModel.h
    Profile.cpp Profile.h
    ColordKCM.cpp ColordKCM.h
    DeviceDescription.cpp DeviceDescription.h
    ProfileDescription.cpp ProfileDescription.h
    ProfileMetaDataModel.cpp ProfileMetaDataModel.h
    ProfileNamedColorsModel.cpp ProfileNamedColorsModel.h
    Profile.cpp Profile.h
)

set(COLORD_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.xml)
set(COLORD_DEVICE_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Device.xml)
set(COLORD_PROFILE_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Profile.xml)
set(COLORD_SENSOR_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Sensor.xml)

set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES CLASSNAME CdInterface)
qt_add_dbus_interface(kcm_colord_SRCS ${COLORD_INTERFACE_XML} CdInterface)

set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES CLASSNAME CdDeviceInterface)
qt_add_dbus_interface(kcm_colord_SRCS ${COLORD_DEVICE_INTERFACE_XML} CdDeviceInterface)

set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES CLASSNAME CdProfileInterface)
qt_add_dbus_interface(kcm_colord_SRCS ${COLORD_PROFILE_INTERFACE_XML} CdProfileInterface)

set_source_files_properties(${COLORD_SENSOR_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_SENSOR_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_SENSOR_INTERFACE_XML} PROPERTIES CLASSNAME CdSensorInterface)
qt_add_dbus_interface(kcm_colord_SRCS ${COLORD_SENSOR_INTERFACE_XML} CdSensorInterface)

kcmutils_add_qml_kcm(kcm_colord SOURCES ${kcm_colord_SRCS})

target_link_libraries(kcm_colord
    Qt::Core
    Qt::DBus
    KF6::KCMUtilsQuick
    KF6::I18n
    KF6::KCMUtils
    PkgConfig::LCMS2
)
