
#include <sstream>
#include <iomanip>

namespace gears {
    template <typename = void> std::istream &expect(std::istream &stream) {
        return stream;
    }
    template <char Ch, char... Chs> std::istream &expect(std::istream &stream) {
        if ((stream >> std::ws).peek() == Ch) {
            stream.ignore();
        }
        else {
            stream.setstate(std::ios_base::failbit);
        }
        return expect<Chs...>(stream);
    }

    struct quoted {
        explicit quoted(std::string &out) : _out(out) {}

        inline friend std::istream &operator >>(std::istream &stream, const quoted &target) {
            (stream >> std::ws).peek();
            (stream >> std::ws).peek() == '\"' ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);

            while (stream.fail() == false && stream.peek() != '\"') {
                target._out.push_back(stream.get());
            }
            if (stream.fail() == false) {
                (stream >> std::ws).peek() == '\"' ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);
            }

            return stream;
        }

    private:
        std::string &_out;
    };
}
