
#include "util.h"

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
    std::size_t strstream::ptoa(std::uint16_t *p, const void *ptr) {
        const int len = 2 * sizeof(std::size_t);
        std::uint16_t *output = p + len - 1;
        std::size_t n = std::size_t(ptr);
        
        for (int i = 0; i < len; i++) {
            *output-- = "0123456789ABCDEF"[n % 16];
            n /= 16;
        }
        return len;
    }
    std::size_t strstream::ltoa(std::uint16_t* p, std::int64_t value) {
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
    std::size_t strstream::ftoa(std::uint16_t *p, double f) {
        std::uint16_t *output = p;
        const double af = std::abs(f);
        std::int64_t ipart = std::int64_t(af);
        double remainder = 1000000000.0 * (af - double(ipart));
        std::int64_t fpart = std::int64_t(remainder);
        
        if ((remainder - double(fpart)) > 0.5) {
            fpart++;
            if (fpart > 1000000000) {
                fpart = 0;
                ipart++;
            }
        }
        
        if (f < 0.0) *output++ = '-';
        output += ltoa(output, ipart);
        
        int width = 9;
        while (fpart % 10 == 0 && width-- > 0) {
            fpart /= 10;
        }
        output += width + 1;
        std::size_t result = output - p;
        while (width--) {
            *--output = fpart % 10 + '0';
            fpart /= 10;
        }
        *--output = '.';
        return result;
    }

}
