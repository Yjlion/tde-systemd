# required stuff

tde_setup_compiler_flags( )
tde_setup_largefiles( )

find_package( TQt )
find_package( TDE )

if( WITH_GCC_VISIBILITY )
  tde_setup_gcc_visibility( )
endif( )
