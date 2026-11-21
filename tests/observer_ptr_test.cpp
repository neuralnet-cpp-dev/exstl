// observer_ptr_test — 非拥有型观察者指针
// 退出码：0 = 通过，1 = 失败。
#include <cstdio>

#include <exstl/observer_ptr.hpp>

#include "test_common.hpp"

namespace
{
struct Base
{
    virtual ~Base() = default;
    int v = 1;
};
struct Derived : Base
{
    int w = 2;
};
} // namespace

int main()
{
    std::printf("observer_ptr:\n");
    int x = 42;

    exstl::observer_ptr<int> p{&x};
    CHECK(p, "truthy when non-null");
    CHECK(*p == 42, "deref");
    CHECK(p.get() == &x, "get");

    exstl::observer_ptr<int> empty;
    CHECK(!empty, "default null");
    CHECK(empty == nullptr, "null compare");

    auto made = exstl::make_observer(x);
    CHECK(made.get() == &x, "make_observer from ref");

    Derived d;
    exstl::observer_ptr<Derived> pd{&d};
    exstl::observer_ptr<Base> pb = pd; // implicit upcast
    CHECK(pb.get() == &d, "upcast conversion");
    CHECK(pb->v == 1, "upcast access");

    p.reset();
    CHECK(!p, "reset nulls");

    return test_summary("observer_ptr_test");
}
