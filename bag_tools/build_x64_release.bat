CALL "c:\Program Files\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsx86_amd64.bat"

SET "INCLUDE=%INCLUDE%;%~dp0..\..\..\..\lz4-1.9.3\lib"
SET "LIB=%LIB%;%~dp0..\..\..\..\lz4-1.9.3\build\VS2019\bin\x64_Release"
SET UseEnv=true

cd "%~dp0"
msbuild.exe "%~dp0bag_tools.sln" /property:Platform=x64 /property:Configuration=Release
