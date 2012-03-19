set(kcm_colord_SRCS
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

kde4_add_plugin(kcm_colord ${kcm_colord_SRCS})

target_link_libraries(kcm_colord
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${LCMS2_LIBRARIES}
)

install(TARGETS kcm_colord DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_colord.desktop DESTINATION ${SERVICES_INSTALL_DIR})
