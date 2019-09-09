#include <iostream>
#include "msgpackcpp.hpp"
#include <vector>

using namespace msgpackcpp;

template <class C>
void check(C c) {
    static std::vector<char> data;
    data.clear();

    OStream os(data);
    os << c;

    C tmp;
    ConstView cv(data.data(), data.size());
    IStream is(cv);
    is >> tmp;

    if (tmp != c) {
        throw std::runtime_error("Test failed");
    }
}

void check_int() {
    for (signed char i = -127; i != 127; ++i) {
        check(i);
    }
    for (short i = -12345; i != -12300; ++i) {
        check(i);
    }
    for (int i = -512; i != 512; ++i) {
        check(i);
    }
    check(10000000ull);
}

void check_string() {
    std::string empty;
    check(empty);

    std::string test = "test is test";
    check(test);
}

void check_bin() {
    std::vector<char> empty;
    check(empty);

    std::string tests = "test is test";
    std::vector<char> test(tests.begin(), tests.end());
    check(test);
}

void check_float() {
    float x = 1;
    check(x);

    double y = -100;
    check(y);
}

void check_array() {
    std::tuple<int> t0{100};
    check(t0);

    std::tuple<int, float, std::string> t1{1, 2, "3"};
    check(t1);
}

int main() {
    check_int();
    check_string();
    check_bin();
    check_float();
    check_array();
}
