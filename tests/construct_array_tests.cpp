#include "minidayz/construct_array.hpp"

#include <cassert>

using namespace minidayz::construct;

int main() {
    Array array;
    const Value size_arguments[] = {Value::from_number(3), Value::from_number(2), Value::from_number(1)};
    assert(array.execute_action(210, size_arguments, 3));
    const Value set_arguments[] = {Value::from_number(1), Value::from_number(1), Value::from_number(42)};
    assert(array.execute_action(199, set_arguments, 3));
    assert(array.set_at_xy(1, 1, Value::from_number(42)));
    assert(array.at(1, 1).as_number() == 42);
    const Value at_arguments[] = {Value::from_number(1), Value::from_number(1)};
    Value expression_result;
    assert(array.evaluate_expression(222, at_arguments, 2, expression_result));
    assert(expression_result.as_number() == 42);
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

    assert(array.set_size(0, 6, 1));
    assert(array.width() == 0 && array.height() == 6 && array.depth() == 1);
    assert(array.at(0, 0).as_number() == 0);
    return 0;
}
