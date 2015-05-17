add_compile_options(-Wall -rdynamic -Wformat-security -g)
add_executable(panda_server panda_server.cpp panda_debug.cpp panda_procedure.cpp)
#add_executable(panda_monitor )
target_link_libraries(panda_server iniparser st execinfo)
