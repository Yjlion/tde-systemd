include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/common
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories( ${TQT_LIBRARY_DIRS} ${TDE_LIB_DIR} )

##### both Control Center modules live in one library, see the X-TDE-FactoryName
##### keys of the .desktop files

tde_add_kpart( kcm_tdesystemd AUTOMOC
  SOURCES kcm_tdesystemd.cpp
  LINK tdesystemdcommon-static tdeio-shared tdeui-shared tdecore-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)

tde_create_translated_desktop( SOURCE tdesystemd.desktop tdejournal.desktop
  DESTINATION ${XDG_APPS_INSTALL_DIR} )
