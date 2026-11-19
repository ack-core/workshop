
#include "runner.h"
#include "thirdparty/upng/upng.h"
#include <cstdlib>
#include <cstring>

namespace {
    // Decodes RGBA8 PNG of TESTING_RENDER_WIDTH x TESTING_RENDER_HEIGHT to RGBA top-down pixels
    // @return nullptr if data is missing or has different format/size
    //
    ByteDataPtr decodeScreenshotPNG(const std::uint8_t *data, std::size_t size) {
        const std::size_t w = foundation::TESTING_RENDER_WIDTH;
        const std::size_t h = foundation::TESTING_RENDER_HEIGHT;

        if (data == nullptr || size < 4 || *reinterpret_cast<const std::uint32_t *>(data) != UPNG_HEAD) {
            return nullptr;
        }

        ByteDataPtr result;
        upng_t *upng = upng_new_from_bytes(data, (unsigned long)(size));

        if (upng != nullptr) {
            if (upng_decode(upng) == UPNG_EOK && upng_get_format(upng) == UPNG_RGBA8 && upng_get_width(upng) == w && upng_get_height(upng) == h) {
                result = std::make_unique<std::uint8_t[]>(w * h * 4);
                std::memcpy(result.get(), upng_get_buffer(upng), w * h * 4);
            }
            upng_free(upng);
        }

        return result;
    }
}

namespace test {
    TestRunner::TestRunner(
        const foundation::PlatformInterfacePtr& platform,
        const foundation::RenderingInterfacePtr& rendering,
        const resource::ResourceProviderPtr& resourceProvider,
        const core::SceneInterfacePtr& scene,
        const core::RaycastInterfacePtr& raycast,
        const core::SimulationInterfacePtr& simulation
    )
    : _api{platform, rendering, resourceProvider, scene, raycast, simulation}
    {

    }

    TestRunner::~TestRunner() {

    }

    void TestRunner::registerUnitTest(const char* name, bool (*testFunc)(const API &api)) {
        _unitTests.emplace_back(name, testFunc);
    }

    void TestRunner::registerStepTest(const char* name, std::unique_ptr<StepTestInterface>(*makeTestInstance)()) {
        _stepTests.emplace_back(name, makeTestInstance);
    }

    void TestRunner::registerPixelTest(const char* name, std::unique_ptr<PixelTestInterface>(*makeTestInstance)()) {
        _pixelTests.emplace_back(name, makeTestInstance);
    }

    void TestRunner::registerScreenshotTest(const char* name, std::unique_ptr<ScreenshotTestInterface>(*makeTestInstance)()) {
        _screenshotTests.emplace_back(name, makeTestInstance);
    }

