#include "object.h"
#include "statement.h"

#include <sstream>
#include <string_view>

using namespace std;

namespace Runtime {

    void ClassInstance::Print(std::ostream &os) {
        try {
            this->Call("__str__", {})->Print(os);
        }
        catch (const runtime_error& e) {
//            os << this->cls_.GetName() << " instance at address " << this;
            os << this;
        }
    }

    bool ClassInstance::HasMethod(const std::string &method, size_t argument_count) const {
        auto method_ptr = cls_.GetMethod(method);
        return method_ptr && method_ptr->formal_params.size() == argument_count;
    }

    const Closure &ClassInstance::Fields() const {
        return fields_;
    }

    Closure &ClassInstance::Fields() {
        return fields_;
    }

    ClassInstance::ClassInstance(const Class &cls) : cls_(cls) {}

    Closure CreateArgsClosure(const std::vector<string> &param_names, ObjectHolder self, const std::vector<ObjectHolder> &actual_args) {
        Closure res;
        res.insert({"self", self});
        for (auto[it_name, it_val] = make_tuple(begin(param_names), begin(actual_args)); it_name != end(param_names); it_name++, it_val++) {
            res.insert({*it_name, *it_val});
        }
        return res;
    }

    ObjectHolder ClassInstance::Call(const std::string &method, const std::vector<ObjectHolder> &actual_args) {
        if (!HasMethod(method, actual_args.size())) {
            throw runtime_error("ObjectHolder ClassInstance::Call " + method);
        }

        Closure actual_args_closure = CreateArgsClosure(
                this->cls_.GetMethod(method)->formal_params,
                ObjectHolder::Share(*this),
                actual_args
        );
        return this->cls_.GetMethod(method)->body->Execute(actual_args_closure);
    }

    Class::Class(std::string name, std::vector<Method> methods, const Class *parent) : name_(move(name)), parent_(parent) {
        for (auto &method : methods) {
            auto it = methods_.insert({method.name, move(method)});
            if (it.first->first != it.first->second.name) {
                throw runtime_error("assert Class::Class");  // todo
            }
        }
    }

    const Method *Class::GetMethod(const std::string &name) const {
        auto it = methods_.find(name);
        if (it != methods_.end()) {
            return &it->second;
        }
        if (parent_ != nullptr) {
            return parent_->GetMethod(name);
        }
        // else
        return nullptr;
    }

    void Class::Print(ostream &os) {
        os << GetName() << " class at address " << this;
    }

    const std::string &Class::GetName() const {
        return name_;
    }

    void Bool::Print(std::ostream &os) {
        if (GetValue()) {
            os << "True";
        } else {
            os << "False";
        }
    }

} /* namespace Runtime */
