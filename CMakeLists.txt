cmake_minimum_required(VERSION 2.6)
project(oolong)

### Version ###
set(oolong_VERSION_MAJOR 1)
set(oolong_VERSION_MINOR 0)
file(READ ${PROJECT_SOURCE_DIR}/LICENSE oolong_LICENSE)
configure_file(
  "${PROJECT_SOURCE_DIR}/src/oolong.h.in"
  "${PROJECT_BINARY_DIR}/oolong.h"
)
include_directories("${PROJECT_BINARY_DIR}")

### sources ###
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
file(GLOB SRCS ${SRC_DIR}/*.cpp)

### bison and flex ###
find_package(BISON "3.0")
find_package(FLEX "2.5")

bison_target(parser ${SRC_DIR}/parser.y ${SRC_DIR}/parser.cpp COMPILE_FLAGS "-v")
flex_target(tokens ${SRC_DIR}/tokens.l ${SRC_DIR}/tokens.cpp COMPILE_FLAGS "--header-file=${SRC_DIR}/tokens.h")
add_flex_bison_dependency(tokens parser)
set_source_files_properties(${SRC_DIR}/parser.cpp PROPERTIES COMPILE_FLAGS -w)
set_source_files_properties(${SRC_DIR}/tokens.cpp PROPERTIES COMPILE_FLAGS -w)

### llvm (find) ###
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

file(GLOB PKG_SRCS ${SRC_DIR}/package/*.c)
add_library(packages STATIC ${PKG_SRCS})
set_target_properties(packages PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/)

### Oolong ###
set(CMAKE_C_FLAGS "-O3 -pipe -fPIC -Wall")
set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -pipe -fstack-protector-strong -fPIC -fvisibility-inlines-hidden -Werror=date-time -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcast-qual -Wno-missing-field-initializers -pedantic -Wno-long-long -Wdelete-non-virtual-dtor -Wno-comment -ffunction-sections -fdata-sections -DNDEBUG -fno-exceptions")
add_executable(oolong ${SRCS} ${BISON_parser_OUTPUTS} ${FLEX_tokens_OUTPUTS})

### LLVM (add flags) ###
if(UNIX AND NOT APPLE)
    # will probably need updated later on
    # seems like a conflict between LLVM and ncurses
    # also unsure of how portable this is to other distros
    target_link_libraries(oolong LLVM-5.0)
endif()
set(COMPONENT_LIST all)
llvm_map_components_to_libnames(LLVM_LIBS ${COMPONENT_LIST})

target_link_libraries(oolong packages ${LLVM_LIBS})

