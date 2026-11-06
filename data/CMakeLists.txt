add_subdirectory( icons )

tde_create_translated_desktop( SOURCE tde-systemd-manager.desktop tde-journal-viewer.desktop
  DESTINATION ${XDG_APPS_INSTALL_DIR} )
