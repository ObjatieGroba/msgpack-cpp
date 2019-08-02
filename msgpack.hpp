#pragma once

#include <optional>
#include <vector>
#include <string>
#include <tuple>

namespace msgpack {

    class TypeError : public std::exception {
        const char * msg;
        unsigned char type;
    public:
        explicit TypeError(const char * msg_, unsigned char type_) : msg(msg_), type(type_) { }

        const char * what() const noexcept override {
            return msg;
        }

        unsigned char getType() const noexcept {
            return type;
        }
    };

    class LengthError : public std::exception {
        const char * msg;
        size_t current;
        size_t expected;

    public:
        explicit LengthError(const char * msg_, size_t cur_, size_t exp_) : msg(msg_), current(cur_), expected(exp_) { }

        const char * what() const noexcept override {
            return msg;
        }

        size_t getExpected() const noexcept {
            return expected;
        }

        size_t getActual() const noexcept {
            return current;
        }
    };

    class EOFError : public std::exception {
        const char * msg;
        size_t current;
        size_t expected;

    public:
        explicit EOFError(const char * msg_, size_t cur_, size_t exp_) : msg(msg_), current(cur_), expected(exp_) { }

        const char * what() const noexcept override {
            return msg;
        }

        size_t getExpected() const noexcept {
            return expected;
        }

        size_t getActual() const noexcept {
            return current;
        }
    };

    class MutableView {
    public:
        char * data;
        size_t size;

        constexpr MutableView(char * data_, size_t size_) : data(data_), size(size_) { }

        template <size_t size_>
        explicit constexpr MutableView(char (&data_)[size_]) : data(data_), size(size_) { }
    };

    class ConstView {
    public:
        const char * data;
        const size_t size;

        constexpr ConstView(const char * data_, size_t size_) : data(data_), size(size_) { }

        explicit constexpr ConstView(const MutableView & mv) : data(mv.data), size(mv.size) { }
    };

    class Nil {};

    class IStream {
        ConstView data;
        const char * current_position;

        constexpr void is_end(size_t n = 1) const {
            if (current_position - data.data + n > data.size)
                throw EOFError("EOF", data.size - (current_position - data.data), n);
        }

        constexpr auto load_uint8() {
            is_end(1);
            auto i = static_cast<unsigned char>(*++current_position);
            return i;
        }

        constexpr auto load_uint16() {
            is_end(2);
            auto i = (static_cast<unsigned short>(static_cast<unsigned char>(*++current_position)) << 8u);
            i += (static_cast<unsigned char>(*++current_position));
            return i;
        }

        constexpr auto load_uint32() {
            is_end(4);
            auto i = static_cast<unsigned int>(static_cast<unsigned char>(*++current_position)) << 24u;
            i += (static_cast<unsigned int>(static_cast<unsigned char>(*++current_position)) << 16u);
            i += (static_cast<unsigned int>(static_cast<unsigned char>(*++current_position)) << 8u);
            i += (static_cast<unsigned char>(*++current_position));
            return i;
        }

