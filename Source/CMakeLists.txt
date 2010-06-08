cmake_minimum_required (VERSION 2.8)

#=============================================
MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
	GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
	SET(PrecompiledBinary "$(IntDir)/${PrecompiledBasename}.pch")
	SET(Sources ${${SourcesVar}})

	SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
								PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
										   OBJECT_OUTPUTS "${PrecompiledBinary}")
	SET_SOURCE_FILES_PROPERTIES(${Sources}
								PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
										   OBJECT_DEPENDS "${PrecompiledBinary}")  
	# Add precompiled header to SourcesVar
	LIST(APPEND ${SourcesVar} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

#=============================================
project (RibTools)

if (MSVC)
	# disable annoying secure CRT warnings
	add_definitions( "/D_CRT_SECURE_NO_WARNINGS /wd4996 /nologo" )
	# disable RTTI
	add_definitions( "/GR-" )
endif (MSVC)

# Find OpenMP and set the flags for the compiler
find_package(OpenMP)
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

# Add the source dir as a common include path
include_directories( . )

# Specify the destination directly into the Distrib directory
# ..also avoid the /Release and /Debug additional subdirs created by
# Visual Studio
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../Distrib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../Distrib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../Distrib )

# Various libs
add_subdirectory( DSystem )
add_subdirectory( DMath )
add_subdirectory( DImage )
add_subdirectory( RI_System )
add_subdirectory( RibRenderLib )
add_subdirectory( RibToolsBase )
add_subdirectory( RSLCompilerLib )

# external libs
add_subdirectory( externals/local/libtiff )
add_subdirectory( externals/local/libjpeg )
add_subdirectory( externals/local/freeglut )

# executables
add_subdirectory( RibRender )
add_subdirectory( RibRenderServer )
add_subdirectory( RibRenderToy )
add_subdirectory( RSLCompilerCmd )
