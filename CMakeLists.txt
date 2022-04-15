cmake_minimum_required(VERSION 3.0)
project(TinyChat)
# 如果代码需要支持C++11，就直接加上这句
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
# 如果想要生成的可执行文件拥有符号表，可以gdb调试，就直接加上这句
#add_definitions("-Wall -g")

######################################################################
# 规定.h头文件路径
include_directories(${PROJECT_SOURCE_DIR}/base)

# 编译子文件夹的CMakeLists.txt
add_subdirectory(base)

# 编译子文件夹的CMakeLists.txt
add_subdirectory(test)
