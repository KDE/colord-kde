include_directories(${CMAKE_SOURCE_DIR}/colord-kcm
                    ${CMAKE_BUILD_DIR}
)

set(kcm_colord_SRCS
#    NoSelectionRectDelegate.cpp
#    DeviceModel.cpp
#    ProfileModel.cpp
#    Profile.cpp
#    ProfileNamedColors.cpp
#    ProfileMetaData.cpp
#    Description.cpp
    ColordKCM.cpp
)

set(COLORD_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.xml)
set(COLORD_DEVICE_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Device.xml)
set(COLORD_PROFILE_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Profile.xml)
set(COLORD_SENSOR_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.Sensor.xml)

set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES CLASSNAME CdInterface)
qt5_add_dbus_interface(kcm_colord_SRCS ${COLORD_INTERFACE_XML} CdInterface)

set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_DEVICE_INTERFACE_XML} PROPERTIES CLASSNAME CdDeviceInterface)
qt5_add_dbus_interface(kcm_colord_SRCS ${COLORD_DEVICE_INTERFACE_XML} CdDeviceInterface)

set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_PROFILE_INTERFACE_XML} PROPERTIES CLASSNAME CdProfileInterface)
qt5_add_dbus_interface(kcm_colord_SRCS ${COLORD_PROFILE_INTERFACE_XML} CdProfileInterface)

set_source_files_properties(${COLORD_SENSOR_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_SENSOR_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_SENSOR_INTERFACE_XML} PROPERTIES CLASSNAME CdSensorInterface)
qt5_add_dbus_interface(kcm_colord_SRCS ${COLORD_SENSOR_INTERFACE_XML} CdSensorInterface)

kcoreaddons_add_plugin(kcm_colord SOURCES ${kcm_colord_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
target_link_libraries(kcm_colord
    Qt5::Core
    Qt5::Widgets
    Qt5::DBus
    KF5::CoreAddons
    KF5::I18n
    KF5::KCMUtils
    KF5::Declarative
    KF5::QuickAddons
    ${LCMS2_LIBRARIES}
)

install(FILES kcm_colord.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(TARGETS kcm_colord  DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcms/systemsettings)
kpackage_install_package(package kcm_colord kcms)
