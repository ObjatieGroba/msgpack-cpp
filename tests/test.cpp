#include <iostream>

#include "msgpack.hpp"

using namespace msgpack;

constexpr long long example() {
    constexpr const char * data = "\xc2\xce\x05\xf5\xe1\x00\xc3\xd0\x9c abcdefghi";
    constexpr size_t size = 10;
    constexpr ConstView cv(data, size);
    IStream is(cv);
    bool kek = false, kok = false;
    long long f = 0;
    long long m = 0;
    is >> kek >> f >> kok >> m;
//    return (kek ^ kok) && f == 100000000ll && m == -100;
    return m;
}

int main() {
    static_assert(example() == -100, "I am not god");
}
