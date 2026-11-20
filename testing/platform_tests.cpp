
#include "runner.h"

namespace test {
    bool platform_uniquty_test(const API &api) {
        EXPECT_EQ(api.platform.get(), foundation::PlatformInterface::instance().get());
        return true;
    }
    bool platform_screen_scale_test(const API &api) {
        EXPECT_LT(0.0f, api.platform->getScreenScale());
        return true;
    }
    bool platform_keyboard_event_handlers_test(const API &api) {
        using Args = foundation::PlatformKeyboardEventArgs;
        std::string calls;
        bool consume = false;

        const foundation::EventHandlerToken first = api.platform->addKeyboardEventHandler([&](const Args &args) {
            calls += args.type == Args::EventType::PRESS ? "1P" : "1R";
            calls += args.key == foundation::Key::K ? "K" : "?";
            return false;
        }, true);
        const foundation::EventHandlerToken second = api.platform->addKeyboardEventHandler([&](const Args &args) {
            calls += "2";
            return consume;
        }, true);

        EXPECT_NOT_EQ(first, foundation::INVALID_EVENT_TOKEN);
        EXPECT_NOT_EQ(second, foundation::INVALID_EVENT_TOKEN);

        api.platform->sendKeyboardEvent(Args { Args::EventType::PRESS, foundation::Key::K });
        EXPECT_EQ(calls, "21PK");

        calls.clear();
        consume = true;
        api.platform->sendKeyboardEvent(Args { Args::EventType::RELEASE, foundation::Key::K });
        EXPECT_EQ(calls, "2");

        calls.clear();
        api.platform->removeEventHandler(second);
        api.platform->sendKeyboardEvent(Args { Args::EventType::RELEASE, foundation::Key::K });
        EXPECT_EQ(calls, "1RK");

        calls.clear();
        api.platform->removeEventHandler(first);
        api.platform->sendKeyboardEvent(Args { Args::EventType::PRESS, foundation::Key::K });
        EXPECT_TRUE(calls.empty());
        return true;
    }
    bool platform_pointer_event_handlers_test(const API &api) {
        using Args = foundation::PlatformPointerEventArgs;
        std::string calls;
        bool consume = false;

        const foundation::EventHandlerToken first = api.platform->addPointerEventHandler([&](const Args &args) {
            calls += "1";
            calls += args.type == Args::EventType::START ? "S" : args.type == Args::EventType::MOVE ? "M" : args.type == Args::EventType::FINISH ? "F" : "?";
            calls += args.pointerID == 0 ? "0" : "?";
            calls += args.coordinateX == 10.0f && args.coordinateY == 20.0f ? "C" : "?";
            calls += args.flags.isLeftButton ? "L" : "";
            calls += args.flags.isRightButton ? "R" : "";
            return false;
        }, true);
        const foundation::EventHandlerToken second = api.platform->addPointerEventHandler([&](const Args &args) {
            calls += "2";
            return consume;
        }, true);

        EXPECT_NOT_EQ(first, foundation::INVALID_EVENT_TOKEN);
        EXPECT_NOT_EQ(second, foundation::INVALID_EVENT_TOKEN);

        Args args {};
        args.pointerID = 0;
        args.coordinateX = 10.0f;
        args.coordinateY = 20.0f;
        args.type = Args::EventType::START;
        args.flags.isLeftButton = 1;
        api.platform->sendPointerEvent(args);
        EXPECT_EQ(calls, "21S0CL");

        calls.clear();
        args.type = Args::EventType::MOVE;
        args.flags.isLeftButton = 0;
        args.flags.isRightButton = 1;
        api.platform->sendPointerEvent(args);
        EXPECT_EQ(calls, "21M0CR");

        calls.clear();
        consume = true;
        args.type = Args::EventType::FINISH;
        args.flags.isRightButton = 0;
        args.flags.isLeftButton = 1;
        api.platform->sendPointerEvent(args);
        EXPECT_EQ(calls, "2");

        calls.clear();
        api.platform->removeEventHandler(second);
        api.platform->sendPointerEvent(args);
        EXPECT_EQ(calls, "1F0CL");

        calls.clear();
        api.platform->removeEventHandler(first);
        api.platform->sendPointerEvent(args);
        EXPECT_TRUE(calls.empty());
        return true;
    }
    bool platform_editor_event_handlers_test(const API &api) {
        std::string calls;
        bool consume = false;

        const foundation::EventHandlerToken first = api.platform->addEditorEventHandler([&](const std::string &msg, const std::string &data) {
            calls += "1" + msg + data;
            return false;
        });
        const foundation::EventHandlerToken second = api.platform->addEditorEventHandler([&](const std::string &msg, const std::string &data) {
            calls += "2" + msg + data;
            return consume;
        }, true);

        EXPECT_NOT_EQ(first, foundation::INVALID_EVENT_TOKEN);
        EXPECT_NOT_EQ(second, foundation::INVALID_EVENT_TOKEN);

        api.platform->editorLoopbackMsg("m", "d");
        EXPECT_EQ(calls, "2md1md");

        calls.clear();
        consume = true;
        api.platform->editorLoopbackMsg("m", "d");
        EXPECT_EQ(calls, "2md");

        calls.clear();
        api.platform->removeEventHandler(second);
        api.platform->editorLoopbackMsg("x", "y");
        EXPECT_EQ(calls, "1xy");

        calls.clear();
        api.platform->removeEventHandler(first);
        api.platform->editorLoopbackMsg("m", "d");
        EXPECT_TRUE(calls.empty());
        return true;
    }
}

