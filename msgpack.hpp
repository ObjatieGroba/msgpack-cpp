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
        char * cur;
        size_t size;

        constexpr MutableView(char * data_, size_t size_) : data(data_), cur(data), size(size_) { }

        template <size_t size_>
        explicit constexpr MutableView(char (&data_)[size_]) : data(data_), cur(data), size(size_) { }

        constexpr void push_back(char c) {
            if (cur >= data + size) {
                throw std::runtime_error("Not enough space for write");
            }
            *cur = c;
            ++cur;
        }
    };

    class ConstView {
    public:
        const char * data;
        const size_t size;

        constexpr ConstView(const char * data_, size_t size_) : data(data_), size(size_) { }

        template <size_t size_>
        explicit constexpr ConstView(char (&data_)[size_]) : data(data_), size(size_) { }

        explicit constexpr ConstView(const MutableView & mv) : data(mv.data), size(mv.size) { }
    };

    class Nil {};

    class IStream {
        ConstView data;
        const char * current_position;

        constexpr void check_eof(size_t n = 1) const {
            if (current_position - data.data + n > data.size)
                throw EOFError("EOF", data.size - (current_position - data.data), n);
        }

        constexpr auto load_uint8() {
            check_eof(1);
            auto i = static_cast<unsigned char>(*++current_position);
            return i;
        }

        constexpr auto load_uint16() {
            check_eof(2);
            auto i = (static_cast<unsigned short>(static_cast<unsigned char>(*++current_position)) << 8u);
            i += (static_cast<unsigned char>(*++current_position));
            return i;
        }

        constexpr auto load_uint32() {
            check_eof(4);
            auto i = static_cast<unsigned int>(static_cast<unsigned char>(*++current_position)) << 24u;
            i += (static_cast<unsigned int>(static_cast<unsigned char>(*++current_position)) << 16u);
            i += (static_cast<unsigned int>(static_cast<unsigned char>(*++current_position)) << 8u);
            i += (static_cast<unsigned char>(*++current_position));
            return i;
        }

        constexpr auto load_uint64() {
            check_eof(8);
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
            check_eof();
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
            check_eof();
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
            check_eof();
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
            check_eof();
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
            check_eof();
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
            check_eof();
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
            check_eof();
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
            check_eof();
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
            check_eof();
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
            check_eof();
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
            check_eof();
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
            check_eof();
            switch (*current_position) {
                case '\xcb': {
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
            check_eof();
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
            check_eof(size);
            s = std::string(current_position, size);
            current_position += size;
            return *this;
        }

        IStream& operator>>(std::vector<char> & s) {
            check_eof();
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
            check_eof(size);
            s = std::vector<char>(current_position, current_position + size);
            current_position += size;
            return *this;
        }

        template <typename... Args>
        constexpr IStream& operator>>(std::tuple<Args&...> tuple) {
            check_eof();
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

        template <typename... Args>
        constexpr IStream& operator>>(std::tuple<Args...> &tuple) {
            check_eof();
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

    template <class MV>
    class OStream {
        MV &data;

        constexpr void push_uint8(unsigned char i) {
            data.push_back(i);
        }

        constexpr void push_uint16(unsigned short i) {
            data.push_back(static_cast<unsigned char>((i >> 8u) & ((1u << 8u) - 1u)));
            data.push_back(static_cast<unsigned char>(i & ((1u << 8u) - 1u)));
        }

        constexpr void push_uint32(unsigned int i) {
            data.push_back(static_cast<unsigned char>((i >> 24u) & ((1u << 8u) - 1u)));
            data.push_back(static_cast<unsigned char>((i >> 16u) & ((1u << 8u) - 1u)));
            data.push_back(static_cast<unsigned char>((i >> 8u) & ((1u << 8u) - 1u)));
            data.push_back(static_cast<unsigned char>(i & ((1u << 8u) - 1u)));
        }

        constexpr void push_uint64(unsigned long long i) {
            data.push_back(static_cast<unsigned char>((i >> 56u) & ((1ull << 8u) - 1ull)));
            data.push_back(static_cast<unsigned char>((i >> 48u) & ((1ull << 8u) - 1ull)));
            data.push_back(static_cast<unsigned char>((i >> 40u) & ((1ull << 8u) - 1ull)));
            data.push_back(static_cast<unsigned char>((i >> 32u) & ((1ull << 8u) - 1ull)));
            data.push_back(static_cast<unsigned char>((i >> 24u) & ((1ull << 8u) - 1ull)));
            data.push_back(static_cast<unsigned char>((i >> 16u) & ((1ull << 8u) - 1ull)));
            data.push_back(static_cast<unsigned char>((i >> 8u) & ((1ull << 8u) - 1ull)));
            data.push_back(static_cast<unsigned char>(i & ((1u << 8u) - 1u)));
        }

        template <typename... Args, std::size_t... Idx>
        constexpr OStream& tuple_stream_helper(const std::tuple<Args...> &tuple, std::index_sequence<Idx...>) {
            return (*this << ... << std::get<Idx>(tuple));
        }

    public:
        explicit constexpr OStream(MV &data_) : data(data_) {}

        constexpr OStream& operator<<(Nil) {
            data.push_back('\xc0');
            return *this;
        }

        constexpr OStream& operator<<(bool b) {
            data.push_back(b ? '\xc3' : '\xc2');
            return *this;
        }

        constexpr OStream& operator<<(long long i) {
            unsigned long long ui = 0;
            if (i >= 0) {
                ui = i;
            } else {
                ui = 0ull - i - 1u;
            }
            if (ui >= 1ull << 31u) {
                data.push_back('\xd3');
                push_uint64(static_cast<unsigned long long>(i));
            } else if (ui >= 1u << 15u) {
                data.push_back('\xd2');
                push_uint32(static_cast<unsigned int>(static_cast<int>(i)));
            } else if (ui >= 1u << 7u) {
                data.push_back('\xd1');
                push_uint16(static_cast<unsigned short>(static_cast<short>(i)));
            } else if (ui >= 1u << 4u) {
                data.push_back('\xd0');
                push_uint8(static_cast<unsigned char>(static_cast<char>(i)));
            } else {
                auto uis = static_cast<unsigned char>(static_cast<char>(i));
                data.push_back(static_cast<unsigned char>((uis & 0x0Fu) | ((uis >> 3u) & 0xF0)));
            }
            return *this;
        }

        constexpr OStream& operator<<(unsigned long long i) {
            if (i >= 1ull << 32u) {
                data.push_back('\xcf');
                push_uint64(i);
            } else if (i >= 1u << 16u) {
                data.push_back('\xce');
                push_uint32(i);
            } else if (i >= 1u << 8u) {
                data.push_back('\xcd');
                push_uint16(i);
            } else if (i >= 1u << 7u) {
                data.push_back('\xcc');
                push_uint8(i);
            } else {
                data.push_back(static_cast<unsigned char>(i));
            }
            return *this;
        }

        constexpr OStream& operator<<(int i) {
            unsigned int ui = 0;
            if (i >= 0) {
                ui = i;
            } else {
                ui = 0u - i - 1u;
            }
            if (ui >= 1u << 15u) {
                data.push_back('\xd2');
                push_uint32(static_cast<unsigned int>(static_cast<int>(i)));
            } else if (ui >= 1u << 7u) {
                data.push_back('\xd1');
                push_uint16(static_cast<unsigned short>(static_cast<short>(i)));
            } else if (ui >= 1u << 4u) {
                data.push_back('\xd0');
                push_uint8(static_cast<unsigned char>(static_cast<char>(i)));
            } else {
                auto uis = static_cast<unsigned char>(static_cast<char>(i));
                data.push_back(static_cast<unsigned char>((uis & 0x0Fu) | ((uis >> 3u) & 0xF0)));
            }
            return *this;
        }

        constexpr OStream& operator<<(unsigned int i) {
            if (i >= 1u << 16u) {
                data.push_back('\xce');
                push_uint32(i);
            } else if (i >= 1u << 8u) {
                data.push_back('\xcd');
                push_uint16(i);
            } else if (i >= 1u << 7u) {
                data.push_back('\xcc');
                push_uint8(i);
            } else {
                data.push_back(static_cast<unsigned char>(i));
            }
            return *this;
        }

        constexpr OStream& operator<<(short i) {
            unsigned short ui = 0;
            if (i >= 0) {
                ui = i;
            } else {
                ui = 0u - i - 1u;
            }
            if (ui >= 1u << 7u) {
                data.push_back('\xd1');
                push_uint16(static_cast<unsigned short>(static_cast<short>(i)));
            } else if (ui >= 1u << 4u) {
                data.push_back('\xd0');
                push_uint8(static_cast<unsigned char>(static_cast<char>(i)));
            } else {
                auto uis = static_cast<unsigned char>(static_cast<char>(i));
                data.push_back(static_cast<unsigned char>((uis & 0x0Fu) | ((uis >> 3u) & 0xF0)));
            }
            return *this;
        }

        constexpr OStream& operator<<(unsigned short i) {
            if (i >= 1u << 8u) {
                data.push_back('\xcd');
                push_uint16(i);
            } else if (i >= 1u << 7u) {
                data.push_back('\xcc');
                push_uint8(i);
            } else {
                data.push_back(static_cast<unsigned char>(i));
            }
            return *this;
        }

        constexpr OStream& operator<<(char i) {
            unsigned char ui = 0;
            if (i >= 0) {
                ui = i;
            } else {
                ui = 0u - i - 1u;
            }
            if (ui >= 1u << 4u) {
                data.push_back('\xd0');
                push_uint8(static_cast<unsigned char>(static_cast<char>(i)));
            } else {
                auto uis = static_cast<unsigned char>(static_cast<char>(i));
                data.push_back(static_cast<unsigned char>((uis & 0x0Fu) | ((uis >> 3u) & 0xF0)));
            }
            return *this;
        }

        constexpr OStream& operator<<(unsigned char i) {
            if (i >= 1u << 7u) {
                data.push_back('\xcc');
                push_uint8(i);
            } else {
                data.push_back(static_cast<unsigned char>(i));
            }
            return *this;
        }

        constexpr OStream& operator<<(float i) {
            data.push_back('\xca');
            static_assert(sizeof(unsigned int) == sizeof(i));
            push_uint32(reinterpret_cast<unsigned int&>(i));
            return *this;
        }

        constexpr OStream& operator<<(double i) {
            data.push_back('\xcb');
            static_assert(sizeof(unsigned long long) == sizeof(i));
            push_uint64(reinterpret_cast<unsigned long long&>(i));
            return *this;
        }

        OStream& operator<<(const std::string & s) {
            if ((s.size() >> 8u) == 0) {
                data.push_back('\xd9');
                *this << static_cast<unsigned char>(s.size());
            } else if ((s.size() >> 16u) == 0) {
                data.push_back('\xda');
                *this << static_cast<unsigned short>(s.size());
            } else {
                data.push_back('\xdb');
                *this << static_cast<unsigned int>(s.size());
            }
            for (char c : s) {
                data.push_back(c);
            }
            return *this;
        }

        OStream& operator<<(const std::vector<char> & s) {
            if ((s.size() >> 8u) == 0) {
                data.push_back('\xc4');
                *this << static_cast<unsigned char>(s.size());
            } else if ((s.size() >> 16u) == 0) {
                data.push_back('\xc5');
                *this << static_cast<unsigned short>(s.size());
            } else {
                data.push_back('\xc6');
                *this << static_cast<unsigned int>(s.size());
            }
            for (char c : s) {
                data.push_back(c);
            }
            return *this;
        }

        template <typename... Args>
        constexpr OStream& operator<<(std::tuple<const Args&...> tuple) {
            if (sizeof...(Args) < (1u << 16u)) {
                data.push_back('\xdc');
                push_uint16(static_cast<unsigned short>(sizeof...(Args)));
            } else {
                data.push_back('\xdd');
                push_uint32(static_cast<unsigned int>(sizeof...(Args)));
            }
            tuple_stream_helper(tuple, std::index_sequence_for<Args...>{});
            return *this;
        }

        template <typename... Args>
        constexpr OStream& operator<<(const std::tuple<Args...> &tuple) {
            if (sizeof...(Args) < (1u << 16u)) {
                data.push_back('\xdc');
                push_uint16(static_cast<unsigned short>(sizeof...(Args)));
            } else {
                data.push_back('\xdd');
                push_uint32(static_cast<unsigned int>(sizeof...(Args)));
            }
            tuple_stream_helper(tuple, std::index_sequence_for<Args...>{});
            return *this;
        }


    };

}
