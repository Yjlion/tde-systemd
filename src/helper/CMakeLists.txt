# The helper is plain C++ without TQt/TDE so the code running as root stays small.

add_executable( tde-systemd-helper helper.cpp )
set_target_properties( tde-systemd-helper PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON )
install( TARGETS tde-systemd-helper DESTINATION ${HELPER_INSTALL_DIR} )

configure_file( org.trinitydesktop.tde-systemd.policy.in
                ${CMAKE_CURRENT_BINARY_DIR}/org.trinitydesktop.tde-systemd.policy @ONLY )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/org.trinitydesktop.tde-systemd.policy
         DESTINATION ${POLKIT_ACTIONS_DIR} )
