@echo off
cmake -S . -B ./workshop_windows_tests -G"Visual Studio 17 2022" -DPLATFORM="PLATFORM_WINDOWS" -DAPPTYPE="IS_TESTS" || exit /b 1
cmake --build workshop_windows_tests --config Debug || exit /b 1
pushd binary_windows_tests
.\tests.exe
set "RESULT=%ERRORLEVEL%"
popd
python tools/refs2png.py -b binary_windows_tests
exit /b %RESULT%
