project(test)

#add_executable(testThreadPool testThreadPool.cpp)
#target_link_libraries(testThreadPool base)

add_executable(testTcpServer01 testTcpServer01.cpp)
target_link_libraries(testTcpServer01 base)

add_executable(testTcpServerClient01 testTcpServerClient01.cpp)
target_link_libraries(testTcpServerClient01 base)