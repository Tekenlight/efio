set(LIBNAME "evsync")
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

#add_library( "${LIBNAME}" ${LIB_MODE} ${SRCS} )
add_library( "${LIBNAME}" STATIC ${SRCS} )
#add_library( "${EV_LIBNAME}" ALIAS "${LIBNAME}")
set_target_properties( "${LIBNAME}"
    PROPERTIES
    VERSION ${SHARED_LIBRARY_VERSION} SOVERSION ${SHARED_LIBRARY_VERSION}
	OUTPUT_NAME ${EV_LIBNAME}
    DEFINE_SYMBOL evsync_EXPORTS
    )

target_link_libraries( "${LIBNAME}" evglobals ${SYSLIBS} ${LIBEV_LIBRARY})
target_include_directories( "${LIBNAME}"
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )
target_compile_definitions("${LIBNAME}" PUBLIC ${LIB_MODE_DEFINITIONS})

POCO_INSTALL("${LIBNAME}")
POCO_GENERATE_PACKAGE("${LIBNAME}")

if (ENABLE_TESTS)
    add_subdirectory(samples)
#	add_subdirectory(testsuite)
endif ()
message(STATUS "All seems to have gone well")
