cmake_minimum_required (VERSION 2.6)
project (glsu)
INCLUDE(FindOpenGL)
if ( OPENGL_FOUND )
	include_directories(${PROJECT_SOURCE_DIR}/include)
	add_subdirectory (src)
	add_subdirectory (test)
endif ( OPENGL_FOUND )
