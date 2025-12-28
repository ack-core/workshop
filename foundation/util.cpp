
#include "util.h"
#include "math.h"

namespace util {
    std::int64_t strstream::atoi(const char *s, std::size_t &len) {
        const char *input = s;
        std::int64_t sign = 1;
        std::int64_t out = 0;
        
        if (*input == '+') input++;
        if (*input == '-' && std::isdigit(*(input + 1))) {
            sign = -1;
            input++;
        }
        while (std::isdigit(*input)) {
            out = out * 10 + (*input - '0');
            input++;
        }
        
        len = input - s;
        return out * sign;
    }
    double strstream::atof(const char *s, std::size_t &len) {
        const char *p = s;

        int sign = 1;
        if (*p == '+' || *p == '-') {
            if (*p == '-') {
                sign = -1;
            }
            ++p;
        }

        double value = 0.0;

        while (std::isdigit(static_cast<unsigned char>(*p))) {
            value = value * 10.0 + (*p - '0');
            ++p;
        }

        if (*p == '.') {
            ++p;
            double factor = 0.1;
            while (std::isdigit(static_cast<unsigned char>(*p))) {
                value += (*p - '0') * factor;
                factor *= 0.1;
                ++p;
            }
        }

        len = static_cast<std::size_t>(p - s);
        return sign * value;
    }
    std::size_t strstream::ptow(std::uint16_t *p, const void *ptr) {
        const int len = 2 * sizeof(std::size_t);
        std::uint16_t *output = p + len - 1;
        std::size_t n = std::size_t(ptr);
        
        for (int i = 0; i < len; i++) {
            *output-- = "0123456789ABCDEF"[n % 16];
            n /= 16;
        }
        return len;
    }
    std::size_t strstream::ltow(std::uint16_t *p, std::int64_t value) {
        std::int64_t absvalue = value < 0 ? -value : value;
        std::uint16_t *output = p;

        do {
            *output++ = "0123456789ABCDEF"[absvalue % 10];
            absvalue /= 10;
        }
        while (absvalue);

        if (value < 0) *output++ = '-';
        std::size_t result = output - p;
        output--;
        
        while(p < output) {
            char tmp = *output;
            *output-- = *p;
            *p++ = tmp;
        }

        return result;
    }
    std::size_t strstream::ftow(std::uint16_t *p, double f, int precision) {
        std::uint16_t *out = p;

        if (f < 0.0) {
            f = -f;
            *out++ = '-';
        }

        double rounding = 0.5;
        for (int i = 0; i < precision; ++i) {
            rounding *= 0.1;
        }
        f += rounding;

        std::uint64_t ipart = static_cast<std::uint64_t>(f);
        double frac = f - ipart;

        if (ipart == 0) {
            *out++ = '0';
        }
        else {
            std::uint16_t *start = out;
            while (ipart > 0) {
                *out++ = std::uint16_t('0' + (ipart % 10));
                ipart /= 10;
            }
            for (std::uint16_t *a = start, *b = out - 1; a < b; ++a, --b) {
                std::swap(*a, *b);
            }
        }

        if (precision > 0) {
            *out++ = '.';
            for (int i = 0; i < precision; ++i) {
                frac *= 10.0;
                int digit = static_cast<int>(frac);
                *out++ = std::uint16_t('0' + digit);
                frac -= digit;
            }
        }

        return static_cast<std::size_t>(out - p);
    }
    std::size_t strstream::ltoa(char *p, std::int64_t value) {
        std::int64_t absvalue = value < 0 ? -value : value;
        char *output = p;

        do {
            *output++ = "0123456789ABCDEF"[absvalue % 10];
            absvalue /= 10;
        }
        while (absvalue);

        if (value < 0) *output++ = '-';
        std::size_t result = output - p;
        output--;
        
        while(p < output) {
            char tmp = *output;
            *output-- = *p;
            *p++ = tmp;
        }

        return result;
    }
    std::string strstream::ftos(double f, int precision) {
        bool negative = f < 0.0;
        if (negative) {
            f = -f;
        }

        double rounding = 0.5;
        for (int i = 0; i < precision; i++) {
            rounding *= 0.1;
        }
        f += rounding;

        std::int64_t ipart = static_cast<std::int64_t>(f);
        double frac = f - ipart;

        std::string result;
        if (negative) {
            result.push_back('-');
        }

        if (ipart == 0) {
            result.push_back('0');
        }
        else {
            std::string tmp;
            while (ipart > 0) {
                tmp.push_back(char('0' + ipart % 10));
                ipart /= 10;
            }
            result.append(tmp.rbegin(), tmp.rend());
        }

        if (precision > 0) {
            result.push_back('.');
            for (int i = 0; i < precision; ++i) {
                frac *= 10.0;
                int digit = static_cast<int>(frac);
                result.push_back(char('0' + digit));
                frac -= digit;
            }
        }

        return result;
    }
}