        constexpr auto load_uint64() {
            is_end(8);
            auto i = (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 56u);
            i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 48u);
            i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 40u);
            i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 32u);
            i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 24u);
            i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 16u);
            i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 8u);
            i += (static_cast<unsigned char>(*++current_position));
            return i;
        }

        template <typename... Args, std::size_t... Idx>
        constexpr IStream& tuple_stream_helper(std::tuple<Args...> &tuple, std::index_sequence<Idx...>) {
            return (*this >> ... >> std::get<Idx>(tuple));
        }

    public:
        explicit constexpr IStream(ConstView cv) : data(cv), current_position(data.data) { }

        constexpr IStream& operator>>(Nil &) {
            is_end();
            switch (*current_position) {
                case '\xc0':
                    break;
                default:
                    throw TypeError("Expected nil", *current_position);
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(bool & b) {
            is_end();
            switch (*current_position) {
                case '\xc2':
                    b = false;
                    break;
                case '\xc3':
                    b = true;
                    break;
                default:
                    throw TypeError("Expected bool", *current_position);
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(long long & i) {
            is_end();
            switch (*current_position) {
                case '\xcc':
                    i = load_uint8();
                    break;
                case '\xcd':
                    i = load_uint16();
                    break;
                case '\xce':
                    i = load_uint32();
                    break;
                case '\xcf':
                    i = load_uint64();
                    break;
                case '\xd0':
                    i = static_cast<char>(load_uint8());
                    break;
                case '\xd1':
                    i = static_cast<short>(load_uint16());
                    break;
                case '\xd2':
                    i = static_cast<int>(load_uint32());
                    break;
                case '\xd3':
                    i = static_cast<long long>(load_uint64());
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0x80u) == 0x00u) {
                        i = static_cast<unsigned char>(*current_position);
                    } else if ((static_cast<unsigned char>(*current_position) & 0xE0u) == 0xE0u) {
                        i = -static_cast<int>((1u << 5u) - static_cast<int>(static_cast<unsigned char>(*current_position) & 0x1Fu));
                    } else {
                        throw TypeError("Expected integer", *current_position);
                    }
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(unsigned long long & i) {
            is_end();
            switch (*current_position) {
                case '\xcc':
                    i = load_uint8();
                    break;
                case '\xcd':
                    i = load_uint16();
                    break;
                case '\xce':
                    i = load_uint32();
                    break;
                case '\xcf':
                    i = load_uint64();
                    break;
                case '\xd0':
                    i = static_cast<char>(load_uint8());
                    break;
                case '\xd1':
                    i = static_cast<short>(load_uint16());
                    break;
                case '\xd2':
                    i = static_cast<int>(load_uint32());
                    break;
                case '\xd3':
                    i = static_cast<long long>(load_uint64());
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0x80u) == 0x00u) {
                        i = static_cast<unsigned char>(*current_position);
                    } else if ((static_cast<unsigned char>(*current_position) & 0xE0u) == 0xE0u) {
                        i = -static_cast<int>((1u << 5u) - static_cast<int>(static_cast<unsigned char>(*current_position) & 0x1Fu));
                    } else {
                        throw TypeError("Expected integer", *current_position);
                    }
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(int & i) {
            is_end();
            switch (*current_position) {
                case '\xcc':
                    i = load_uint8();
                    break;
                case '\xcd':
                    i = load_uint16();
                    break;
                case '\xce':
                    i = load_uint32();
                    break;
                case '\xd0':
                    i = static_cast<char>(load_uint8());
                    break;
                case '\xd1':
                    i = static_cast<short>(load_uint16());
                    break;
                case '\xd2':
                    i = static_cast<int>(load_uint32());
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0x80u) == 0x00u) {
                        i = static_cast<unsigned char>(*current_position);
                    } else if ((static_cast<unsigned char>(*current_position) & 0xE0u) == 0xE0u) {
                        i = -static_cast<int>((1u << 5u) - static_cast<int>(static_cast<unsigned char>(*current_position) & 0x1Fu));
                    } else {
                        throw TypeError("Expected integer", *current_position);
                    }
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(unsigned int & i) {
            is_end();
            switch (*current_position) {
                case '\xcc':
                    i = load_uint8();
                    break;
                case '\xcd':
                    i = load_uint16();
                    break;
                case '\xce':
                    i = load_uint32();
                    break;
                case '\xd0':
                    i = static_cast<char>(load_uint8());
                    break;
                case '\xd1':
                    i = static_cast<short>(load_uint16());
                    break;
                case '\xd2':
                    i = static_cast<int>(load_uint32());
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0x80u) == 0x00u) {
                        i = static_cast<unsigned char>(*current_position);
                    } else if ((static_cast<unsigned char>(*current_position) & 0xE0u) == 0xE0u) {
                        i = -static_cast<int>((1u << 5u) - static_cast<int>(static_cast<unsigned char>(*current_position) & 0x1Fu));
                    } else {
                        throw TypeError("Expected integer", *current_position);
                    }
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(short & i) {
            is_end();
            switch (*current_position) {
                case '\xcc':
                    i = load_uint8();
                    break;
                case '\xcd':
                    i = load_uint16();
                    break;
                case '\xd0':
                    i = static_cast<char>(load_uint8());
                    break;
                case '\xd1':
                    i = static_cast<short>(load_uint16());
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0x80u) == 0x00u) {
                        i = static_cast<unsigned char>(*current_position);
                    } else if ((static_cast<unsigned char>(*current_position) & 0xE0u) == 0xE0u) {
                        i = -static_cast<int>((1u << 5u) - static_cast<int>(static_cast<unsigned char>(*current_position) & 0x1Fu));
                    } else {
                        throw TypeError("Expected integer", *current_position);
                    }
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(unsigned short & i) {
            is_end();
            switch (*current_position) {
                case '\xcc':
                    i = load_uint8();
                    break;
                case '\xcd':
                    i = load_uint16();
                    break;
                case '\xd0':
                    i = static_cast<char>(load_uint8());
                    break;
                case '\xd1':
                    i = static_cast<short>(load_uint16());
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0x80u) == 0x00u) {
                        i = static_cast<unsigned char>(*current_position);
                    } else if ((static_cast<unsigned char>(*current_position) & 0xE0u) == 0xE0u) {
                        i = -static_cast<int>((1u << 5u) - static_cast<int>(static_cast<unsigned char>(*current_position) & 0x1Fu));
                    } else {
                        throw TypeError("Expected integer", *current_position);
                    }
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(signed char & i) {
            is_end();
            switch (*current_position) {
                case '\xcc':
                    i = load_uint8();
                    break;
                case '\xd0':
                    i = static_cast<char>(load_uint8());
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0x80u) == 0x00u) {
                        i = static_cast<unsigned char>(*current_position);
                    } else if ((static_cast<unsigned char>(*current_position) & 0xE0u) == 0xE0u) {
                        i = -static_cast<int>((1u << 5u) - static_cast<int>(static_cast<unsigned char>(*current_position) & 0x1Fu));
                    } else {
                        throw TypeError("Expected integer", *current_position);
                    }
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(unsigned char & i) {
            is_end();
            switch (*current_position) {
                case '\xcc':
                    i = load_uint8();
                    break;
                case '\xd0':
                    i = static_cast<char>(load_uint8());
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0x80u) == 0x00u) {
                        i = static_cast<unsigned char>(*current_position);
                    } else if ((static_cast<unsigned char>(*current_position) & 0xE0u) == 0xE0u) {
                        i = -static_cast<int>((1u << 5u) -
                                              static_cast<int>(static_cast<unsigned char>(*current_position) & 0x1Fu));
                    } else {
                        throw TypeError("Expected integer", *current_position);
                    }
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(float & i) {
            is_end();
            switch (*current_position) {
                case '\xca': {
                    auto bin_val = load_uint32();
                    static_assert(sizeof(bin_val) == sizeof(i));
                    i = reinterpret_cast<decltype(i) &>(bin_val);
                }
                    break;
                default:
                    throw TypeError("Expected float", *current_position);
            }
            ++current_position;
            return *this;
        }

        constexpr IStream& operator>>(double & i) {
            is_end();
            switch (*current_position) {
                case '\xca': {
                    auto bin_val = load_uint64();
                    static_assert(sizeof(bin_val) == sizeof(i));
                    i = reinterpret_cast<decltype(i) &>(bin_val);
                }
                    break;
                default:
                    throw TypeError("Expected double", *current_position);
            }
            ++current_position;
            return *this;
        }

        IStream& operator>>(std::string & s) {
            is_end();
            size_t size = 0;
            switch (*current_position) {
                case '\xd9':
                    size = load_uint8();
                    break;
                case '\xda':
                    size = load_uint16();
                    break;
                case '\xdb':
                    size = load_uint32();
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0xE0u) == 0xA0u) {
                        size = static_cast<unsigned char>(*current_position) & 0x1Fu;
                    } else {
                        throw TypeError("Expected string", *current_position);
                    }
            }
            ++current_position;
            is_end(size);
            s = std::string(current_position, size);
            current_position += size;
            return *this;
        }

        IStream& operator>>(std::vector<char> & s) {
            is_end();
            size_t size = 0;
            switch (*current_position) {
                case '\xc4':
                    size = load_uint8();
                    break;
                case '\xc5':
                    size = load_uint16();
                    break;
                case '\xc6':
                    size = load_uint32();
                    break;
                default:
                    throw TypeError("Expected binary", *current_position);
            }
            ++current_position;
            is_end(size);
            s = std::vector<char>(current_position, current_position + size);
            current_position += size;
            return *this;
        }

        template<typename... Args>
        constexpr IStream& operator>>(std::tuple<Args&...> tuple) {
            is_end();
            size_t size = 0;
            switch (*current_position) {
                case '\xdc':
                    size = load_uint16();
                    break;
                case '\xdd':
                    size = load_uint32();
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0xF0u) == 0x90u) {
                        size = static_cast<unsigned char>(*current_position) & 0x0Fu;
                    } else {
                        throw TypeError("Expected array", *current_position);
                    }
            }
            ++current_position;
            if (size != sizeof...(Args)) {
                throw LengthError("Bad array size", size, sizeof...(Args));
            }
            tuple_stream_helper(tuple, std::index_sequence_for<Args...>{});
            return *this;
        }

        template<typename... Args>
        constexpr IStream& operator>>(std::tuple<Args...> &tuple) {
            is_end();
            size_t size = 0;
            switch (*current_position) {
                case '\xdc':
                    size = load_uint16();
                    break;
                case '\xdd':
                    size = load_uint32();
                    break;
                default:
                    if ((static_cast<unsigned char>(*current_position) & 0xF0u) == 0x90u) {
                        size = static_cast<unsigned char>(*current_position) & 0x0Fu;
                    } else {
                        throw TypeError("Expected array", *current_position);
                    }
            }
            ++current_position;
            if (size != sizeof...(Args)) {
                throw LengthError("Bad array size", size, sizeof...(Args));
            }
            tuple_stream_helper(tuple, std::index_sequence_for<Args...>{});
            return *this;
        }
    };

}
