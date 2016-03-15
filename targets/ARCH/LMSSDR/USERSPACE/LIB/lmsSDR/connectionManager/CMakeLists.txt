set(ENABLE_USB_CONNECTION "YES" CACHE BOOL INTERNAL)
set(ENABLE_SPI_CONNECTION "NO" CACHE BOOL INTERNAL)
set(CONNECTION_MANAGER_DIR connectionManager)
set(connectionManager_src_files
	ConnectionManager.cpp
	ConnectionCOM.cpp
	ConnectionManager.h
	ConnectionCOM.h
	lmsComms.h
	lmsComms.cpp
)
if(ENABLE_USB_CONNECTION)
	list(APPEND connectionManager_src_files ConnectionUSB.cpp ConnectionUSB.h)
    add_definitions(-DENABLE_USB_CONNECTION)
endif()
if(ENABLE_SPI_CONNECTION)
	list(APPEND connectionManager_src_files ConnectionSPI.cpp ConnectionSPI.h)
    add_definitions(-DENABLE_SPI_CONNECTION)
endif()

add_library(ConnectionManager STATIC ${connectionManager_src_files})
target_include_directories(ConnectionManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32 AND ENABLE_USB_CONNECTION)
	find_package(CyAPI REQUIRED)
	LINK_DIRECTORIES(${CYAPI_LIBRARIES})	
	include_directories(${CYAPI_INCLUDE_DIRS})
	set(CONNECTION_MANAGER_LIBS ${CYAPI_LIBRARIES} SetupAPI)
	target_link_libraries(ConnectionManager ${CONNECTION_MANAGER_LIBS})
endif()
if(UNIX AND ENABLE_USB_CONNECTION)	
	set(CONNECTION_MANAGER_LIBS usb-1.0 -lpthread)
	target_link_libraries(ConnectionManager ${CONNECTION_MANAGER_LIBS})
endif()
