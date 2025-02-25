#include <string>
#include <iostream>
#include <metaleaves.hpp>

using metaleaves::metaclass;
using metaleaves::method;

METALEAVES_DEFINE_SYMBOL(test)
using Testable = metaclass<
    method<test, void(), void(std::string_view)>
>;

struct Foo {
    void test() { std::cout << "Hello foo!\n"; }
    void test(std::string_view text) { std::cout << "Say: " << text << '\n'; }
};

int main() {
    auto foo = Foo{};
    auto ifoo = Testable(foo);
    ifoo[test]();
    ifoo[test]("AW MAN!");
}
