#pragma once
#ifndef __METALEAVES_HPP__
#define __METALEAVES_HPP__

#include <cstring>
#include <concepts>
#include <type_traits>
#include <utility>

#ifdef _MSC_VER
#define _METALEAVES_INLINE [[msvc::forceinline]]
#elif defined(__GNUC__) || defined(__clang__)
#define _METALEAVES_INLINE [[gnu::always_inline]]
#else
#define _METALEAVES_INLINE
#endif

#ifdef _MSC_VER
#define _METALEAVES_LAMBDA_INLINE [[msvc::forceinline]]
#elif defined(__GNUC__) || defined(__clang__)
#define _METALEAVES_LAMBDA_INLINE __attribute__((always_inline))
#else
#define _METALEAVES_LAMBDA_INLINE
#endif

#define METALEAVES_FORWARD_METHOD(name) ([] <typename T, typename... Args> (T& obj, Args&&... args) _METALEAVES_LAMBDA_INLINE { return obj.name(std::forward<Args>(args)...); })
#define METALEAVES_DEFINE_SYMBOL(name) inline constexpr auto name = metaleaves::symbol<METALEAVES_FORWARD_METHOD(name)>{};

namespace metaleaves {

namespace utils {

template <typename T>
struct type_token { using type = T; };

template <typename T>
inline constexpr auto type_token_value = type_token<T>{};

template <typename... Ts>
struct type_list {
private:
    template <template <typename> typename Pred, typename Result>
    static consteval Result _filter(Result result, type_list<>) { return result; }
    template <template <typename> typename Pred, typename Result, typename T, typename... Left>
    static consteval auto _filter(Result result, type_list<T, Left...>) {
        using Added = Result::template extend<T>;
        if constexpr (Pred<T>::value) return _filter<Pred>(Added{}, type_list<Left...>{});
        else return _filter<Pred>(result, type_list<Left...>{});
    }
public:
    static constexpr auto value() { return type_list<Ts...>{}; }
    template <template <typename...> typename NewType>
    using replace = NewType<Ts...>;
    template <typename... Adds>
    using extend = type_list<Ts..., Adds...>;
    template <template <typename> typename Conv>
    using map = type_list<typename Conv<Ts>::type...>;
    template <template <typename> typename Pred>
    using filter = decltype(_filter<Pred>(std::declval<type_list<>>(), std::declval<type_list<Ts...>>()));
    template <template <typename> typename Pred>
    static constexpr bool any = (Pred<Ts>::value || ...);
    template <template <typename> typename Pred>
    static constexpr bool all = (Pred<Ts>::value && ...);
};

template <typename K, typename V>
struct type_entry {
    using key_type = K;
    using value_type = V;
    auto operator()(type_token<K>) -> type_token<V>;
};

template <typename T>
struct is_type_entry : std::false_type {};
template <typename K, typename V>
struct is_type_entry<type_entry<K, V>> : std::true_type {};
template <typename T>
inline constexpr bool is_type_entry_v = is_type_entry<T>::value;

template <typename... Entries> requires (is_type_entry_v<Entries> && ...)
struct type_dict {
private:
    struct _Mapper : Entries... { using Entries::operator()...; };
public:
    template <typename T>
    static constexpr bool has = std::is_invocable_v<_Mapper, type_token<T>>;
    template <typename T>
    using get = typename std::invoke_result_t<_Mapper, type_token<T>>::type;
};

template <typename... Ts>
struct type_set {
private:
    struct mapper : type_entry<Ts, void>... {};
public:
    template <typename T>
    static constexpr bool has = std::is_invocable_v<mapper, type_token<T>>;
};

} // namespace utils

namespace _ {

template <typename T>
struct export_method_signature {};
template <typename Ret, typename... Args>
struct export_method_signature<Ret(Args...)> { using type = Ret(void*, Args...); };
template <typename T>
using export_method_signature_t = typename export_method_signature<T>::type;

template <typename This, typename Overload>
struct enable_call;
template <typename This, typename Ret, typename... Args>
struct enable_call<This, Ret(Args...)> {
    _METALEAVES_INLINE Ret operator()(Args... args) {
        return static_cast<This*>(this)->funcs(
            static_cast<This*>(this)->self,
            std::forward<Args>(args)...
        );
    }
};

template <typename This, typename... Overloads>
struct enable_call_overloads : enable_call<This, Overloads>... {
    using enable_call<This, Overloads>::operator()...;
};

template <typename Overload>
struct overload_item;
template <typename Ret, typename... Args>
struct overload_item<Ret(Args...)> {
    export_method_signature_t<Ret(Args...)>* func;
    _METALEAVES_INLINE Ret operator()(void* self, Args... args) {
        return func(self, std::forward<Args>(args)...);
    }
};

} // namespace _

template <typename... Overloads> requires (sizeof...(Overloads) > 0)
struct overloads_table : _::overload_item<Overloads>... {
private:
    using _Set = utils::type_set<Overloads...>;
public:
    using _::overload_item<Overloads>::operator()...;

