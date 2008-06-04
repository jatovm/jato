project (cafebabe)

set (CMAKE_C_FLAGS "-Wall -std=gnu99")

include_directories ($(CAFEBABE_SOURCE_DIR)/include)

add_library (cafebabe
	src/attribute_info.c
	src/class.c
	src/constant_pool.c
	src/error.c
	src/field_info.c
	src/method_info.c
	src/stream.c
)

add_executable (dump main.c)

target_link_libraries (dump cafebabe)

install (TARGETS cafebabe DESTINATION "${LIB_INSTALL_DIR}")
install (DIRECTORY include DESTINATION "${INCLUDE_INSTALL_DIR}")
