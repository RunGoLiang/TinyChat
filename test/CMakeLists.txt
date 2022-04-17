project(test)

#add_executable(testThreadPool testThreadPool.cpp)
#target_link_libraries(testThreadPool base)

add_executable(serverTest serverTest.cpp)
target_link_libraries(serverTest base)

add_executable(clientTest clientTest.cpp)
target_link_libraries(clientTest base)