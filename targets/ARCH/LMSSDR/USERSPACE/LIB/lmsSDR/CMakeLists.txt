cmake_minimum_required(VERSION 2.8)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE TYPE INTERNAL FORCE)
project("matlab_StreamDLL")
#include modules for finding CyAPI
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
cmake_policy(SET CMP0015 OLD)
if(${CMAKE_MAJOR_VERSION} GREATER 2)
	cmake_policy(SET CMP0043 NEW)
endif()
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
if(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -fPIC")
endif()

# Add sub-directories
add_subdirectory(LMS_StreamBoard)
include_directories(LMS_StreamBoard)
add_subdirectory(connectionManager)
include_directories(connectionManager)

set(BINARY_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
	
if (MSVC)
	SET( MSVC_MULTITHREAD_COMPILE_FLAGS "/MP")
	SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${MSVC_MULTITHREAD_COMPILE_FLAGS}" )
endif(MSVC)

set(Library_file_list LMS_SDR.cpp )

set(BUILD_STATIC_LIB OFF CACHE BOOL "Build static or dynamic library")
if( BUILD_STATIC_LIB )
	add_library(LMS_SDR STATIC ${Library_file_list})
else()
	add_library(LMS_SDR SHARED ${Library_file_list})
endif(BUILD_STATIC_LIB)

set(AUTO_UPDATE_VERSION OFF CACHE BOOL "Uses python script to increment version")
if(AUTO_UPDATE_VERSION)
	add_custom_target(updateVersion ALL COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/auto_version.py WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})	
	add_dependencies(LMS_SDR updateVersion)
endif()

target_link_libraries(LMS_SDR
	LMS_StreamBoard 
	${CYAPI_LIBRARIES}
	)
	
if(WIN32)
	find_package(CyAPI REQUIRED)
	LINK_DIRECTORIES(${CYAPI_LIBRARIES})	
	include_directories(${CYAPI_INCLUDE_DIRS})
	set(CONNECTION_MANAGER_LIBS ${CYAPI_LIBRARIES} SetupAPI)	
endif()
if(UNIX)	
	set(CONNECTION_MANAGER_LIBS usb-1.0 -lpthread)	
endif()

target_link_libraries(LMS_SDR ${CONNECTION_MANAGER_LIBS})
