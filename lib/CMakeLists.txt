macro(add_depend_lib name)
    add_custom_command(
          OUTPUT ${name}
          COMMAND tar xvf ${name}.tar.gz
          DEPEND ${name}.tar.gz)
    add_subdirectory(${name})
endmacro()
add_depend_lib(iniparser)
add_depend_lib(st)
