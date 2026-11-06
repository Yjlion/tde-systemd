include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${TDE_LIB_DIR}
)

##### tdesystemdcommon (static, position independent: also linked into the kcm modules)

tde_add_library( tdesystemdcommon STATIC_PIC AUTOMOC
  SOURCES
    sdprocess.cpp sdjson.cpp sdunits.cpp sdpage.cpp
    servicespage.cpp timerspage.cpp overviewpage.cpp
    datetimepage.cpp hostnamepage.cpp localepage.cpp managerwidget.cpp
    journalwidget.cpp maintenancedialog.cpp
  LINK tdeio-shared tdeui-shared tdecore-shared
)
