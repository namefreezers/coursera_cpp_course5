#include <iostream>

using namespace std;

int main() {
    int64_t a, b;
    cin >> a >> b;

    uint64_t a_u = static_cast<uint64_t>(a), b_u = static_cast<uint64_t>(b);

    if (a > 0 && b > 0) {
        if (a_u + b_u > INT64_MAX) {
            cout << "Overflow!";
        } else {
            cout << a + b;
        };
    } else if (a < 0 && b < 0) {
        if (a_u + b_u <= INT64_MAX) {
            cout << "Overflow!";
        } else {
            cout << a + b;
        };
    } else {
        cout << a + b;
    }

    return 0;
}
