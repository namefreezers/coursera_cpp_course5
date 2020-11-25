#include "statement.h"
#include "object.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace Ast {

    using Runtime::Closure;

    vector<ObjectHolder> TransformFromStToObj(const vector<unique_ptr<Statement>> &arg_sttms, Closure &closure) {
        vector<ObjectHolder> obj_args;
        obj_args.reserve(arg_sttms.size());
        for (const auto &arg_statement : arg_sttms) {
            obj_args.push_back(arg_statement->Execute(closure));
        }
        return obj_args;
    }

    ObjectHolder Assignment::Execute(Closure &closure) {
        closure[var_name] = right_value->Execute(closure);
        return closure[var_name];
    }

    Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) : var_name(move(var)), right_value(move(rv)) {}

    VariableValue::VariableValue(std::string var_name) : dotted_ids({move(var_name)}) {}

    VariableValue::VariableValue(std::vector<std::string> dotted_ids) : dotted_ids(move(dotted_ids)) {}

    ObjectHolder VariableValue::Execute(Closure &closure) {
        Closure *cur_cl = &closure;
        for (int i = 0; i < dotted_ids.size() - 1; i++) {
            auto it_cur_cl = cur_cl->find(dotted_ids.at(i));
            if (it_cur_cl == cur_cl->end()) {
                throw runtime_error("not found variable");
            }
            cur_cl = &it_cur_cl->second.TryAs<Runtime::ClassInstance>()->Fields();
        }
        auto it_cur_cl = cur_cl->find(dotted_ids.at(dotted_ids.size() - 1));
        if (it_cur_cl == cur_cl->end()) {
            throw runtime_error("not found variable");
        }
        return it_cur_cl->second;
    }

    unique_ptr<Print> Print::Variable(std::string var) {
        return make_unique<Print>(make_unique<VariableValue>(move(var)));
    }

    Print::Print(unique_ptr<Statement> argument) {
        args.push_back(move(argument));
    }

    Print::Print(vector<unique_ptr<Statement>> args) : args(move(args)) {}

    ObjectHolder Print::Execute(Closure &closure) {
        if (!args.empty()) {
            args.at(0)->Execute(closure)->Print(*output);
        }

        for (int i = 1; i < args.size(); i++) {
            *output << ' ';
            args.at(i)->Execute(closure)->Print(*output);
        }
        *output << '\n';
        return ObjectHolder();
    }

    ostream *Print::output = &cout;

    void Print::SetOutputStream(ostream &output_stream) {
        output = &output_stream;
    }

    MethodCall::MethodCall(
            std::unique_ptr<Statement> object, std::string method, std::vector<std::unique_ptr<Statement>> args
    ) : object(move(object)), method(move(method)), args(move(args)) {
    }

    ObjectHolder MethodCall::Execute(Closure &closure) {
        return object->Execute(closure).TryAs<Runtime::ClassInstance>()->Call(this->method, TransformFromStToObj(this->args, closure));
    }

    ObjectHolder Stringify::Execute(Closure &closure) {
        stringstream ss;
        argument->Execute(closure)->Print(ss);
        return ObjectHolder::Own(Runtime::String(ss.str()));
    }

    ObjectHolder Add::Execute(Closure &closure) {
        auto lhs_op = lhs->Execute(closure), rhs_op = rhs->Execute(closure);
        if (auto lhs_str_ptr = lhs_op.TryAs<Runtime::String>(); lhs_str_ptr != nullptr) {
            auto rhs_str_ptr = rhs_op.TryAs<Runtime::String>();
            if (rhs_str_ptr == nullptr) { throw runtime_error("statement.cpp Add::Execute [str + not_str]"); }
            return ObjectHolder::Own(Runtime::String(lhs_str_ptr->GetValue() + rhs_str_ptr->GetValue()));
        }
        if (auto lhs_num_ptr = lhs_op.TryAs<Runtime::Number>(); lhs_num_ptr != nullptr) {
            auto rhs_num_ptr = rhs_op.TryAs<Runtime::Number>();
            if (rhs_num_ptr == nullptr) { throw runtime_error("statement.cpp Add::Execute [num + not_num]"); }
            return ObjectHolder::Own(Runtime::Number(lhs_num_ptr->GetValue() + rhs_num_ptr->GetValue()));
        }
        if (auto lhs_inst_ptr = lhs_op.TryAs<Runtime::ClassInstance>(); lhs_inst_ptr != nullptr) {
            try {
                return lhs_inst_ptr->Call("__add__", {rhs_op});
            }
            catch (const runtime_error &e) {
                throw runtime_error("statement.cpp Add::Execute [inst wo __add__()]");
            }
        }
        throw runtime_error("statement.cpp Add::Execute");
    }

    ObjectHolder Sub::Execute(Closure &closure) {
        auto lhs_op = lhs->Execute(closure), rhs_op = rhs->Execute(closure);
        if (auto[lhs_num_ptr, rhs_num_ptr] = pair(lhs_op.TryAs<Runtime::Number>(), rhs_op.TryAs<Runtime::Number>());
                lhs_num_ptr != nullptr && rhs_num_ptr != nullptr) {
            return ObjectHolder::Own(Runtime::Number(lhs_num_ptr->GetValue() - rhs_num_ptr->GetValue()));
        }
        if (auto lhs_inst_ptr = lhs_op.TryAs<Runtime::ClassInstance>(); lhs_inst_ptr != nullptr) {
            try {
                return lhs_inst_ptr->Call("__sub__", {rhs_op});
            }
            catch (const runtime_error &e) {
                throw runtime_error("statement.cpp Sub::Execute [inst wo __sub__()]");
            }
        }
        throw runtime_error("statement.cpp Sub::Execute");
    }

    ObjectHolder Mult::Execute(Runtime::Closure &closure) {
        auto lhs_op = lhs->Execute(closure), rhs_op = rhs->Execute(closure);
        if (auto[lhs_num_ptr, rhs_num_ptr] = pair(lhs_op.TryAs<Runtime::Number>(), rhs_op.TryAs<Runtime::Number>());
                lhs_num_ptr != nullptr && rhs_num_ptr != nullptr) {
            return ObjectHolder::Own(Runtime::Number(lhs_num_ptr->GetValue() * rhs_num_ptr->GetValue()));
        }
        if (auto lhs_inst_ptr = lhs_op.TryAs<Runtime::ClassInstance>(); lhs_inst_ptr != nullptr) {
            try {
                return lhs_inst_ptr->Call("__mul__", {rhs_op});
            }
            catch (const runtime_error &e) {
                throw runtime_error("statement.cpp Mul::Execute [inst wo __mul__()]");
            }
        }
        throw runtime_error("statement.cpp Mult::Execute");
    }

    ObjectHolder Div::Execute(Runtime::Closure &closure) {
        auto lhs_op = lhs->Execute(closure), rhs_op = rhs->Execute(closure);
        if (auto[lhs_num_ptr, rhs_num_ptr] = pair(lhs_op.TryAs<Runtime::Number>(), rhs_op.TryAs<Runtime::Number>());
                lhs_num_ptr != nullptr && rhs_num_ptr != nullptr) {
            return ObjectHolder::Own(Runtime::Number(lhs_num_ptr->GetValue() / rhs_num_ptr->GetValue()));
        }
        if (auto lhs_inst_ptr = lhs_op.TryAs<Runtime::ClassInstance>(); lhs_inst_ptr != nullptr) {
            try {
                return lhs_inst_ptr->Call("__div__", {rhs_op});
            }
            catch (const runtime_error &e) {
                throw runtime_error("statement.cpp Div::Execute [inst wo __div__()]");
            }
        }
        throw runtime_error("statement.cpp Div::Execute");
    }

    ObjectHolder Compound::Execute(Closure &closure) {
        ObjectHolder res;
        for (const auto &st : this->statements) {
            res = st->Execute(closure);
            if (dynamic_cast<Return *>(st.get()) != nullptr) {  // if return is before end of compound
                return res;
            }
            if (dynamic_cast<IfElse *>(st.get()) != nullptr) {
                if (res) {
                    return res;
                }
            }
        }
        return ObjectHolder::None();
    }

    ObjectHolder Return::Execute(Closure &closure) {
        return statement->Execute(closure);
    }

    ClassDefinition::ClassDefinition(ObjectHolder class_) : cls(move(class_)), class_name(cls.TryAs<Runtime::Class>()->GetName()) {}

    ObjectHolder ClassDefinition::Execute(Runtime::Closure &closure) {
        closure.insert({class_name, cls});
        return cls;
    }

    FieldAssignment::FieldAssignment(
            VariableValue object, std::string field_name, std::unique_ptr<Statement> rv
    )
            : object(std::move(object)), field_name(std::move(field_name)), right_value(std::move(rv)) {
    }

    ObjectHolder FieldAssignment::Execute(Runtime::Closure &closure) {
        Closure *closure_to_assign = &closure;
        for (int i = 0; i < object.dotted_ids.size(); i++) {
            closure_to_assign = &closure_to_assign->at(object.dotted_ids.at(i)).TryAs<Runtime::ClassInstance>()->Fields();
        }
        closure_to_assign->insert_or_assign(this->field_name, right_value->Execute(closure));
        return closure_to_assign->at(this->field_name);
    }

    IfElse::IfElse(
            std::unique_ptr<Statement> condition,
            std::unique_ptr<Statement> if_body,
            std::unique_ptr<Statement> else_body
    ) : condition(move(condition)), if_body(move(if_body)), else_body(move(else_body)) {}

    ObjectHolder IfElse::Execute(Runtime::Closure &closure) {
        if (Runtime::IsTrue(condition->Execute(closure))) {
            return if_body->Execute(closure);
        } else {
            if (else_body) {
                return else_body->Execute(closure);
            } else {
                return ObjectHolder::None();
            }
        }
    }

    ObjectHolder Or::Execute(Runtime::Closure &closure) {
        auto lhs_op = lhs->Execute(closure), rhs_op = rhs->Execute(closure);
        return ObjectHolder::Own(Runtime::Bool(Runtime::IsTrue(lhs_op) || Runtime::IsTrue(rhs_op)));
    }

    ObjectHolder And::Execute(Runtime::Closure &closure) {
        auto lhs_op = lhs->Execute(closure), rhs_op = rhs->Execute(closure);
        return ObjectHolder::Own(Runtime::Bool(Runtime::IsTrue(lhs_op) && Runtime::IsTrue(rhs_op)));
    }

    ObjectHolder Not::Execute(Runtime::Closure &closure) {
        auto op = argument->Execute(closure);
        return ObjectHolder::Own(Runtime::Bool(!Runtime::IsTrue(op)));
    }

    Comparison::Comparison(
            Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs
    ) : comparator(move(cmp)), left(move(lhs)), right(move(rhs)) {}

    ObjectHolder Comparison::Execute(Runtime::Closure &closure) {
        auto lhs_op = left->Execute(closure), rhs_op = right->Execute(closure);
        return ObjectHolder::Own(Runtime::Bool(comparator(lhs_op, rhs_op)));
    }

    NewInstance::NewInstance(
            const Runtime::Class &class_, std::vector<std::unique_ptr<Statement>> args
    )
            : class_(class_), args(std::move(args)) {
    }

    NewInstance::NewInstance(const Runtime::Class &class_) : NewInstance(class_, {}) {

    }

    ObjectHolder NewInstance::Execute(Runtime::Closure &closure) {
        ObjectHolder res = ObjectHolder::Own(Runtime::ClassInstance(class_));

        if (res.TryAs<Runtime::ClassInstance>()->HasMethod("__init__", this->args.size())) {
            res.TryAs<Runtime::ClassInstance>()->Call("__init__", TransformFromStToObj(this->args, closure));
        }

        return res;
    }


} /* namespace Ast */
