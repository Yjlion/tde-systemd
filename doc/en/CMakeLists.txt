tde_create_handbook( DESTINATION tde-systemd )

# Help > Handbook of each application looks for a handbook named after the
# application; point both names to the shared handbook.
foreach( _app tde-systemd-manager tde-journal-viewer )
  install( CODE "execute_process( COMMAND ln -sfn tde-systemd \$ENV{DESTDIR}${HTML_INSTALL_DIR}/en/${_app} )" )
endforeach( )