    explicit constexpr overloads_table(_::export_method_signature_t<Overloads>*... funcs):
        _::overload_item<Overloads>{funcs}... {}

    template <typename... Subset>
    static constexpr bool is_subset = (_Set::template has<Subset> && ...);
    template <typename... Subset> requires (sizeof...(Subset) > 0)
    decltype(auto) subset() const
        { return overloads_table<Subset...>(_::overload_item<Subset>::func...); }
    template <typename... Subset>
    decltype(auto) subset(utils::type_list<Subset...>) const { return subset<Subset...>(); }
};

template <typename... Overloads> requires (sizeof...(Overloads) > 0)
struct bound_method : _::enable_call_overloads<bound_method<Overloads...>, Overloads...> {
    using method_type = overloads_table<Overloads...>;
    void* self; method_type funcs;

    explicit constexpr bound_method(void* self_, const method_type& funcs_) : self(self_), funcs(funcs_) {}

    template <typename... Subset> requires (sizeof...(Subset) > 0)
    decltype(auto) subset() const
        { return bound_method<Subset...>(self, funcs.template subset<Subset...>()); }
};

template <typename... Overloads>
bound_method(void* self, const overloads_table<Overloads...>& funcs) -> bound_method<Overloads...>;

/**
 * @brief Represents a method name.
 * 
 * The template parameter should be a lambda that forwards parameters to
 * corresponding method. The macro `METALEAVES_FORWARD_METHOD` can help you
 * write such a lambda simply, but most times you simply use the other macro
 * `METALEAVES_DEFINE_SYMBOL` to define a symbol.
 */
template <auto forwarder_>
struct symbol {
    static constexpr auto forwarder = forwarder_;

    template <typename Overload>
    struct overload;
    template <typename Ret, typename... Args>
    struct overload<Ret(Args...)> {
        template <typename T>
        static constexpr bool capable = std::is_invocable_r_v<Ret, decltype(forwarder), T&, Args&&...>;
        template <typename T>
        static Ret export_method(void* self, Args... args)
            { return forwarder(*reinterpret_cast<T*>(self), std::forward<Args>(args)...); }
    };
};

template <typename T>
struct is_symbol : std::false_type {};
template <auto forwarder>
struct is_symbol<symbol<forwarder>> : std::true_type {};
template <typename T>
inline constexpr bool is_symbol_v = is_symbol<T>::value;

/**
 * @brief Method marker type. For usage see `metaclass`.
 */
template <auto symbol, typename... Overloads>
    requires (is_symbol_v<decltype(symbol)> && (std::is_function_v<Overloads> && ...))
struct method {
    using symbol_type = decltype(symbol);
    using export_type = overloads_table<Overloads...>;
    using bound_type = bound_method<Overloads...>;
    using overload_list = utils::type_list<Overloads...>;

    template <typename SubMethod>
    static constexpr bool is_submethod =
        SubMethod::overload_list::template replace<export_type::template is_subset>;
    template <typename T>
    static constexpr bool capable =
        (symbol_type::template overload<Overloads>::template capable<T> && ...);
    template <typename T>
    static decltype(auto) export_table()
        { return export_type(symbol_type::template overload<Overloads>::template export_method<T>...); }
};

template <typename T>
struct is_method : std::false_type {};
template <auto symbol, typename... Overloads>
struct is_method<method<symbol, Overloads...>> : std::true_type {};
template <typename T>
inline constexpr bool is_method_v = is_method<T>::value;

namespace _ {

template <typename Method>
struct method_item : Method::export_type {
private:
    using _Base = typename Method::export_type;
public:
    explicit constexpr method_item(const _Base& funcs) : _Base(funcs) {}
    template <typename T>
    explicit constexpr method_item(utils::type_token<T>) : _Base(Method::template export_table<T>()) {}
};

} // namespace _

template <typename... Methods>
struct methods_table : _::method_item<Methods>... {
private:
    template <typename Method>
    using _Entry = utils::type_entry<typename Method::symbol_type, Method>;
    using _Mapping = utils::type_dict<_Entry<Methods>...>;
public:
    using methods_list = utils::type_list<Methods...>;

    explicit constexpr methods_table(const typename Methods::export_type&... meths):
        _::method_item<Methods>(meths)... {}
    template <typename T>
    explicit constexpr methods_table(utils::type_token<T> t):
        _::method_item<Methods>(t)... {}

    template <typename Symbol>
    static constexpr bool has = _Mapping::template has<Symbol>;
    template <typename Symbol>
    decltype(auto) get() const {
        using Method = _Mapping::template get<Symbol>;
        return static_cast<const _::method_item<Method>&>(*this);
    }

