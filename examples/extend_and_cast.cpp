#include <string>
#include <iostream>
#include <metaleaves.hpp>

using metaleaves::metaclass;
using metaleaves::method;
using metaleaves::extends;
using metaleaves::meta_cast;
using metaleaves::reify_cast;

METALEAVES_DEFINE_SYMBOL(info)
using IAnimal = metaclass<
    method<info, void()>
>;
METALEAVES_DEFINE_SYMBOL(woof)
using IDog = extends<IAnimal>::with<
    method<woof, void()>
>;
METALEAVES_DEFINE_SYMBOL(meow)
using ICat = extends<IAnimal>::with<
    method<meow, void()>
>;

struct Dog {
    std::string name;
    void info() { std::cout << "A dog named " << name << ".\n"; }
    void woof() { std::cout << "Woof.\n"; }
};
struct Cat {
    std::string name;
    void info() { std::cout << "A cat named " << name << ".\n"; }
    void meow() { std::cout << "Meow.\n"; }
};

int main() {
    auto cat = Cat{"foo"};
    auto icat = ICat(cat);
    icat[meow]();

    auto dog = Dog{"bar"};
    auto idog = IDog(dog);
    idog[woof]();

    auto animal1 = meta_cast<IAnimal>(idog);
    animal1[info]();
    auto animal2 = meta_cast<IAnimal>(icat);
    animal2[info]();

    std::cout << reify_cast<Dog>(animal1) << '\n';
    std::cout << reify_cast<Dog>(animal2) << '\n';
    std::cout << reify_cast<Cat>(animal1) << '\n';
    std::cout << reify_cast<Cat>(animal2) << '\n';
}
