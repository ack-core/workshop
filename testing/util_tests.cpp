
#include "util_tests.h"

namespace {
    int g_tmp_call_counter = 0;

    class CallbackTarget {
    public:
        int add(int v) {
            _sum += v;
            return _sum;
        }

    private:
        int _sum = 0;
    };
}

namespace test {
    bool util_callback_test(const API &api) {
        // empty
        util::callback<int(int)> empty;
        EXPECT_FALSE(empty);
        EXPECT_EQ(empty(5), 0);

        // stateless lambda
        g_tmp_call_counter = 0;
        util::callback<void()> cb0 = []() {
            g_tmp_call_counter++;
        };
        EXPECT_TRUE(cb0);
        cb0();
        cb0();
        EXPECT_EQ(g_tmp_call_counter, 2);

        // captured fields are saved by value
        int base = 10;
        const std::string text = "abc";
        util::callback<int(int)> cb1 = [base, text](int v) {
            return base + v + int(text.length());
        };
        base = 100;
        EXPECT_EQ(cb1(5), 18);

        // mutable lambda keeps its state between calls
        util::callback<int()> cb2 = [n = 0]() mutable {
            return ++n;
        };
        EXPECT_EQ(cb2(), 1);
        EXPECT_EQ(cb2(), 2);
        EXPECT_EQ(cb2(), 3);

        // captured objects are released with callback
        auto token = std::make_shared<int>(7);
        {
            util::callback<int()> cb3 = [token]() {
                return *token;
            };
            EXPECT_EQ(token.use_count(), 2);
            EXPECT_EQ(cb3(), 7);
        }
        EXPECT_EQ(token.use_count(), 1);

        // move ctor and move assignment
        util::callback<int(int)> cb4 = [](int v) {
            return v * 2;
        };
        util::callback<int(int)> cb5 (std::move(cb4));
        EXPECT_FALSE(cb4);
        EXPECT_TRUE(cb5);
        EXPECT_EQ(cb5(4), 8);

        util::callback<int(int)> cb6 = [](int v) {
            return v * 3;
        };
        cb6 = std::move(cb5);
        EXPECT_FALSE(cb5);
        EXPECT_EQ(cb6(4), 8);

        // call and reset
        EXPECT_EQ(cb6.callAndReset(5), 10);
        EXPECT_FALSE(cb6);
        EXPECT_EQ(cb6(5), 0);

        // weak object + method
        auto target = std::make_shared<CallbackTarget>();
        util::callback<int(int)> cb7 (std::weak_ptr<CallbackTarget>(target), &CallbackTarget::add);
        EXPECT_EQ(cb7(2), 2);
        EXPECT_EQ(cb7(3), 5);
        target = nullptr;
        EXPECT_TRUE(cb7);
        EXPECT_EQ(cb7(3), 0);

        return true;
    }

    bool util_strstream_atoi_atof_test(const API &api) {
        std::size_t len = 0;

        // atoi
        EXPECT_EQ(util::strstream::atoi("123", len), 123);
        EXPECT_EQ(len, 3u);
        EXPECT_EQ(util::strstream::atoi("0", len), 0);
        EXPECT_EQ(len, 1u);
        EXPECT_EQ(util::strstream::atoi("-45", len), -45);
        EXPECT_EQ(len, 3u);
        EXPECT_EQ(util::strstream::atoi("+45", len), 45);
        EXPECT_EQ(len, 3u);
        EXPECT_EQ(util::strstream::atoi("0x1F", len), 31);
        EXPECT_EQ(len, 4u);
        EXPECT_EQ(util::strstream::atoi("0xff", len), 255);
        EXPECT_EQ(len, 4u);
        EXPECT_EQ(util::strstream::atoi("-0x10", len), -16);
        EXPECT_EQ(len, 5u);
        EXPECT_EQ(util::strstream::atoi("9000000000", len), 9000000000ll);
        EXPECT_EQ(len, 10u);
        EXPECT_EQ(util::strstream::atoi("42abc", len), 42);
        EXPECT_EQ(len, 2u);
        EXPECT_EQ(util::strstream::atoi("12 34", len), 12);
        EXPECT_EQ(len, 2u);
        EXPECT_EQ(util::strstream::atoi("abc", len), 0);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ(util::strstream::atoi("", len), 0);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ(util::strstream::atoi("+", len), 0);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ(util::strstream::atoi("-", len), 0);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ(util::strstream::atoi("+x", len), 0);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ(util::strstream::atoi("0x", len), 0);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ(util::strstream::atoi("-0x", len), 0);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ(util::strstream::atoi("0xg", len), 0);
        EXPECT_EQ(len, 0u);

        // atof
        EXPECT_EQ_F(util::strstream::atof("1.5", len), 1.5f);
        EXPECT_EQ(len, 3u);
        EXPECT_EQ_F(util::strstream::atof("42", len), 42.0f);
        EXPECT_EQ(len, 2u);
        EXPECT_EQ_F(util::strstream::atof("-0.25", len), -0.25f);
        EXPECT_EQ(len, 5u);
        EXPECT_EQ_F(util::strstream::atof("+3.75", len), 3.75f);
        EXPECT_EQ(len, 5u);
        EXPECT_EQ_F(util::strstream::atof(".5", len), 0.5f);
        EXPECT_EQ(len, 2u);
        EXPECT_EQ_F(util::strstream::atof("7.", len), 7.0f);
        EXPECT_EQ(len, 2u);
        EXPECT_EQ_F(util::strstream::atof("3.14159", len), 3.14159f);
        EXPECT_EQ(len, 7u);
        EXPECT_EQ_F(util::strstream::atof("2.5x", len), 2.5f);
        EXPECT_EQ(len, 3u);
        EXPECT_EQ_F(util::strstream::atof("1.25 2", len), 1.25f);
        EXPECT_EQ(len, 4u);
        EXPECT_LT(util::strstream::atof("0.1", len), util::strstream::atof("0.2", len));
        EXPECT_LT(util::strstream::atof("-1", len), util::strstream::atof("1", len));
        EXPECT_EQ_F(util::strstream::atof("abc", len), 0.0f);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ_F(util::strstream::atof("", len), 0.0f);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ_F(util::strstream::atof("+", len), 0.0f);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ_F(util::strstream::atof("-", len), 0.0f);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ_F(util::strstream::atof(".", len), 0.0f);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ_F(util::strstream::atof("-.", len), 0.0f);
        EXPECT_EQ(len, 0u);
        EXPECT_EQ_F(util::strstream::atof("+.x", len), 0.0f);
        EXPECT_EQ(len, 0u);

        return true;
    }
}


namespace test {
    void registerUtilTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(util_callback_test);
        REGISTER_UNIT_TEST(util_strstream_atoi_atof_test);
    }
}
