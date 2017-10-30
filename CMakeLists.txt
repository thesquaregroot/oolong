cmake_minimum_required(VERSION 2.6)
project(oolong)

### Version ###
set(oolong_VERSION_MAJOR 1)
set(oolong_VERSION_MINOR 0)
configure_file(
  "${PROJECT_SOURCE_DIR}/src/oolong.h.in"
  "${PROJECT_BINARY_DIR}/oolong.h"
)
include_directories("${PROJECT_BINARY_DIR}")

### sources ###
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
file(GLOB SRCS ${SRC_DIR}/*.cpp)

### bison and flex ###
find_package(BISON)
message(STATUS "Found bison ${BISON_PACKAGE_VERSION}")
find_package(FLEX)
message(STATUS "Found flex ${FLEX_PACKAGE_VERSION}")

bison_target(parser ${SRC_DIR}/parser.y ${SRC_DIR}/parser.cpp COMPILE_FLAGS "-v -d")
flex_target(tokens ${SRC_DIR}/tokens.l ${SRC_DIR}/tokens.cpp)
add_flex_bison_dependency(tokens parser)

### llvm ###
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(LLVM_LIBS core native)

### Oolong ###
set(CMAKE_CXX_FLAGS "-L/usr/lib -lLLVM-5.0 -I/usr/include -march=x86-64 -mtune=generic -O2 -pipe -fstack-protector-strong -fno-plt -fPIC -fvisibility-inlines-hidden -Werror=date-time -std=c++11 -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcast-qual -Wno-missing-field-initializers -pedantic -Wno-long-long -Wno-maybe-uninitialized -Wdelete-non-virtual-dtor -Wno-comment -ffunction-sections -fdata-sections -O3 -DNDEBUG  -fno-exceptions -DLLVM_BUILD_GLOBAL_ISEL -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
add_executable(oolong ${SRCS} ${BISON_parser_OUTPUTS} ${FLEX_tokens_OUTPUTS})
target_link_libraries(oolong ${LLVM_LIBS})

### packages ###
add_custom_target(lib_dir ALL COMMAND mkdir -p ${PROJECT_SOURCE_DIR}/lib)
add_custom_target(packages ALL DEPENDS lib_dir COMMAND find ${PROJECT_SOURCE_DIR}/src/package -name *.c -execdir gcc -c {} -o ${PROJECT_SOURCE_DIR}/lib/{}.o \\\\;)

