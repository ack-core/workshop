
#include "runner.h"

namespace {
    // Decodes uncompressed 32-bit TGA of TESTING_RENDER_WIDTH x TESTING_RENDER_HEIGHT to RGBA top-down pixels
    // @return nullptr if data is missing or has different format/size
    //
    ByteDataPtr decodeScreenshotTGA(const std::uint8_t *data, std::size_t size) {
        const std::size_t w = foundation::TESTING_RENDER_WIDTH;
        const std::size_t h = foundation::TESTING_RENDER_HEIGHT;

        if (data == nullptr || size < 18) {
            return nullptr;
        }
        const std::size_t width = std::size_t(data[12]) | (std::size_t(data[13]) << 8);
        const std::size_t height = std::size_t(data[14]) | (std::size_t(data[15]) << 8);
        const std::size_t offset = 18 + data[0];

        if (data[2] != 2 || data[16] != 32 || width != w || height != h || size < offset + w * h * 4) {
            return nullptr;
        }

        const bool topDown = (data[17] & 0x20) != 0;
        ByteDataPtr result = std::make_unique<std::uint8_t[]>(w * h * 4);

        for (std::size_t y = 0; y < h; y++) {
            const std::uint8_t *src = data + offset + (topDown ? y : h - 1 - y) * w * 4;
            std::uint8_t *dst = result.get() + y * w * 4;

            for (std::size_t x = 0; x < w; x++) {
                dst[x * 4 + 0] = src[x * 4 + 2];
                dst[x * 4 + 1] = src[x * 4 + 1];
                dst[x * 4 + 2] = src[x * 4 + 0];
                dst[x * 4 + 3] = src[x * 4 + 3];
            }
        }

        return result;
    }
}

namespace test {
    TestRunner::TestRunner(
        const foundation::PlatformInterfacePtr& platform,
        const foundation::RenderingInterfacePtr& rendering,
        const resource::ResourceProviderPtr& resourceProvider,
        const resource::FontAtlasProviderPtr& fontAtlasProvider,
        const core::SceneInterfacePtr& scene,
        const core::WorldInterfacePtr& world,
        const core::RaycastInterfacePtr& raycast,
        const core::SimulationInterfacePtr& simulation,
        const ui::StageInterfacePtr& ui
    ) : _api{platform, rendering, resourceProvider, fontAtlasProvider, scene, world, raycast, simulation, ui} {

    }

    TestRunner::~TestRunner() {

    }

    void TestRunner::registerUnitTest(const char* name, bool (*testFunc)(const API &api)) {
        _unitTests.emplace_back(name, testFunc);
    }

    void TestRunner::registerStepTest(const char* name, std::unique_ptr<StepTestInterface>(*makeTestInstance)()) {
        _stepTests.emplace_back(name, makeTestInstance);
    }

    void TestRunner::registerScreenshotTest(const char* name, std::unique_ptr<ScreenshotTestInterface>(*makeTestInstance)()) {
        _screenshotTests.emplace_back(name, makeTestInstance);
    }

    void TestRunner::nextFrame() {
        // Unit tests
        if (_unitTestIndex < _unitTests.size()) {
            for (std::size_t i = 0; i < MAX_UNIT_TESTS_PER_FRAME && _unitTestIndex < _unitTests.size(); i++, _unitTestIndex++) {
                const auto &test = _unitTests[_unitTestIndex];

                if (test.second(_api)) {
                    _api.platform->logMsg("[UNIT TEST] '%s' passed", test.first.data());
                }
                else {
                    _api.platform->logMsg("[UNIT TEST] '%s' failed", test.first.data());
                    _testsFailed++;
                }
            }
            return;
        }

        // Step tests
        if (_currentStepTest) {
            if (_api.platform->isAsyncWorkInProgress() == false) {
                if (_currentStepTest->testResult(_api)) {
                    _api.platform->logMsg("[STEP TEST] '%s' passed", _stepTests[_stepTestIndex].first.data());
                }
                else {
                    _api.platform->logMsg("[STEP TEST] '%s' failed", _stepTests[_stepTestIndex].first.data());
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

        // Screenshot tests
        if (_currentScreenshotTest) {
            _currentScreenshotTest->updateAndDraw(_api);
            ByteDataPtr frame = _api.rendering->presentFrame();

            if (_api.platform->isAsyncWorkInProgress() == false) {
                const std::string &name = _screenshotTests[_screenshotTestIndex].first;

                if (frame == nullptr) {
                    _api.platform->logMsg("[SCREENSHOT TEST] '%s' failed : frame capture isn't supported", name.data());
                    _testsFailed++;
                }
                else if (_screenshotRef && _currentScreenshotTest->testResult(_api, _screenshotRef.get(), frame.get())) {
                    _api.platform->logMsg("[SCREENSHOT TEST] '%s' passed", name.data());
                }
                else {
                    if (_screenshotRef) {
                        _api.platform->logMsg("[SCREENSHOT TEST] '%s' failed", name.data());
                        _testsFailed++;
                    }
                    else {
                        _api.platform->logMsg("[SCREENSHOT TEST] '%s' has no reference, ref saved", name.data());
                    }

                    auto tga = util::writeTGA(frame.get(), foundation::TESTING_RENDER_WIDTH, foundation::TESTING_RENDER_HEIGHT);
                    _screenshotFrameTGA = std::move(tga.first);
                    _api.platform->saveFile(("rendering_tests/" + name + "_ref.tga").data(), _screenshotFrameTGA.get(), tga.second, [this, name](bool saved) {
                        if (saved == false) {
                            _api.platform->logMsg("[SCREENSHOT TEST] '%s' ref saving failed", name.data());
                        }
                        _screenshotFrameTGA = nullptr;
                    });
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
            _api.platform->loadFile(("rendering_tests/" + test.first + "_ref.tga").data(), [this](ByteDataPtr &&data, std::size_t size) {
                _screenshotRef = decodeScreenshotTGA(data.get(), size);
            });
            return;
        }

        // Wait for the last frame saving
        if (_api.platform->isAsyncWorkInProgress() == false) {
            _api.platform->exit(_testsFailed);
        }
    }
}
