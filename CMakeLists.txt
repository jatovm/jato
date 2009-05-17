cmake_minimum_required(VERSION 2.6)

project (cafebabe)

set (CMAKE_C_FLAGS "-Wall -std=gnu99")

include_directories ($(CAFEBABE_SOURCE_DIR)/include)

add_library (cafebabe SHARED
	src/cafebabe/attribute_info.c
	src/cafebabe/class.c
	src/cafebabe/constant_pool.c
	src/cafebabe/error.c
	src/cafebabe/field_info.c
	src/cafebabe/method_info.c
	src/cafebabe/stream.c
)

add_executable (dump src/dump.c)
target_link_libraries (dump cafebabe)

add_executable (java src/java.c)
target_link_libraries (java cafebabe)

set (LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib
	CACHE PATH "Installation prefix for user executables" FORCE)
set (INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include
	CACHE PATH "Installation prefix for C header files" FORCE)

install (TARGETS cafebabe DESTINATION "${LIB_INSTALL_DIR}")
install (DIRECTORY include/cafebabe DESTINATION "${INCLUDE_INSTALL_DIR}")
