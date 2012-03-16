pkg_check_modules(LCMS2 lcms2)

set(colord_kded_SRCS
    DmiUtils.cpp
    Edid.cpp
    Output.cpp
    ProfileUtils.cpp
    XEventHandler.cpp
    ColorD.cpp
)

kde4_add_plugin(kded_colord ${colord_kded_SRCS})
target_link_libraries(kded_colord
    ${KDE4_KDEUI_LIBS}
    ${X11_Xrandr_LIB}
    ${X11_X11_LIB}
    ${LCMS2_LIBRARIES}
)

install(TARGETS kded_colord DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES colord.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
