
#include <sstream>
#include <string>
#include <vector>

// TODO:
// endline to skip comments
// read bool

namespace expect {
    namespace details {
        template<typename T> struct nlist {
            std::vector<T> &_output;
            
            inline friend std::istream &operator >>(std::istream &stream, const nlist &target) {
                stream >> std::ws;
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
            
            inline friend std::istream &operator >>(std::istream &stream, const sequence &target) {
                stream >> std::ws;
                const char *cur = target._current;
                
                while (stream.fail() == false && *cur != 0) {
                    char ch = stream.peek();
                    stream.peek() == *cur++ ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);
                }
                
                return stream;
            }
        };
        
        struct braced {
            std::string &_out;
            char _opening, _closing;
            
            inline friend std::istream &operator >>(std::istream &stream, const braced &target) {
                int counter = 1;
                stream >> std::ws;
                stream.peek() == target._opening ? (void)stream.ignore() : stream.setstate(std::ios_base::failbit);
                
                while (stream.fail() == false) {
                    char ch = stream.peek();
                    if (stream.peek() == target._closing) {
                        if (--counter == 0) {
                            stream.get();
                            break;
                        }
                    }
                    if (stream.peek() == target._opening) {
                        counter++;
                    }
                    
                    target._out.push_back((char)stream.get());
                }
                
                return stream;
            }
        };
    }
    
    auto braced(std::string &output, char opening, char closing) {
        return details::braced{output, opening, closing};
    }
    
    template<std::size_t N> auto sequence(const char (&seq)[N]) {
        return details::sequence{seq};
    }
    
    template<typename T> auto nlist(std::vector<T> &output) {
        return details::nlist<T>{output};
    }
}

