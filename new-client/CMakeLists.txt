cmake_minimum_required(VERSION 3.16)
project(AbyssClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network UiTools WebSockets)

qt_standard_project_setup()

add_executable(abyss-client
	main.cc
	src/app/app.cc
	includes/app/app.h
	src/windows/login.cc
	includes/windows/login/login.h
	src/net/net.cc
	includes/net/net.h
	src/net/ws.cc
	includes/net/ws.h
	src/input/input.cc
	includes/input/input.h
	includes/shell/message.h
	src/utils/utils.cc
	includes/utils/utils.h
)

target_include_directories(abyss-client PRIVATE includes)
target_link_libraries(abyss-client PRIVATE Qt6::Widgets Qt6::Network Qt6::UiTools Qt6::WebSockets)
