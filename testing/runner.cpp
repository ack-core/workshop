
#include "runner.h"

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

    void TestRunner::registerScreenshotTest(const char* name, void (*testFunc)(const API &api)) {
        _screenshotTests.emplace_back(name, testFunc);
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
            if (_currentStepTest->getType() == StepTestInterface::StepType::ONE_FRAME || _api.platform->isAsyncWorkInProgress() == false) {
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

        // TODO: screenshot tests

        _api.platform->exit(_testsFailed);
    }
}
