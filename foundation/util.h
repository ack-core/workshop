
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <variant>
#include <any>
#include "math.h"

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
        const char *current() const {
            return _current;
        }
        
        void setError() {
            _error = true;
        }
        bool isError() const {
            return _error;
        }
        bool isEof() const {
            return _current >= _end;
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
        static std::size_t ftow(std::uint16_t *p, double f, int precision = 6);
        static std::size_t ltoa(char *p, std::int64_t value);
        static std::string ftos(double f, int precision = 4);

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
    struct Description : private std::multimap<
        std::string,
        std::variant<Description, bool, std::int64_t, double, std::string, math::vector2f, math::vector3f, math::vector4f, math::vector2i, math::vector3i, math::vector4i>
    >
    {
        static Description parse(const std::uint8_t *data, std::size_t length);
        static std::string serialize(const util::Description &desc);
        
        Description() {}
        
        using multimap::empty;
        using multimap::size;
        using multimap::clear;
        using multimap::begin;
        using multimap::end;
        using multimap::find;
        
        bool setBool(const char *name, bool value, bool replace = true) { return _setValue(name, value, replace); }
        bool setNumber(const char *name, double value, bool replace = true) { return _setValue(name, value, replace); }
        bool setInteger(const char *name, std::int64_t value, bool replace = true) { return _setValue(name, value, replace); }
        bool setString(const char *name, const std::string &value, bool replace = true) { return _setValue(name, value, replace); }
        bool setVector2f(const char *name, const math::vector2f &value, bool replace = true) { return _setValue(name, value, replace); }
        bool setVector3f(const char *name, const math::vector3f &value, bool replace = true) { return _setValue(name, value, replace); }
        bool setVector4f(const char *name, const math::vector4f &value, bool replace = true) { return _setValue(name, value, replace); }
        bool setVector2i(const char *name, const math::vector2i &value, bool replace = true) { return _setValue(name, value, replace); }
        bool setVector3i(const char *name, const math::vector3i &value, bool replace = true) { return _setValue(name, value, replace); }
        bool setVector4i(const char *name, const math::vector4i &value, bool replace = true) { return _setValue(name, value, replace); }
        auto setDescription(const char *name, bool replace = true) -> util::Description * {
            std::string nameString (name);
            auto index = this->find(nameString);
            if (index != this->end()) {
                if (Description *desc = std::get_if<Description>(&index->second)) {
                    if (replace) {
                        desc->clear();
                    }
                    return desc;
                }
                else return nullptr;
            }
            return std::get_if<Description>(&this->emplace(std::move(nameString), Description{})->second);
        }
        
        auto getBool(const char *name) const -> const bool * { return _getAnyValue<bool>(name); }
        auto getInteger(const char *name) const -> const std::int64_t * { return _getAnyValue<std::int64_t>(name); }
        auto getNumber(const char *name) const -> const double * { return _getAnyValue<double>(name); }
        auto getString(const char *name) const -> const std::string * { return _getAnyValue<std::string>(name); }
        auto getVector2f(const char *name) const -> const math::vector2f * { return _getAnyValue<math::vector2f>(name); }
        auto getVector3f(const char *name) const -> const math::vector3f * { return _getAnyValue<math::vector3f>(name); }
        auto getVector4f(const char *name) const -> const math::vector4f * { return _getAnyValue<math::vector4f>(name); }
        auto getVector2i(const char *name) const -> const math::vector2i * { return _getAnyValue<math::vector2i>(name); }
        auto getVector3i(const char *name) const -> const math::vector3i * { return _getAnyValue<math::vector3i>(name); }
        auto getVector4i(const char *name) const -> const math::vector4i * { return _getAnyValue<math::vector4i>(name); }
        auto getBool(const char *name, const bool &def) const -> const bool & { return _getAnyOrDefault<bool>(name, def); }
        auto getInteger(const char *name, const std::int64_t &def) const -> const std::int64_t & { return _getAnyOrDefault<std::int64_t>(name, def); }
        auto getNumber(const char *name, const double &def) const -> const double & { return _getAnyOrDefault<double>(name, def); }
        auto getString(const char *name, const std::string &def) const -> const std::string & { return _getAnyOrDefault<std::string>(name, def); }
        auto getVector2f(const char *name, const math::vector2f &def) const -> const math::vector2f & { return _getAnyOrDefault<math::vector2f>(name, def); }
        auto getVector3f(const char *name, const math::vector3f &def) const -> const math::vector3f & { return _getAnyOrDefault<math::vector3f>(name, def); }
        auto getVector4f(const char *name, const math::vector4f &def) const -> const math::vector4f & { return _getAnyOrDefault<math::vector4f>(name, def); }
        auto getVector2i(const char *name, const math::vector2i &def) const -> const math::vector2i & { return _getAnyOrDefault<math::vector2i>(name, def); }
        auto getVector3i(const char *name, const math::vector3i &def) const -> const math::vector3i & { return _getAnyOrDefault<math::vector3i>(name, def); }
        auto getVector4i(const char *name, const math::vector4i &def) const -> const math::vector4i & { return _getAnyOrDefault<math::vector4i>(name, def); }
        auto getDescription(const char *name) const -> const util::Description * {
            auto index = this->find(name);
            if (index != this->end()) {
                return std::get_if<Description>(&index->second);
            }
            return nullptr;
        }
        auto getIntegers() const -> std::unordered_map<std::string, std::int64_t> { return _getAllValues<std::int64_t>(); }
        auto getIntegers(const char *name) const -> std::vector<std::int64_t> { return _getAllValues<std::int64_t>(name); }
        auto getNumbers() const -> std::unordered_map<std::string, double> { return _getAllValues<double>(); }
        auto getNumbers(const char *name) const -> std::vector<double> { return _getAllValues<double>(name); }
        auto getStrings() const -> std::unordered_map<std::string, std::string> { return _getAllValues<std::string>(); }
        auto getStrings(const char *name) const -> std::vector<std::string> { return _getAllValues<std::string>(name); }
        auto getVector2fs() const -> std::unordered_map<std::string, math::vector2f> { return _getAllValues<math::vector2f>(); }
        auto getVector3fs() const -> std::unordered_map<std::string, math::vector3f> { return _getAllValues<math::vector3f>(); }
        auto getVector4fs() const -> std::unordered_map<std::string, math::vector4f> { return _getAllValues<math::vector4f>(); }
        auto getVector2is() const -> std::unordered_map<std::string, math::vector2i> { return _getAllValues<math::vector2i>(); }
        auto getVector3is() const -> std::unordered_map<std::string, math::vector3i> { return _getAllValues<math::vector3i>(); }
        auto getVector4is() const -> std::unordered_map<std::string, math::vector4i> { return _getAllValues<math::vector4i>(); }
        auto getVector2fs(const char *name) const -> std::vector<math::vector2f> { return _getAllValues<math::vector2f>(name); }
        auto getVector3fs(const char *name) const -> std::vector<math::vector3f> { return _getAllValues<math::vector3f>(name); }
        auto getVector4fs(const char *name) const -> std::vector<math::vector4f> { return _getAllValues<math::vector4f>(name); }
        auto getVector2is(const char *name) const -> std::vector<math::vector2i> { return _getAllValues<math::vector2i>(name); }
        auto getVector3is(const char *name) const -> std::vector<math::vector3i> { return _getAllValues<math::vector3i>(name); }
        auto getVector4is(const char *name) const -> std::vector<math::vector4i> { return _getAllValues<math::vector4i>(name); }
        auto getDescriptions(const char *name) const -> std::vector<const util::Description *> {
            std::vector<const util::Description *> result;
            auto range = this->equal_range(name);
            for (auto index = range.first; index != range.second; ++index) {
                if (std::holds_alternative<Description>(index->second)) {
                    result.emplace_back(std::get_if<Description>(&index->second));
                }
            }
            return result;
        }
        auto getDescriptions() const -> std::unordered_map<std::string, const util::Description *> {
            std::unordered_map<std::string, const util::Description *> result;
            for (auto index = this->begin(); index != this->end(); ++index) {
                if (std::holds_alternative<Description>(index->second)) {
                    result.emplace(index->first, std::get_if<Description>(&index->second));
                }
            }
            return result;
        }
        
    private:
        template <typename T> bool _setValue(const char *name, const T &value, bool replace) {
            std::string nameString (name);
            auto index = this->find(nameString);
            if (index != this->end()) {
                if (std::holds_alternative<T>(index->second)) {
                    if (replace) {
                        index->second = value;
                    }
                }
                else return false;
            }
            this->emplace(std::move(nameString), value);
            return true;
        }
        template <typename T> const T *_getAnyValue(const char *name) const {
            auto index = this->find(name);
            if (index != this->end()) {
                return std::get_if<T>(&index->second);
            }
            return nullptr;
        }
        template <typename T> const T &_getAnyOrDefault(const char *name, const T &def) const {
            auto index = this->find(name);
            if (index != this->end()) {
                return *std::get_if<T>(&index->second);
            }
            else {
                return def;
            }
        }
        template <typename T> std::vector<T> _getAllValues(const char *name) const {
            std::vector<T> result;
            auto range = this->equal_range(name);
            for (auto index = range.first; index != range.second; ++index) {
                if (std::holds_alternative<T>(index->second)) {
                    result.emplace_back(*std::get_if<T>(&index->second));
                }
            }
            return result;
        }
        template <typename T> std::unordered_map<std::string, T> _getAllValues() const {
            std::unordered_map<std::string, T> result;
            for (auto index = this->begin(); index != this->end(); ++index) {
                if (std::holds_alternative<T>(index->second)) {
                    result.emplace(index->first, *std::get_if<T>(&index->second));
                }
            }
            return result;
        }

    };
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
