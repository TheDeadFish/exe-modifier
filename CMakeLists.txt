cmake_minimum_required(VERSION 3.12)
include(lib/cotire.cmake)

set( EXECUTABLE_OUTPUT_PATH ../bin)
ADD_EXECUTABLE ( exe_mod
	src/def_ops.cc src/exe_mod.cc src/modfile.cc
	src/peFile/peBlock.cc src/peFile/peExport.cc
	src/peFile/peFile.cc src/peFile/peFile_.cc 
	src/peFile/peImport.cc src/linker/linker.cc
	src/main.cpp lib/stdafx.cc )
	
target_include_directories(exe_mod PRIVATE lib)
target_link_libraries( exe_mod -limagehlp -lexshit -lstdshit)

# precompiled header support
set_target_properties(exe_mod PROPERTIES 
	COTIRE_CXX_PREFIX_HEADER_INIT "lib/stdafx.h")
cotire(exe_mod)
