#include "Error.hpp"

bool operator==(const Status_t&a, const Status_t& b) {
    return a.has_value() == b.has_value() &&
        (!a.has_value() && !b.has_value() || a.value().Code == b.value().Code);  
}