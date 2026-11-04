
add_executable(liberx_contracts contracts.c)
set_module_type(liberx_contracts win32cui)
add_dependencies(liberx_contracts psdk)
add_importlibs(liberx_contracts msvcrt kernel32 dwrite xinput dinput8 hid setupapi)

add_executable(liberx_input_contracts input-contracts.c)
set_module_type(liberx_input_contracts win32cui)
add_dependencies(liberx_input_contracts psdk)
add_importlibs(liberx_input_contracts msvcrt kernel32 xinput dinput8 hid setupapi)
