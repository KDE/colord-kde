include_directories(${CMAKE_SOURCE_DIR}/colord-kcm
                    ${CMAKE_BUILD_DIR}
)

set(kcm_colord_SRCS
    NoSelectionRectDelegate.cpp
    DeviceModel.cpp
    ProfileModel.cpp
    Profile.cpp
    ProfileNamedColors.cpp
    ProfileMetaData.cpp
    Description.cpp
    ColordKCM.cpp

    NoSelectionRectDelegate.h
    DeviceModel.h
    ProfileModel.h
    Profile.h
    ProfileNamedColors.h
    ProfileMetaData.h
    Description.h
    ColordKCM.h
)

ki18n_wrap_ui(kcm_colord_SRCS
    ProfileNamedColors.ui
    ProfileMetaData.ui
    Description.ui
    ColordKCM.ui
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

add_library(kcm_colord MODULE ${kcm_colord_SRCS})
target_link_libraries(kcm_colord
    Qt::Core
    Qt::Widgets
    Qt::DBus
    KF5::CoreAddons
    KF5::I18n
    KF5::KCMUtils
    PkgConfig::LCMS2
    KF5::KIOGui
)

install(TARGETS kcm_colord DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES kcm_colord.desktop DESTINATION ${KDE_INSTALL_KSERVICESDIR})
