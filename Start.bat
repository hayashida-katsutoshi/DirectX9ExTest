pushd %~dp0
cd x64/Debug
MainProcess.exe --primary 2160x3840 --secondary 1920x1080 --msaa
popd