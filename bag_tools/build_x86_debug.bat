CALL "c:\Program Files\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat"

SET "INCLUDE=%INCLUDE%;%~dp0..\..\..\..\lz4-1.9.3\lib"
SET "LIB=%LIB%;%~dp0..\..\..\..\lz4-1.9.3\build\VS2019\bin\Win32_Debug"
SET UseEnv=true

cd "%~dp0"
msbuild.exe "%~dp0bag_tools.sln" /property:Platform=x86 /property:Configuration=Debug
