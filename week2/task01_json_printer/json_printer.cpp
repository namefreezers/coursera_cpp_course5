#include "test_runner.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <stack>
#include <string>

using namespace std;

void PrintJsonString(std::ostream &out, std::string_view str) {
    size_t first_escaped_pos;
    out << "\"";
    while ((first_escaped_pos = str.find_first_of("\"\\")) != std::string::npos) {
        out << str.substr(0, first_escaped_pos);
        out << (str.at(first_escaped_pos) == '\\' ? "\\\\" : "\\\"");
        str = str.substr(first_escaped_pos + 1);
    }
    out << str;
    out << "\"";
}

template<typename OuterContextT>
class ArrayContext_;

template<typename OuterContextT>
class ObjectContextKey;

template<typename OuterContextT>
class ObjectContextValue;

class OuterContext {};

template<typename OuterContextT>
class ArrayContext_ {
public:
    ArrayContext_(std::ostream &out_, OuterContextT &outer_obj_) : out(out_), outer_obj(outer_obj_) {
        out << "[";
    }

    void FinalizeIfNot() {
        if (!is_finalized) {
            out << "]";
            is_finalized = true;
        }
    }

    ~ArrayContext_() {
        FinalizeIfNot();
    }

    ArrayContext_<OuterContextT> &Number(int64_t n) {
        out_comma_if_needed();
        out << n;
        return *this;
    }

    ArrayContext_<OuterContextT> &String(std::string_view str) {
        out_comma_if_needed();
        PrintJsonString(out, str);
        return *this;
    }

    ArrayContext_<OuterContextT> &Boolean(bool b) {
        out_comma_if_needed();
        out << (b ? "true" : "false");
        return *this;
    }

    ArrayContext_<OuterContextT> &Null() {
        out_comma_if_needed();
        out << "null";
        return *this;
    }

    ArrayContext_<ArrayContext_<OuterContextT>> BeginArray() {
        out_comma_if_needed();
        return ArrayContext_<ArrayContext_<OuterContextT>>(out, *this);
    }

    ObjectContextKey<ArrayContext_<OuterContextT>> BeginObject() {
        out_comma_if_needed();
        return ObjectContextKey<ArrayContext_<OuterContextT>>(out, *this, true);
    }

    OuterContextT& EndArray() {
        FinalizeIfNot();
        return outer_obj;
    }

private:
    void out_comma_if_needed() {
        out << (is_first ? "" : ",");
        is_first = false;
    }

    std::ostream &out;
    OuterContextT &outer_obj;
    bool is_first = true;
    bool is_finalized = false;
};

template<typename OuterContextT>
class ObjectContextKey {
public:
    ObjectContextKey(std::ostream &out_, OuterContextT &outer_obj_, bool is_first_) : out(out_), outer_obj(outer_obj_), is_first(is_first_) {
        out << "{";
    }

    void FinalizeIfNot() {
        if (!is_finalized) {
            out << "}";
            is_finalized = true;
        }
    }

    ~ObjectContextKey() {
        FinalizeIfNot();
    }

    ObjectContextValue<OuterContextT> Key(std::string_view str) {
        out_comma_if_needed();
        PrintJsonString(out, str);
        out << ':';
        return ObjectContextValue<OuterContextT>(out, *this);
    }

    OuterContextT& EndObject() {
        FinalizeIfNot();
        return outer_obj;
    }

private:
    void out_comma_if_needed() {
        out << (is_first ? "" : ",");
        is_first = false;
    }

    std::ostream &out;
    OuterContextT &outer_obj;
    bool is_first = true;
    bool is_finalized = false;
};

template<typename OuterContextT>
class ObjectContextValue {
public:
    ObjectContextValue(std::ostream &out_, ObjectContextKey<OuterContextT>& outer_object_key_) : out(out_), outer_object_key(outer_object_key_) {

    }

    ~ObjectContextValue() {
        if (!is_finalized) {
            Null();
            is_finalized = true;
        }
    }

    ObjectContextKey<OuterContextT> &Number(int64_t n) {
        out << n;
        is_finalized = true;
        return outer_object_key;
    }

    ObjectContextKey<OuterContextT> &String(std::string_view str) {
        PrintJsonString(out, str);
        is_finalized = true;
        return outer_object_key;
    }

    ObjectContextKey<OuterContextT> &Boolean(bool b) {
        out << (b ? "true" : "false");
        is_finalized = true;
        return outer_object_key;
    }

    ObjectContextKey<OuterContextT> &Null() {
        out << "null";
        is_finalized = true;
        return outer_object_key;
    }

    ArrayContext_<ObjectContextKey<OuterContextT>> BeginArray() {
        is_finalized = true;
        return ArrayContext_<ObjectContextKey<OuterContextT>>(out, outer_object_key);
    }

    ObjectContextKey<ObjectContextKey<OuterContextT>> BeginObject() {
        is_finalized = true;
        return ObjectContextKey<ObjectContextKey<OuterContextT>>(out, outer_object_key, true);
    }

private:
    std::ostream &out;
    ObjectContextKey<OuterContextT>& outer_object_key;
    bool is_finalized = false;
};

using ArrayContext = ArrayContext_<OuterContext>;

ArrayContext PrintJsonArray(std::ostream &out) {
    auto outer_context = OuterContext();
    return ArrayContext_<OuterContext>(out, outer_context);
}

using ObjectContext = ObjectContextKey<OuterContext>;

ObjectContext PrintJsonObject(std::ostream &out) {
    auto outer_context = OuterContext();
    return ObjectContextKey<OuterContext>(out, outer_context, true);
}

void TestArray() {
    std::ostringstream output;

    {
        auto json = PrintJsonArray(output);
        json
                .Number(5)
                .Number(6)
                .BeginArray()
                .Number(7)
                .EndArray()
                .Number(8)
                .String("bingo!");
    }

    ASSERT_EQUAL(output.str(), R"([5,6,[7],8,"bingo!"])");
}

void TestObject() {
    std::ostringstream output;

    {
        auto json = PrintJsonObject(output);
        json
                .Key("id1").Number(1234)
                .Key("id2").Boolean(false)
                .Key("").Null()
                .Key("\"").String("\\");
    }

    ASSERT_EQUAL(output.str(), R"({"id1":1234,"id2":false,"":null,"\"":"\\"})");
}

void TestAutoClose() {
    std::ostringstream output;

    {
        auto json = PrintJsonArray(output);
        json.BeginArray().BeginObject();
    }

    ASSERT_EQUAL(output.str(), R"([[{}]])");
}

int main() {
    TestRunner tr;
    RUN_TEST(tr, TestArray);
    RUN_TEST(tr, TestObject);
    RUN_TEST(tr, TestAutoClose);

    return 0;
}