namespace test {
    class platform_file_loading_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<platform_file_loading_test>();
        }
        void init(const API &api) override {
            api.platform->loadFile("platform_tests/file_5.txt", [this](ByteDataPtr &&data, std::size_t size) {
                if (data) {
                    _content.assign(reinterpret_cast<const char *>(data.get()), size);
                }
            }, true);
            _loadTasksCount = api.platform->getStatistics().fileLoadTasksCount;
        }
        bool testResult(const API &api) override {
            EXPECT_EQ(_loadTasksCount, 1);
            EXPECT_EQ(api.platform->getStatistics().fileLoadTasksCount, 0);
            EXPECT_EQ(_content.size(), 5);
            EXPECT_EQ(_content, "test5");
            return true;
        }

    private:
        std::size_t _loadTasksCount = 0;
        std::string _content;
    };
}

namespace test {
    class platform_file_save_and_loading_chain_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<platform_file_save_and_loading_chain_test>();
        }
        void init(const API &api) override {
            const char *content = "write-read";
            api.platform->saveFile("platform_tests/file_saved.txt", reinterpret_cast<const std::uint8_t *>(content), 10, [this, platform = api.platform](bool result) {
                _saved = result;
                platform->loadFile("platform_tests/file_saved.txt", [this](ByteDataPtr &&data, std::size_t size) {
                    if (data) {
                        _content.assign(reinterpret_cast<const char *>(data.get()), size);
                    }
                }, true);
            }, true);
            _saveTasksCount = api.platform->getStatistics().fileSaveTasksCount;
        }
        bool testResult(const API &api) override {
            EXPECT_EQ(_saveTasksCount, 1);
            EXPECT_EQ(api.platform->getStatistics().fileSaveTasksCount, 0);
            EXPECT_EQ(api.platform->getStatistics().fileLoadTasksCount, 0);
            EXPECT_TRUE(_saved);
            EXPECT_EQ(_content.size(), 10);
            EXPECT_EQ(_content, "write-read");
            return true;
        }

    private:
        std::size_t _saveTasksCount = 0;
        bool _saved = false;
        std::string _content;
    };
}

namespace test {
    class platform_file_delete_and_loading_fail_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<platform_file_delete_and_loading_fail_test>();
        }
        void init(const API &api) override {
            api.platform->saveFile("platform_tests/file_saved.txt", nullptr, 0, [this, platform = api.platform](bool result) {
                _deleted = result;
                platform->loadFile("platform_tests/file_saved.txt", [this](ByteDataPtr &&data, std::size_t size) {
                    _loaded = true;
                    _failed = data == nullptr && size == 0;
                }, true);
            }, true);
        }
        bool testResult(const API &api) override {
            EXPECT_EQ(api.platform->getStatistics().fileSaveTasksCount, 0);
            EXPECT_EQ(api.platform->getStatistics().fileLoadTasksCount, 0);
            EXPECT_TRUE(_deleted);
            EXPECT_TRUE(_loaded);
            EXPECT_TRUE(_failed);
            return true;
        }

    private:
        bool _deleted = false;
        bool _loaded = false;
        bool _failed = false;
    };
}

namespace test {
    class platform_execute_async_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<platform_execute_async_test>();
        }
        void init(const API &api) override {
            struct Context {
                int result = 0;
            };
            api.platform->executeAsync(std::make_unique<foundation::CommonAsyncTask<Context>>([](Context &ctx) {
                ctx.result = 42;
            },
            [this](Context &ctx) {
                _result = ctx.result;
            }));
        }
        bool testResult(const API &api) override {
            EXPECT_EQ(_result, 42);
            return true;
        }

    private:
        int _result = 0;
    };
}


namespace test {
    void registerPlatformTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(platform_uniquty_test);
        REGISTER_UNIT_TEST(platform_screen_scale_test);
        REGISTER_UNIT_TEST(platform_keyboard_event_handlers_test);
        REGISTER_UNIT_TEST(platform_pointer_event_handlers_test);
        REGISTER_UNIT_TEST(platform_editor_event_handlers_test);
        REGISTER_STEP_TEST(platform_file_loading_test);
        REGISTER_STEP_TEST(platform_file_save_and_loading_chain_test);
        REGISTER_STEP_TEST(platform_file_delete_and_loading_fail_test);
        REGISTER_STEP_TEST(platform_execute_async_test);
    }
}
