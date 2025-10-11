
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <any>

/*
TODO:
            template <class C> callback(const std::weak_ptr<C> &weak, R(C::*method)(Args...)) : callback([weak, method](Args... args) -> R {
                if (auto target = weak.lock()) {
                    return (target.get()->*method)(args...);
                }
                
                return {};
            }) {}

 */

namespace util {
    struct IntegerOffset3D {
        int x;
        int y;
        int z;
    };
}

namespace util {
    template <typename M> class callback final {};
    template <typename R, typename... Args> class callback <R(Args...)> final {
    public:
        callback() {}
        template <typename L, R(L::*)(Args...) const = &L::operator()> callback(L&& lambda) {
            _data.target = new L (std::move(lambda));
            _data.call = [](void *ptr, Args... args) {
                return (static_cast<L *>(ptr))->operator()(std::forward<Args>(args)...);
            };
            _data.clean = [](void *ptr) {
                delete static_cast<L *>(ptr);
            };
        }
        template <typename L, R(L::*)(Args...) = &L::operator()> callback(L&& lambda) {
            _data.target = new L (std::move(lambda));
            _data.call = [](void *ptr, Args... args) {
                return (static_cast<L *>(ptr))->operator()(std::forward<Args>(args)...);
            };
            _data.clean = [](void *ptr) {
                delete static_cast<L *>(ptr);
            };
        }
        
        callback(callback &&other) : _data(other._data) {
            other._data = {};
        }
        callback& operator =(callback &&other) {
            _data.clean(_data.target);
            _data = other._data;
            other._data = {};
            return *this;
        }
        
        ~callback() {
            _data.clean(_data.target);
        }
        
        operator bool () const {
            return _data.target != nullptr;
        }
        
        decltype(auto) operator ()(Args... args) const {
            return _data.call(_data.target, std::forward<Args>(args)...);
        }
        
    private:
        struct Data {
            R(*call)(void *ptr, Args...) = nullptr;
            void (*clean)(void *ptr) = [](void *){};
            void *target = nullptr;
        }
        _data;
        
    private:
        callback(const callback &) = delete;
        callback& operator =(const callback &) = delete;
    };
}

namespace util {
    class strstream {
    public:
        strstream(const char *start, std::size_t length) : _current(start), _end(start + length), _error(false) {}
        ~strstream() {}
        
        char getChar() {
            return _current < _end ? *_current++ : 0;
        }
        char peekChar() const {
            return *_current;
        }
        void skipws() {
            while (_current < _end && std::isgraph(*_current) == false) _current++;
        }
        
        void setError() {
            _error = true;
        }
        bool isError() const {
            return _error;
        }
        
        operator bool() const {
            return _current <= _end && !_error;
        }
        
        template<typename T, typename std::enable_if_t<std::is_integral_v<T>>* = nullptr> strstream &operator >> (T& value) {
            skipws();
            std::size_t len = 0;
            const std::int64_t v = atoi(_current, len);
            if (len) {
                value = T(v);
                _current += len;
            }
            else {
                _error = true;
            }
            return *this;
        }
        template<typename T, typename std::enable_if_t<std::is_floating_point_v<T>>* = nullptr> strstream &operator >> (T& value) {
            skipws();
            std::size_t len = 0;
            const double v = atof(_current, len);
            if (len) {
                value = T(v);
                _current += len;
            }
            else {
                _error = true;
            }
            return *this;
        }
        template<typename T, typename std::enable_if_t<std::is_base_of_v<std::basic_string<char>, T>>* = nullptr> strstream &operator >> (T& value) {
            skipws();
            value = "";
            while (_current < _end && std::isgraph(*_current)) {
                value.append(1, *_current++);
            }
            if (value.length() == 0) {
                _error = true;
            }
            return *this;
        }
        
        static std::int64_t atoi(const char *s, std::size_t &len);
        static double atof(const char *s, std::size_t &len);
        
        static std::size_t ptow(std::uint16_t *p, const void *ptr);
        static std::size_t ltow(std::uint16_t *p, std::int64_t value);
        static std::size_t ftow(std::uint16_t *p, double f);
        static std::size_t ltoa(char *p, std::int64_t value);
        static std::string ftos(double f, int precision = 6);

        const char *_end;
        const char *_current;
        bool _error;
    };
    
    namespace details {
        template<typename T> struct nlist {
            std::vector<T> &_output;
            
            inline friend strstream &operator >>(strstream &stream, const nlist &target) {
                stream.skipws();
                int count;
                T temp;
                
                if (stream >> count) {
                    for (int i = 0; i < count; i++) {
                        if (stream >> temp) {
                            target._output.emplace_back(std::move(temp));
                        }
                        else break;
                    }
                }
                
                return stream;
            }
        };
        
        struct sequence {
            const char *_current;
            
            inline friend strstream &operator >>(strstream &stream, const sequence &target) {
                stream.skipws();
                const char *cur = target._current;
                
                while (stream.isError() == false && *cur != 0) {
                    (void)stream.peekChar();
                    stream.peekChar() == *cur++ ? (void)stream.getChar() : stream.setError();
                }
                
                return stream;
            }
        };
        
        struct braced {
            std::string &_out;
            char _opening, _closing;
            
