cmake -S . -B ./workshop_ios_tests -G"Xcode" -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain_ios.cmake" -DPLATFORM="PLATFORM_IOS" -DAPPTYPE="IS_TESTS"

