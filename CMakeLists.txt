cmake_minimum_required(VERSION 3.20)
project(jai)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -g -O0")

include_directories(${PROJECT_SOURCE_DIR}/src)

# ====================================================================================
# # [[ LLVM ]]
# if (LLVM)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
    message(STATUS "LLVM include: ${LLVM_INCLUDE_DIRS}")
    message(STATUS "LLVM library: ${LLVM_LIBRARY_DIRS}")
    message(STATUS "LLVM tools binary: ${LLVM_TOOLS_BINARY_DIR}")
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})

    llvm_map_components_to_libnames(LLVM_LIBS orcjit native)
    message(STATUS "LLVM libs: ${LLVM_LIBS}")
# endif (LLVM)
# ====================================================================================
    
file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS})
