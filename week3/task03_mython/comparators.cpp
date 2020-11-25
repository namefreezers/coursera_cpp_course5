#include "comparators.h"
#include "object.h"
#include "object_holder.h"

#include <functional>
#include <optional>
#include <sstream>

using namespace std;

namespace Runtime {

    bool Equal(ObjectHolder lhs, ObjectHolder rhs) {
        if (auto num_ptr = lhs.TryAs<Number>(); num_ptr != nullptr) {
            return num_ptr->GetValue() == rhs.TryAs<Number>()->GetValue();
        }
        if (auto bool_ptr = lhs.TryAs<Bool>(); bool_ptr != nullptr) {
            return bool_ptr->GetValue() == rhs.TryAs<Bool>()->GetValue();
        }
        if (auto str_ptr = lhs.TryAs<String>(); str_ptr != nullptr) {
            return str_ptr->GetValue() == rhs.TryAs<String>()->GetValue();
        }
        throw runtime_error("");
    }

    bool Less(ObjectHolder lhs, ObjectHolder rhs) {
        if (auto num_ptr = lhs.TryAs<Number>(); num_ptr != nullptr) {
            return num_ptr->GetValue() < rhs.TryAs<Number>()->GetValue();
        }
        if (auto bool_ptr = lhs.TryAs<Bool>(); bool_ptr != nullptr) {
            return bool_ptr->GetValue() < rhs.TryAs<Bool>()->GetValue();
        }
        if (auto str_ptr = lhs.TryAs<String>(); str_ptr != nullptr) {
            return str_ptr->GetValue() < rhs.TryAs<String>()->GetValue();
        }
        throw runtime_error("");
    }

} /* namespace Runtime */