namespace util {
    Description parseDescription(const std::uint8_t *data, std::size_t length) {
        struct fn {
            static bool parseScope(const std::string &src, Description& result) {
                strstream input(src.data(), src.length());
                std::string elementName;
                
                while (input >> elementName) {
                    input.skipws();
                    
                    if (input.peekChar() == '{') {
                        std::string scopeText;
                        
                        if (input >> braced(scopeText, '{', '}')) {
                            Description block = {};
                            if (fn::parseScope(scopeText, block)) {
                                result.emplace(elementName, std::move(block));
                            }
                            else return {};
                        }
                        else return {};
                    }
                    else if (input.getChar() == ':') {
                        std::string type;
                        std::string valueString;
                        float valueFloat[4] = {};
                        int valueInt = 0;

                        if (input >> type) {
                            if (type == "string" && input >> sequence("=") >> braced(valueString, '"', '"')) {
                                auto r = result.emplace(std::make_pair(elementName, std::make_any<std::string>(std::move(valueString))));                                
                            }
                            else if (type == "integer" && input >> sequence("=") >> valueInt) {
                                result.emplace(std::make_pair(elementName, std::make_any<std::int64_t>(valueInt)));
                            }
                            else if (type == "number" && input >> sequence("=") >> valueFloat[0]) {
                                result.emplace(std::make_pair(elementName, std::make_any<double>(valueFloat[0])));
                            }
                            else if (type == "bool" && input >> sequence("=") >> valueString) {
                                result.emplace(std::make_pair(elementName, std::make_any<bool>(valueString == "true")));
                            }
                            else if (type == "vector2f" && input >> sequence("=") >> valueFloat[0] >> valueFloat[1]) {
                                result.emplace(std::make_pair(elementName, std::make_any<math::vector2f>(valueFloat[0], valueFloat[1])));
                            }
                            else if (type == "vector3f" && input >> sequence("=") >> valueFloat[0] >> valueFloat[1] >> valueFloat[2]) {
                                result.emplace(std::make_pair(elementName, std::make_any<math::vector3f>(valueFloat[0], valueFloat[1], valueFloat[2])));
                            }
                            else if (type == "vector4f" && input >> sequence("=") >> valueFloat[0] >> valueFloat[1] >> valueFloat[2] >> valueFloat[3]) {
                                result.emplace(std::make_pair(elementName, std::make_any<math::vector4f>(valueFloat[0], valueFloat[1], valueFloat[2], valueFloat[3])));
                            }
                            else return false;
                        }
                        else return false;
                    }
                    else return false;
                }
                
                return true;
            }
        };
        
        Description result = {};
        if (fn::parseScope(std::string((const char *)data, length), result)) {
            return result;
        }
        else return {};
    }

    std::string serializeDescription(const Description &cfg) {
        struct fn {
            static bool serializeScope(int ident, const Description &cfg, std::string &result) {
                for (auto &item : cfg) {
                    if (const Description *block = std::any_cast<Description>(&item.second)) {
                        result += std::string(ident, ' ') + item.first + " {\r\n";
                        if (fn::serializeScope(ident + 4, *block, result)) {
                            result += std::string(ident, ' ') + "}\r\n\r\n";
                        }
                        else return false;
                    }
                    else {
                        if (const std::string *v = std::any_cast<std::string>(&item.second)) {
                            result += std::string(ident, ' ') + item.first + " : string = \"" + *v + "\"\r\n";
                        }
                        else if (const std::int64_t *v = std::any_cast<std::int64_t>(&item.second)) {
                            result += std::string(ident, ' ') + item.first + " : integer = " + std::to_string(*v) + "\r\n";
                        }
                        else if (const double *v = std::any_cast<double>(&item.second)) {
                            result += std::string(ident, ' ') + item.first + " : number = " + strstream::ftos(*v) + "\r\n";
                        }
                        else if (const bool *v = std::any_cast<bool>(&item.second)) {
                            result += std::string(ident, ' ') + item.first + " : bool = " + ((*v) ? "true\r\n" : "false\r\n");
                        }
                        else if (const math::vector2f *v = std::any_cast<math::vector2f>(&item.second)) {
                            const std::string stringValue = strstream::ftos(v->x) + " " + strstream::ftos(v->y);
                            result += std::string(ident, ' ') + item.first + " : vector2f = " + stringValue + "\r\n";
                        }
                        else if (const math::vector3f *v = std::any_cast<math::vector3f>(&item.second)) {
                            const std::string stringValue = strstream::ftos(v->x) + " " + strstream::ftos(v->y) + " " + strstream::ftos(v->z);
                            result += std::string(ident, ' ') + item.first + " : vector3f = " + stringValue + "\r\n";
                        }
                        else if (const math::vector4f *v = std::any_cast<math::vector4f>(&item.second)) {
                            const std::string stringValue = strstream::ftos(v->x) + " " + strstream::ftos(v->y) + " " + strstream::ftos(v->z) + " " + strstream::ftos(v->w);
                            result += std::string(ident, ' ') + item.first + " : vector4f = " + stringValue + "\r\n";
                        }
                        else return false;
                    }
                }
                
                return true;
            }
        };
        
        std::string result;
        if (fn::serializeScope(0, cfg, result)) {
            return result;
        }
        else return {};
    }
}
