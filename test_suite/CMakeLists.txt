cmake_minimum_required(VERSION 3.14)
project(TEST_SUITE)

set(CMAKE_MODE_PATH "$(PROJECT_SOURCE_DIR)")

find_package(wxWidgets COMPONENTS core base REQUIRED)
include("${wxWidgets_USE_FILE}")
set(SRCS
	main.cc main.h
	../src/TestSuite.h ../src/TestSuite.cc		
	../src/id_cat.h
	../src/TextWindow.h ../src/TextWindow.cc
	)
if(WIN32)
	set(
		SRCS ${SRCS}
		wxWidgetTest.rc
		main.cc main.h
		../src/TestSuite.h ../src/TestSuite.cc
		../src/id_cat.h
		../src/TextWindow.h ../src/TextWindow.cc
	)

endif(WIN32)

add_executable(${PROJECT_NAME} WIN32 ${SRCS})
target_link_libraries(${PROJECT_NAME} ${wxWidgets_LIBRARIES})

