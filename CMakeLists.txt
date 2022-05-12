cmake_minimum_required(VERSION 3.2.0)

project(efio)

#IF(APPLE)
#SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
# set the value of CMAKE_OSX_ARCHITECTURES appropriately via -D commandline of cmake generation step
#ENDIF(APPLE)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_STANDARD 14)
set(CMAKE_INSTALL_PREFIX /usr/local)

file(STRINGS "${PROJECT_SOURCE_DIR}/libversion" SHARED_LIBRARY_VERSION)

# Read the version information from the VERSION file
file(STRINGS "${PROJECT_SOURCE_DIR}/VERSION" PACKAGE_VERSION)
message(STATUS "EF package version: ${PACKAGE_VERSION}")
string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" CPACK_PACKAGE_VERSION_MAJOR ${PACKAGE_VERSION})
string(REGEX REPLACE "[0-9]+\\.([0-9])+\\.[0-9]+.*" "\\1" CPACK_PACKAGE_VERSION_MINOR ${PACKAGE_VERSION})
string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" CPACK_PACKAGE_VERSION_PATCH ${PACKAGE_VERSION})

set(COMPLETE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
set(RELEASE_NAME "Unstable-trunk")
set(PROJECT_VERSION ${COMPLETE_VERSION})

# Put the libaries and binaries that get built into directories at the
# top of the build tree rather than in hard-to-find leaf
# directories. This simplifies manual testing and the use of the build
# tree rather than installed Boost libraries.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# Windows DLLs are "runtime" for CMake. Output them to "bin" like the Visual Studio projects do.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Append our module directory to CMake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

#IF(APPLE)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch arm64 -arch x86_64 ")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -arch arm64 -arch x86_64 ")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -arch arm64 -arch x86_64 ")
#set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -arch arm64 -arch x86_64 ")
#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -arch arm64 -arch x86_64 ")

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch arm64 -arch x86_64 ")
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -arch arm64 -arch x86_64 ")
#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -arch arm64 -arch x86_64 ")
#set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -arch arm64 -arch x86_64 ")
#set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -arch arm64 -arch x86_64 ")
#ENDIF (APPLE)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address ")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address ")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fsanitize=address ")
# set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fsanitize=address ")
# set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fsanitize=address ")

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address ")
# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address ")
# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fsanitize=address ")
# set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -fsanitize=address ")
# set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fsanitize=address ")

include(CXX1x)
check_for_cxx11_compiler(CXX11_COMPILER)
# If a C++11 compiler is available, then set the appropriate flags
if(CXX11_COMPILER)
	enable_cxx11()
	check_for_cxx14_compiler(CXX14_COMPILER)
	# If a C++14 compiler is available, then set the appropriate flags
	if(CXX14_COMPILER)
		enable_cxx14()
	endif()
endif()
message(STATUS "CXX11_COMPILER = " ${CXX11_COMPILER})
message(STATUS "CXX14_COMPILER = " ${CXX14_COMPILER})
message(STATUS "CXX_COMPILER = " ${CMAKE_CXX_COMPILER_ID})

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address ")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address ")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fsanitize=address ")
# set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fsanitize=address ")
# set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fsanitize=address ")
# 
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address ")
# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address ")
# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fsanitize=address ")
# set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -fsanitize=address ")
# set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fsanitize=address ")

if(NOT MSVC_IDE)
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: None Debug Release" FORCE)
	endif()
	message(STATUS "Setting EF build type - ${CMAKE_BUILD_TYPE}")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

message(STATUS "HI ${CMAKE_HOST_SYSTEM_NAME}")
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	string(APPEND CMAKE_C_FLAGS "-D_GNU_SOURCE -Wno-format-zero-length -DEV_DEBUG")
	string(APPEND CMAKE_EXE_LINKER_FLAGS "-pthread")
else (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	string(APPEND CMAKE_C_FLAGS "-DEV_DEBUG")
endif()

include(PocoMacros)
#include(GNU)


option(EF_STATIC "Set to OFF|ON (default is OFF) " OFF)

if(MSVC)
	option(EF_MT
		"Set to OFF|ON (default is OFF) to control build as /MT instead of /MD" OFF)

	option(ENABLE_MSVC_MP
		"Set to OFF|ON (default is OFF) to control parallel build with MSVC" OFF)
endif()

option(ENABLE_TESTS
	"Set to OFF|ON (default is OFF) to control build of EF tests & samples" OFF)

if(EF_STATIC)
	set(LIB_MODE_DEFINITIONS -DEF_STATIC -DEF_NO_AUTOMATIC_LIBS)
	set(LIB_MODE STATIC)
	message(STATUS "Building static libraries")
else(EF_STATIC)
	set(LIB_MODE SHARED)
	set(LIB_MODE_DEFINITIONS -DEF_NO_AUTOMATIC_LIBS)
	message(STATUS "Building dynamic libraries")
endif(EF_STATIC)

include(DefinePlatformSpecifc)

if(ENABLE_TESTS)
	option(ENABLE_LONG_RUNNING_TESTS "Enable long running test" ON)
	include(CTest)
	enable_testing()
	message(STATUS "Building with unittests & samples")
else()
	message(STATUS "Building without tests & samples")
endif()

# Collect the built libraries and include dirs, they will be used to create the EFConfig.cmake file

set(EF_COMPONENTS "")

if(ENABLE_TESTS)
	add_subdirectory(cutest)
endif()

# evglobals
add_subdirectory(evglobals)
list(APPEND EF_COMPONENTS "evglobals")
# core_utils
add_subdirectory(core_utils)
list(APPEND EF_COMPONENTS "core_utils")
# evsync
add_subdirectory(evsync)
list(APPEND EF_COMPONENTS "evsync")
# evds
add_subdirectory(evds)
list(APPEND EF_COMPONENTS "evds")
# evthrpool
add_subdirectory(evthrpool)
list(APPEND EF_COMPONENTS "evthrpool")
# evio
add_subdirectory(evio)
list(APPEND EF_COMPONENTS "evio")
#add_subdirectory(cutest)
#list(APPEND EF_COMPONENTS "cutest")

#############################################################
# Uninstall stuff see: http://www.vtk.org/Wiki/CMake_FAQ
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

#############################################################
# Enable packaging

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "EF Libraries")
set(CPACK_PACKAGE_VENDOR "Tekelinght Solutions Pvt Ltd")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
#set(CPACK_PACKAGE_INSTALL_DIRECTORY "/usr/local")

include(CPack)

#############################################################
# cmake config files

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/${PROJECT_NAME}Config.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake" @ONLY)

install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake
	DESTINATION
		"lib${LIB_SUFFIX}/cmake/${PROJECT_NAME}"
	COMPONENT
		Devel
)

message(STATUS "CMake ${CMAKE_VERSION} successfully configured ${PROJECT_NAME} using ${CMAKE_GENERATOR} generator")
message(STATUS "Installation target path: ${CMAKE_INSTALL_PREFIX}")

message(STATUS "C_FLAGS:  =${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_C_FLAGS_DEBUG:=${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CMAKE_C_FLAGS_RELEASE:=${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CMAKE_C_FLAGS_MINSIZEREL:=${CMAKE_C_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_C_FLAGS_RELWITHDEBINFO:=${CMAKE_C_FLAGS_RELWITHDEBINFO}")
message(STATUS "")
message(STATUS "")
message(STATUS "CXX_FLAGS:=${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG:=${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE:=${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_MINSIZEREL:=${CMAKE_CXX_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO:=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

foreach(component ${EF_COMPONENTS})
message(STATUS "Building: ${component}")
endforeach()

