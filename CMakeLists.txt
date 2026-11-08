#################################################
#
#  tde-systemd - systemd manager and journal viewer for TDE
#
#  This file is released under GPL >= 2
#
#################################################

include( TDEVersion )
cmake_minimum_required( VERSION ${TDE_CMAKE_MINIMUM_VERSION} )

project( tde-systemd )
set( VERSION "1.0.0" )

include( FindPkgConfig )
include( CheckIncludeFile )
include( CheckCXXSourceCompiles )
include( TDEMacros )
include( TDESetupPaths )
tde_setup_paths( )

##### options

option( WITH_ALL_OPTIONS "Enable all optional support" OFF )
option( WITH_GCC_VISIBILITY "Enable fvisibility and fvisibility-inlines-hidden" ${WITH_ALL_OPTIONS} )
option( BUILD_ALL "Build all" ON )
option( BUILD_DOC "Build documentation" ${BUILD_ALL} )
option( BUILD_KCM "Build TDE Control Center modules" ${BUILD_ALL} )

set( POLKIT_ACTIONS_DIR "/usr/share/polkit-1/actions" CACHE PATH "Directory for polkit action files" )
set( HELPER_INSTALL_DIR "${LIBEXEC_INSTALL_DIR}" CACHE PATH "Directory for the privileged helper" )
set( HELPER_PATH "${HELPER_INSTALL_DIR}/tde-systemd-helper" )

##### configure checks

include( ConfigureChecks.cmake )

##### global compiler settings

add_definitions( -DHAVE_CONFIG_H )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TQT_CXX_FLAGS}" )
set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined" )

# the TDE macros do not pass the linker flags of the environment
# (e.g. the hardening flags of dpkg-buildflags) to every target
if( DEFINED ENV{LDFLAGS} )
  separate_arguments( _env_ldflags UNIX_COMMAND "$ENV{LDFLAGS}" )
  add_link_options( ${_env_ldflags} )
endif( )

##### directories

add_subdirectory( src )
add_subdirectory( data )

tde_conditional_add_project_docs( BUILD_DOC )

##### write configure files

configure_file( config.h.cmake config.h @ONLY )
