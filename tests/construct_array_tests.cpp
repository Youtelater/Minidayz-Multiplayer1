#include "minidayz/construct_array.hpp"

#include <cassert>

using namespace minidayz::construct;

int main() {
    Array array;
    assert(array.set_size(3, 2, 1));
    assert(array.set_at_xy(1, 1, Value::from_number(42)));
    assert(array.at(1, 1).as_number() == 42);
    assert(array.at(9, 9).as_number() == 0);
    assert(!array.set_at_xy(9, 9, Value::from_number(1)));

    assert(array.set_size(4, 3, 1));
    assert(array.at(1, 1).as_number() == 42);
    assert(array.at(3, 2).as_number() == 0);

    assert(array.set_size(2, 2, 1));
    assert(array.at(1, 1).as_number() == 42);
    assert(array.at(2, 1).as_number() == 0);

    array.clear();
    assert(array.width() == 2 && array.height() == 2);
    assert(array.at(1, 1).as_number() == 0);

    assert(array.set_size(0, 2, 1));
    assert(array.width() == 0 && array.height() == 0 && array.depth() == 0);
    return 0;
}