            inline friend strstream &operator >>(strstream &stream, const braced &target) {
                int counter = 1;
                stream.skipws();
                stream.peekChar() == target._opening ? (void)stream.getChar() : stream.setError();
                
                while (stream.isError() == false) {
                    (void)stream.peekChar();
                    if (stream.peekChar() == target._closing) {
                        if (--counter == 0) {
                            stream.getChar();
                            break;
                        }
                    }
                    if (stream.peekChar() == target._opening) {
                        counter++;
                    }
                    
                    target._out.push_back(stream.getChar());
                }
                
                return stream;
            }
        };
        
        struct word {
            std::string &_out;

            inline friend strstream &operator >>(strstream &stream, const word &target) {
                stream.skipws();
                if (std::isgraph(stream.peekChar())) {
                    while (std::isalnum(stream.peekChar())) {
                        target._out.push_back(stream.getChar());
                    }
                }
                else {
                    stream.setError();
                }

                return stream;
            }
        };
    }
    
    inline auto braced(std::string &output, char opening, char closing) {
        return details::braced{output, opening, closing};
    }
    template<std::size_t N> inline auto sequence(const char (&seq)[N]) {
        return details::sequence{seq};
    }
    template<typename T> inline auto nlist(std::vector<T> &output) {
        return details::nlist<T>{output};
    }
    inline auto word(std::string &output) {
        return details::word{output};
    }
}

namespace util {
    using Config = std::unordered_map<std::string, std::any>;
    Config parseConfig(const std::uint8_t *data, std::size_t length, const std::string &name);
}

// TODO: move to dedicated shader generator
namespace shaderUtils {
    struct ShaderTypeInfo{
        const char *typeName;
        const char *nativeTypeName;
        std::size_t sizeInBytes;
    };
    inline int getArrayMultiply(const std::string &varname) {
        int  multiply = 1;
        auto braceStart = varname.find('[');
        auto braceEnd = varname.rfind(']');
        
        if (braceStart != std::string::npos && braceEnd != std::string::npos) {
            std::size_t len;
            std::int64_t value = util::strstream::atoi(varname.data() + braceStart + 1, len);
            multiply = std::max(int(value), multiply);
        }
        
        return multiply;
    }
    inline bool shaderGetTypeSize(const std::string &typeName, const ShaderTypeInfo *types, std::size_t typeCount, std::string &outNativeTypeName, std::size_t &outSize) {
        for (std::size_t i = 0; i < typeCount; i++) {
            if (types[i].typeName == typeName) {
                outNativeTypeName = types[i].nativeTypeName;
                outSize = types[i].sizeInBytes;
                return true;
            }
        }
        
        return false;
    };
    inline void replace(std::string& target, const std::string& bit, const std::string& replacement, const std::string& prevAllowed, const std::initializer_list<std::string>& exceptions = {}) {
        std::size_t start_pos = 0;
        while ((start_pos = target.find(bit, start_pos)) != std::string::npos) {
            bool skip = false;

            for (const std::string &exception : exceptions) {
                if (::memcmp(target.data() + start_pos, exception.data(), exception.length()) == 0) {
                    skip = true;
                }
            }
            if (skip == false) {
                if (prevAllowed.find(target.data()[start_pos - 1]) != std::string::npos) {
                    target.replace(start_pos, bit.length(), replacement);
                    start_pos += replacement.length();
                    continue;
                }
            }

            start_pos++;
        }
    }
    inline void replace(std::string& target, const std::string& bit, const std::string& replacement, const std::string& prevAllowed, const std::string& postAllowed) {
        std::size_t start_pos = 0;
        while ((start_pos = target.find(bit, start_pos)) != std::string::npos) {
            if (start_pos == 0 || prevAllowed.find(target.data()[start_pos - 1]) != std::string::npos) {
                if (postAllowed.find(target.data()[start_pos + bit.length()]) != std::string::npos) {
                    target.replace(start_pos, bit.length(), replacement);
                    start_pos += replacement.length();
                    continue;
                }
            }
            
            start_pos++;
        }
    }
    inline bool formCodeBlock(const std::string &indent, util::strstream &stream, std::string &output) {
        stream.skipws();
        int  braceCounter = 1;
        char prev = '\n', next;
        
        while (stream && ((next = stream.getChar()) != '}' || --braceCounter > 0)) {
            if (prev == '\n') {
                for (int i = 0; i < braceCounter; i++) {
                    output += indent;
                }
            }
            if (next == '{') braceCounter++;
            output += next;
            if (next == '\n')  stream.skipws();
            prev = next;
        }
        
        return braceCounter == 0;
    }
    inline std::string makeLines(const std::string &src) {
        std::string result;
        std::string::const_iterator left = std::begin(src);
        std::string::const_iterator right;
        char line[] = "/* 0000 */  ";
        int counter = 1;

        while ((right = std::find(left, std::end(src), '\n')) != std::end(src)) {
            line[6] = '0' + counter % 10;
            line[5] = '0' + (counter / 10) % 10;
            line[4] = '0' + (counter / 100) % 10;
            line[3] = '0' + (counter / 1000) % 10;
            result += std::string(line) + std::string(left, ++right);
            left = right;
            counter++;
        }

        return result;
    }
}
