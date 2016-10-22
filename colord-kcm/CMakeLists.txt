include_directories(${CMAKE_SOURCE_DIR}/colord-kcm
                    ${CMAKE_BUILD_DIR}
                    ${X11_Xrandr_INCLUDE_PATH}
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

add_library(kcm_colord MODULE ${kcm_colord_SRCS})
target_link_libraries(kcm_colord
    Qt5::Core
    Qt5::Widgets
    Qt5::DBus
    KF5::CoreAddons
    KF5::I18n
    KF5::KCMUtils
    ${LCMS2_LIBRARIES}
)

install(TARGETS kcm_colord DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_colord.desktop DESTINATION ${SERVICES_INSTALL_DIR})
