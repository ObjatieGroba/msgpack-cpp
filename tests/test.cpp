#include <iostream>
#include <vector>

#include "msgpack.hpp"

using namespace msgpack;

constexpr long long example() {
    constexpr const char * data = "\xc2\xce\x05\xf5\xe1\x00\xc3\xd0\x9c\xe0 abcdefghi";
    constexpr size_t size = 10;
    constexpr ConstView cv(data, size);
    IStream is(cv);
    bool kek = false, kok = false;
    long long f = 0;
    long long m = 0;
    long long m5 = 0;
    is >> kek >> f >> kok >> m >> m5;
//    return (kek ^ kok) && f == 100000000ll && m == -100;
    return m5;
}

constexpr long long sum() {
    constexpr const char * data = "\x92\x01\x02";
    constexpr size_t size = 3;
    constexpr ConstView cv(data, size);
    IStream is(cv);
    long long a = 0, b = 0;
    is >> std::tie(a, b);
    return a + b;
}

void kek() {
    constexpr const char * data = "\x92\x01\xA6\x61\x62\x63\x64\x65\x66";
    constexpr size_t size = 13;
    constexpr ConstView cv(data, size);
    IStream is(cv);
    long long a;
    std::string f;
    is >> std::tie(a, f);
    std::cerr << a << ' ' << f << std::endl;
}

void ostream_test() {
    char data[10]="\0\0\0\0\0\0\0\0";
    MutableView mc(data);
    OStream os(mc);
    os << 257ull;
    std::cout << (int)data[0] << std::endl;
    std::cout << (int)data[1] << std::endl;
    std::cout << (int)data[2] << std::endl;
}

constexpr auto kek1() {
    char data[100]{};
    MutableView mv(data);
    OStream os(mv);
    os << std::tuple(1, 2, 3, 4, 5, 100, 10000);
    ConstView cv(data);
    IStream is(cv);
    int a=0, b=0, c=0, d=0, e=0, f=0, g=0;
    is >> std::tie(a, b, c, d, e, f, g);
    return a + b + c + d + e + f + g;
}

int main() {
    ostream_test();
    static_assert(kek1() == 10115);

//    kek();
//    static_assert(sum() == 3);
//
//    std::cerr << example() << std::endl;
//
//    std::string data = "\xA6\x61\x62\x63\x64\x65\x66";
//    ConstView cv(data.data(), data.size());
//    IStream is(cv);
//    std::string kek;
//    is >> kek;
//    std::cerr << kek << std::endl;

//    static_assert(example() == -100, "I am not god");
}
