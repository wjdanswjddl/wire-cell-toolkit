#include "WireCellUtil/DetectionIdiom.h"

#include "WireCellUtil/doctest.h"

#include "WireCellUtil/Logging.h"

using namespace WireCell;
using spdlog::debug;

struct Quax { } ;

struct Foo {
    int foo(const Quax* q) const {
        debug("Foo::foo");
        return 1;
    }

    int foo(Quax* q) {
        debug("Bar::foo");
        return 2;
    }

    int foo(Quax* q) const {
        debug("Bar::foo");
        return 3;
    }

    int foo(const Quax* q) {
        debug("Bar::foo");
        return 4;
    }
};

struct Bar {

    // should never get called, wrong method name
    int bar(const Quax* q) const { 
        debug("Bar::bar");
        return 0;
    }
};


template <typename T, typename ...Ts>
using foo_type = decltype(std::declval<T>().foo(std::declval<Ts>()...));

template<typename T>
using has_foo_const_quax = is_detected<foo_type, T, const Quax*>;

template<typename T>
using has_foo_mutable_quax = is_detected<foo_type, T, Quax*>;


// Something to use a type T differently depending on if it has a
// method of the desired signature or not.
struct Use {

    template <class T, std::enable_if_t<has_foo_const_quax<T>::value>* = nullptr>
    auto call_const (const T& t, const Quax* cq) {
        return t.foo(cq);
    }

    template <class T, std::enable_if_t<!has_foo_const_quax<T>::value>* = nullptr>
    int call_const (const T& t, const Quax* q) {
        return 42;
    }

    template <class T, std::enable_if_t<has_foo_mutable_quax<T>::value>* = nullptr>
    auto call_mutable (const T& t, Quax* cq) {
        return t.foo(cq);
    }

    template <class T, std::enable_if_t<!has_foo_mutable_quax<T>::value>* = nullptr>
    int call_mutable (const T& t, Quax* q) {
        return 43;
    }
};

TEST_CASE("detected idiom method signature")
{
    Quax q;

    Foo foo;
    Bar bar;
    Use use;

    {
        auto f = use.call_const(foo,&q);
        debug("use.call(foo,q) -> {}",f);
        CHECK(f == 1);
    }
    {
        auto b = use.call_const(bar,&q);
        debug("use.call(bar,q) -> {}", b);
        CHECK(b == 42);
    }
    {
        auto f = use.call_mutable(foo,&q);
        debug("use.call(foo,q) -> {}",f);
        CHECK(f == 3);
    }
    {
        auto b = use.call_mutable(bar,&q);
        debug("use.call(bar,q) -> {}", b);
        CHECK(b == 43);
    }

}
