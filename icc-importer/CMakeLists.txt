set(iccimporter
    ../colord-kcm/Profile.cpp
    main.cpp
)

kde4_add_executable(colord-kde-icc-importer ${iccimporter})
target_link_libraries(colord-kde-icc-importer
    ${KDE4_KDEUI_LIBS}
    ${LCMS2_LIBRARIES}
)

install(TARGETS colord-kde-icc-importer DESTINATION ${BIN_INSTALL_DIR})
install(FILES colordkdeiccimporter.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
