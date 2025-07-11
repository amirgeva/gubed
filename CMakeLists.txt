cmake_minimum_required(VERSION 3.16)
project(gubed)

set(CMAKE_CXX_STANDARD 17)

find_package(GTest CONFIG REQUIRED)

if (UNIX)
	add_definitions(-fPIC)
endif()

# Define folder organization
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Libraries folder
set(LIBRARIES_FOLDER "Libraries")

# Wren library
add_library(wren STATIC
        wren/src/vm/wren_primitive.c
        wren/src/vm/wren_compiler.c
        wren/src/vm/wren_value.c
        wren/src/vm/wren_utils.c
        wren/src/vm/wren_debug.c
        wren/src/vm/wren_vm.c
        wren/src/vm/wren_core.c
        wren/src/optional/wren_opt_random.c
        wren/src/optional/wren_opt_meta.c
)
target_include_directories(wren
PUBLIC
        wren/src/include
PRIVATE
        wren/src/optional wren/src/vm
)
set_target_properties(wren PROPERTIES FOLDER ${LIBRARIES_FOLDER})

# Add subdirectories organized by purpose
add_subdirectory(utils)  # Library
add_subdirectory(conwin) # Library
add_subdirectory(native_sample)
add_subdirectory(gubed)   # App
add_subdirectory(tests)   # Tests

