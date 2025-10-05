
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
    std::string strstream::ftos(double f) {
        std::string result = std::string(48, 0);
        char *output = result.data();
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
        output += ltoa(output, ipart);
        
        int width = 10;
        while (fpart % 10 == 0 && width-- > 0) {
            fpart /= 10;
        }
        *output++ = '.';
        
        output += ltoa(output, fpart);
        result.resize(output - result.data());
        return result;
    }
}

namespace util {
    Config parseConfig(const std::uint8_t *data, std::size_t length, const std::string &name, std::string &error) {
        strstream input(reinterpret_cast<const char *>(data), length);
        Config result;
        std::string scopeName;
        std::string scopeText;
        error = {};
        
        struct fn {
            static bool parseScope(const std::string &name, const std::string &src, Config& result, std::string &error) {
                strstream input(src.data(), src.length());
                std::string elementName;
                
                while (input >> elementName) {
                    std::string type;
                    std::string valueString;
                    float valueFloat[3] = {};
                    int valueInt = 0;
                    
                    if (input >> sequence(":") >> type) {
                        if (type == "string" && input >> sequence("=") >> braced(valueString, '"', '"')) {
                            result.emplace(std::make_pair(elementName, std::make_any<std::string>(std::move(valueString))));
                        }
                        else if (type == "integer" && input >> sequence("=") >> valueInt) {
                            result.emplace(std::make_pair(elementName, std::make_any<int>(valueInt)));
                        }
                        else if (type == "number" && input >> sequence("=") >> valueFloat[0]) {
                            result.emplace(std::make_pair(elementName, std::make_any<float>(valueFloat[0])));
                        }
                        else if (type == "vector2f" && input >> sequence("=") >> valueFloat[0] >> valueFloat[1]) {
                            result.emplace(std::make_pair(elementName, std::make_any<math::vector2f>(valueFloat[0], valueFloat[1])));
                        }
                        else if (type == "vector3f" && input >> sequence("=") >> valueFloat[0] >> valueFloat[1] >> valueFloat[2]) {
                            result.emplace(std::make_pair(elementName, std::make_any<math::vector3f>(valueFloat[0], valueFloat[1], valueFloat[2])));
                        }
                        else if (type == "bool" && input >> sequence("=") >> valueString) {
                            result.emplace(std::make_pair(elementName, std::make_any<bool>(valueString == "true")));
                        }
                        else {
                            error = "Invalid initialisation for '" + elementName + "'";
                            return false;
                        }
                    }
                    else {
                        error = "Expected ':' and type after name '" + elementName + "'";
                        return false;
                    }
                }
                
                return true;
            }
        };
        
        while (input >> scopeName) {
            if (scopeName == name) {
                if (input >> braced(scopeText, '{', '}')) {
                    if (fn::parseScope(name, scopeText, result, error) == false) {
                        break;
                    }
                }
                else {
                    error = "Braces aren't match";
                    break;
                }
            }
        }
        
        return result;
    }
}
