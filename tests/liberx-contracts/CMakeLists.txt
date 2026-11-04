
add_executable(liberx_contracts contracts.c)
set_module_type(liberx_contracts win32cui)
add_dependencies(liberx_contracts psdk)
add_importlibs(liberx_contracts msvcrt kernel32 dwrite)

add_executable(liberx_dwrite_hittest dwrite-hittest.c)
set_module_type(liberx_dwrite_hittest win32cui)
add_dependencies(liberx_dwrite_hittest psdk)
add_importlibs(liberx_dwrite_hittest msvcrt kernel32 dwrite)
