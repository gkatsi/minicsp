cmake_minimum_required(VERSION 2.6)

project(minicsp)

option(BUILD_SHARED_LIBS OFF "True for building shared object")
set(CMAKE_CXX_FLAGS "-std=c++1z")
include_directories("/usr/local/include/")
include_directories(/usr/local/opt/libxml2/include/libxml2/)

# Dependencies {{{
find_package(ZLIB REQUIRED)
# }}}
set(main "minicsp/xcsp/main-xcsp.cpp")

# Basic Library
file(GLOB lib_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} minicsp/mtl/*.cpp minicsp/core/*.cpp minicsp/xcsp/*.cpp minicsp/xcsp3/*.cpp)
list(REMOVE_ITEM lib_srcs ${main})

include(ExternalProject)
ExternalProject_Add(XCSP3-CPP-Parser
        PREFIX parser
        GIT_REPOSITORY https://github.com/xcsp3team/XCSP3-CPP-Parser.git
        CONFIGURE_COMMAND ""
        BINARY_DIR parser/src/XCSP3-CPP-Parser/samples
        BUILD_COMMAND make lib
        INSTALL_COMMAND ""
        )

add_library(parserxcsp3core STATIC IMPORTED)
set_target_properties(parserxcsp3core PROPERTIES IMPORTED_LOCATION
        parser/src/XCSP3-CPP-Parser/lib/libparserxcsp3core.a)



if(BUILD_SHARED_LIBS)
    set(lib_type "SHARED")
else()
    set(lib_type "STATIC")
endif()


include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ZLIB_INCLUDE_DIR}
        "minicsp/xcsp3/include"
)

link_libraries(${ZLIB_LIBRARY})


#add_library(minicsp ${lib_type} ${lib_srcs})

add_executable(minicsp ${main})
target_link_libraries(minicsp)

