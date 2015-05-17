include (configure/ConfigSystemInfo.cmake.${UNAME_INFO})
add_library(st event.c io.c key.c sched.c stk.c sync.c)
