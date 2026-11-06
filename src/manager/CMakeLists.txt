include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/common
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories( ${TQT_LIBRARY_DIRS} ${TDE_LIB_DIR} )

tde_add_executable( tde-systemd-manager AUTOMOC
  SOURCES main.cpp mainwindow.cpp
  LINK tdesystemdcommon-static tdeio-shared tdeui-shared tdecore-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
