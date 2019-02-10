# Other compilers then MSVC don't have a static STATIC_POSTFIX at the moment
set(STATIC_POSTFIX "" CACHE STRING "Set static library postfix" FORCE)
set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG}   -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
# Add a d postfix to the debug libraries
if(EF_STATIC)
	set(CMAKE_DEBUG_POSTFIX "${STATIC_POSTFIX}d" CACHE STRING "Set Debug library postfix" FORCE)
	set(CMAKE_RELEASE_POSTFIX "${STATIC_POSTFIX}" CACHE STRING "Set Release library postfix" FORCE)
	set(CMAKE_MINSIZEREL_POSTFIX "${STATIC_POSTFIX}" CACHE STRING "Set MinSizeRel library postfix" FORCE)
	set(CMAKE_RELWITHDEBINFO_POSTFIX "${STATIC_POSTFIX}" CACHE STRING "Set RelWithDebInfo library postfix" FORCE)
else(EF_STATIC)
	set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Set Debug library postfix" FORCE)
	set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "Set Release library postfix" FORCE)
	set(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "Set MinSizeRel library postfix" FORCE)
	set(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "Set RelWithDebInfo library postfix" FORCE)
endif()

