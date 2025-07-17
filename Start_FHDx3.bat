pushd %~dp0
cd x64/Debug
MainProcess.exe --primary 1920x1080 --secondary 1920x1080 --tertiary 1920x1080 --msaa
popd