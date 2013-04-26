set(iccimporter_SRCS
    ../colord-kcm/Profile.cpp
    main.cpp
)

set(COLORD_INTERFACE_XML
    ../colord-tmp-interfaces/org.freedesktop.ColorManager.xml
)
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_INTERFACE_XML} PROPERTIES CLASSNAME CdInterface)
qt4_add_dbus_interface(iccimporter_SRCS ${COLORD_INTERFACE_XML} CdInterface)

kde4_add_executable(colord-kde-icc-importer ${iccimporter_SRCS})
target_link_libraries(colord-kde-icc-importer
    ${KDE4_KDEUI_LIBS}
    ${LCMS2_LIBRARIES}
)

install(TARGETS colord-kde-icc-importer DESTINATION ${BIN_INSTALL_DIR})
install(FILES colordkdeiccimporter.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
