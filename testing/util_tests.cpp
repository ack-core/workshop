
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

    bool util_strstream_char_reading_test(const API &api) {
        // '#' is outside of the stream and must stay invisible
        const char *text = " \t\n ab c#";
        util::strstream stream (text, 8);
        EXPECT_FALSE(stream.isEof());

        // skipws stops at first printable char
        stream.skipws();
        EXPECT_EQ(stream.current(), text + 4);

        // peekChar doesn't advance, getChar does
        EXPECT_EQ(stream.peekChar(), 'a');
        EXPECT_EQ(stream.peekChar(), 'a');
        EXPECT_EQ(stream.current(), text + 4);
        EXPECT_EQ(stream.getChar(), 'a');
        EXPECT_EQ(stream.current(), text + 5);
        EXPECT_EQ(stream.getChar(), 'b');

        stream.skipws();
        EXPECT_EQ(stream.current(), text + 7);
        stream.skipws();
        EXPECT_EQ(stream.current(), text + 7);
        EXPECT_FALSE(stream.isEof());
        EXPECT_EQ(stream.getChar(), 'c');

        // end of stream
        EXPECT_TRUE(stream.isEof());
        EXPECT_TRUE(stream);
        EXPECT_EQ(stream.peekChar(), 0);
        EXPECT_EQ(stream.getChar(), 0);
        EXPECT_EQ(stream.current(), text + 8);

        // skipws stops at the end of stream
        const char *blank = "  #";
        util::strstream spaces (blank, 2);
        spaces.skipws();
        EXPECT_EQ(spaces.current(), blank + 2);
        EXPECT_TRUE(spaces.isEof());

        // empty stream
        util::strstream empty ("", 0);
        EXPECT_TRUE(empty.isEof());
        EXPECT_TRUE(empty);
        EXPECT_EQ(empty.getChar(), 0);

        // error state is independent of position and stays until the end
        util::strstream errors ("ab", 2);
        EXPECT_TRUE(errors);
        EXPECT_FALSE(errors.isError());
        errors.setError();
        EXPECT_TRUE(errors.isError());
        EXPECT_FALSE(errors);
        EXPECT_EQ(errors.getChar(), 'a');
        EXPECT_FALSE(errors.isEof());
        EXPECT_TRUE(errors.isError());
        EXPECT_FALSE(errors);

        return true;
    }

    bool util_strstream_extraction_test(const API &api) {
        // integers of different types separated by any whitespace
        int negative = 0, hex = 0;
        std::int64_t large = 0;
        std::uint8_t byte = 0;
        util::strstream integers ("-12\t0x1F\n 9000000000 200", 24);
        EXPECT_TRUE(integers >> negative >> hex >> large >> byte);
        EXPECT_EQ(negative, -12);
        EXPECT_EQ(hex, 31);
        EXPECT_EQ(large, 9000000000ll);
        EXPECT_EQ(byte, 200);
        EXPECT_TRUE(integers.isEof());

        // floats
        float f0 = 0.0f, f1 = 0.0f;
        double d = 0.0;
        util::strstream floats ("1.5 -0.25 7", 11);
        EXPECT_TRUE(floats >> f0 >> d >> f1);
        EXPECT_EQ_F(f0, 1.5f);
        EXPECT_EQ_F(d, -0.25f);
        EXPECT_EQ_F(f1, 7.0f);
        EXPECT_TRUE(floats.isEof());

        // strings are split by whitespace only and replace the previous value
        std::string s0 = "old", s1;
        util::strstream strings ("  foo,bar\tbaz", 13);
        EXPECT_TRUE(strings >> s0 >> s1);
        EXPECT_EQ(s0, "foo,bar");
        EXPECT_EQ(s1, "baz");

        // different types in a row, number ends at the first non-number char
        int i = 0;
        std::string s2;
        float f2 = 0.0f;
        util::strstream mixed ("12abc 2.5", 9);
        EXPECT_TRUE(mixed >> i >> s2 >> f2);
        EXPECT_EQ(i, 12);
        EXPECT_EQ(s2, "abc");
        EXPECT_EQ_F(f2, 2.5f);

        return true;
    }

    bool util_strstream_extraction_errors_test(const API &api) {
        int i = 7;
        float f = 7.0f;
        std::string s = "old";

        // bad input sets error, keeps the value and doesn't advance
        const char *bad = "  abc";
        util::strstream badInt (bad, 5);
        EXPECT_FALSE(badInt >> i);
        EXPECT_TRUE(badInt.isError());
        EXPECT_EQ(i, 7);
        EXPECT_EQ(badInt.current(), bad + 2);

        util::strstream badFloat (bad, 5);
        EXPECT_FALSE(badFloat >> f);
        EXPECT_EQ_F(f, 7.0f);
        EXPECT_EQ(badFloat.current(), bad + 2);

        // no string in stream
        util::strstream blank ("   ", 3);
        EXPECT_FALSE(blank >> s);
        EXPECT_EQ(s, "");

        // error stays in a chain, the rest of values are not touched
        int a = 0, b = 5, c = 5;
        util::strstream chain ("1 x 3", 5);
        EXPECT_FALSE(chain >> a >> b >> c);
        EXPECT_EQ(a, 1);
        EXPECT_EQ(b, 5);
        EXPECT_EQ(c, 5);

        // chars outside of the stream must stay invisible
        util::strstream limitedInt ("12345", 2);
        EXPECT_TRUE(limitedInt >> i);
        EXPECT_EQ(i, 12);
        EXPECT_TRUE(limitedInt.isEof());
        EXPECT_FALSE(limitedInt >> i);
        EXPECT_EQ(i, 12);

        util::strstream limitedFloat ("1.5678 9", 3);
        EXPECT_TRUE(limitedFloat >> f);
        EXPECT_EQ_F(f, 1.5f);
        EXPECT_TRUE(limitedFloat.isEof());

        util::strstream limitedString ("abcd", 2);
        EXPECT_TRUE(limitedString >> s);
        EXPECT_EQ(s, "ab");

        return true;
    }
}


namespace test {
    void registerUtilTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(util_callback_test);
        REGISTER_UNIT_TEST(util_strstream_atoi_atof_test);
        REGISTER_UNIT_TEST(util_strstream_char_reading_test);
        REGISTER_UNIT_TEST(util_strstream_extraction_test);
        REGISTER_UNIT_TEST(util_strstream_extraction_errors_test);
    }
}
