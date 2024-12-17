cmake_minimum_required(VERSION 3.0)
project(mymuduo)

#设置库的生成位置
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#添加调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fPIC")



aux_source_directory(. SRC_LIST)

add_library(mymuduo SHARED ${SRC_LIST})