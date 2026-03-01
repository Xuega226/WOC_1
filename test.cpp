#include "smartptr.h"

#include <cassert>
#include <iostream>
#include <string>

struct Trace {
    static int alive;
    int id;

    explicit Trace(int value) : id(value) { ++alive; }
    ~Trace() { --alive; }
};

int Trace::alive = 0;

void testUniquePtrBasic() {
    uniquePtr<int, normalDeleter> p(new int(42));
    assert(p.get() != nullptr);
    assert(*p == 42);

    uniquePtr<int, normalDeleter> moved(std::move(p));
    assert(p.get() == nullptr);
    assert(moved.get() != nullptr);
    assert(*moved == 42);

    moved.reset(new int(7));
    assert(*moved == 7);

    moved.reset();
    assert(moved.get() == nullptr);
}

void testUniquePtrArray() {
    uniquePtr<int[], arrayDeleter> arr(new int[3]{1, 2, 3});
    assert(arr.get() != nullptr);
    assert(arr[0] == 1);
    assert(arr[1] == 2);
    assert(arr[2] == 3);

    arr[1] = 20;
    assert(arr[1] == 20);
}

void testMakeUnique() {
    auto p = make_unique<std::string>("smart");
    assert(p.get() != nullptr);
    assert(*p == "smart");
}

void testSharedPtrCount() {
    assert(Trace::alive == 0);

    {
        sharedPtr<Trace> p1(new Trace(1));
        assert(Trace::alive == 1);
        assert(p1.useCount() == 1);

        {
            sharedPtr<Trace> p2 = p1;
            assert(p1.useCount() == 2);
            assert(p2.useCount() == 2);
            assert(p2->id == 1);

            sharedPtr<Trace> p3 = std::move(p2);
            assert(p2.get() == nullptr);
            assert(p3.useCount() == 2);
        }

        assert(p1.useCount() == 1);
        p1.reset();
        assert(p1.get() == nullptr);
        assert(p1.useCount() == 0);
    }

    assert(Trace::alive == 0);
}

void testMakeShared() {
    assert(Trace::alive == 0);

    {
        auto p = make_shared<Trace>(99);
        assert(p.get() != nullptr);
        assert(p->id == 99);
        assert(p.useCount() == 1);
        assert(Trace::alive == 1);
    }

    assert(Trace::alive == 0);
}

int main() {
    testUniquePtrBasic();
    testUniquePtrArray();
    testMakeUnique();
    testSharedPtrCount();
    testMakeShared();

    std::cout << "All tests passed.\n";
    return 0;
}
