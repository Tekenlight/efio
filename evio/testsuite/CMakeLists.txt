file(COPY test tmpl reading_file reading_file_sample1 reading_file_sample2 DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Sources
set(TESTUNIT1 "${LIBNAME}-file-testrunner1")
POCO_SOURCES_AUTO( TEST_SRCS_1 file_test_case_1.c)

add_executable( ${TESTUNIT1} ${TEST_SRCS_1} )
target_link_libraries( "${TESTUNIT1}" evglobals evsync evds evthrpool evio cutest "${SYSLIBS}")
if(ANDROID)
	add_test(NAME ${TESTUNIT1} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} 
		COMMAND ${CMAKE_COMMAND} -DANDROID_NDK=${ANDROID_NDK} -DLIBRARY_DIR=${CMAKE_BINARY_DIR}/lib -DUNITTEST=${CMAKE_BINARY_DIR}/bin/${TESTUNIT1} -DTEST_PARAMETER=-all -P ${CMAKE_SOURCE_DIR}/cmake/ExecuteOnAndroid.cmake)
else()
	add_test(NAME ${TESTUNIT1} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TESTUNIT1} -all)
endif()

# Sources
set(TESTUNIT2 "${LIBNAME}-file-testrunner2")
POCO_SOURCES_AUTO( TEST_SRCS_2 file_test_case_2.c)

add_executable( ${TESTUNIT2} ${TEST_SRCS_2} )
target_link_libraries( "${TESTUNIT2}" evglobals evsync evds evthrpool evio cutest "${SYSLIBS}")
if(ANDROID)
	add_test(NAME ${TESTUNIT2} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} 
		COMMAND ${CMAKE_COMMAND} -DANDROID_NDK=${ANDROID_NDK} -DLIBRARY_DIR=${CMAKE_BINARY_DIR}/lib -DUNITTEST=${CMAKE_BINARY_DIR}/bin/${TESTUNIT2} -DTEST_PARAMETER=-all -P ${CMAKE_SOURCE_DIR}/cmake/ExecuteOnAndroid.cmake)
else()
	add_test(NAME ${TESTUNIT2} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TESTUNIT2} -all)
endif()

# Sources
set(TESTUNIT3 "${LIBNAME}-file-testrunner3")
POCO_SOURCES_AUTO( TEST_SRCS_3 file_test_case_3.c)

add_executable( ${TESTUNIT3} ${TEST_SRCS_3} )
target_link_libraries( "${TESTUNIT3}" evglobals evsync evds evthrpool evio cutest "${SYSLIBS}")
if(ANDROID)
	add_test(NAME ${TESTUNIT3} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} 
		COMMAND ${CMAKE_COMMAND} -DANDROID_NDK=${ANDROID_NDK} -DLIBRARY_DIR=${CMAKE_BINARY_DIR}/lib -DUNITTEST=${CMAKE_BINARY_DIR}/bin/${TESTUNIT3} -DTEST_PARAMETER=-all -P ${CMAKE_SOURCE_DIR}/cmake/ExecuteOnAndroid.cmake)
else()
	add_test(NAME ${TESTUNIT3} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TESTUNIT3} -all)
endif()

# Sources
set(TESTUNIT4 "${LIBNAME}-file-testrunner4")
POCO_SOURCES_AUTO( TEST_SRCS_4 file_test_case_4.c)

add_executable( ${TESTUNIT4} ${TEST_SRCS_4} )
target_link_libraries( "${TESTUNIT4}" evglobals evsync evds evthrpool evio cutest "${SYSLIBS}")
if(ANDROID)
	add_test(NAME ${TESTUNIT4} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} 
		COMMAND ${CMAKE_COMMAND} -DANDROID_NDK=${ANDROID_NDK} -DLIBRARY_DIR=${CMAKE_BINARY_DIR}/lib -DUNITTEST=${CMAKE_BINARY_DIR}/bin/${TESTUNIT4} -DTEST_PARAMETER=-all -P ${CMAKE_SOURCE_DIR}/cmake/ExecuteOnAndroid.cmake)
else()
	add_test(NAME ${TESTUNIT4} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TESTUNIT4} -all)
endif()

# Sources
set(TESTUNIT5 "${LIBNAME}-file-testrunner5")
POCO_SOURCES_AUTO( TEST_SRCS_5 file_test_case_5.c)

add_executable( ${TESTUNIT5} ${TEST_SRCS_5} )
target_link_libraries( "${TESTUNIT5}" evglobals evsync evds evthrpool evio cutest "${SYSLIBS}")
if(ANDROID)
	add_test(NAME ${TESTUNIT5} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} 
		COMMAND ${CMAKE_COMMAND} -DANDROID_NDK=${ANDROID_NDK} -DLIBRARY_DIR=${CMAKE_BINARY_DIR}/lib -DUNITTEST=${CMAKE_BINARY_DIR}/bin/${TESTUNIT5} -DTEST_PARAMETER=-all -P ${CMAKE_SOURCE_DIR}/cmake/ExecuteOnAndroid.cmake)
else()
	add_test(NAME ${TESTUNIT5} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TESTUNIT5} -all)
endif()

