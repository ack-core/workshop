
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
        const char *input = s;
        std::size_t length = 0;
        std::int64_t ipart = atoi(input, length);
        
        double power = ipart < 0 ? -1.0 : 1.0;
        double out = 0.0;
        
        if (length) {
            input += length;
            if (*input == '.') {
                while (std::isdigit(*++input)) {
                    power *= 10.0;
                    out += (*input - '0') / power;
                }
            }
        }
        
        len = input - s;
        return out + double(ipart);
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
    std::size_t strstream::ftow(std::uint16_t *p, double f) {
        std::uint16_t *output = p;
        const double af = std::abs(f);
        std::int64_t ipart = std::int64_t(af);
        double remainder = 10000000.0 * (af - double(ipart));
        std::int64_t fpart = std::int64_t(remainder);
        
        if ((remainder - double(fpart)) > 0.5) {
            fpart++;
            if (fpart > 10000000) {
                fpart = 0;
                ipart++;
            }
        }
        
        if (f < 0.0) *output++ = '-';
        output += ltow(output, ipart);
        
        int width = 10;
        while (fpart % 10 == 0 && width-- > 0) {
            fpart /= 10;
        }
        *output++ = '.';
        
        output += ltow(output, fpart);
        return output - p;
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
        std::string result = std::string(48, 0);
        char *output = result.data();
        
        const double af = std::abs(f);
        std::int64_t ipart = std::int64_t(af);
        
        double scale = std::pow(10.0, precision);
        double remainder = scale * (af - double(ipart));
        std::int64_t fpart = std::int64_t(remainder);
        
        if ((remainder - double(fpart)) > 0.5) {
            fpart++;
            if (fpart > 10000000) {
                fpart = 0;
                ipart++;
            }
        }
        
        if (f < 0.0) *output++ = '-';
        output += ltoa(output, ipart);
  
        if (precision > 0) {
            *output++ = '.';

            std::int64_t power = 1;
            for (int i = 1; i < precision; ++i) power *= 10;
            while (fpart < power && power > 1) {
                *output++ = '0';
                power /= 10;
            }

            output += ltoa(output, fpart);
        }
        
        result.resize(output - result.data());
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
                            result += std::string(ident, ' ') + item.first + " : string = " + *v + "\r\n";
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
