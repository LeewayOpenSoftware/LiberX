
add_executable(liberx_contracts contracts.c)
set_module_type(liberx_contracts win32cui)
add_dependencies(liberx_contracts psdk)
add_importlibs(liberx_contracts msvcrt kernel32 dwrite)

add_executable(liberx_dwrite_hittest dwrite-hittest.c)
set_module_type(liberx_dwrite_hittest win32cui)
add_dependencies(liberx_dwrite_hittest psdk)
add_importlibs(liberx_dwrite_hittest msvcrt kernel32 dwrite)

add_executable(liberx_d2d_geometry d2d-geometry.c)
set_module_type(liberx_d2d_geometry win32cui)
add_dependencies(liberx_d2d_geometry psdk)
add_importlibs(liberx_d2d_geometry msvcrt kernel32 d2d1 dwrite dxgi)
