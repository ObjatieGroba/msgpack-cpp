#pragma once

#include <optional>

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

    class MutableView {
    public:
        char * data;
        size_t size;

        constexpr MutableView(char * data_, size_t size_) : data(data_), size(size_) { }

        /// Currently constexpr mutable c array is not possible
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
                throw LengthError("EOF", data.size - (current_position - data.data), n);
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
                    is_end(1);
                    i = static_cast<unsigned char>(*++current_position);
                    break;
                case '\xcd':
                    is_end(2);
                    i = (static_cast<unsigned short>(static_cast<unsigned char>(*++current_position)) << 8u);
                    i += (static_cast<unsigned char>(*++current_position));
                    break;
                case '\xce':
                    is_end(4);
                    i = (static_cast<unsigned int>(static_cast<unsigned char>(*++current_position)) << 24u);
                    i += (static_cast<unsigned int>(static_cast<unsigned char>(*++current_position)) << 16u);
                    i += (static_cast<unsigned int>(static_cast<unsigned char>(*++current_position)) << 8u);
                    i += (static_cast<unsigned char>(*++current_position));
                    break;
                case '\xcf':
                    is_end(8);
                    i = (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 56u);
                    /// @TODO prevent overflow
                    i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 48u);
                    i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 40u);
                    i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 32u);
                    i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 24u);
                    i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 16u);
                    i += (static_cast<unsigned long long>(static_cast<unsigned char>(*++current_position)) << 8u);
                    i += (static_cast<unsigned char>(*++current_position));
                    break;
                case '\xd0':
                    is_end(1);
                    i = static_cast<char>(*++current_position);
                    break;
                case '\xd1':
                    is_end(2);
                    i = (static_cast<unsigned short>(static_cast<unsigned char>(*++current_position)) << 8u);
                    i += (static_cast<unsigned char>(*++current_position));
                    break;
                default:
                    throw TypeError("Expected int", *current_position);
            }
            ++current_position;
            return *this;
        }
    };

}
