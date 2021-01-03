#include "vector.h"

#include "test_runner.h"

using namespace std;

class ClassWithStrangeConstructor {
public:
    int x, y;

    ClassWithStrangeConstructor(int &r, const int &cr) : x(r), y(cr) {
    }
};

void TestInsert() {
    Vector<int> v;
    v.PushBack(1);
    v.PushBack(2);
    auto it = v.Insert(v.cbegin(), 0);
    ASSERT(v.Size() == 3 && v[0] == 0 && v[1] == 1 && v[2] == 2 && it == v.begin());

    it = v.Insert(v.cend(), 3);
    ASSERT(v.Size() == 4 && v[0] == 0 && v[1] == 1 && v[2] == 2 && v[3] == 3 && it + 1 == v.end());
};

void TestEmplace() {
    Vector<ClassWithStrangeConstructor> v;
    int x = 1;
    const int y = 2;
    int z = 3;
    ClassWithStrangeConstructor c(z, z);
    v.PushBack(c);
    auto it = v.Emplace(v.cbegin(), x, y);
    ASSERT(v.Size() == 2 && v[0].x == x && v[0].y == y && v[1].x == z && v[1].y == z && it == v.begin());
};

void TestErase() {
    Vector<int> v;
    v.PushBack(1);
    v.PushBack(2);
    v.PushBack(3);
    auto it = v.Erase(v.cbegin() + 1);
    ASSERT(v.Size() == 2 && v[0] == 1 && v[1] == 3 && it == v.begin() + 1);
};

class X {
public:
    X() : id(static_id++), num(id) {
        cout << "X Ctor(): " << id << ' ' << num << endl;
    }

    X(const X& other) : id(static_id++), num(other.num) {
        cout << "X Ctor(&):" << id << ' ' << num << endl;
    }

    X(X&& other) : id(static_id++), num(other.num) {
        cout << "X Ctor(&&):" << id << ' ' << num << endl;
    }

    X& operator=(const X& other) {
        cout << "X operator=(&):" << id << ' ' << num << " = "  << other.id << ' ' << other.num << endl;
        num = other.num;
        return *this;
    }

    X& operator=(X&& other) {
        cout << "X operator=(&&):" << id << ' ' << num << " = "  << other.id << ' ' << other.num << endl;
        num = other.num;
        return *this;
    }

    ~X() {
        cout << "X Dtor():" << id << ' ' << num << endl;
    }

    static int static_id;
    int id;
    int num;
};

int X::static_id = 0;

ostream& operator<<(ostream& os, const X& x) {
    return os << x.id << ' ' << x.num << endl;
}

void TestInsert2() {
    Vector<X> v(3);
    v.Insert(v.cbegin() + 1, X());
    cout << v[0];
    cout << v[1];
    cout << v[2];
    cout << v[3];
    cout << v[4];
}

int main() {
    TestRunner tr;
    RUN_TEST(tr, TestInsert);
    RUN_TEST(tr, TestEmplace);
    RUN_TEST(tr, TestErase);
    RUN_TEST(tr, TestInsert2);
    return 0;
}
