
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

    class SignalTarget {
    public:
        void add(int a, int b) {
            sum += a + b;
            calls++;
        }

        int sum = 0;
        int calls = 0;
    };

    class DerivedSignalTarget : public SignalTarget {};
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

    bool util_signal_test(const API &api) {
        // notify without subscribers
        util::signal<void(int, int)> empty;
        empty.notify(1, 2);

        auto t0 = std::make_shared<SignalTarget>();
        auto t1 = std::make_shared<DerivedSignalTarget>();
        auto t2 = std::make_shared<SignalTarget>();
        {
            util::signal<void(int, int)> sig;
            sig.subscribe(std::weak_ptr<SignalTarget>(t0), &SignalTarget::add);
            sig.subscribe(std::weak_ptr<DerivedSignalTarget>(t1), &SignalTarget::add);
            sig.subscribe(std::weak_ptr<SignalTarget>(t2), &SignalTarget::add);
            sig.subscribe(std::weak_ptr<SignalTarget>(t0), &SignalTarget::add);

            // signal doesn't own targets
            EXPECT_EQ(t0.use_count(), 1);
            EXPECT_EQ(t1.use_count(), 1);

            // every subscription is called, the same target twice
            sig.notify(1, 2);
            EXPECT_EQ(t0->calls, 2);
            EXPECT_EQ(t0->sum, 6);
            EXPECT_EQ(t1->calls, 1);
            EXPECT_EQ(t1->sum, 3);
            EXPECT_EQ(t2->calls, 1);

            // expired target is skipped, others are still called
            t2 = nullptr;
            sig.notify(10, 20);
            EXPECT_EQ(t0->calls, 4);
            EXPECT_EQ(t0->sum, 66);
            EXPECT_EQ(t1->calls, 2);
            EXPECT_EQ(t1->sum, 33);

            t0 = nullptr;
            t1 = nullptr;
            sig.notify(1, 1);
        }

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

    bool util_strstream_helpers_test(const API &api) {
        // shader-like signature
        std::string name, args, type;
        util::strstream signature ("  calc0(a(b) c)->float {", 24);
        EXPECT_TRUE(signature >> util::word(name) >> util::braced(args, '(', ')') >> util::sequence("->") >> type >> util::sequence("{"));
        EXPECT_EQ(name, "calc0");
        EXPECT_EQ(args, "a(b) c");
        EXPECT_EQ(type, "float");
        EXPECT_TRUE(signature.isEof());

        // nested braces and same opening/closing chars
        std::string nested, quoted;
        util::strstream braces (" {a{b{c}}d} \"x y\"", 17);
        EXPECT_TRUE(braces >> util::braced(nested, '{', '}') >> util::braced(quoted, '"', '"'));
        EXPECT_EQ(nested, "a{b{c}}d");
        EXPECT_EQ(quoted, "x y");

        // word stops at first non-alphanumeric char
        std::string w0, w1;
        util::strstream words ("abc12_def", 9);
        EXPECT_TRUE(words >> util::word(w0) >> util::sequence("_") >> util::word(w1));
        EXPECT_EQ(w0, "abc12");
        EXPECT_EQ(w1, "def");

        // nlist reads count then values and appends to output
        std::vector<int> ints = {9};
        std::vector<std::string> strings;
        util::strstream lists ("3 1 -2 0x10 2 aa bb", 19);
        EXPECT_TRUE(lists >> util::nlist(ints) >> util::nlist(strings));
        EXPECT_EQ(ints, std::vector<int>({9, 1, -2, 16}));
        EXPECT_EQ(strings, std::vector<std::string>({"aa", "bb"}));

        std::vector<float> empty;
        util::strstream emptyList ("0", 1);
        EXPECT_TRUE(emptyList >> util::nlist(empty));
        EXPECT_TRUE(empty.empty());

        return true;
    }

    bool util_strstream_helpers_errors_test(const API &api) {
        // sequence mismatch
        util::strstream badSequence ("-> x", 4);
        EXPECT_FALSE(badSequence >> util::sequence("=>"));
        util::strstream shortSequence ("-", 1);
        EXPECT_FALSE(shortSequence >> util::sequence("->"));

        // braced without opening or closing
        std::string out;
        util::strstream noOpening ("a}", 2);
        EXPECT_FALSE(noOpening >> util::braced(out, '{', '}'));
        util::strstream noClosing ("{a{b}", 5);
        EXPECT_FALSE(noClosing >> util::braced(out, '{', '}'));

        // braced content outside of the stream must stay invisible
        util::strstream limitedBraced ("{ab}", 3);
        EXPECT_FALSE(limitedBraced >> util::braced(out, '{', '}'));

        // word in empty stream
        std::string w;
        util::strstream blank ("   ", 3);
        EXPECT_FALSE(blank >> util::word(w));

        // nlist without count or with less values than count
        std::vector<int> ints;
        util::strstream noCount ("x 1", 3);
        EXPECT_FALSE(noCount >> util::nlist(ints));
        EXPECT_TRUE(ints.empty());
        util::strstream lessValues ("3 1 2", 5);
        EXPECT_FALSE(lessValues >> util::nlist(ints));
        EXPECT_EQ(ints, std::vector<int>({1, 2}));

        return true;
    }

    bool util_description_set_get_test(const API &api) {
        util::Description desc;
        EXPECT_TRUE(desc.setBool("b", true));
        EXPECT_TRUE(desc.setInteger("i", -5));
        EXPECT_TRUE(desc.setNumber("n", 2.5));
        EXPECT_TRUE(desc.setString("s", "abc"));
        EXPECT_TRUE(desc.setVector3f("v", math::vector3f(1.0f, 2.0f, 3.0f)));
        EXPECT_TRUE(desc.setVector3i("vi", math::vector3i(4, 5, 6)));
        EXPECT_EQ(desc.size(), 6u);

        EXPECT_EQ(*desc.getBool("b"), true);
        EXPECT_EQ(*desc.getInteger("i"), -5);
        EXPECT_EQ_F(*desc.getNumber("n"), 2.5f);
        EXPECT_EQ(*desc.getString("s"), "abc");
        EXPECT_EQ_F(desc.getVector3f("v")->z, 3.0f);
        EXPECT_EQ(desc.getVector3i("vi")->y, 5);

        // missing name or another type
        EXPECT_EQ(desc.getInteger("missing"), nullptr);
        EXPECT_EQ(desc.getInteger("n"), nullptr);
        EXPECT_EQ(desc.getVector3i("v"), nullptr);
        EXPECT_EQ(desc.getDescription("s"), nullptr);

        // type mismatch doesn't change anything
        EXPECT_FALSE(desc.setString("i", "x"));
        EXPECT_FALSE(desc.setNumber("i", 1.0, false));
        EXPECT_EQ(desc.setDescription("i"), nullptr);
        EXPECT_EQ(*desc.getInteger("i"), -5);
        EXPECT_EQ(desc.size(), 6u);

        // replace overwrites, no replace adds one more value
        EXPECT_TRUE(desc.setInteger("i", 7));
        EXPECT_EQ(desc.size(), 6u);
        EXPECT_EQ(*desc.getInteger("i"), 7);
        EXPECT_TRUE(desc.setInteger("i", 8, false));
        EXPECT_EQ(desc.size(), 7u);
        EXPECT_EQ(desc.getIntegers("i"), std::vector<std::int64_t>({7, 8}));

        // nested description is cleared only with replace
        desc.setDescription("d")->setInteger("x", 1);
        EXPECT_EQ(*desc.getDescription("d")->getInteger("x"), 1);
        EXPECT_EQ(desc.setDescription("d", false)->size(), 1u);
        EXPECT_TRUE(desc.setDescription("d")->empty());
        EXPECT_EQ(desc.size(), 8u);

        return true;
    }

    bool util_description_default_values_test(const API &api) {
        util::Description desc;
        desc.setInteger("i", 3);
        desc.setString("s", "abc");

        EXPECT_EQ(desc.getInteger("i", 9), 3);
        EXPECT_EQ(desc.getString("s", "def"), "abc");

        // missing name
        EXPECT_EQ(desc.getInteger("missing", 9), 9);
        EXPECT_EQ(desc.getBool("missing", true), true);
        EXPECT_EQ_F(desc.getNumber("missing", 1.5), 1.5f);
        EXPECT_EQ(desc.getString("missing", "def"), "def");
        EXPECT_EQ(desc.getVector2i("missing", math::vector2i(1, 2)).y, 2);

        // another type
        EXPECT_EQ(desc.getInteger("s", 9), 9);
        EXPECT_EQ(desc.getString("i", "def"), "def");
        EXPECT_EQ_F(desc.getNumber("i", 1.5), 1.5f);

        return true;
    }

    bool util_description_parse_test(const API &api) {
        const util::Description desc = util::Description::parse(
            "i : integer = -5\n"
            "n : number = 2.5\n"
            "b : bool = true\n"
            "s : string = \"a b\"\n"
            "v2f : vector2f = 1.5 -2\n"
            "v3f : vector3f = 1 2 3\n"
            "v4f : vector4f = 1 2 3 4.5\n"
            "v2i : vector2i = 1 2\n"
            "v3i : vector3i = 4 5 6\n"
            "v4i : vector4i = 0x10 -1 2 3\n"
            "ints : integer[3] = 1 2 3\n"
            "strs : string[2] = \"x\" \"y\"\n"
            "d {\n"
            "    x : integer = 1\n"
            "    inner { f : bool = false }\n"
            "}\n"
        );
        EXPECT_EQ(desc.size(), 16u);
        EXPECT_EQ(*desc.getInteger("i"), -5);
        EXPECT_EQ_F(*desc.getNumber("n"), 2.5f);
        EXPECT_EQ(*desc.getBool("b"), true);
        EXPECT_EQ(*desc.getString("s"), "a b");
        EXPECT_EQ_F(desc.getVector2f("v2f")->y, -2.0f);
        EXPECT_EQ_F(desc.getVector3f("v3f")->z, 3.0f);
        EXPECT_EQ_F(desc.getVector4f("v4f")->w, 4.5f);
        EXPECT_EQ(desc.getVector2i("v2i")->y, 2);
        EXPECT_EQ(desc.getVector3i("v3i")->z, 6);
        EXPECT_EQ(desc.getVector4i("v4i")->x, 16);
        EXPECT_EQ(desc.getVector4i("v4i")->y, -1);
        EXPECT_EQ(desc.getIntegers("ints"), std::vector<std::int64_t>({1, 2, 3}));
        EXPECT_EQ(desc.getStrings("strs"), std::vector<std::string>({"x", "y"}));

        const util::Description *d = desc.getDescription("d");
        EXPECT_TRUE(d != nullptr);
        EXPECT_EQ(*d->getInteger("x"), 1);
        EXPECT_EQ(*d->getDescription("inner")->getBool("f"), false);

        // any error makes the whole result empty
        EXPECT_TRUE(util::Description::parse("a : unknown = 1").empty());
        EXPECT_TRUE(util::Description::parse("a : integer[2] = 1").empty());
        EXPECT_TRUE(util::Description::parse("a : integer 1").empty());
        EXPECT_TRUE(util::Description::parse("d { a : integer = 1").empty());
        EXPECT_TRUE(util::Description::parse("d { a : integer = x }").empty());
        EXPECT_TRUE(util::Description::parse("a = 1").empty());

        return true;
    }

    bool util_description_serialize_test(const API &api) {
        util::Description src;
        src.setInteger("i", -5);
        src.setNumber("n", -2.25);
        src.setBool("b", false);
        src.setString("s", "a b");
        src.setVector2f("v2f", math::vector2f(1.5f, -2.0f));
        src.setVector3f("v3f", math::vector3f(1.0f, 2.0f, 3.0f));
        src.setVector4f("v4f", math::vector4f(1.0f, 2.0f, 3.0f, 4.5f));
        src.setVector2i("v2i", math::vector2i(1, 2));
        src.setVector3i("v3i", math::vector3i(4, 5, 6));
        src.setVector4i("v4i", math::vector4i(7, -1, 2, 3));
        src.setInteger("ints", 1, false);
        src.setInteger("ints", 2, false);
        src.setString("strs", "x", false);
        src.setString("strs", "y", false);
        src.setBool("bools", true, false);
        src.setBool("bools", false, false);
        util::Description *d = src.setDescription("d");
        d->setInteger("x", 1);
        d->setDescription("inner")->setString("t", "z");
        d->setDescription("empty");

        const util::Description desc = util::Description::parse(util::Description::serialize(src));
        EXPECT_EQ(desc.size(), src.size());
        EXPECT_EQ(*desc.getInteger("i"), -5);
        EXPECT_EQ_F(*desc.getNumber("n"), -2.25f);
        EXPECT_EQ(*desc.getBool("b"), false);
        EXPECT_EQ(*desc.getString("s"), "a b");
        EXPECT_EQ_F(desc.getVector2f("v2f")->y, -2.0f);
        EXPECT_EQ_F(desc.getVector3f("v3f")->z, 3.0f);
        EXPECT_EQ_F(desc.getVector4f("v4f")->w, 4.5f);
        EXPECT_EQ(desc.getVector2i("v2i")->y, 2);
        EXPECT_EQ(desc.getVector3i("v3i")->z, 6);
        EXPECT_EQ(desc.getVector4i("v4i")->y, -1);
        EXPECT_EQ(desc.getIntegers("ints"), std::vector<std::int64_t>({1, 2}));
        EXPECT_EQ(desc.getStrings("strs"), std::vector<std::string>({"x", "y"}));
        EXPECT_EQ(*desc.getBool("bools"), true);

        const util::Description *rd = desc.getDescription("d");
        EXPECT_TRUE(rd != nullptr);
        EXPECT_EQ(rd->size(), 3u);
        EXPECT_EQ(*rd->getInteger("x"), 1);
        EXPECT_EQ(*rd->getDescription("inner")->getString("t"), "z");
        EXPECT_TRUE(rd->getDescription("empty")->empty());

        return true;
    }
}


namespace test {
    void registerUtilTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(util_callback_test);
        REGISTER_UNIT_TEST(util_signal_test);
        REGISTER_UNIT_TEST(util_strstream_atoi_atof_test);
        REGISTER_UNIT_TEST(util_strstream_char_reading_test);
        REGISTER_UNIT_TEST(util_strstream_extraction_test);
        REGISTER_UNIT_TEST(util_strstream_extraction_errors_test);
        REGISTER_UNIT_TEST(util_strstream_helpers_test);
        REGISTER_UNIT_TEST(util_strstream_helpers_errors_test);
        REGISTER_UNIT_TEST(util_description_set_get_test);
        REGISTER_UNIT_TEST(util_description_default_values_test);
        REGISTER_UNIT_TEST(util_description_parse_test);
        REGISTER_UNIT_TEST(util_description_serialize_test);
    }
}
