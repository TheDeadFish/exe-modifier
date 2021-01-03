project(exe_modifier)
cmake_minimum_required(VERSION 3.16)


set( EXECUTABLE_OUTPUT_PATH ../bin)
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ../bin)



add_library(peFile src/peFile/peHead.cc 
	src/peFile/peFile.cc src/peFile/peBlock.cc 
	src/peFile/peExport.cc src/peFile/peImport.cc 
	src/peFile/peMapp.cc src/peFile/peFile_.cc 
	src/peFile/peReloc.cc
	
	)

ADD_EXECUTABLE ( exe_mod
	src/def_ops.cc src/exe_mod.cc src/modfile.cc
	src/peFile/peImport.cc src/linker/linker.cc
	src/linker/except.cc src/main.cpp lib/stdafx.cc 
	lib/dlldef.cc src/debug.cpp)
	
include_directories(exe_mod PRIVATE lib)
target_link_libraries( exe_mod peFile -limagehlp -lexshit -lstdshit)

# precompiled header support
target_precompile_headers(peFile PUBLIC "lib/stdafx.h")
target_precompile_headers(exe_mod REUSE_FROM peFile)
