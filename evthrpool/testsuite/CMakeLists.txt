set(TESTUNIT "${LIBNAME}-testrunner")

# Sources
POCO_SOURCES_AUTO( TEST_SRCS test_thread_pool.c)

# Headers

#POCO_SOURCES_AUTO_PLAT( TEST_SRCS OFF
#    src/WinDriver.cpp
#)

#POCO_SOURCES_AUTO_PLAT( TEST_SRCS WINCE
#    src/WinCEDriver.cpp
#)

add_executable( ${TESTUNIT} ${TEST_SRCS} )

target_link_libraries( "${TESTUNIT}" evglobals evsync evds evthrpool cutest "${SYSLIBS}")


if(ANDROID)
	add_test(NAME ${LIBNAME} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} 
	        COMMAND ${CMAKE_COMMAND} -DANDROID_NDK=${ANDROID_NDK} -DLIBRARY_DIR=${CMAKE_BINARY_DIR}/lib -DUNITTEST=${CMAKE_BINARY_DIR}/bin/${TESTUNIT} -DTEST_PARAMETER=-all -P ${CMAKE_SOURCE_DIR}/cmake/ExecuteOnAndroid.cmake)
else()
	add_test(NAME ${TESTUNIT} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${TESTUNIT} -all)
endif()
