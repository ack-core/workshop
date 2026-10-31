#!/bin/bash
set -e
BUNDLE=com.ack-games.workshop
BINARY=binary_ios_tests

UDID=$(xcrun simctl list devices available | grep -m1 iPhone | grep -oE '[0-9A-F-]{36}')
xcrun simctl bootstatus "$UDID" -b > /dev/null &
BOOT=$!

cmake -S . -B ./workshop_ios_tests -G"Xcode" -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain_ios.cmake" -DPLATFORM="PLATFORM_IOS" -DAPPTYPE="IS_TESTS"
xcodebuild -project workshop_ios_tests/workshop.xcodeproj -target app -configuration Debug -sdk iphonesimulator ARCHS=$(uname -m) CODE_SIGNING_ALLOWED=NO build

wait $BOOT
xcrun simctl install "$UDID" "$BINARY/workshop.app"
DATA=$(xcrun simctl get_app_container "$UDID" "$BUNDLE" data)
rm -rf "$DATA"/Documents/*
xcrun simctl launch --console-pty "$UDID" "$BUNDLE" || true

RESULT=$(cat "$DATA/Documents/exit_code" 2>/dev/null || echo 1)
cp "$DATA"/Documents/rendering_tests/*.tga "$BINARY/data/rendering_tests/" 2>/dev/null || true
python3 tools/refs2png.py -b "$BINARY"
exit "$RESULT"
