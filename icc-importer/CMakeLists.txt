set(iccimporter_SRCS
    ../colord-kcm/Profile.cpp ../colord-kcm/Profile.h
    main.cpp
)

set(COLORD_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorManager.xml)

set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES CLASSNAME CdInterface)
qt_add_dbus_interface(iccimporter_SRCS ${COLORD_INTERFACE_XML} CdInterface)

add_executable(colord-kde-icc-importer ${iccimporter_SRCS})
target_link_libraries(colord-kde-icc-importer
    Qt::Core
    Qt::Widgets
    Qt::DBus
    KF5::CoreAddons
    KF5::I18n
    KF5::WidgetsAddons
    PkgConfig::LCMS2
)

install(TARGETS colord-kde-icc-importer DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES colordkdeiccimporter.desktop DESTINATION ${KDE_INSTALL_APPDIR})