    void TestRunner::nextFrame(float dtSec) {
        _api.resourceProvider->update(dtSec);
        _api.raycast->update(dtSec);
        _api.simulation->update(dtSec);
        _api.scene->updateAndDraw(dtSec);

        // Unit tests
        if (_unitTestIndex < _unitTests.size()) {
            for (std::size_t i = 0; i < MAX_UNIT_TESTS_PER_FRAME && _unitTestIndex < _unitTests.size(); i++, _unitTestIndex++) {
                const auto &test = _unitTests[_unitTestIndex];

                if (test.second(_api)) {
                    //_api.platform->logMsg("[PASSED UNIT TEST] '%s'", test.first.data());
                }
                else {
                    _api.platform->logMsg("[FAILED UNIT TEST] '%s'", test.first.data());
                    _testsFailed++;
                }
            }
            return;
        }

        // Step tests
        if (_currentStepTest) {
            if (_api.platform->isAsyncWorkInProgress() == false) {
                if (_currentStepTest->testResult(_api)) {
                    //_api.platform->logMsg("[PASSED STEP TEST] '%s'", _stepTests[_stepTestIndex].first.data());
                }
                else {
                    _api.platform->logMsg("[FAILED STEP TEST] '%s'", _stepTests[_stepTestIndex].first.data());
                    _testsFailed++;
                }
                _currentStepTest.reset();
                _stepTestIndex++;
            }
            return;
        }
        if (_stepTestIndex < _stepTests.size()) {
            const auto &test = _stepTests[_stepTestIndex];
            _currentStepTest = test.second();
            _currentStepTest->init(_api);
            return;
        }

        // Pixel tests
        if (_currentPixelTest) {
            _currentPixelTest->updateAndDraw(_api);
            ByteDataPtr frame = _api.rendering->presentFrame();

            if (_api.platform->isAsyncWorkInProgress() == false) {
                const std::string &name = _pixelTests[_pixelTestIndex].first;

                if (frame == nullptr) {
                    _api.platform->logMsg("[FAILED PIXEL TEST] '%s' : frame capture isn't supported", name.data());
                    _testsFailed++;
                }
                else if (_screenshotRef) {
                    if (_currentPixelTest->testResult(_api, _screenshotRef.get(), frame.get())) {
                        //_api.platform->logMsg("[PASSED PIXEL TEST] '%s'", name.data());
                    }
                    else {
                        _api.platform->logMsg("[FAILED PIXEL TEST] '%s', frame saved", name.data());
                        _testsFailed++;
                        saveScreenshot(name, frame.get(), "_fail");
                    }
                }
                else {
                    _api.platform->logMsg("[PIXEL TEST] '%s' has no reference, ref saved", name.data());
                    saveScreenshot(name, frame.get(), "_ref");
                }

                _currentPixelTest.reset();
                _screenshotRef = nullptr;
                _pixelTestIndex++;
            }
            return;
        }
        if (_pixelTestIndex < _pixelTests.size()) {
            const auto &test = _pixelTests[_pixelTestIndex];
            _currentPixelTest = test.second();
            _currentPixelTest->init(_api);
            loadScreenshotRef(test.first);
            return;
        }

        // Screenshot tests
        if (_currentScreenshotTest) {
            _currentScreenshotTest->updateAndDraw(_api);
            ByteDataPtr frame = _api.rendering->presentFrame();

            if (_api.platform->isAsyncWorkInProgress() == false) {
                const std::string &name = _screenshotTests[_screenshotTestIndex].first;

                if (frame == nullptr) {
                    _api.platform->logMsg("[FAILED SCREENSHOT TEST] '%s' : frame capture isn't supported", name.data());
                    _testsFailed++;
                }
                else if (_screenshotRef) {
                    const std::size_t size = foundation::TESTING_RENDER_WIDTH * foundation::TESTING_RENDER_HEIGHT * 4;
                    const int threshold = _currentScreenshotTest->getThreshold();
                    ByteDataPtr diff = std::make_unique<std::uint8_t[]>(size);
                    bool passed = true;

                    for (std::size_t i = 0; i < size; i++) {
                        diff[i] = std::uint8_t(std::abs(int(frame[i]) - int(_screenshotRef[i])));
                        passed = passed && diff[i] <= threshold;
                    }

                    if (passed == false) {
                        _api.platform->logMsg("[FAILED SCREENSHOT TEST] '%s', frame and diff saved", name.data());
                        _testsFailed++;
                        saveScreenshot(name, frame.get(), "_fail");
                        saveScreenshot(name, diff.get(), "_diff");
                    }
                }
                else {
                    _api.platform->logMsg("[SCREENSHOT TEST] '%s' has no reference, ref saved", name.data());
                    saveScreenshot(name, frame.get(), "_ref");
                }

                _currentScreenshotTest.reset();
                _screenshotRef = nullptr;
                _screenshotTestIndex++;
            }
            return;
        }
        if (_screenshotTestIndex < _screenshotTests.size()) {
            const auto &test = _screenshotTests[_screenshotTestIndex];
            _currentScreenshotTest = test.second();
            _currentScreenshotTest->init(_api);
            loadScreenshotRef(test.first);
            return;
        }

        // Wait for the last frame saving
        if (_api.platform->isAsyncWorkInProgress() == false) {
            _api.platform->exit(_testsFailed);
        }
    }

    void TestRunner::loadScreenshotRef(const std::string &name) {
        _api.platform->loadFile(("rendering_tests/" + name + "_ref.png").data(), [this](ByteDataPtr &&data, std::size_t size) {
            _screenshotRef = decodeScreenshotPNG(data.get(), size);
        }, true);
    }

    void TestRunner::saveScreenshot(const std::string &name, const std::uint8_t *frame, const char *suffix) {
        auto tga = util::writeTGA(frame, foundation::TESTING_RENDER_WIDTH, foundation::TESTING_RENDER_HEIGHT);
        const std::uint8_t *data = tga.first.get();
        _api.platform->saveFile(("rendering_tests/" + name + suffix + ".tga").data(), data, tga.second, [this, name, suffix, tga = std::move(tga.first)](bool saved) {
            if (saved == false) {
                _api.platform->logMsg("[TEST] '%s' %s saving failed", name.data(), suffix);
            }
        }, true);
    }
}
