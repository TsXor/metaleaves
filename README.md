# metaleaves
> Users keep reinventing polymorphic reference themselves, which shows the value of Rust's `dyn trait`.  
> -- ["Kurenai_Misuzu" on ZhiHu](https://www.zhihu.com/people/Kurenai_Misuzu)

And here's another.

~~Honestly, you can simply try Rust and enjoy such polymorphism as a built-in feature if you don't mind.~~

# Requirements
C++20 or later: `requires` is heavily used to ensure correct usage and readable error messages.

# What is it like?
```c++
using metaleaves::metaclass;
using metaleaves::method;

METALEAVES_DEFINE_SYMBOL(add)
METALEAVES_DEFINE_SYMBOL(read)
using MyClass = metaclass<
    method<add, string(string, string), int(int, int)>,
    method<read, void()>
>;

auto cls = MyClass(compatible_object);
cls[read]();
```

# Um... features?
- Define interfaces with a syntax that is quite similar to classes.
- Combine interfaces with a syntax that is similar to inheritance (and also behaves like).
- Almost no macros, IDE can highlight correctly.
- IDE can hint exact overloads in a metaclass for a given symbol.
- Single header, copy then paste then include.
- May have better performance than virtual functions as there are less indirections?
- Data layout for the whole metaclass is truly predictable as there are nothing `virtual`, enabling it to be used on library API. It is even trivially copyable, so you can store it in anywhere with enough space for its size.

# However...
- Sadly, IDE cannot hint symbols like class members, but it can give you red squiggles if you used a symbol that doesn't exist in the metaclass.
- Operator overloading are not propagated. You can manually define them as special symbols and use them like normal member function, but you cannot direct use operators on metaclass objects. (This may be implemented with an update.)
- For inter-language operations, you can export the fat pointer as a piece of binary data and write wrapper functions to specify how to call methods inside it. This will need you to manually repeat methods in the metaclass.
- Parameters passed by value will need an extra move. (For most classes this should be acceptable.)
- Extending is rather composition with special rules than traditional inheritance. It simply removes symbols that exist in extension methods from the base then adds extension methods to tail of method list. Thus, derived metaclasses may not be castable to its base, when it overrides a method in the base class with less overloads.

# Motivation
I saw [Metaclass](https://github.com/Mr-enthalpy/Metaclass-Next-Generation-Polymorphism-in-C-) and [its intro on ZhiHu](https://zhuanlan.zhihu.com/p/690073471) and got some inspiration, then wrote this to catch the thoughts in my brain.

# What does this name mean?
I'm not good at naming and I got this name quite casually from the term `metaclass` and `tuple_leaf`.

> Note: the name `metaclass` is chosen by [Teacher ΔH](https://github.com/Mr-enthalpy) in respect to [Herb Sutter's idea](https://herbsutter.com/2017/07/26/metaclasses-thoughts-on-generative-c/) (although it's hard to say that a polymorphism library has relation with HS's idea).

The `metaclass` is basically a big tuple of function pointers. Operations like casting and extending are actually taking leaves from tuples and re-composing them into new tuples with different member order.

> Wait, you did't see any usage of `std::tuple` in that header? That's right, it uses the same tricks as `tuplet`, but doesn't explicitly use a tuple.
