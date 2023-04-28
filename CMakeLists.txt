cmake_minimum_required(VERSION 3.20)    
project(basic-wm VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)

find_package(X11 REQUIRED)
find_package(glog 0.6.0 REQUIRED)

set(SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/")

add_executable(basic-wm ${SOURCES}/main.cpp)

add_library(WindowManager ${SOURCES}/wm/wm.cpp ${SOURCES}/wm/wm.hpp)
target_link_libraries(basic-wm PUBLIC glog X11 WindowManager) 


