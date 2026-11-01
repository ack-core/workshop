#!/bin/bash
set -e
BINARY=binary_mac_tests

cmake -S . -B ./workshop_mac_tests -G"Xcode" -DPLATFORM="PLATFORM_MAC" -DAPPTYPE="IS_TESTS"
xcodebuild -quiet -project workshop_mac_tests/workshop.xcodeproj -target app -configuration Debug ARCHS=$(uname -m) CODE_SIGNING_ALLOWED=NO build

RESULT=0
"$BINARY/workshop.app/Contents/MacOS/workshop" || RESULT=$?

python3 tools/refs2png.py -b "$BINARY"
exit "$RESULT"
