file(GLOB QML_SRC qml/*.qml qml/*.js)

set(calibrate_SRCS
    ${QML_SRC}
    main.cpp
)

qt5_add_resources(calibrate_SRCS
    application.qrc
    OPTIONS -compress 3 -threshold 3
)

set(COLORD_HELPER_INTERFACE_XML ${CMAKE_SOURCE_DIR}/colord-tmp-interfaces/org.freedesktop.ColorHelper.xml)
set_source_files_properties(${COLORD_HELPER_INTERFACE_XML} PROPERTIES INCLUDE "dbus-types.h")
set_source_files_properties(${COLORD_HELPER_INTERFACE_XML} PROPERTIES NO_NAMESPACE true)
set_source_files_properties(${COLORD_HELPER_INTERFACE_XML} PROPERTIES CLASSNAME CdHelperInterfaceDisplay)
qt5_add_dbus_interface(calibrate_SRCS ${COLORD_HELPER_INTERFACE_XML} CdHelperInterface)

add_executable(quick-color-calibrate
    ${calibrate_SRCS}
)

target_link_libraries(quick-color-calibrate
    Qt5::Gui
)
