set(LIBNAME "core_utils")
set(EV_LIBNAME "${LIBNAME}")

# Sources
file(GLOB SRCS_G "*.c")
POCO_SOURCES_AUTO( SRCS ${SRCS_G})

# Headers
file(GLOB_RECURSE HDRS_G "*.h" )
POCO_HEADERS_AUTO( SRCS ${HDRS_G})

message(STATUS "INCLUSION HAS HAPPENED")

# Windows and WindowsCE need additional libraries
if(WIN32)
    if(WINCE)
        set(SYSLIBS ${SYSLIBS} "ws2.lib" "iphlpapi.lib")
    else()    
        set(SYSLIBS ${SYSLIBS} "ws2_32.lib" "iphlpapi.lib")
    endif()    
endif(WIN32)

add_library( "${LIBNAME}" SHARED ${SRCS} )
if (DEFINED LIBZ_BUILD_PATH)
	target_include_directories( "${LIBNAME}" PUBLIC "${LIBZ_BUILD_PATH}/usr/local/include")
	find_library(LIBZ NAMES "libz.a" NO_DEFAULT_PATH PATHS "${LIBZ_BUILD_PATH}/usr/local/lib" REQUIRED)
else(DEFINED LIBZ_BUILD_PATH)
	find_library(LIBZ NAMES z REQUIRED)
endif(DEFINED LIBZ_BUILD_PATH)

message(STATUS "LIBZ ${LIBZ}")

target_link_libraries( "${LIBNAME}" ${LIBZ})

set_target_properties( "${LIBNAME}"
    PROPERTIES
    VERSION ${SHARED_LIBRARY_VERSION} SOVERSION ${SHARED_LIBRARY_VERSION}
	OUTPUT_NAME ${EV_LIBNAME}
    DEFINE_SYMBOL core_utils_EXPORTS
    )

target_link_libraries( "${LIBNAME}")
target_include_directories( "${LIBNAME}"
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )

#get_target_property(CORE_UTILS_INCLUDES ${LIBNAME} INCLUDE_DIRECTORIES)
#message(STATUS "INCLUDE DIRECTORIES OF ${LIBNAME} ${CORE_UTILS_INCLUDES}")


target_compile_definitions("${LIBNAME}" PUBLIC ${LIB_MODE_DEFINITIONS})

POCO_INSTALL("${LIBNAME}")
POCO_GENERATE_PACKAGE("${LIBNAME}")

if (ENABLE_TESTS)
#	add_subdirectory(samples)
#	add_subdirectory(testsuite)
endif ()
message(STATUS "All seems to have gone well")