    template <typename Method>
    static constexpr bool is_submethod = 
        _Mapping::template has<typename Method::symbol_type> &&
        _Mapping::template get<typename Method::symbol_type>::template is_submethod<Method>;
    template <typename Method>
    decltype(auto) submethod() const
        { return get<typename Method::symbol_type>().subset(Method::overload_list::value()); }
    template <typename... Subset>
    static constexpr bool is_subset = (is_submethod<Subset> && ...);
    template <typename... Subset> requires (sizeof...(Subset) > 0)
    decltype(auto) subset() const { return methods_table<Subset...>(submethod<Subset>()...); }
    template <typename... Subset>
    decltype(auto) subset(utils::type_list<Subset...>) const { return subset<Subset...>(); }
};

/**
 * @brief This is what you want, or call it `dyn trait`.
 * 
 * Usage example:
 * ```
 * // define an interface
 * using MyInterface = metaclass<
 *   method<simple, int(int)>,
 *   method<overloaded, void(), void(const std::string&)>
 * >;
 * // extract interface from any compatible object reference
 * auto iface = MyInterface(object);
 * // call method from interface
 * int n = iface[simple](42);
 * ```
 */
template <typename... Methods> requires (is_method_v<Methods> && ...)
struct metaclass {
    void* self;
    methods_table<Methods...> methods;

    using methods_list = utils::type_list<Methods...>;
    template <typename T>
    static constexpr bool capable = (Methods::template capable<T> && ...);
    template <typename Symbol>
    static constexpr bool contains = decltype(methods)::template has<Symbol>;

    explicit constexpr metaclass(void* self_, methods_table<Methods...> methods_):
        self(self_), methods(methods_) {}
    template <typename T> requires (capable<T>)
    explicit constexpr metaclass(T& obj) : self(&obj), methods(utils::type_token_value<T>) {}

    template <typename Symbol> requires (is_symbol_v<Symbol> && contains<Symbol>)
    decltype(auto) operator[](Symbol) const { return bound_method(self, methods.template get<Symbol>()); }
};

template <typename T>
struct is_metaclass : std::false_type {};
template <typename... Methods>
struct is_metaclass<metaclass<Methods...>> : std::true_type {};
template <typename T>
inline constexpr bool is_metaclass_v = is_metaclass<T>::value;

/**
 * @brief Cast a metaclass to a "subset".
 * 
 * When cast is not possible, it will cause a compile-time error.
 * 
 * Note that this is structural, a class can be casted to another when they have no
 * relationship with each other.
 */
template <typename To, typename From>
    requires (is_metaclass_v<From> && is_metaclass_v<To>)
To meta_cast(const From& source) {
    return To(source.self, source.methods.subset(To::methods_list::value()));
}

/**
 * @brief Cast a metaclass to pointer of a specified type, returns `nullptr`
 * when the metaclass is not created from that type.
 * 
 * When it is known that the metaclass cannot be created from target type, a
 * compile-time error will be raised.
 * 
 * Note that cast is successful only when the referenced object has exactly
 * the same type as target type. Cast will fail when target type is base type
 * of the real type of the referenced object, whether `virtual` or not.
 */
template <typename To, typename From> requires (is_metaclass_v<From>)
To* reify_cast(const From& source) {
    static_assert(From::template capable<To>, "impossible cast");
    using MethodsTable = decltype(source.methods);
    auto should_be = MethodsTable(utils::type_token_value<To>);
    bool okay = std::memcmp(&source.methods, &should_be, sizeof(MethodsTable)) == 0;
    return okay ? reinterpret_cast<To*>(source.self) : nullptr;
}

namespace _ {

template <typename... ExtMethods>
struct extend_methods {
private:
    using _Set = utils::type_set<typename ExtMethods::symbol_type...>;
    template <typename Method>
    struct _NotDup { static constexpr bool value = !_Set::template has<typename Method::symbol_type>; };
public:
    template <typename BaseClass>
    using on = typename BaseClass::methods_list
        ::template filter<_NotDup>::template extend<ExtMethods...>::template replace<metaclass>;
};

template <typename... Classes>
struct merge_bases;
template <typename Class>
struct merge_bases<Class> { using type = Class; };
template <typename... AMethods, typename... BMethods, typename... Left>
struct merge_bases<metaclass<AMethods...>, metaclass<BMethods...>, Left...> {
private:
    using _Set = utils::type_set<typename AMethods::symbol_type...>;
    static constexpr bool _has_duplicate = (_Set::template has<typename BMethods::symbol_type> || ...);
    static_assert(!_has_duplicate, "duplicate methods in bases");
    using _Merge = metaclass<AMethods..., BMethods...>;
public:
    using type = typename merge_bases<_Merge, Left...>::type;
};
template <typename... Classes>
using merge_bases_t = typename merge_bases<Classes...>::type;

} // namespace _

/**
 * @brief Extends one or more base metaclass with methods.
 * 
 * Duplicate symbols in bases results in compile-time error.
 * Extension methods will completely override the method with same symbol in
 * bases, i.e. any overload from bases will be dropped instead of merged.
 * 
 * Usage example:
 * ```
 * using Derived = extends<Base>::with<
 *   method<meth, void()>
 * >;
 * ```
 */
template <typename... Bases>
struct extends {
private:
    using _SumBase = _::merge_bases_t<Bases...>;
public:
    template <typename... Methods>
    using with = typename _::extend_methods<Methods...>::template on<_SumBase>;
};

} // namespace metaleaves

#endif // __METALEAVES_HPP__
