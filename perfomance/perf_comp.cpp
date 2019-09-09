#include <iostream>

#include "msgpackcpp.hpp"
#include "perfomance/msgpack-c/include/msgpack.hpp"

#include <ctime>

using namespace msgpackcpp;

auto old() {
    msgpack::type::tuple<msgpack::type::tuple<int, int, int>, bool, std::string> src(msgpack::type::make_tuple(1, 1, 1), true, "example"), dst;

    std::stringstream buffer;
    msgpack::pack(buffer, src);

    buffer.seekg(0);

    std::string str(buffer.str());

    msgpack::object_handle oh =
            msgpack::unpack(str.data(), str.size());

    msgpack::object deserialized = oh.get();

    deserialized.convert(dst);
    return dst;
}

auto nold() {
    std::tuple<std::tuple<int, int, int>, bool, std::string> src(std::make_tuple(1, 1, 1), true, "example"), dst;

    std::vector<char> buf;
    OStream os(buf);

    os << src;

    ConstView cv(buf.data(), buf.size());
    IStream is(cv);

    is >> dst;
    return dst;
}

int main() {
    if (old() != nold()) {
        throw std::runtime_error("Smth went wrong");
    }

    auto best = nold();

    size_t times = 100000;

    auto ob = std::clock();
    for (size_t i = 0; i != times; ++i) {
        if (old() != best) {
            throw std::runtime_error("Smth went wrong");
        }
    }
    auto oe = std::clock();


    auto nb = std::clock();
    for (size_t i = 0; i != times; ++i) {
        if (nold() != best) {
            throw std::runtime_error("Smth went wrong");
        }
    }
    auto ne = std::clock();

    std::cout << oe - ob << ' ' << ne - nb << std::endl;
}
