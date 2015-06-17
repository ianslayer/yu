@echo off

set compilerFlags=/MP -Od -Zi -MTd -nologo -fp:fast -Gm- -GR- -EHa- -Zo -Oi  -W4 -wd4201 -wd4100 -wd4512 -wd4324 -wd4189 -wd4127 -wd4505
set linkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib

IF NOT EXIST ..\build mkdir ..\build

pushd ..\build

cl %compilerFlags% /Feyu.exe ^
					..\src\core\core_win32.cpp ^
					..\src\core\crc32.cpp ^
					..\src\renderer\gl_win32.cpp ^
					..\src\renderer\gl3w.cpp ^
					..\src\renderer\renderer_gl.cpp ^
					..\src\renderer\renderer_dx11.cpp ^
					..\src\renderer\shader_compiler_dx11.cpp ^
					..\src\main_win32.cpp ^
					..\src\yu.cpp ^
					/link %linkerFlags% /MAP:yu_core.map

echo WAITING FOR PDB > lock.tmp
cl %compilerFlags% 	..\src\module\test_module.cpp -LD /link -incremental:no -PDB:yu_%random%.pdb -EXPORT:ModuleUpdate
del lock.tmp

popd