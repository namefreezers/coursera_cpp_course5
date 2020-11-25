#include "object_holder.h"
#include "object.h"

#include <stdexcept>

namespace Runtime {

    ObjectHolder ObjectHolder::Share(Object &object) {
        return ObjectHolder(std::shared_ptr<Object>(&object, [](auto *) { /* do nothing */ }));
    }

    ObjectHolder ObjectHolder::None() {
        return ObjectHolder::Own(NoneObject());
    }

    Object &ObjectHolder::operator*() {
        return *Get();
    }

    const Object &ObjectHolder::operator*() const {
        return *Get();
    }

    Object *ObjectHolder::operator->() {
        return Get();
    }

    const Object *ObjectHolder::operator->() const {
        return Get();
    }

    Object *ObjectHolder::Get() {
        return data.get();
    }

    const Object *ObjectHolder::Get() const {
        return data.get();
    }

    ObjectHolder::operator bool() const {
        return Get() && TryAs<Runtime::NoneObject>() == nullptr;
    }

    bool IsTrue(ObjectHolder object) {
        if (!object.Get()) {
            return false;
        }
        if (auto num = object.TryAs<Number>(); num != nullptr) {
            return num->GetValue();
        }
        if (auto b = object.TryAs<Bool>(); b != nullptr) {
            return b->GetValue();
        }
        if (auto str = object.TryAs<String>(); str != nullptr) {
            return !str->GetValue().empty();
        }
        if (auto str = object.TryAs<NoneObject>(); str != nullptr) {
            return false;
        }
        throw std::runtime_error("object_holder.cpp IsTrue");
    }

}
