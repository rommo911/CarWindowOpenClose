cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)
set(SOURCES avdweb_Switch.cpp main.cpp Config.cpp)
idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS ".") 