//
// Copyright (c) 2012-2016 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#if (__cplusplus < 201305L && _MSC_VER < 1900)
#error "Boost.DI requires C++14 support (Clang-3.4+, GCC-5.1+, MSVC-2015+)"
#else
#define BOOST_DI_VERSION 1'0'1
#define BOOST_DI_NAMESPACE_BEGIN \
  namespace boost {              \
  namespace di {                 \
  inline namespace v1_0_1 {
#define BOOST_DI_NAMESPACE_END \
  }                            \
  }                            \
  }
#if !defined(BOOST_DI_CFG_DIAGNOSTICS_LEVEL)
#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 1
#endif
#if defined(BOOST_DI_CFG_FWD)
BOOST_DI_CFG_FWD
#endif
#define __BOOST_DI_COMPILER(arg, ...) __BOOST_DI_COMPILER_IMPL(arg, __VA_ARGS__)
#define __BOOST_DI_COMPILER_IMPL(arg, ...) arg##__VA_ARGS__
#if defined(__clang__)
#define __CLANG__ __BOOST_DI_COMPILER(__clang_major__, __clang_minor__)
#define __BOOST_DI_UNUSED __attribute__((unused))
#define __BOOST_DI_DEPRECATED(...) [[deprecated(__VA_ARGS__)]]
#define __BOOST_DI_TYPE_WKND(T)
#elif defined(__GNUC__)
#define __GCC__
#define __BOOST_DI_UNUSED __attribute__((unused))
#define __BOOST_DI_DEPRECATED(...) [[deprecated(__VA_ARGS__)]]
#define __BOOST_DI_TYPE_WKND(T)
#elif defined(_MSC_VER)
#define __MSVC__
#define __has_include(...) 0
#define __BOOST_DI_UNUSED
#define __BOOST_DI_DEPRECATED(...) __declspec(deprecated(__VA_ARGS__))
#define __BOOST_DI_TYPE_WKND(T) (T &&)
#endif
#if !defined(__has_builtin)
#define __has_builtin(...) 0
#endif
#if !defined(__has_extension)
#define __has_extension(...) 0
#endif
#if defined(__CLANG__)
#if (!BOOST_DI_CFG_DIAGNOSTICS_LEVEL)
#pragma clang diagnostic error "-Wdeprecated-declarations"
#else
#pragma clang diagnostic warning "-Wdeprecated-declarations"
#endif
#pragma clang diagnostic push
#pragma clang diagnostic error "-Wundefined-inline"
#pragma clang diagnostic error "-Wundefined-internal"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined(__GCC__)
#pragma GCC diagnostic error "-Wdeprecated-declarations"
#pragma GCC diagnostic push
#if (__GNUC__ < 6)
#pragma GCC diagnostic error "-Werror"
#endif
#elif defined(__MSVC__)
#pragma warning(disable : 4503)
#pragma warning(disable : 4822)
#pragma warning(disable : 4505)
#endif
#if __has_include(<__config>)
#include <__config>
#endif
#if defined(_LIBCPP_VERSION)
_LIBCPP_BEGIN_NAMESPACE_STD
#else
namespace std {
#endif
template <class>
class shared_ptr;
template <class>
class weak_ptr;
template <class, class>
class unique_ptr;
template <class>
struct char_traits;
template <class...>
class tuple;
template <class, class>
class vector;
template <class, class, class>
class set;
template <class>
class move_iterator;
#if defined(_LIBCPP_VERSION)
_LIBCPP_END_NAMESPACE_STD
#else
}
#endif
namespace std {
template <class>
class initializer_list;
}
namespace boost {
template <class>
class shared_ptr;
}
BOOST_DI_NAMESPACE_BEGIN
struct _ {
  _(...) {}
};
namespace aux {
template <class T>
using owner = T;
template <class...>
struct valid {
  using type = int;
};
template <class... Ts>
using valid_t = typename valid<Ts...>::type;
template <class...>
struct type {};
struct none_type {};
template <class T, T>
struct non_type {};
template <class...>
struct always {
  static constexpr auto value = true;
};
template <class...>
struct never {
  static constexpr auto value = false;
};
template <class T, class...>
struct identity {
  using type = T;
};
template <class...>
struct type_list {
  using type = type_list;
};
template <bool...>
struct bool_list {
  using type = bool_list;
};
template <class T1, class T2>
struct pair {
  using type = pair;
  using first = T1;
  using second = T2;
};
template <class... Ts>
struct inherit : Ts... {
  using type = inherit;
};
template <class... Ts>
struct join {
  using type = type_list<>;
};
template <class T>
struct join<T> {
  using type = T;
};
template <class... T1s, class... T2s, class... Ts>
struct join<type_list<T1s...>, type_list<T2s...>, Ts...> : join<type_list<T1s..., T2s...>, Ts...> {};
template <class... Ts, class... T1s, class... T2s, class... T3s, class... T4s, class... T5s, class... T6s, class... T7s,
          class... T8s, class... T9s, class... T10s, class... T11s, class... T12s, class... T13s, class... T14s, class... T15s,
          class... T16s, class... Us>
struct join<type_list<Ts...>, type_list<T1s...>, type_list<T2s...>, type_list<T3s...>, type_list<T4s...>, type_list<T5s...>,
            type_list<T6s...>, type_list<T7s...>, type_list<T8s...>, type_list<T9s...>, type_list<T10s...>, type_list<T11s...>,
            type_list<T12s...>, type_list<T13s...>, type_list<T14s...>, type_list<T15s...>, type_list<T16s...>, Us...>
    : join<type_list<Ts..., T1s..., T2s..., T3s..., T4s..., T5s..., T6s..., T7s..., T8s..., T9s..., T10s..., T11s..., T12s...,
                     T13s..., T14s..., T15s..., T16s...>,
           Us...> {};
template <class... TArgs>
using join_t = typename join<TArgs...>::type;
template <int...>
struct index_sequence {
  using type = index_sequence;
};
#if __has_builtin(__make_integer_seq)
template <class T, T...>
struct integer_sequence;
template <int... Ns>
struct integer_sequence<int, Ns...> {
  using type = index_sequence<Ns...>;
};
template <int N>
struct make_index_sequence_impl {
  using type = typename __make_integer_seq<integer_sequence, int, N>::type;
};
#else
template <int>
struct make_index_sequence_impl;
template <>
struct make_index_sequence_impl<0> : index_sequence<> {};
template <>
struct make_index_sequence_impl<1> : index_sequence<0> {};
template <>
struct make_index_sequence_impl<2> : index_sequence<0, 1> {};
template <>
struct make_index_sequence_impl<3> : index_sequence<0, 1, 2> {};
template <>
struct make_index_sequence_impl<4> : index_sequence<0, 1, 2, 3> {};
template <>
struct make_index_sequence_impl<5> : index_sequence<0, 1, 2, 3, 4> {};
template <>
struct make_index_sequence_impl<6> : index_sequence<0, 1, 2, 3, 4, 5> {};
template <>
struct make_index_sequence_impl<7> : index_sequence<0, 1, 2, 3, 4, 5, 6> {};
template <>
struct make_index_sequence_impl<8> : index_sequence<0, 1, 2, 3, 4, 5, 6, 7> {};
template <>
struct make_index_sequence_impl<9> : index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8> {};
template <>
struct make_index_sequence_impl<10> : index_sequence<0, 1, 2, 3, 4, 5, 6, 7, 8, 9> {};
#endif
template <int N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;
}
template <class, class = void>
struct named {};
struct no_name {
  constexpr auto operator()() const noexcept { return ""; }
};
template <class, class = int>
struct ctor_traits;
template <class>
struct self {};
struct ignore_policies {};
namespace core {
template <class>
struct any_type_fwd;
template <class>
struct any_type_ref_fwd;
template <class>
struct any_type_1st_fwd;
template <class>
struct any_type_1st_ref_fwd;
struct dependency_base {};
struct injector_base {};
template <class T>
struct dependency__ : T {
  using T::try_create;
  using T::is_referable;
  using T::create;
};
template <class T>
struct injector__ : T {
  using T::try_create;
  using T::create_impl;
  using T::create_successful_impl;
#if defined(__MSVC__)
  template <class... Ts>
  using is_creatable = typename T::template is_creatable<Ts...>;
#else
  using T::is_creatable;
#endif
};
template <class, class...>
struct array;
struct deduced {};
template <class, class TExpected = deduced, class = TExpected, class = no_name, class = void>
class dependency;
}
namespace scopes {
class deduce;
class instance;
class singleton;
class unique;
}
#define __BOOST_DI_REQUIRES(...) typename ::boost::di::v1_0_1::aux::enable_if<__VA_ARGS__, int>::type
#define __BOOST_DI_REQUIRES_MSG(...) typename ::boost::di::v1_0_1::aux::concept_check<__VA_ARGS__>::type
namespace aux {
template <class T>
T&& declval();
template <class T, T V>
struct integral_constant {
  using type = integral_constant;
  static constexpr T value = V;
};
using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;
template <bool B, class T, class F>
struct conditional {
  using type = T;
};
template <class T, class F>
struct conditional<false, T, F> {
  using type = F;
};
template <bool B, class T, class F>
using conditional_t = typename conditional<B, T, F>::type;
template <bool B, class T = void>
struct enable_if {};
template <class T>
struct enable_if<true, T> {
  using type = T;
};
template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;
template <class T>
struct concept_check {
  static_assert(T::value, "constraint not satisfied");
};
template <>
struct concept_check<true_type> {
  using type = int;
};
template <class T>
struct remove_reference {
  using type = T;
};
template <class T>
struct remove_reference<T&> {
  using type = T;
};
template <class T>
struct remove_reference<T&&> {
  using type = T;
};
template <class T>
using remove_reference_t = typename remove_reference<T>::type;
template <class T>
struct remove_pointer {
  using type = T;
};
template <class T>
struct remove_pointer<T*> {
  using type = T;
};
template <class T>
using remove_pointer_t = typename remove_pointer<T>::type;
template <class T>
struct remove_smart_ptr {
  using type = T;
};
template <class T, class TDeleter>
struct remove_smart_ptr<std::unique_ptr<T, TDeleter>> {
  using type = T;
};
template <class T>
struct remove_smart_ptr<std::shared_ptr<T>> {
  using type = T;
};
template <class T>
struct remove_smart_ptr<std::weak_ptr<T>> {
  using type = T;
};
template <class T>
struct remove_smart_ptr<boost::shared_ptr<T>> {
  using type = T;
};
template <class T>
using remove_smart_ptr_t = typename remove_smart_ptr<T>::type;
template <class T>
struct remove_qualifiers {
  using type = T;
};
template <class T>
struct remove_qualifiers<const T> {
  using type = T;
};
template <class T>
struct remove_qualifiers<T&> {
  using type = T;
};
template <class T>
struct remove_qualifiers<const T&> {
  using type = T;
};
template <class T>
struct remove_qualifiers<T*> {
  using type = T;
};
template <class T>
struct remove_qualifiers<const T*> {
  using type = T;
};
template <class T>
struct remove_qualifiers<T* const&> {
  using type = T;
};
template <class T>
struct remove_qualifiers<T* const> {
  using type = T;
};
template <class T>
struct remove_qualifiers<const T* const> {
  using type = T;
};
template <class T>
struct remove_qualifiers<T&&> {
  using type = T;
};
template <class T>
using remove_qualifiers_t = typename remove_qualifiers<T>::type;
template <class T>
struct remove_extent {
  using type = T;
};
template <class T>
struct remove_extent<T[]> {
  using type = T;
};
template <class T>
using remove_extent_t = typename remove_extent<T>::type;
template <class T>
struct deref_type {
  using type = T;
};
template <class T, class TDeleter>
struct deref_type<std::unique_ptr<T, TDeleter>> {
  using type = remove_qualifiers_t<typename deref_type<T>::type>;
};
template <class T>
struct deref_type<std::shared_ptr<T>> {
  using type = remove_qualifiers_t<typename deref_type<T>::type>;
};
template <class T>
struct deref_type<boost::shared_ptr<T>> {
  using type = remove_qualifiers_t<typename deref_type<T>::type>;
};
template <class T>
struct deref_type<std::weak_ptr<T>> {
  using type = remove_qualifiers_t<typename deref_type<T>::type>;
};
template <class T, class TAllocator>
struct deref_type<std::vector<T, TAllocator>> {
  using type = core::array<remove_qualifiers_t<typename deref_type<T>::type>>;
};
template <class TKey, class TCompare, class TAllocator>
struct deref_type<std::set<TKey, TCompare, TAllocator>> {
  using type = core::array<remove_qualifiers_t<typename deref_type<TKey>::type>>;
};
template <class T>
using decay_t = typename deref_type<remove_qualifiers_t<T>>::type;
template <class, class>
struct is_same : false_type {};
template <class T>
struct is_same<T, T> : true_type {};
template <class T, class U>
struct is_base_of : integral_constant<bool, __is_base_of(T, U)> {};
template <class T>
struct is_class : integral_constant<bool, __is_class(T)> {};
template <class T>
struct is_abstract : integral_constant<bool, __is_abstract(T)> {};
template <class T>
struct is_polymorphic : integral_constant<bool, __is_polymorphic(T)> {};
template <class...>
using is_valid_expr = true_type;
#if __has_extension(is_constructible) && !((__clang_major__ == 3) && (__clang_minor__ == 5))
template <class T, class... TArgs>
using is_constructible = integral_constant<bool, __is_constructible(T, TArgs...)>;
#else
template <class T, class... TArgs>
decltype(void(T(declval<TArgs>()...)), true_type{}) test_is_constructible(int);
template <class, class...>
false_type test_is_constructible(...);
template <class T, class... TArgs>
struct is_constructible : decltype(test_is_constructible<T, TArgs...>(0)) {};
#endif
template <class T, class... TArgs>
using is_constructible_t = typename is_constructible<T, TArgs...>::type;
template <class T, class... TArgs>
decltype(void(T{declval<TArgs>()...}), true_type{}) test_is_braces_constructible(int);
template <class, class...>
false_type test_is_braces_constructible(...);
template <class T, class... TArgs>
using is_braces_constructible = decltype(test_is_braces_constructible<T, TArgs...>(0));
template <class T, class... TArgs>
using is_braces_constructible_t = typename is_braces_constructible<T, TArgs...>::type;
#if defined(__MSVC__)
template <class T>
struct is_copy_constructible : integral_constant<bool, __is_constructible(T, const T&)> {};
template <class T>
struct is_default_constructible : integral_constant<bool, __is_constructible(T)> {};
#else
template <class T>
using is_copy_constructible = is_constructible<T, const T&>;
template <class T>
using is_default_constructible = is_constructible<T>;
#endif
#if defined(__CLANG__) || defined(__MSVC__)
template <class T, class U>
struct is_convertible : integral_constant<bool, __is_convertible_to(T, U)> {};
#else
struct test_is_convertible__ {
  template <class T>
  static void test(T);
};
template <class T, class U, class = decltype(test_is_convertible__::test<U>(declval<T>()))>
true_type test_is_convertible(int);
template <class, class>
false_type test_is_convertible(...);
template <class T, class U>
using is_convertible = decltype(test_is_convertible<T, U>(0));
#endif
template <class TSrc, class TDst, class U = remove_qualifiers_t<TDst>>
using is_narrowed = integral_constant<bool, !is_class<TSrc>::value && !is_class<U>::value && !is_same<TSrc, U>::value>;
template <class, class...>
struct is_array : false_type {};
template <class T, class... Ts>
struct is_array<T[], Ts...> : true_type {};
template <class T, class = decltype(sizeof(T))>
true_type is_complete_impl(int);
template <class T>
false_type is_complete_impl(...);
template <class T>
struct is_complete : decltype(is_complete_impl<T>(0)) {};
template <class T, class U, class = decltype(sizeof(U))>
is_base_of<T, U> is_a_impl(int);
template <class T, class U>
false_type is_a_impl(...);
template <class T, class U>
struct is_a : decltype(is_a_impl<T, U>(0)) {};
template <class, class...>
struct is_unique_impl;
template <class...>
struct not_unique : false_type {
  using type = not_unique;
};
template <>
struct not_unique<> : true_type {
  using type = not_unique;
};
template <class T>
struct is_unique_impl<T> : not_unique<> {};
template <class T1, class T2, class... Ts>
struct is_unique_impl<T1, T2, Ts...>
    : conditional_t<is_base_of<type<T2>, T1>::value, not_unique<T2>, is_unique_impl<inherit<T1, type<T2>>, Ts...>> {};
template <class... Ts>
using is_unique = is_unique_impl<none_type, Ts...>;
template <class...>
struct unique;
template <class... Rs, class T, class... Ts>
struct unique<type<Rs...>, T, Ts...> : conditional_t<is_base_of<type<T>, inherit<type<Rs>...>>::value,
                                                     unique<type<Rs...>, Ts...>, unique<type<Rs..., T>, Ts...>> {};
template <class... Rs>
struct unique<type<Rs...>> : type_list<Rs...> {};
template <class... Ts>
using unique_t = typename unique<type<>, Ts...>::type;
template <class T, class... TArgs>
decltype(::boost::di::v1_0_1::aux::declval<T>().operator()(::boost::di::v1_0_1::aux::declval<TArgs>()...),
         ::boost::di::v1_0_1::aux::true_type())
is_callable_with_impl(int);
template <class, class...>
::boost::di::v1_0_1::aux::false_type is_callable_with_impl(...);
template <class T, class... TArgs>
struct is_callable_with : decltype(is_callable_with_impl<T, TArgs...>(0)) {};
struct callable_base_impl {
  void operator()(...) {}
};
template <class T>
struct callable_base : callable_base_impl, aux::conditional_t<aux::is_class<T>::value, T, aux::none_type> {};
template <typename T>
aux::false_type is_callable_impl(T*, aux::non_type<void (callable_base_impl::*)(...), &T::operator()>* = 0);
aux::true_type is_callable_impl(...);
template <class T>
struct is_callable : decltype(is_callable_impl((callable_base<T>*)0)) {};
template <class>
struct function_traits;
template <class R, class... TArgs>
struct function_traits<R (*)(TArgs...)> {
  using result_type = R;
  using args = type_list<TArgs...>;
};
template <class R, class... TArgs>
struct function_traits<R(TArgs...)> {
  using result_type = R;
  using args = type_list<TArgs...>;
};
template <class R, class T, class... TArgs>
struct function_traits<R (T::*)(TArgs...)> {
  using result_type = R;
  using args = type_list<TArgs...>;
};
template <class R, class T, class... TArgs>
struct function_traits<R (T::*)(TArgs...) const> {
  using result_type = R;
  using args = type_list<TArgs...>;
};
template <class T>
using function_traits_t = typename function_traits<T>::args;
}
namespace core {
template <class T, class = typename aux::is_a<injector_base, T>::type>
struct bindings_impl;
template <class T>
struct bindings_impl<T, aux::true_type> {
  using type = typename T::deps;
};
template <class T>
struct bindings_impl<T, aux::false_type> {
  using type = aux::type_list<T>;
};
#if defined(__MSVC__)
template <class... Ts>
struct bindings : aux::join_t<typename bindings_impl<Ts>::type...> {};
template <class... Ts>
using bindings_t = typename bindings<Ts...>::type;
#else
template <class... Ts>
using bindings_t = aux::join_t<typename bindings_impl<Ts>::type...>;
#endif
}
namespace concepts {
template <class T, class...>
struct type_ {
  template <class TName>
  struct named {
    struct is_bound_more_than_once : aux::false_type {};
  };
  struct is_bound_more_than_once : aux::false_type {};
  struct is_neither_a_dependency_nor_an_injector : aux::false_type {};
  struct has_disallowed_qualifiers : aux::false_type {};
  struct is_abstract :
#if (BOOST_DI_CFG_DIAGNOSTICS_LEVEL >= 2)
      // clang-format off
      decltype(
    T{}
   ),
// clang-format on
#endif
      aux::false_type {
  };
  template <class>
  struct is_not_related_to : aux::false_type {};
};
template <class...>
struct any_of : aux::false_type {};
template <class... TDeps>
struct is_supported : aux::is_same<aux::bool_list<aux::always<TDeps>::value...>,
                                   aux::bool_list<(aux::is_constructible<TDeps, TDeps&&>::value &&
                                                   (aux::is_a<core::injector_base, TDeps>::value ||
                                                    aux::is_a<core::dependency_base, TDeps>::value))...>> {};
template <class...>
struct get_not_supported;
template <class T>
struct get_not_supported<T> {
  using type = T;
};
template <class T, class... TDeps>
struct get_not_supported<T, TDeps...>
    : aux::conditional<aux::is_a<core::injector_base, T>::value || aux::is_a<core::dependency_base, T>::value,
                       typename get_not_supported<TDeps...>::type, T> {};
template <class>
struct is_unique;
template <class T, class = int>
struct unique_dependency : aux::type<T> {};
template <class T>
struct unique_dependency<T, __BOOST_DI_REQUIRES(aux::is_a<core::dependency_base, T>::value)>
    : aux::pair<aux::pair<typename T::expected, typename T::name>, typename T::priority> {};
template <class... TDeps>
struct is_unique<aux::type_list<TDeps...>> : aux::is_unique<typename unique_dependency<TDeps>::type...> {};
template <class>
struct get_is_unique_error_impl : aux::true_type {};
template <class T, class TName, class TPriority>
struct get_is_unique_error_impl<aux::not_unique<aux::pair<aux::pair<T, TName>, TPriority>>> {
  using type = typename type_<T>::template named<TName>::is_bound_more_than_once;
};
template <class T, class TPriority>
struct get_is_unique_error_impl<aux::not_unique<aux::pair<aux::pair<T, no_name>, TPriority>>> {
  using type = typename type_<T>::is_bound_more_than_once;
};
template <class T>
struct get_is_unique_error_impl<aux::not_unique<T>> {
  using type = typename type_<T>::is_bound_more_than_once;
};
template <class>
struct get_is_unique_error;
template <class... TDeps>
struct get_is_unique_error<aux::type_list<TDeps...>>
    : get_is_unique_error_impl<typename aux::is_unique<typename unique_dependency<TDeps>::type...>::type> {};
template <class... TDeps>
using boundable_bindings =
    aux::conditional_t<is_supported<TDeps...>::value, typename get_is_unique_error<core::bindings_t<TDeps...>>::type,
                       typename type_<typename get_not_supported<TDeps...>::type>::is_neither_a_dependency_nor_an_injector>;
template <class... Ts>
using get_any_of_error = aux::conditional_t<
    aux::is_same<aux::bool_list<aux::always<Ts>::value...>, aux::bool_list<aux::is_same<aux::true_type, Ts>::value...>>::value,
    aux::true_type, any_of<Ts...>>;
template <bool, class...>
struct is_related {
  static constexpr auto value = true;
};
template <class I, class T>
struct is_related<true, I, T> {
  static constexpr auto value =
      aux::is_callable<T>::value ||
      (aux::is_base_of<I, T>::value || (aux::is_convertible<T, I>::value && !aux::is_narrowed<I, T>::value));
};
template <bool, class>
struct is_abstract {
  static constexpr auto value = false;
};
template <class T>
struct is_abstract<true, T> {
  static constexpr auto value = aux::is_abstract<T>::value;
};
auto boundable_impl(any_of<> &&) -> aux::true_type;
template <class T, class... Ts>
auto boundable_impl(any_of<T, Ts...> &&)
    -> aux::conditional_t<aux::is_same<T, aux::decay_t<T>>::value, decltype(boundable_impl(aux::declval<any_of<Ts...>>())),
                          typename type_<T>::has_disallowed_qualifiers>;
template <class I, class T>
using boundable_impl__ = aux::conditional_t<
    is_related<aux::is_complete<I>::value && aux::is_complete<T>::value, I, T>::value,
    aux::conditional_t<is_abstract<aux::is_complete<T>::value, T>::value, typename type_<T>::is_abstract, aux::true_type>,
    typename type_<T>::template is_not_related_to<I>>;
template <class I, class T>
auto boundable_impl(I&&, T &&) -> aux::conditional_t<aux::is_same<T, aux::decay_t<T>>::value, boundable_impl__<I, T>,
                                                     typename type_<T>::has_disallowed_qualifiers>;
template <class I, class T>
auto boundable_impl(I&&, T&&, aux::valid<> &&)
    -> aux::conditional_t<is_related<aux::is_complete<I>::value && aux::is_complete<T>::value, I, T>::value, aux::true_type,
                          typename type_<T>::template is_not_related_to<I>>;
template <class I, class T>
auto boundable_impl(I* [], T &&) -> aux::conditional_t<aux::is_same<I, aux::decay_t<I>>::value, boundable_impl__<I, T>,
                                                       typename type_<I>::has_disallowed_qualifiers>;
template <class I, class T>
auto boundable_impl(I[], T &&) -> aux::conditional_t<aux::is_same<I, aux::decay_t<I>>::value, boundable_impl__<I, T>,
                                                     typename type_<I>::has_disallowed_qualifiers>;
template <class... TDeps>
auto boundable_impl(aux::type_list<TDeps...> &&) -> boundable_bindings<TDeps...>;
template <class T, class... Ts>
auto boundable_impl(concepts::any_of<Ts...>&&, T &&)
    -> get_any_of_error<decltype(boundable_impl(aux::declval<Ts>(), aux::declval<T>()))...>;
template <class... TDeps>
auto boundable_impl(aux::type<TDeps...> &&) -> typename get_is_unique_error_impl<typename aux::is_unique<TDeps...>::type>::type;
aux::true_type boundable_impl(...);
template <class... Ts>
struct boundable__ {
  using type = decltype(boundable_impl(aux::declval<Ts>()...));
};
template <class... Ts>
using boundable = typename boundable__<Ts...>::type;
}
namespace type_traits {
struct stack {};
struct heap {};
template <class T, class = int>
struct memory_traits {
  using type = stack;
};
template <class T>
struct memory_traits<T*> {
  using type = heap;
};
template <class T>
struct memory_traits<const T&> {
  using type = typename memory_traits<T>::type;
};
template <class T, class TDeleter>
struct memory_traits<std::unique_ptr<T, TDeleter>> {
  using type = heap;
};
template <class T>
struct memory_traits<std::shared_ptr<T>> {
  using type = heap;
};
template <class T>
struct memory_traits<boost::shared_ptr<T>> {
  using type = heap;
};
template <class T>
struct memory_traits<std::weak_ptr<T>> {
  using type = heap;
};
template <class T>
struct memory_traits<T, __BOOST_DI_REQUIRES(aux::is_polymorphic<T>::value)> {
  using type = heap;
};
template <class T>
using memory_traits_t = typename memory_traits<T>::type;
}
namespace concepts {
template <class...>
struct scope {
  struct is_referable {};
  struct try_create {};
  struct create {};
  template <class...>
  struct requires_ : aux::false_type {};
};
template <class T>
struct provider__ {
  template <class TMemory = type_traits::heap>
  aux::conditional_t<aux::is_same<TMemory, type_traits::stack>::value, T, T*> try_get(const TMemory& = {}) const;
  template <class TMemory = type_traits::heap>
  T* get(const TMemory& = {}) const {
    return nullptr;
  }
};
template <class T>
typename scope<T>::template requires_<typename scope<_, _>::is_referable, typename scope<_, _>::try_create,
                                      typename scope<_, _>::create>
    scopable_impl(...);
template <class T>
auto scopable_impl(T &&)
    -> aux::is_valid_expr<typename T::template scope<_, _>::template is_referable<_>,
                          decltype(T::template scope<_, _>::template try_create<_, _>(provider__<_>{})),
                          decltype(aux::declval<typename T::template scope<_, _>>().template create<_, _>(provider__<_>{}))>;
template <class T>
struct scopable__ {
  using type = decltype(scopable_impl<T>(aux::declval<T>()));
};
template <class T>
using scopable = typename scopable__<T>::type;
}
#if !defined(BOOST_DI_CFG_CTOR_LIMIT_SIZE)
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 10
#endif
namespace type_traits {
template <class, class = int>
struct is_injectable : ::boost::di::v1_0_1::aux::false_type {};
template <class T>
struct is_injectable<T, ::boost::di::v1_0_1::aux::valid_t<typename T::boost_di_inject__>>
    : ::boost::di::v1_0_1::aux::true_type {};
struct direct {};
struct uniform {};
template <class T, int>
using get = T;
template <template <class...> class, class, class, class = int>
struct ctor_impl;
template <template <class...> class TIsConstructible, class T>
struct ctor_impl<TIsConstructible, T, aux::index_sequence<>> : aux::type_list<> {};
template <template <class...> class TIsConstructible, class T>
struct ctor_impl<TIsConstructible, T, aux::index_sequence<0>,
                 __BOOST_DI_REQUIRES(TIsConstructible<T, core::any_type_1st_fwd<T>>::value)>
    : aux::type_list<core::any_type_1st_fwd<T>> {};
template <template <class...> class TIsConstructible, class T>
struct ctor_impl<TIsConstructible, T, aux::index_sequence<0>,
                 __BOOST_DI_REQUIRES(!TIsConstructible<T, core::any_type_1st_fwd<T>>::value)>
    : aux::conditional_t<TIsConstructible<T, core::any_type_1st_ref_fwd<T>>::value,
                         aux::type_list<core::any_type_1st_ref_fwd<T>>, aux::type_list<>> {};
template <template <class...> class TIsConstructible, class T, int... Ns>
struct ctor_impl<TIsConstructible, T, aux::index_sequence<Ns...>,
                 __BOOST_DI_REQUIRES((sizeof...(Ns) > 1) && TIsConstructible<T, get<core::any_type_fwd<T>, Ns>...>::value)>
    : aux::type_list<get<core::any_type_fwd<T>, Ns>...> {};
template <template <class...> class TIsConstructible, class T, int... Ns>
struct ctor_impl<TIsConstructible, T, aux::index_sequence<Ns...>,
                 __BOOST_DI_REQUIRES((sizeof...(Ns) > 1) && !TIsConstructible<T, get<core::any_type_fwd<T>, Ns>...>::value)>
    : aux::conditional<TIsConstructible<T, get<core::any_type_ref_fwd<T>, Ns>...>::value,
                       aux::type_list<get<core::any_type_ref_fwd<T>, Ns>...>,
                       typename ctor_impl<TIsConstructible, T, aux::make_index_sequence<sizeof...(Ns)-1>>::type> {};
template <template <class...> class TIsConstructible, class T>
using ctor_impl_t = typename ctor_impl<TIsConstructible, T, aux::make_index_sequence<BOOST_DI_CFG_CTOR_LIMIT_SIZE>>::type;
template <class...>
struct ctor;
template <class T>
struct ctor<T, aux::type_list<>> : aux::pair<uniform, ctor_impl_t<aux::is_braces_constructible, T>> {};
template <class T, class... TArgs>
struct ctor<T, aux::type_list<TArgs...>> : aux::pair<direct, aux::type_list<TArgs...>> {};
template <class T, class = void, class = typename is_injectable<T>::type>
struct ctor_traits__;
template <class T, class, class = typename is_injectable<ctor_traits<T>>::type>
struct ctor_traits_impl;
template <class T, class _>
struct ctor_traits__<T, _, aux::true_type> : aux::pair<T, aux::pair<direct, typename T::boost_di_inject__::type>> {};
template <class T, class _>
struct ctor_traits__<T, _, aux::false_type> : ctor_traits_impl<T, _> {};
template <class T, class _>
struct ctor_traits_impl<T, _, aux::true_type>
    : aux::pair<T, aux::pair<direct, typename ctor_traits<T>::boost_di_inject__::type>> {};
template <class T, class _>
struct ctor_traits_impl<T, _, aux::false_type> : aux::pair<T, typename ctor_traits<T>::type> {};
}
template <class T, class>
struct ctor_traits : type_traits::ctor<T, type_traits::ctor_impl_t<aux::is_constructible, T>> {};
template <class T>
struct ctor_traits<std::initializer_list<T>> {
  using boost_di_inject__ = aux::type_list<>;
};
template <class... Ts>
struct ctor_traits<std::tuple<Ts...>> {
  using boost_di_inject__ = aux::type_list<Ts...>;
};
template <class T>
struct ctor_traits<T, __BOOST_DI_REQUIRES(aux::is_same<std::char_traits<char>, typename T::traits_type>::value)> {
  using boost_di_inject__ = aux::type_list<>;
};
template <class T>
struct ctor_traits<T, __BOOST_DI_REQUIRES(!aux::is_class<T>::value)> {
  using boost_di_inject__ = aux::type_list<>;
};
namespace type_traits {
template <class T>
struct remove_named {
  using type = T;
};
template <class TName, class T>
struct remove_named<named<TName, T>> {
  using type = T;
};
template <class T>
using remove_named_t = typename remove_named<T>::type;
template <class T>
struct add_named {
  using type = named<no_name, T>;
};
template <class TName, class T>
struct add_named<named<TName, T>> {
  using type = named<TName, T>;
};
template <class T>
using add_named_t = typename add_named<T>::type;
template <class T>
struct named_decay {
  using type = aux::decay_t<T>;
};
template <class TName, class T>
struct named_decay<named<TName, T>> {
  using type = named<TName, aux::decay_t<T>>;
};
template <class T>
using named_decay_t = typename named_decay<T>::type;
}
namespace type_traits {
template <class, class T>
struct rebind_traits {
  using type = T;
};
template <class T, class TName, class _>
struct rebind_traits<T, named<TName, _>> {
  using type = named<TName, T>;
};
template <class T, class D, class U>
struct rebind_traits<std::unique_ptr<T, D>, U> {
  using type = std::unique_ptr<U, D>;
};
template <class T, class D, class TName, class _>
struct rebind_traits<std::unique_ptr<T, D>, named<TName, _>> {
  using type = named<TName, std::unique_ptr<T, D>>;
};
template <class T, class U>
struct rebind_traits<std::shared_ptr<T>, U> {
  using type = std::shared_ptr<U>;
};
template <class T, class TName, class _>
struct rebind_traits<std::shared_ptr<T>, named<TName, _>> {
  using type = named<TName, std::shared_ptr<T>>;
};
template <class T, class U>
struct rebind_traits<std::weak_ptr<T>, U> {
  using type = std::weak_ptr<U>;
};
template <class T, class TName, class _>
struct rebind_traits<std::weak_ptr<T>, named<TName, _>> {
  using type = named<TName, std::weak_ptr<T>>;
};
template <class T, class U>
struct rebind_traits<boost::shared_ptr<T>, U> {
  using type = boost::shared_ptr<U>;
};
template <class T, class TName, class _>
struct rebind_traits<boost::shared_ptr<T>, named<TName, _>> {
  using type = named<TName, boost::shared_ptr<T>>;
};
template <class T, class U>
using rebind_traits_t = typename rebind_traits<T, U>::type;
}
namespace core {
template <class T, class... Ts>
struct array_impl {
  using boost_di_inject__ = aux::type_list<Ts...>;
  explicit array_impl(type_traits::remove_named_t<Ts>&&... args)
      : array{static_cast<type_traits::remove_named_t<Ts>&&>(args)...} {}
  T array[sizeof...(Ts)];
};
template <class T, class... Ts>
struct array<T(), Ts...> : T {
  using value_type = typename T::value_type;
  using array_t = array_impl<value_type, type_traits::rebind_traits_t<value_type, Ts>...>;
  using boost_di_inject__ = aux::type_list<array_t&&>;
  template <__BOOST_DI_REQUIRES(
                aux::is_constructible<T, std::move_iterator<value_type*>, std::move_iterator<value_type*>>::value) = 0>
  explicit array(array_t&& a)
      : T(std::move_iterator<value_type*>(a.array), std::move_iterator<value_type*>(a.array + sizeof...(Ts))) {}
};
template <class T>
struct array<T()> : T {
  using boost_di_inject__ = aux::type_list<>;
};
}
namespace type_traits {
template <class _, class T, class... Ts>
struct ctor_traits__<core::array<_, Ts...>, T, aux::false_type>
    : type_traits::ctor_traits__<core::array<aux::remove_smart_ptr_t<aux::remove_qualifiers_t<T>>(), Ts...>> {};
}
namespace wrappers {
template <class TScope, class T, class TObject = std::shared_ptr<T>>
struct shared {
  using scope = TScope;
  template <class>
  struct is_referable_impl : aux::true_type {};
  template <class I>
  struct is_referable_impl<std::shared_ptr<I>> : aux::is_same<I, T> {};
  template <class I>
  struct is_referable_impl<boost::shared_ptr<I>> : aux::false_type {};
  template <class T_>
  using is_referable = is_referable_impl<aux::remove_qualifiers_t<T_>>;
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T*, I*>::value) = 0>
  inline operator std::shared_ptr<I>() const noexcept {
    return object;
  }
  inline operator std::shared_ptr<T>&() noexcept { return object; }
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T*, I*>::value) = 0>
  inline operator boost::shared_ptr<I>() const noexcept {
    struct sp_holder {
      std::shared_ptr<T> object;
      void operator()(...) noexcept { object.reset(); }
    };
    return {object.get(), sp_holder{object}};
  }
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T*, I*>::value) = 0>
  inline operator std::weak_ptr<I>() const noexcept {
    return object;
  }
  inline operator T&() noexcept { return *object; }
  inline operator const T&() const noexcept { return *object; }
  TObject object;
};
template <class TScope, class T>
struct shared<TScope, T&> {
  using scope = TScope;
  template <class>
  struct is_referable : aux::true_type {};
  explicit shared(T& object) : object(&object) {}
  template <class I>
  explicit shared(I);
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T, I>::value) = 0>
  inline operator I() const noexcept {
    return *object;
  }
  inline operator T&() const noexcept { return *object; }
  T* object = nullptr;
};
}
namespace scopes {
aux::false_type has_shared_ptr__(...);
template <class T>
auto has_shared_ptr__(T &&) -> aux::is_valid_expr<decltype(std::shared_ptr<T>{})>;
class singleton {
 public:
  template <class, class T, class = decltype(has_shared_ptr__(aux::declval<T>()))>
  class scope {
   public:
    template <class T_>
    using is_referable = typename wrappers::shared<singleton, T&>::template is_referable<T_>;
    template <class, class, class TProvider>
    static decltype(wrappers::shared<singleton, T&>{aux::declval<TProvider>().get(type_traits::stack{})}) try_create(
        const TProvider&);
    template <class, class, class TProvider>
    auto create(const TProvider& provider) {
      return create_impl(provider);
    }

   private:
    template <class TProvider>
    wrappers::shared<singleton, T&> create_impl(const TProvider& provider) {
      static auto object(provider.get(type_traits::stack{}));
      return wrappers::shared<singleton, T&>(object);
    }
  };
  template <class _, class T>
  class scope<_, T, aux::true_type> {
   public:
    template <class T_>
    using is_referable = typename wrappers::shared<singleton, T>::template is_referable<T_>;
    template <class, class, class TProvider, class T_ = aux::decay_t<decltype(aux::declval<TProvider>().get())>>
    static decltype(wrappers::shared<singleton, T_>{std::shared_ptr<T_>{std::shared_ptr<T_>{aux::declval<TProvider>().get()}}})
    try_create(const TProvider&);
    template <class, class, class TProvider>
    auto create(const TProvider& provider) {
      return create_impl<aux::decay_t<decltype(provider.get())>>(provider);
    }

   private:
    template <class T_, class TProvider>
    auto create_impl(const TProvider& provider) {
      static std::shared_ptr<T_> object{provider.get()};
      return wrappers::shared<singleton, T_, std::shared_ptr<T_>&>{object};
    }
  };
};
}
namespace wrappers {
template <class TScope, class T>
struct unique {
  using scope = TScope;
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T, I>::value) = 0>
  inline operator I() const noexcept {
    return object;
  }
  inline operator T &&() noexcept { return static_cast<T&&>(object); }
  T object;
};
template <class TScope, class T>
struct unique<TScope, T*> {
  using scope = TScope;
#if defined(__MSVC__)
  explicit unique(T* object) : object(object) {}
#endif
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T, I>::value) = 0>
  inline operator I() const noexcept {
    struct scoped_ptr {
      aux::owner<T*> ptr;
      ~scoped_ptr() noexcept { delete ptr; }
    };
    return *scoped_ptr{object}.ptr;
  }
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T*, I*>::value) = 0>
  inline operator aux::owner<I*>() const noexcept {
    return object;
  }
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T*, const I*>::value) = 0>
  inline operator aux::owner<const I*>() const noexcept {
    return object;
  }
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T*, I*>::value) = 0>
  inline operator std::shared_ptr<I>() const noexcept {
    return std::shared_ptr<I>{object};
  }
  template <class I, __BOOST_DI_REQUIRES(aux::is_convertible<T*, I*>::value) = 0>
  inline operator boost::shared_ptr<I>() const noexcept {
    return boost::shared_ptr<I>{object};
  }
  template <class I, class D, __BOOST_DI_REQUIRES(aux::is_convertible<T*, I*>::value) = 0>
  inline operator std::unique_ptr<I, D>() const noexcept {
    return std::unique_ptr<I, D>{object};
  }
  T* object = nullptr;
};
}
namespace scopes {
class unique {
 public:
  template <class, class>
  class scope {
   public:
    template <class>
    using is_referable = aux::false_type;
    template <class T, class, class TProvider>
    static decltype(wrappers::unique<unique, decltype(aux::declval<TProvider>().get(type_traits::memory_traits_t<T>{}))>{
        aux::declval<TProvider>().get(type_traits::memory_traits_t<T>{})})
    try_create(const TProvider&);
    template <class T, class, class TProvider>
    auto create(const TProvider& provider) const {
      using memory = type_traits::memory_traits_t<T>;
      using wrapper = wrappers::unique<unique, decltype(provider.get(memory{}))>;
      return wrapper{provider.get(memory{})};
    }
  };
};
}
namespace type_traits {
template <class T>
struct scope_traits {
  using type = scopes::unique;
};
template <class T>
struct scope_traits<T&> {
  using type = scopes::singleton;
};
template <class T>
struct scope_traits<std::shared_ptr<T>> {
  using type = scopes::singleton;
};
template <class T>
struct scope_traits<boost::shared_ptr<T>> {
  using type = scopes::singleton;
};
template <class T>
struct scope_traits<std::weak_ptr<T>> {
  using type = scopes::singleton;
};
template <class T>
using scope_traits_t = typename scope_traits<T>::type;
}
namespace scopes {
class deduce {
 public:
  template <class TExpected, class TGiven>
  class scope {
   public:
    template <class T>
    using is_referable = typename type_traits::scope_traits_t<T>::template scope<TExpected, TGiven>::template is_referable<T>;
    template <class T, class TName, class TProvider>
    static decltype(typename type_traits::scope_traits_t<T>::template scope<TExpected, TGiven>{}.template try_create<T, TName>(
        aux::declval<TProvider>()))
    try_create(const TProvider&);
    template <class T, class TName, class TProvider>
    auto create(const TProvider& provider) {
      using scope_traits = type_traits::scope_traits_t<T>;
      using scope = typename scope_traits::template scope<TExpected, TGiven>;
      return scope{}.template create<T, TName>(provider);
    }
  };
};
}
namespace concepts {
template <class T>
struct abstract_type {
  struct is_not_bound {
    operator T*() const {
      using constraint_not_satisfied = is_not_bound;
      return constraint_not_satisfied{}.error();
    }
    // clang-format off
    static inline T*
 error(_ = "type is not bound, did you forget to add: 'di::bind<interface>.to<implementation>()'?");
    // clang-format on
  };
  template <class TName>
  struct named {
    struct is_not_bound {
      operator T*() const {
        using constraint_not_satisfied = is_not_bound;
        return constraint_not_satisfied{}.error();
      }
      // clang-format off
      static inline T*
   error(_ = "type is not bound, did you forget to add: 'di::bind<interface>.named(name).to<implementation>()'?");
      // clang-format on
    };
  };
};
template <class TScope, class T>
struct scoped {
  template <class To>
  struct is_not_convertible_to {
    operator To() const {
      using constraint_not_satisfied = is_not_convertible_to;
      return constraint_not_satisfied{}.error();
    }
    // clang-format off
    static inline To
 error(_ = "scoped object is not convertible to the requested type, did you mistake the scope: 'di::bind<T>.in(scope)'?");
    // clang-format on
  };
};
template <class T>
struct scoped<scopes::instance, T> {
  template <class To>
  struct is_not_convertible_to {
    operator To() const {
      using constraint_not_satisfied = is_not_convertible_to;
      return constraint_not_satisfied{}.error();
    }
    // clang-format off
    static inline To
 error(_ = "instance is not convertible to the requested type, verify binding: 'di::bind<T>.to(value)'?");
    // clang-format on
  };
};
template <class T>
struct type {
  struct has_ambiguous_number_of_constructor_parameters {
    template <int Given>
    struct given {
      template <int Expected>
      struct expected {
        operator T*() const {
          using constraint_not_satisfied = expected;
          return constraint_not_satisfied{}.error();
        }
        // clang-format off
        static inline T*
  error(_ = "verify BOOST_DI_INJECT_TRAITS or di::ctor_traits");
        // clang-format on
      };
    };
  };
  struct has_to_many_constructor_parameters {
    template <int TMax>
    struct max {
      operator T*() const {
        using constraint_not_satisfied = max;
        return constraint_not_satisfied{}.error();
      }
      // clang-format off
      static inline T*
   error(_ = "increase BOOST_DI_CFG_CTOR_LIMIT_SIZE value or reduce number of constructor parameters");
      // clang-format on
    };
  };
  struct is_not_exposed {
    operator T() const {
      using constraint_not_satisfied = is_not_exposed;
      return constraint_not_satisfied{}.error();
    }
    // clang-format off
      static inline T
   error(_ = "type is not exposed, did you forget to add: 'di::injector<T>'?");
    // clang-format on
  };
  template <class TName>
  struct named {
    struct is_not_exposed {
      operator T() const {
        using constraint_not_satisfied = is_not_exposed;
        return constraint_not_satisfied{}.error();
      }
      // clang-format off
      static inline T
   error(_ = "type is not exposed, did you forget to add: 'di::injector<BOOST_DI_EXPOSE((named = name)T)>'?");
      // clang-format on
    };
  };
};
template <class>
struct ctor_size;
template <class TInit, class... TCtor>
struct ctor_size<aux::pair<TInit, aux::type_list<TCtor...>>> : aux::integral_constant<int, sizeof...(TCtor)> {};
template <class... TCtor>
struct ctor_size<aux::type_list<TCtor...>> : aux::integral_constant<int, sizeof...(TCtor)> {};
template <class T>
using ctor_size_t = ctor_size<typename type_traits::ctor<T, type_traits::ctor_impl_t<aux::is_constructible, T>>::type>;
template <class TInitialization, class TName, class _, class TCtor, class T = aux::decay_t<_>>
struct creatable_error_impl
    : aux::conditional_t<
          aux::is_polymorphic<T>::value,
          aux::conditional_t<aux::is_same<TName, no_name>::value, typename abstract_type<T>::is_not_bound,
                             typename abstract_type<T>::template named<TName>::is_not_bound>,
          aux::conditional_t<ctor_size_t<T>::value == ctor_size<TCtor>::value,
                             typename type<T>::has_to_many_constructor_parameters::template max<BOOST_DI_CFG_CTOR_LIMIT_SIZE>,
                             typename type<T>::has_ambiguous_number_of_constructor_parameters::template given<
                                 ctor_size<TCtor>::value>::template expected<ctor_size_t<T>::value>>> {};
template <class TInit, class T, class... TArgs>
struct creatable {
  static constexpr auto value = aux::is_constructible<T, TArgs...>::value;
};
template <class T, class... TArgs>
struct creatable<type_traits::uniform, T, TArgs...> {
  static constexpr auto value = aux::is_braces_constructible<T, TArgs...>::value;
};
template <class TInitialization, class TName, class T, class... TArgs>
T creatable_error() {
  return creatable_error_impl<TInitialization, TName, T, aux::type_list<TArgs...>>{};
}
}
namespace scopes {
class instance;
namespace detail {
template <class T, class TExpected, class TGiven>
struct arg {
  using type = T;
  using expected = TExpected;
  using given = TGiven;
};
template <class T>
struct wrapper_traits {
  using type = wrappers::unique<instance, T>;
};
template <class T>
struct wrapper_traits<std::shared_ptr<T>> {
  using type = wrappers::shared<instance, T>;
};
template <class T>
using wrapper_traits_t = typename wrapper_traits<T>::type;
template <class, class = int>
struct has_result_type : ::boost::di::v1_0_1::aux::false_type {};
template <class T>
struct has_result_type<T, ::boost::di::v1_0_1::aux::valid_t<typename T::result_type>> : ::boost::di::v1_0_1::aux::true_type {};
template <class TGiven, class TProvider, class... Ts>
struct is_expr : aux::integral_constant<bool, aux::is_callable_with<TGiven, typename TProvider::injector_t, Ts...>::value &&
                                                  !has_result_type<TGiven>::value> {};
}
template <class T>
struct wrapper {
  inline operator T() noexcept { return static_cast<T&&>(object); }
  T object;
};
class instance {
 public:
  template <class, class TGiven, class = int>
  struct scope {
    template <class>
    using is_referable = aux::false_type;
    explicit scope(const TGiven& object) : object_{object} {}
    template <class, class, class TProvider>
    static wrappers::unique<instance, TGiven> try_create(const TProvider&);
    template <class, class, class TProvider>
    auto create(const TProvider&) const noexcept {
      return wrappers::unique<instance, TGiven>{object_};
    }
    TGiven object_;
  };
  template <class TExpected, class TGiven>
  struct scope<TExpected, std::shared_ptr<TGiven>> {
    template <class T>
    using is_referable = typename wrappers::shared<instance, TGiven>::template is_referable<aux::remove_qualifiers_t<T>>;
    explicit scope(const std::shared_ptr<TGiven>& object) : object_{object} {}
    template <class, class, class TProvider>
    static wrappers::shared<instance, TGiven> try_create(const TProvider&);
    template <class, class, class TProvider>
    auto create(const TProvider&) const noexcept {
      return wrappers::shared<instance, TGiven>{object_};
    }
    std::shared_ptr<TGiven> object_;
  };
  template <class TExpected, class TGiven>
  struct scope<TExpected, std::initializer_list<TGiven>> {
    template <class>
    using is_referable = aux::false_type;
    scope(const std::initializer_list<TGiven>& object) : object_(object) {}
    template <class, class, class TProvider>
    static std::initializer_list<TGiven> try_create(const TProvider&);
    template <class, class, class TProvider>
    auto create(const TProvider&) const noexcept {
      return wrappers::unique<instance, std::initializer_list<TGiven>>{object_};
    }
    std::initializer_list<TGiven> object_;
  };
  template <class TExpected, class TGiven>
  struct scope<TExpected, TGiven&, __BOOST_DI_REQUIRES(!aux::is_callable<TGiven>::value)> {
    template <class>
    using is_referable = aux::true_type;
    explicit scope(TGiven& object) : object_{object} {}
    template <class, class, class TProvider>
    static wrappers::shared<instance, TGiven&> try_create(const TProvider&);
    template <class, class, class TProvider>
    auto create(const TProvider&) const noexcept {
      return object_;
    }
    wrappers::shared<instance, TGiven&> object_;
  };
  template <class TExpected, class TGiven>
  struct scope<TExpected, TGiven, __BOOST_DI_REQUIRES(aux::is_callable<TGiven>::value)> {
    template <class>
    using is_referable =
        aux::integral_constant<bool, !aux::is_callable<TExpected>::value || !detail::has_result_type<TExpected>::value>;
    explicit scope(const TGiven& object) : object_(object) {}
#if defined(__MSVC__)
    template <class T, class, class TProvider>
    static T try_create(const TProvider&) noexcept;
#else
    template <class, class, class TProvider,
              __BOOST_DI_REQUIRES(!detail::is_expr<TGiven, TProvider>::value && aux::is_callable<TGiven>::value &&
                                  aux::is_callable<TExpected>::value) = 0>
    static wrappers::unique<instance, TExpected> try_create(const TProvider&) noexcept;
    template <class T, class, class TProvider,
              __BOOST_DI_REQUIRES(!detail::is_expr<TGiven, TProvider>::value && aux::is_callable_with<TGiven>::value &&
                                  !aux::is_callable<TExpected>::value) = 0>
    static auto try_create(const TProvider&) noexcept
        -> detail::wrapper_traits_t<decltype(aux::declval<typename aux::identity<TGiven, T>::type>()())>;
    template <class, class, class TProvider, __BOOST_DI_REQUIRES(detail::is_expr<TGiven, TProvider>::value) = 0>
    static detail::wrapper_traits_t<decltype(aux::declval<TGiven>()(aux::declval<typename TProvider::injector_t>()))>
    try_create(const TProvider&) noexcept;
    template <class T, class, class TProvider,
              __BOOST_DI_REQUIRES(detail::is_expr<TGiven, TProvider, const detail::arg<T, TExpected, TGiven>&>::value) = 0>
    static detail::wrapper_traits_t<decltype(aux::declval<TGiven>()(aux::declval<typename TProvider::injector_t>(),
                                                                    aux::declval<detail::arg<T, TExpected, TGiven>>()))>
    try_create(const TProvider&) noexcept;
#endif
    template <class, class, class TProvider,
              __BOOST_DI_REQUIRES(!detail::is_expr<TGiven, TProvider>::value && aux::is_callable<TGiven>::value &&
                                  aux::is_callable<TExpected>::value) = 0>
    auto create(const TProvider&) const noexcept {
      return wrappers::unique<instance, TExpected>{object_};
    }
    template <class T, class, class TProvider,
              __BOOST_DI_REQUIRES(!detail::is_expr<TGiven, TProvider>::value && aux::is_callable_with<TGiven>::value &&
                                  !aux::is_callable<TExpected>::value) = 0>
    auto create(const TProvider&) const noexcept {
      using wrapper = detail::wrapper_traits_t<decltype(aux::declval<TGiven>()())>;
      return wrapper{object_()};
    }
    template <class, class, class TProvider, __BOOST_DI_REQUIRES(detail::is_expr<TGiven, TProvider>::value) = 0>
    auto create(const TProvider& provider) noexcept {
      using wrapper = detail::wrapper_traits_t<decltype((object_)(*provider.injector_))>;
      return wrapper{(object_)(*provider.injector_)};
    }
    template <class T, class, class TProvider,
              __BOOST_DI_REQUIRES(detail::is_expr<TGiven, TProvider, const detail::arg<T, TExpected, TGiven>&>::value) = 0>
    auto create(const TProvider& provider) noexcept {
      using wrapper = detail::wrapper_traits_t<decltype((object_)(*provider.injector_, detail::arg<T, TExpected, TGiven>{}))>;
      return wrapper{(object_)(*provider.injector_, detail::arg<T, TExpected, TGiven>{})};
    }
    TGiven object_;
  };
  template <class _, class... Ts>
  class scope<_, aux::type_list<Ts...>> {
    template <class>
    struct injector__;
    template <class TName, class T>
    struct injector__<named<TName, T>> {
      T (*f)(const injector__*) = nullptr;
      explicit injector__(const decltype(f)& ptr) : f(ptr) {}
    };
    struct injector : injector__<Ts>... {
      void (*dtor)(injector*) = nullptr;
      ~injector() noexcept { static_cast<injector*>(this)->dtor(this); }
      template <class TName, class T>
      T create(const named<TName, T>&, const aux::true_type&) const noexcept {
        return static_cast<const injector__<named<TName, T>>*>(this)->f(static_cast<const injector__<named<TName, T>>*>(this));
      }
      template <class T>
      T create(const named<no_name, T>&, const aux::false_type&) const noexcept {
        return typename concepts::type<T>::is_not_exposed{};
      }
      template <class TName, class T>
      T create(const named<TName, T>&, const aux::false_type&) const noexcept {
        return typename concepts::type<T>::template named<TName>::is_not_exposed{};
      }
    };
    template <class TInjector>
    class injector_impl : injector__<Ts>... {
      void (*dtor)(injector_impl*) = nullptr;
      static void dtor_impl(injector_impl* object) { object->~injector_impl(); }
      template <class, class>
      struct create;
      template <class TName, class T>
      struct create<named<TName, T>, aux::true_type> {
        static T impl(const injector__<named<TName, T>>* object) noexcept {
          using type = aux::type<aux::conditional_t<aux::is_same<TName, no_name>::value, T, named<TName, T>>>;
          return static_cast<const core::injector__<TInjector>&>(static_cast<const injector_impl*>(object)->injector_)
              .create_successful_impl(type{});
        }
      };
      template <class TName, class T>
      struct create<named<TName, T>, aux::false_type> {
        static T impl(const injector__<named<TName, T>>* object) noexcept {
          using type = aux::type<aux::conditional_t<aux::is_same<TName, no_name>::value, T, named<TName, T>>>;
          return static_cast<const core::injector__<TInjector>&>(static_cast<const injector_impl*>(object)->injector_)
              .create_impl(type{});
        }
      };
      template <class T>
      struct is_creatable : aux::integral_constant<bool, core::injector__<TInjector>::template is_creatable<T>::value> {};
      template <class TName, class T>
      struct is_creatable<named<TName, T>>
          : aux::integral_constant<bool, core::injector__<TInjector>::template is_creatable<T, TName>::value> {};

     public:
      explicit injector_impl(TInjector&& injector) noexcept
          : injector__<Ts>(&injector_impl::template create<Ts, typename is_creatable<Ts>::type>::impl)...,
            dtor(&injector_impl::dtor_impl),
            injector_(static_cast<TInjector&&>(injector)) {}

     private:
      TInjector injector_;
    };

   public:
    template <class>
    using is_referable = aux::true_type;
    template <class TInjector, __BOOST_DI_REQUIRES(aux::is_a<core::injector_base, TInjector>::value) = 0>
    explicit scope(TInjector&& i) noexcept : injector_((injector*)new injector_impl<TInjector>{static_cast<TInjector&&>(i)}) {}
    scope(scope&& other) noexcept : injector_(other.injector_) { other.injector_ = nullptr; }
    ~scope() noexcept { delete injector_; }
    template <class T, class TName, class TProvider>
    static aux::conditional_t<aux::is_base_of<injector__<named<TName, T>>, injector>::value, T, void> try_create(
        const TProvider&);
    template <class T, class TName, class TProvider>
    auto create(const TProvider&) {
      return wrapper<T>{injector_->create(named<TName, T>{}, aux::is_base_of<injector__<named<TName, T>>, injector>{})};
    }

   private:
    injector* injector_;
  };
};
}
namespace core {
template <class, class>
struct dependency_concept {};
template <class T, class TDependency>
struct dependency_impl : aux::pair<T, TDependency> {};
template <class T>
struct make_dependency_concept {
  using type = dependency_concept<T, no_name>;
};
template <class TName, class T>
struct make_dependency_concept<named<TName, T>> {
  using type = dependency_concept<T, TName>;
};
template <class... Ts, class TName, class TDependency>
struct dependency_impl<dependency_concept<concepts::any_of<Ts...>, TName>, TDependency>
    : aux::pair<dependency_concept<Ts, TName>, TDependency>... {};
template <class... Ts, class TDependency>
struct dependency_impl<dependency_concept<aux::type_list<Ts...>, no_name>, TDependency>
    : aux::pair<typename make_dependency_concept<Ts>::type, TDependency>... {};
struct override {};
template <class TScope, class TExpected, class TGiven, class TName, class TPriority>
class dependency
    : dependency_base,
      TScope::template scope<TExpected, TGiven>,
      public dependency_impl<dependency_concept<TExpected, TName>, dependency<TScope, TExpected, TGiven, TName, TPriority>> {
  template <class, class, class, class, class>
  friend class dependency;
  using scope_t = typename TScope::template scope<TExpected, TGiven>;
  template <class T>
  using externable = aux::integral_constant<bool, aux::always<T>::value && aux::is_same<TScope, scopes::deduce>::value &&
                                                      aux::is_same<TExpected, TGiven>::value>;
  template <class T>
  struct ref_traits {
    using type = T;
  };
  template <int N>
  struct ref_traits<const char (&)[N]> {
    using type = TExpected;
  };
  template <class R, class... Ts>
  struct ref_traits<R (&)(Ts...)> {
    using type = TExpected;
  };
  template <class T>
  struct ref_traits<std::shared_ptr<T>&> {
    using type = std::shared_ptr<T>;
  };
  template <class T, class>
  struct deduce_traits {
    using type = T;
  };
  template <class T>
  struct deduce_traits<deduced, T> {
    using type = aux::decay_t<T>;
  };
  template <class T, class U>
  using deduce_traits_t = typename deduce_traits<T, U>::type;

 public:
  using scope = TScope;
  using expected = TExpected;
  using given = TGiven;
  using name = TName;
  using priority = TPriority;
  dependency() noexcept {}
  template <class T>
  explicit dependency(T&& object) noexcept : scope_t(static_cast<T&&>(object)) {}
  template <class T, __BOOST_DI_REQUIRES(aux::is_same<TName, no_name>::value && !aux::is_same<T, no_name>::value) = 0>
  auto named() noexcept {
    return dependency<TScope, TExpected, TGiven, T, TPriority>{static_cast<dependency&&>(*this)};
  }
  template <class T, __BOOST_DI_REQUIRES(aux::is_same<TName, no_name>::value && !aux::is_same<T, no_name>::value) = 0>
  auto named(const T&) noexcept {
    return dependency<TScope, TExpected, TGiven, T, TPriority>{static_cast<dependency&&>(*this)};
  }
  template <class T, __BOOST_DI_REQUIRES_MSG(concepts::scopable<T>) = 0>
  auto in(const T&)noexcept {
    return dependency<T, TExpected, TGiven, TName, TPriority>{};
  }
  template <class T, __BOOST_DI_REQUIRES(!aux::is_array<TExpected, T>::value) = 0,
            __BOOST_DI_REQUIRES_MSG(concepts::boundable<TExpected, T>) = 0>
  auto to() noexcept {
    return dependency<TScope, TExpected, T, TName, TPriority>{};
  }
  template <class... Ts, __BOOST_DI_REQUIRES(aux::is_array<TExpected, Ts...>::value) = 0>
  auto to() noexcept {
    using type = aux::remove_pointer_t<aux::remove_extent_t<TExpected>>;
    return dependency<TScope, array<type>, array<type, Ts...>, TName, TPriority>{};
  }
  template <class T, __BOOST_DI_REQUIRES_MSG(concepts::boundable<TExpected, T>) = 0>
  auto to(std::initializer_list<T>&& object) noexcept {
    using type = aux::remove_pointer_t<aux::remove_extent_t<TExpected>>;
    using dependency = dependency<scopes::instance, array<type>, std::initializer_list<T>, TName, TPriority>;
    return dependency{object};
  }
  template <class T, __BOOST_DI_REQUIRES(externable<T>::value) = 0,
            __BOOST_DI_REQUIRES_MSG(concepts::boundable<deduce_traits_t<TExpected, T>, aux::decay_t<T>, aux::valid<>>) = 0>
  auto to(T&& object) noexcept {
    using dependency =
        dependency<scopes::instance, deduce_traits_t<TExpected, T>, typename ref_traits<T>::type, TName, TPriority>;
    return dependency{static_cast<T&&>(object)};
  }
  template <class...>
  dependency& to(...) const noexcept;
  auto operator[](const override&) noexcept {
    return dependency<TScope, TExpected, TGiven, TName, override>{static_cast<dependency&&>(*this)};
  }
#if defined(__cpp_variable_templates)
  dependency& operator()() noexcept { return *this; }
#endif
 protected:
  using scope_t::is_referable;
  using scope_t::create;
  using scope_t::try_create;
  template <class, class>
  static void try_create(...);
};
}
namespace core {
template <class = aux::type_list<>>
struct pool;
template <class... TArgs>
using pool_t = pool<aux::type_list<TArgs...>>;
template <class... TArgs>
struct pool<aux::type_list<TArgs...>> : TArgs... {
  template <class... Ts>
  explicit pool(Ts... args) noexcept : Ts(static_cast<Ts&&>(args))... {}
  template <class... Ts, class TPool>
  pool(const aux::type_list<Ts...>&, TPool p) noexcept : pool(static_cast<Ts&&>(p)...) {
    (void)p;
  }
};
}
namespace concepts {
struct call_operator_with_one_argument {};
template <class>
struct policy {
  template <class>
  struct requires_ : aux::false_type {};
};
struct arg_wrapper {
  using type = _;
  using expected = _;
  using given = _;
  using name = no_name;
  using arity = aux::integral_constant<int, 0>;
  using scope = scopes::deduce;
  using is_root = aux::false_type;
  template <class, class, class>
  struct resolve;
};
aux::false_type callable_impl(...);
template <class T, class TArg>
auto callable_impl(const T&& t, TArg&& arg) -> aux::is_valid_expr<decltype(t(arg))>;
template <class...>
struct is_callable_impl;
template <class T, class... Ts>
struct is_callable_impl<T, Ts...> {
  using callable_with_arg = decltype(callable_impl(aux::declval<T>(), arg_wrapper{}));
  using type = aux::conditional_t<callable_with_arg::value, typename is_callable_impl<Ts...>::type,
                                  typename policy<T>::template requires_<call_operator_with_one_argument>>;
};
template <>
struct is_callable_impl<> : aux::true_type {};
template <class... Ts>
struct is_callable : is_callable_impl<Ts...> {};
template <class... Ts>
struct is_callable<core::pool<aux::type_list<Ts...>>> : is_callable_impl<Ts...> {};
template <>
struct is_callable<void> {
  using type = policy<void>::requires_<call_operator_with_one_argument>;
};
template <class... Ts>
using callable = typename is_callable<Ts...>::type;
}
namespace providers {
class stack_over_heap {
 public:
  template <class TInitialization, class T, class... TArgs>
  struct is_creatable {
    static constexpr auto value = concepts::creatable<TInitialization, T, TArgs...>::value;
  };
  template <class T, class... TArgs>
  auto get(const type_traits::direct&, const type_traits::heap&, TArgs&&... args) {
    return new T(static_cast<TArgs&&>(args)...);
  }
  template <class T, class... TArgs>
  auto get(const type_traits::uniform&, const type_traits::heap&, TArgs&&... args) {
    return new T{static_cast<TArgs&&>(args)...};
  }
  template <class T, class... TArgs>
  auto get(const type_traits::direct&, const type_traits::stack&, TArgs&&... args) const noexcept {
    return T(static_cast<TArgs&&>(args)...);
  }
  template <class T, class... TArgs>
  auto get(const type_traits::uniform&, const type_traits::stack&, TArgs&&... args) const noexcept {
    return T{static_cast<TArgs&&>(args)...};
  }
};
}
#if !defined(BOOST_DI_CFG)
#define BOOST_DI_CFG ::boost::di::v1_0_1::config
#endif
template <class... TPolicies, __BOOST_DI_REQUIRES_MSG(concepts::callable<TPolicies...>) = 0>
inline auto make_policies(TPolicies... args) noexcept {
  return core::pool_t<TPolicies...>(static_cast<TPolicies&&>(args)...);
}
struct config {
  template <class T>
  static auto provider(T*) noexcept {
    return providers::stack_over_heap{};
  }
  template <class T>
  static auto policies(T*) noexcept {
    return make_policies();
  }
};
namespace detail {
template <class...>
struct bind;
template <class TScope>
struct bind<int, TScope> {
  using type = core::dependency<TScope>;
};
template <class TScope, class T>
struct bind<int, TScope, T> {
  using type = core::dependency<TScope, T>;
};
template <class TScope, class... Ts>
struct bind<int, TScope, Ts...> {
  using type = core::dependency<TScope, concepts::any_of<Ts...>>;
};
}
template <class... Ts>
#if defined(__cpp_variable_templates)
typename
#else
struct bind :
#endif
    detail::bind<__BOOST_DI_REQUIRES_MSG(concepts::boundable<concepts::any_of<Ts...>>), scopes::deduce, Ts...>::type
#if defined(__cpp_variable_templates)
        bind
#endif
{};
static constexpr __BOOST_DI_UNUSED core::override override{};
static constexpr __BOOST_DI_UNUSED scopes::deduce deduce{};
static constexpr __BOOST_DI_UNUSED scopes::unique unique{};
static constexpr __BOOST_DI_UNUSED scopes::singleton singleton{};
namespace concepts {
struct get {};
struct is_creatable {};
template <class>
struct provider {
  template <class...>
  struct requires_ : aux::false_type {};
};
template <class T>
typename provider<T>::template requires_<get, is_creatable> providable_impl(...);
template <class T>
auto providable_impl(T&& t)
    -> aux::is_valid_expr<decltype(t.template get<_>(type_traits::direct{}, type_traits::heap{})),
                          decltype(t.template get<_>(type_traits::direct{}, type_traits::heap{}, int{})),
                          decltype(t.template get<_>(type_traits::uniform{}, type_traits::stack{})),
                          decltype(t.template get<_>(type_traits::uniform{}, type_traits::stack{}, int{})),
                          decltype(T::template is_creatable<type_traits::direct, type_traits::heap, _>::value),
                          decltype(T::template is_creatable<type_traits::uniform, type_traits::stack, _, int>::value)>;
template <class T>
struct providable__ {
  using type = decltype(providable_impl<T>(aux::declval<T>()));
};
template <class T>
using providable = typename providable__<T>::type;
}
namespace concepts {
template <class>
struct policies {};
struct providable_type {};
struct callable_type {};
template <class>
struct config {
  template <class...>
  struct requires_ : aux::false_type {};
};
template <class TConfig>
struct injector {
  using config = TConfig;
  using deps = aux::type_list<>;
  template <class T>
  T create() const;
};
aux::false_type configurable_impl(...);
template <class T>
auto configurable_impl(T &&)
    -> aux::is_valid_expr<decltype(T::provider((injector<T>*)0)), decltype(T::policies((injector<T>*)0))>;
template <class T1, class T2>
struct get_configurable_error : aux::type_list<T1, T2> {};
template <class T>
struct get_configurable_error<aux::true_type, T> {
  using type = T;
};
template <class T>
struct get_configurable_error<T, aux::true_type> {
  using type = T;
};
template <>
struct get_configurable_error<aux::true_type, aux::true_type> : aux::true_type {};
template <class T>
auto is_configurable(const aux::true_type&) {
  return typename get_configurable_error<decltype(providable<decltype(T::provider((injector<T>*)0))>()),
                                         decltype(callable<decltype(T::policies((injector<T>*)0))>())>::type{};
}
template <class T>
auto is_configurable(const aux::false_type&) {
  return typename config<T>::template requires_<provider<providable_type(...)>, policies<callable_type(...)>>{};
}
template <class T>
struct configurable__ {
  using type = decltype(is_configurable<T>(decltype(configurable_impl(aux::declval<T>())){}));
};
template <class T>
using configurable = typename configurable__<T>::type;
}
namespace core {
class binder {
  template <class TDefault, class>
  static TDefault resolve_impl(...) noexcept {
    return {};
  }
  template <class, class TConcept, class TDependency>
  static decltype(auto) resolve_impl(aux::pair<TConcept, TDependency>* dep) noexcept {
    return static_cast<TDependency&>(*dep);
  }
  template <class, class TConcept, class TScope, class TExpected, class TGiven, class TName>
  static decltype(auto) resolve_impl(
      aux::pair<TConcept, dependency<TScope, TExpected, TGiven, TName, override>>* dep) noexcept {
    return static_cast<dependency<TScope, TExpected, TGiven, TName, override>&>(*dep);
  }
  template <class TDefault, class>
  static TDefault resolve_impl__(...);
  template <class, class TConcept, class TDependency>
  static TDependency resolve_impl__(aux::pair<TConcept, TDependency>*);
  template <class, class TConcept, class TScope, class TExpected, class TGiven, class TName>
  static dependency<TScope, TExpected, TGiven, TName, override> resolve_impl__(
      aux::pair<TConcept, dependency<TScope, TExpected, TGiven, TName, override>>*);
  template <class TDeps, class T, class TName, class TDefault>
  struct resolve__ {
    using type = decltype(resolve_impl__<TDefault, dependency_concept<aux::decay_t<T>, TName>>((TDeps*)0));
  };
#if (defined(__CLANG__) && __CLANG__ >= 3'9)  //
  template <class TDeps, class T>
  static T& resolve_(TDeps* deps, const aux::type<T&>&) noexcept {
    return static_cast<T&>(*deps);
  }
  template <class TDeps, class T>
  static T resolve_(TDeps*, const aux::type<T>&) noexcept {
    return {};
  }
#endif
 public:
  template <class T, class TName = no_name, class TDefault = dependency<scopes::deduce, aux::decay_t<T>>, class TDeps>
  static decltype(auto) resolve(TDeps* deps) noexcept {
    using dependency = dependency_concept<aux::decay_t<T>, TName>;
#if (defined(__CLANG__) && __CLANG__ >= 3'9)  //
    return resolve_(deps, aux::type<decltype(resolve_impl<TDefault, dependency>((TDeps*)0))>{});
#else
    return resolve_impl<TDefault, dependency>(deps);
#endif
  }
  template <class TDeps, class T, class TName = no_name, class TDefault = dependency<scopes::deduce, aux::decay_t<T>>>
  using resolve_t = typename resolve__<TDeps, T, TName, TDefault>::type;
};
}
namespace core {
template <class T, class TInjector, class TError = aux::false_type>
struct is_referable__ {
  static constexpr auto value = dependency__<binder::resolve_t<TInjector, T>>::template is_referable<T>::value;
};
template <class T, class TInjector>
struct is_referable__<T, TInjector, aux::true_type> {
  static constexpr auto value = true;
};
template <class T, class TInjector, class TError>
struct is_creatable__ {
  static constexpr auto value = injector__<TInjector>::template is_creatable<T>::value;
};
template <class T, class TInjector>
struct is_creatable__<T, TInjector, aux::false_type> {
  static constexpr auto value = true;
};
template <class, class>
struct is_copy_ctor__ : aux::false_type {};
template <class T>
struct is_copy_ctor__<T, T> : aux::true_type {};
#if defined(__GCC__) || defined(__MSVC__)
template <class T>
struct is_copy_ctor__<T, const T> : aux::true_type {};
#endif
template <class TParent, class TInjector, class TError = aux::false_type>
struct any_type {
  template <class T, class = __BOOST_DI_REQUIRES(is_creatable__<T, TInjector, TError>::value)>
  operator T() {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<T>{});
  }
  const TInjector& injector_;
};
template <class TParent, class TInjector, class TError = aux::false_type, class TRefError = aux::false_type>
struct any_type_ref {
  template <class T, class = __BOOST_DI_REQUIRES(is_creatable__<T, TInjector, TError>::value)>
  operator T() {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<T>{});
  }
#if defined(__GCC__)
  template <class T, class = __BOOST_DI_REQUIRES(is_referable__<T&&, TInjector, TRefError>::value),
            class = __BOOST_DI_REQUIRES(is_creatable__<T&&, TInjector, TError>::value)>
  operator T &&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<T&&>{});
  }
#endif
  template <class T, class = __BOOST_DI_REQUIRES(is_referable__<T&, TInjector, TRefError>::value),
            class = __BOOST_DI_REQUIRES(is_creatable__<T&, TInjector, TError>::value)>
  operator T&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<T&>{});
  }
  template <class T, class = __BOOST_DI_REQUIRES(is_referable__<const T&, TInjector, TRefError>::value),
            class = __BOOST_DI_REQUIRES(is_creatable__<const T&, TInjector, TError>::value)>
  operator const T&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<const T&>{});
  }
  const TInjector& injector_;
};
template <class TParent, class TInjector, class TError = aux::false_type>
struct any_type_1st {
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value),
            class = __BOOST_DI_REQUIRES(is_creatable__<T, TInjector, TError>::value)>
  operator T() {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<T>{});
  }
  const TInjector& injector_;
};
template <class TParent, class TInjector, class TError = aux::false_type, class TRefError = aux::false_type>
struct any_type_1st_ref {
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value),
            class = __BOOST_DI_REQUIRES(is_creatable__<T, TInjector, TError>::value)>
  operator T() {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<T>{});
  }
#if defined(__GCC__)
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value),
            class = __BOOST_DI_REQUIRES(is_referable__<T&&, TInjector, TRefError>::value),
            class = __BOOST_DI_REQUIRES(is_creatable__<T&&, TInjector, TError>::value)>
  operator T &&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<T&&>{});
  }
#endif
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value),
            class = __BOOST_DI_REQUIRES(is_referable__<T&, TInjector, TRefError>::value),
            class = __BOOST_DI_REQUIRES(is_creatable__<T&, TInjector, TError>::value)>
  operator T&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<T&>{});
  }
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value),
            class = __BOOST_DI_REQUIRES(is_referable__<const T&, TInjector, TRefError>::value),
            class = __BOOST_DI_REQUIRES(is_creatable__<const T&, TInjector, TError>::value)>
  operator const T&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_impl(aux::type<const T&>{});
  }
  const TInjector& injector_;
};
namespace successful {
template <class TParent, class TInjector>
struct any_type {
  template <class T>
  operator T() {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<T>{});
  }
  const TInjector& injector_;
};
template <class TParent, class TInjector>
struct any_type_ref {
  template <class T>
  operator T() {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<T>{});
  }
#if defined(__GCC__)
  template <class T, class = __BOOST_DI_REQUIRES(is_referable__<T&&, TInjector>::value)>
  operator T &&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<T&&>{});
  }
#endif
  template <class T, class = __BOOST_DI_REQUIRES(is_referable__<T&, TInjector>::value)>
  operator T&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<T&>{});
  }
  template <class T, class = __BOOST_DI_REQUIRES(is_referable__<const T&, TInjector>::value)>
  operator const T&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<const T&>{});
  }
  const TInjector& injector_;
};
template <class TParent, class TInjector>
struct any_type_1st {
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value)>
  operator T() {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<T>{});
  }
  const TInjector& injector_;
};
template <class TParent, class TInjector>
struct any_type_1st_ref {
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value)>
  operator T() {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<T>{});
  }
#if defined(__GCC__)
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value),
            class = __BOOST_DI_REQUIRES(is_referable__<T&&, TInjector>::value)>
  operator T &&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<T&&>{});
  }
#endif
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value),
            class = __BOOST_DI_REQUIRES(is_referable__<T&, TInjector>::value)>
  operator T&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<T&>{});
  }
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value),
            class = __BOOST_DI_REQUIRES(is_referable__<const T&, TInjector>::value)>
  operator const T&() const {
    return static_cast<const core::injector__<TInjector>&>(injector_).create_successful_impl(aux::type<const T&>{});
  }
  const TInjector& injector_;
};
}
template <class>
struct any_type_fwd {
  template <class T>
  operator T();

 private:
  template <class T>
  operator const T&() const;
};
template <class>
struct any_type_ref_fwd {
  template <class T>
  operator T();
  template <class T>
  operator T&() const;
#if defined(__GCC__)
  template <class T>
  operator T &&() const;
#endif
  template <class T>
  operator const T&() const;
};
template <class TParent>
struct any_type_1st_fwd {
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value)>
  operator T();

 private:
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value)>
  operator const T&() const;
};
template <class TParent>
struct any_type_1st_ref_fwd {
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value)>
  operator T();
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value)>
  operator T&() const;
#if defined(__GCC__)
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value)>
  operator T &&() const;
#endif
  template <class T, class = __BOOST_DI_REQUIRES(!is_copy_ctor__<TParent, T>::value)>
  operator const T&() const;
};
}
namespace core {
template <class...>
struct arg_wrapper;
template <class T, class TName, class TIsRoot, template <class...> class TList, class... TCtor, class TDependency, class TDeps>
struct arg_wrapper<T, TName, TIsRoot, TList<TCtor...>, TDependency, TDeps> {
  using type __BOOST_DI_UNUSED = T;
  using expected __BOOST_DI_UNUSED = typename TDependency::expected;
  using given __BOOST_DI_UNUSED = typename TDependency::given;
  using name __BOOST_DI_UNUSED = TName;
  using arity __BOOST_DI_UNUSED = aux::integral_constant<int, sizeof...(TCtor)>;
  using scope __BOOST_DI_UNUSED = typename TDependency::scope;
  using is_root __BOOST_DI_UNUSED = TIsRoot;
  template <class T_, class TName_, class TDefault_>
  using resolve = decltype(core::binder::resolve<T_, TName_, TDefault_>((TDeps*)0));
};
template <class T>
struct allow_void : T {};
template <>
struct allow_void<void> : aux::true_type {};
class policy {
  template <class TArg, class TPolicy, class TPolicies>
  static void call_impl(const TPolicies& policies) noexcept {
    static_cast<const TPolicy&>(policies)(TArg{});
  }
  template <class TArg, class TPolicy>
  struct try_call_impl : allow_void<decltype((aux::declval<TPolicy>())(aux::declval<TArg>()))> {};

 public:
  template <class, class>
  struct try_call;
  template <class TArg, class... TPolicies>
  struct try_call<TArg, pool_t<TPolicies...>> : aux::is_same<aux::bool_list<aux::always<TPolicies>::value...>,
                                                             aux::bool_list<try_call_impl<TArg, TPolicies>::value...>> {};
  template <class TArg, class... TPolicies>
  static void call(__BOOST_DI_UNUSED const pool_t<TPolicies...>& policies) noexcept {
    int _[]{0, (call_impl<TArg, TPolicies>(policies), 0)...};
    (void)_;
  }
};
}
namespace core {
#if (BOOST_DI_CFG_DIAGNOSTICS_LEVEL >= 2)
template <class T>
struct creating {
  creating() { type(); }
  static inline T type(_ = "creating...");
};
#endif
template <class, class, class>
struct try_provider;
template <class T, class TInjector, class TProvider, class TInitialization, template <class...> class TList, class... TCtor>
struct try_provider<aux::pair<T, aux::pair<TInitialization, TList<TCtor...>>>, TInjector, TProvider> {
  using injector_t = TInjector;
  template <class>
  struct is_creatable {
    static constexpr auto value =
        TProvider::template is_creatable<TInitialization, T,
                                         typename injector__<TInjector>::template try_create<TCtor>::type...>::value;
  };
  template <class TMemory = type_traits::heap>
  auto get(const TMemory& = {}) const
      -> aux::enable_if_t<is_creatable<TMemory>::value,
                          aux::conditional_t<aux::is_same<TMemory, type_traits::stack>::value, T, aux::remove_reference_t<T>*>>;
};
template <class, class, class>
struct provider;
template <class T, class TName, class TInjector, class TInitialization, template <class...> class TList, class... TCtor>
struct provider<aux::pair<T, aux::pair<TInitialization, TList<TCtor...>>>, TName, TInjector> {
  using provider_t = decltype(TInjector::config::provider((TInjector*)0));
  using injector_t = TInjector;
  template <class, class... TArgs>
  struct is_creatable {
    static constexpr auto value = provider_t::template is_creatable<TInitialization, T, TArgs...>::value;
  };
  template <class TMemory = type_traits::heap>
  auto get(const TMemory& memory = {}) const {
    return get_impl(memory, static_cast<const injector__<TInjector>*>(injector_)->create_impl(aux::type<TCtor>{})...);
  }
  template <class TMemory, class... TArgs, __BOOST_DI_REQUIRES(is_creatable<TMemory, TArgs...>::value) = 0>
  auto get_impl(const TMemory& memory, TArgs&&... args) const {
#if (BOOST_DI_CFG_DIAGNOSTICS_LEVEL >= 2)
    (void)aux::conditional_t<injector__<TInjector>::template is_creatable<T>::value, _, creating<T>>{};
#endif
    return TInjector::config::provider(injector_).template get<T>(TInitialization{}, memory, static_cast<TArgs&&>(args)...);
  }
  template <class TMemory, class... TArgs, __BOOST_DI_REQUIRES(!is_creatable<TMemory, TArgs...>::value) = 0>
  auto get_impl(const TMemory&, TArgs&&...) const {
#if (BOOST_DI_CFG_DIAGNOSTICS_LEVEL > 0)
    return concepts::creatable_error<TInitialization, TName, T*, TArgs...>();
#else
    return nullptr;
#endif
  }
  const TInjector* injector_;
};
namespace successful {
template <class, class>
struct provider;
template <class T, class TInjector, class TInitialization, template <class...> class TList, class... TCtor>
struct provider<aux::pair<T, aux::pair<TInitialization, TList<TCtor...>>>, TInjector> {
  using injector_t = TInjector;
  template <class TMemory = type_traits::heap>
  auto get(const TMemory& memory = {}) const {
    return TInjector::config::provider(injector_).template get<T>(
        TInitialization{}, memory,
        static_cast<const injector__<TInjector>*>(injector_)->create_successful_impl(aux::type<TCtor>{})...);
  }
  const TInjector* injector_;
};
}
}
namespace core {
namespace successful {
template <class T, class TWrapper>
struct wrapper {
  inline operator T() noexcept { return __BOOST_DI_TYPE_WKND(T) wrapper_; }
  TWrapper wrapper_;
};
}
template <class T, class TWrapper, class = int>
struct wrapper_impl {
  inline operator T() noexcept { return wrapper_; }
  TWrapper wrapper_;
};
template <class T, template <class...> class TWrapper, class TScope, class T_, class... Ts>
struct wrapper_impl<T, TWrapper<TScope, T_, Ts...>,
                    __BOOST_DI_REQUIRES(!aux::is_convertible<TWrapper<TScope, T_, Ts...>, T>::value)> {
  inline operator T() noexcept {
    return typename concepts::scoped<TScope, aux::remove_qualifiers_t<T_>>::template is_not_convertible_to<T>{};
  }
  TWrapper<TScope, T_, Ts...> wrapper_;
};
template <class T, class TWrapper>
using wrapper = wrapper_impl<T, TWrapper>;
}
namespace core {
struct from_injector {};
struct from_deps {};
struct init {};
struct with_error {};
template <class>
struct copyable;
template <class T>
struct copyable_impl
    : aux::conditional<
          aux::is_default_constructible<typename T::scope::template scope<typename T::expected, typename T::given>>::value,
          aux::type_list<>, aux::type_list<T>> {};
template <class... TDeps>
struct copyable<aux::type_list<TDeps...>> : aux::join<typename copyable_impl<TDeps>::type...> {};
template <class TDeps>
using copyable_t = typename copyable<TDeps>::type;
template <class T, class>
struct referable {
  using type = T;
};
template <class T, class TDependency>
struct referable<T&, TDependency> {
  using type = aux::conditional_t<TDependency::template is_referable<T&>::value, T&, T>;
};
template <class T, class TDependency>
struct referable<const T&, TDependency> {
  using type = aux::conditional_t<TDependency::template is_referable<const T&>::value, const T&, T>;
};
#if defined(__MSVC__)
template <class T, class TDependency>
struct referable<T&&, TDependency> {
  using type = aux::conditional_t<TDependency::template is_referable<T&&>::value, T&&, T>;
};
#endif
template <class T, class TDependency>
using referable_t = typename referable<T, TDependency>::type;
#if defined(__MSVC__)
template <class T, class TInjector>
inline auto build(TInjector&& injector) noexcept {
  return T{static_cast<TInjector&&>(injector)};
}
#endif
template <class TConfig, class TPolicies = pool<>, class... TDeps>
class injector : injector_base, pool<bindings_t<TDeps...>> {
  friend class binder;
  template <class>
  friend struct pool;
  using pool_t = pool<bindings_t<TDeps...>>;

 protected:
  template <class T, class TName = no_name, class TIsRoot = aux::false_type>
  struct is_creatable {
    using dependency_t = binder::resolve_t<injector, T, TName>;
    using ctor_t = typename type_traits::ctor_traits__<typename dependency_t::given, T>::type;
    using ctor_args_t = typename ctor_t::second::second;
    static constexpr auto value =
        aux::is_convertible<decltype(dependency__<dependency_t>::template try_create<T, TName>(
                                try_provider<ctor_t, injector, decltype(TConfig::provider((injector*)0))>{})),
                            T>::value &&
        policy::template try_call<arg_wrapper<T, TName, TIsRoot, ctor_args_t, dependency_t, pool_t>, TPolicies>::value;
  };

 public:
  using deps = bindings_t<TDeps...>;
  using config = TConfig;
  injector(injector&&) = default;
  template <class... TArgs>
  explicit injector(const init&, TArgs... args) noexcept : injector{from_deps{}, static_cast<TArgs&&>(args)...} {}
  template <class TConfig_, class TPolicies_, class... TDeps_>
  explicit injector(injector<TConfig_, TPolicies_, TDeps_...>&& other) noexcept
      : injector{from_injector{}, static_cast<injector<TConfig_, TPolicies_, TDeps_...>&&>(other), deps{}} {}
  template <class T, __BOOST_DI_REQUIRES(is_creatable<T, no_name, aux::true_type>::value) = 0>
  T create() const {
    return __BOOST_DI_TYPE_WKND(T) create_successful_impl<aux::true_type>(aux::type<T>{});
  }
  template <class T, __BOOST_DI_REQUIRES(!is_creatable<T, no_name, aux::true_type>::value) = 0>
  __BOOST_DI_DEPRECATED("creatable constraint not satisfied")
  T
      // clang-format off
  create
      // clang-format on
      () const {
    return __BOOST_DI_TYPE_WKND(T) create_impl<aux::true_type>(aux::type<T>{});
  }

 protected:
  template <class T>
  struct try_create {
    using type = aux::conditional_t<is_creatable<T>::value, T, void>;
  };
  template <class TParent>
  struct try_create<any_type_fwd<TParent>> {
    using type = any_type<TParent, injector, with_error>;
  };
  template <class TParent>
  struct try_create<any_type_ref_fwd<TParent>> {
    using type = any_type_ref<TParent, injector, with_error>;
  };
  template <class TParent>
  struct try_create<any_type_1st_fwd<TParent>> {
    using type = any_type_1st<TParent, injector, with_error>;
  };
  template <class TParent>
  struct try_create<any_type_1st_ref_fwd<TParent>> {
    using type = any_type_1st_ref<TParent, injector, with_error>;
  };
  template <class TName, class T>
  struct try_create<::boost::di::v1_0_1::named<TName, T>> {
    using type = aux::conditional_t<is_creatable<T, TName>::value, T, void>;
  };
  template <class T>
  struct try_create<self<T>> {
    using type = injector;
  };
  template <class TIsRoot = aux::false_type, class T>
  auto create_impl(const aux::type<T>&) const {
    return create_impl__<TIsRoot, T>();
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_impl(const aux::type<any_type_fwd<TParent>>&) const {
    return any_type<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
    return any_type_ref<TParent, injector, aux::false_type, aux::true_type>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_impl(const aux::type<any_type_1st_fwd<TParent>>&) const {
    return any_type_1st<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_impl(const aux::type<any_type_1st_ref_fwd<TParent>>&) const {
    return any_type_1st_ref<TParent, injector, aux::false_type, aux::true_type>{*this};
  }
  template <class TIsRoot = aux::false_type, class T, class TName>
  auto create_impl(const aux::type<::boost::di::v1_0_1::named<TName, T>>&) const {
    return create_impl__<TIsRoot, T, TName>();
  }
  template <class TIsRoot = aux::false_type, class T>
  auto create_successful_impl(const aux::type<T>&) const {
    return create_successful_impl__<TIsRoot, T>();
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_successful_impl(const aux::type<any_type_fwd<TParent>>&) const {
    return successful::any_type<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_successful_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
    return successful::any_type_ref<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_successful_impl(const aux::type<any_type_1st_fwd<TParent>>&) const {
    return successful::any_type_1st<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_successful_impl(const aux::type<any_type_1st_ref_fwd<TParent>>&) const {
    return successful::any_type_1st_ref<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class T, class TName>
  auto create_successful_impl(const aux::type<::boost::di::v1_0_1::named<TName, T>>&) const {
    return create_successful_impl__<TIsRoot, T, TName>();
  }
  template <class TIsRoot = aux::false_type, class T>
  decltype(auto) create_successful_impl(const aux::type<self<T>>&) const {
    return *this;
  }

 private:
  template <class... TArgs>
  explicit injector(const from_deps&, TArgs... args) noexcept
      : pool_t{copyable_t<deps>{}, core::pool_t<TArgs...>{static_cast<TArgs&&>(args)...}} {}
  template <class TInjector, class... TArgs>
  explicit injector(const from_injector&, TInjector&& injector, const aux::type_list<TArgs...>&) noexcept
#if defined(__MSVC__)
      : pool_t {
    copyable_t<deps>{}, pool_t { build<TArgs>(static_cast<TInjector&&>(injector))... }
  }
#else
      : pool_t {
    copyable_t<deps>{}, pool_t { TArgs{static_cast<TInjector&&>(injector)}... }
  }
#endif
  {}
  template <class TIsRoot = aux::false_type, class T, class TName = no_name>
  auto create_impl__() const {
    auto&& dependency = binder::resolve<T, TName>((injector*)this);
    using dependency_t = aux::remove_reference_t<decltype(dependency)>;
    using ctor_t = typename type_traits::ctor_traits__<typename dependency_t::given, T>::type;
    using provider_t = core::provider<ctor_t, TName, injector>;
    using wrapper_t =
        decltype(static_cast<dependency__<dependency_t>&>(dependency).template create<T, TName>(provider_t{this}));
    using ctor_args_t = typename ctor_t::second::second;
    policy::template call<arg_wrapper<T, TName, TIsRoot, ctor_args_t, dependency_t, pool_t>>(TConfig::policies(this));
    return wrapper<T, wrapper_t>{
        static_cast<dependency__<dependency_t>&>(dependency).template create<T, TName>(provider_t{this})};
  }
  template <class TIsRoot = aux::false_type, class T, class TName = no_name>
  auto create_successful_impl__() const {
    auto&& dependency = binder::resolve<T, TName>((injector*)this);
    using dependency_t = aux::remove_reference_t<decltype(dependency)>;
    using ctor_t = typename type_traits::ctor_traits__<typename dependency_t::given, T>::type;
    using provider_t = successful::provider<ctor_t, injector>;
    using wrapper_t =
        decltype(static_cast<dependency__<dependency_t>&>(dependency).template create<T, TName>(provider_t{this}));
    using create_t = referable_t<T, dependency__<dependency_t>>;
    using ctor_args_t = typename ctor_t::second::second;
    policy::template call<arg_wrapper<T, TName, TIsRoot, ctor_args_t, dependency_t, pool_t>>(TConfig::policies(this));
    return successful::wrapper<create_t, wrapper_t>{
        static_cast<dependency__<dependency_t>&>(dependency).template create<T, TName>(provider_t{this})};
  }
};
template <class TConfig, class... TDeps>
class injector<TConfig, pool<>, TDeps...> : injector_base, pool<bindings_t<TDeps...>> {
  friend class binder;
  template <class>
  friend struct pool;
  using pool_t = pool<bindings_t<TDeps...>>;

 protected:
  template <class T, class TName = no_name, class TIsRoot = aux::false_type>
  struct is_creatable {
    using dependency_t = binder::resolve_t<injector, T, TName>;
    using ctor_t = typename type_traits::ctor_traits__<typename dependency_t::given, T>::type;
    using ctor_args_t = typename ctor_t::second::second;
    static constexpr auto value =
        aux::is_convertible<decltype(dependency__<dependency_t>::template try_create<T, TName>(
                                try_provider<ctor_t, injector, decltype(TConfig::provider((injector*)0))>{})),
                            T>::value;
  };

 public:
  using deps = bindings_t<TDeps...>;
  using config = TConfig;
  injector(injector&&) = default;
  template <class... TArgs>
  explicit injector(const init&, TArgs... args) noexcept : injector{from_deps{}, static_cast<TArgs&&>(args)...} {}
  template <class TConfig_, class TPolicies_, class... TDeps_>
  explicit injector(injector<TConfig_, TPolicies_, TDeps_...>&& other) noexcept
      : injector{from_injector{}, static_cast<injector<TConfig_, TPolicies_, TDeps_...>&&>(other), deps{}} {}
  template <class T, __BOOST_DI_REQUIRES(is_creatable<T, no_name, aux::true_type>::value) = 0>
  T create() const {
    return __BOOST_DI_TYPE_WKND(T) create_successful_impl<aux::true_type>(aux::type<T>{});
  }
  template <class T, __BOOST_DI_REQUIRES(!is_creatable<T, no_name, aux::true_type>::value) = 0>
  __BOOST_DI_DEPRECATED("creatable constraint not satisfied")
  T
      // clang-format off
  create
      // clang-format on
      () const {
    return __BOOST_DI_TYPE_WKND(T) create_impl<aux::true_type>(aux::type<T>{});
  }

 protected:
  template <class T>
  struct try_create {
    using type = aux::conditional_t<is_creatable<T>::value, T, void>;
  };
  template <class TParent>
  struct try_create<any_type_fwd<TParent>> {
    using type = any_type<TParent, injector, with_error>;
  };
  template <class TParent>
  struct try_create<any_type_ref_fwd<TParent>> {
    using type = any_type_ref<TParent, injector, with_error>;
  };
  template <class TParent>
  struct try_create<any_type_1st_fwd<TParent>> {
    using type = any_type_1st<TParent, injector, with_error>;
  };
  template <class TParent>
  struct try_create<any_type_1st_ref_fwd<TParent>> {
    using type = any_type_1st_ref<TParent, injector, with_error>;
  };
  template <class TName, class T>
  struct try_create<::boost::di::v1_0_1::named<TName, T>> {
    using type = aux::conditional_t<is_creatable<T, TName>::value, T, void>;
  };
  template <class T>
  struct try_create<self<T>> {
    using type = injector;
  };
  template <class TIsRoot = aux::false_type, class T>
  auto create_impl(const aux::type<T>&) const {
    return create_impl__<TIsRoot, T>();
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_impl(const aux::type<any_type_fwd<TParent>>&) const {
    return any_type<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
    return any_type_ref<TParent, injector, aux::false_type, aux::true_type>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_impl(const aux::type<any_type_1st_fwd<TParent>>&) const {
    return any_type_1st<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_impl(const aux::type<any_type_1st_ref_fwd<TParent>>&) const {
    return any_type_1st_ref<TParent, injector, aux::false_type, aux::true_type>{*this};
  }
  template <class TIsRoot = aux::false_type, class T, class TName>
  auto create_impl(const aux::type<::boost::di::v1_0_1::named<TName, T>>&) const {
    return create_impl__<TIsRoot, T, TName>();
  }
  template <class TIsRoot = aux::false_type, class T>
  auto create_successful_impl(const aux::type<T>&) const {
    return create_successful_impl__<TIsRoot, T>();
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_successful_impl(const aux::type<any_type_fwd<TParent>>&) const {
    return successful::any_type<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_successful_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
    return successful::any_type_ref<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_successful_impl(const aux::type<any_type_1st_fwd<TParent>>&) const {
    return successful::any_type_1st<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class TParent>
  auto create_successful_impl(const aux::type<any_type_1st_ref_fwd<TParent>>&) const {
    return successful::any_type_1st_ref<TParent, injector>{*this};
  }
  template <class TIsRoot = aux::false_type, class T, class TName>
  auto create_successful_impl(const aux::type<::boost::di::v1_0_1::named<TName, T>>&) const {
    return create_successful_impl__<TIsRoot, T, TName>();
  }
  template <class TIsRoot = aux::false_type, class T>
  decltype(auto) create_successful_impl(const aux::type<self<T>>&) const {
    return *this;
  }

 private:
  template <class... TArgs>
  explicit injector(const from_deps&, TArgs... args) noexcept
      : pool_t{copyable_t<deps>{}, core::pool_t<TArgs...>{static_cast<TArgs&&>(args)...}} {}
  template <class TInjector, class... TArgs>
  explicit injector(const from_injector&, TInjector&& injector, const aux::type_list<TArgs...>&) noexcept
#if defined(__MSVC__)
      : pool_t {
    copyable_t<deps>{}, pool_t { build<TArgs>(static_cast<TInjector&&>(injector))... }
  }
#else
      : pool_t {
    copyable_t<deps>{}, pool_t { TArgs{static_cast<TInjector&&>(injector)}... }
  }
#endif
  {}
  template <class TIsRoot = aux::false_type, class T, class TName = no_name>
  auto create_impl__() const {
    auto&& dependency = binder::resolve<T, TName>((injector*)this);
    using dependency_t = aux::remove_reference_t<decltype(dependency)>;
    using ctor_t = typename type_traits::ctor_traits__<typename dependency_t::given, T>::type;
    using provider_t = core::provider<ctor_t, TName, injector>;
    using wrapper_t =
        decltype(static_cast<dependency__<dependency_t>&>(dependency).template create<T, TName>(provider_t{this}));
    return wrapper<T, wrapper_t>{
        static_cast<dependency__<dependency_t>&>(dependency).template create<T, TName>(provider_t{this})};
  }
  template <class TIsRoot = aux::false_type, class T, class TName = no_name>
  auto create_successful_impl__() const {
    auto&& dependency = binder::resolve<T, TName>((injector*)this);
    using dependency_t = aux::remove_reference_t<decltype(dependency)>;
    using ctor_t = typename type_traits::ctor_traits__<typename dependency_t::given, T>::type;
    using provider_t = successful::provider<ctor_t, injector>;
    using wrapper_t =
        decltype(static_cast<dependency__<dependency_t>&>(dependency).template create<T, TName>(provider_t{this}));
    using create_t = referable_t<T, dependency__<dependency_t>>;
    return successful::wrapper<create_t, wrapper_t>{
        static_cast<dependency__<dependency_t>&>(dependency).template create<T, TName>(provider_t{this})};
  }
};
}
namespace detail {
template <class>
void create(const aux::true_type&) {}
// clang-format off
template <class>
__BOOST_DI_DEPRECATED("creatable constraint not satisfied") void
create
(const aux::false_type&) {}
// clang-format on
template <class, class, class...>
struct injector;
template <class, class>
struct is_creatable_impl;
template <class TInjector, class TName, class T>
struct is_creatable_impl<TInjector, named<TName, T>> {
  static constexpr auto value = core::injector__<TInjector>::template is_creatable<T, TName>::value;
};
template <class TConfig, class T, class... TGivens>
struct injector<TConfig, int, core::dependency<scopes::instance, T, aux::type_list<TGivens...>>>
    : core::injector<TConfig, core::pool<>, core::dependency<scopes::instance, T, aux::type_list<TGivens...>>> {
  template <class... Ts>
  injector(core::injector<Ts...>&& injector) noexcept
      : core::injector<TConfig, core::pool<>, core::dependency<scopes::instance, T, aux::type_list<TGivens...>>>(
            static_cast<core::injector<Ts...>&&>(injector)) {
    using injector_t = core::injector<Ts...>;
    int _[]{0,
            // clang-format off
            (detail::
create<T> (
     aux::integral_constant<bool, is_creatable_impl<injector_t, TGivens>::value>{}),
             0)...};
    // clang-format on
    (void)_;
  }
};
}
template <class T, class... Ts>
using injector = detail::injector<
    BOOST_DI_CFG, __BOOST_DI_REQUIRES_MSG(concepts::boundable<aux::type<T, Ts...>>),
    core::dependency<scopes::instance, aux::unique_t<type_traits::named_decay_t<T>, type_traits::named_decay_t<Ts>...>,
                     aux::type_list<type_traits::add_named_t<T>, type_traits::add_named_t<Ts>...>>>;
// clang-format off
#define __BOOST_DI_EXPOSE_IMPL__(...) decltype(::boost::di::v1_0_1::detail::__VA_ARGS__),
#define __BOOST_DI_EXPOSE_IMPL(...) ::boost::di::v1_0_1::named<__BOOST_DI_EXPOSE_IMPL__ __VA_ARGS__>
#define BOOST_DI_EXPOSE(...) __BOOST_DI_IF(__BOOST_DI_IBP(__VA_ARGS__), __BOOST_DI_EXPOSE_IMPL, __BOOST_DI_EXPAND)(__VA_ARGS__)
// clang-format on
#if defined(__MSVC__)
#define __BOOST_DI_MAKE_INJECTOR(...) __VA_ARGS__
#else
namespace detail {
static auto make_injector = [](auto injector) {
  using injector_t = decltype(injector);
  struct i : injector_t {
    explicit i(injector_t&& other) : injector_t(static_cast<injector_t&&>(other)) {}
  };
  return i{static_cast<injector_t&&>(injector)};
};
}
#define __BOOST_DI_MAKE_INJECTOR(...) detail::make_injector(__VA_ARGS__)
#endif
template <class TConfig = BOOST_DI_CFG, class... TDeps,
          __BOOST_DI_REQUIRES_MSG(concepts::boundable<aux::type_list<TDeps...>>) = 0,
          __BOOST_DI_REQUIRES_MSG(concepts::configurable<TConfig>) = 0>
inline auto make_injector(TDeps... args) noexcept {
  return __BOOST_DI_MAKE_INJECTOR(
      core::injector<TConfig, decltype(((TConfig*)0)->policies((concepts::injector<TConfig>*)0)), TDeps...>{
          core::init{}, static_cast<TDeps&&>(args)...});
}
namespace policies {
namespace detail {
struct type_op {};
template <class T, class = int>
struct apply_impl {
  template <class>
  struct apply : T {};
};
template <template <class...> class T, class... Ts>
struct apply_impl<T<Ts...>, __BOOST_DI_REQUIRES(!aux::is_base_of<type_op, T<Ts...>>::value)> {
  template <class TOp, class>
  struct apply_placeholder_impl {
    using type = TOp;
  };
  template <class TOp>
  struct apply_placeholder_impl<_, TOp> {
    using type = TOp;
  };
  template <template <class...> class TExpr, class TOp, class... TArgs>
  struct apply_placeholder {
    using type = TExpr<typename apply_placeholder_impl<TArgs, TOp>::type...>;
  };
  template <class TArg>
  struct apply : apply_placeholder<T, typename TArg::type, Ts...>::type {};
};
template <class T>
struct apply_impl<T, __BOOST_DI_REQUIRES(aux::is_base_of<type_op, T>::value)> {
  template <class TArg>
  struct apply : T::template apply<TArg>::type {};
};
template <class T>
struct not_ : detail::type_op {
  template <class TArg>
  struct apply : aux::integral_constant<bool, !detail::apply_impl<T>::template apply<TArg>::value> {};
};
template <class... Ts>
struct and_ : detail::type_op {
  template <class TArg>
  struct apply : aux::is_same<aux::bool_list<detail::apply_impl<Ts>::template apply<TArg>::value...>,
                              aux::bool_list<aux::always<Ts>::value...>> {};
};
template <class... Ts>
struct or_ : detail::type_op {
  template <class TArg>
  struct apply
      : aux::integral_constant<bool, !aux::is_same<aux::bool_list<detail::apply_impl<Ts>::template apply<TArg>::value...>,
                                                   aux::bool_list<aux::never<Ts>::value...>>::value> {};
};
}
template <class T>
struct type {
  template <class TPolicy>
  struct not_allowed_by {
    operator aux::false_type() const {
      using constraint_not_satisfied = not_allowed_by;
      return constraint_not_satisfied{}.error();
    }
    // clang-format off
    static inline aux::false_type
 error(_ = "type disabled by constructible policy, added by BOOST_DI_CFG or make_injector<CONFIG>!");
    // clang-format on
  };
};
template <class T>
struct is_bound : detail::type_op {
  struct not_resolved {};
  template <class TArg>
  struct apply
      : aux::integral_constant<
            bool,
            !aux::is_same<typename TArg::template resolve<aux::conditional_t<aux::is_same<T, _>::value, typename TArg::type, T>,
                                                          typename TArg::name, not_resolved>,
                          not_resolved>::value> {};
};
template <class T>
struct is_injected : detail::type_op {
  template <class TArg, class U = aux::decay_t<aux::conditional_t<aux::is_same<T, _>::value, typename TArg::type, T>>>
  struct apply : aux::conditional_t<aux::is_class<U>::value, typename type_traits::is_injectable<U>::type, aux::true_type> {};
};
namespace operators {
template <class X, class Y>
inline auto operator||(const X&, const Y&) {
  return detail::or_<X, Y>{};
}
template <class X, class Y>
inline auto operator&&(const X&, const Y&) {
  return detail::and_<X, Y>{};
}
template <class T>
inline auto operator!(const T&) {
  return detail::not_<T>{};
}
}
template <class T>
struct constructible_impl {
  template <class TArg, __BOOST_DI_REQUIRES(TArg::is_root::value || T::template apply<TArg>::value) = 0>
  aux::true_type operator()(const TArg&) const {
    return {};
  }
  template <class TArg, __BOOST_DI_REQUIRES(!TArg::is_root::value && !T::template apply<TArg>::value) = 0>
  aux::false_type operator()(const TArg&) const {
    return typename type<typename TArg::type>::template not_allowed_by<T>{};
  }
};
template <class T = aux::never<_>, __BOOST_DI_REQUIRES(aux::is_base_of<detail::type_op, T>::value) = 0>
inline auto constructible(const T& = {}) {
  return constructible_impl<T>{};
}
template <class T = aux::never<_>, __BOOST_DI_REQUIRES(!aux::is_base_of<detail::type_op, T>::value) = 0>
inline auto constructible(const T& = {}) {
  return constructible_impl<detail::or_<T>>{};
}
}
namespace providers {
class heap {
 public:
  template <class TInitialization, class T, class... TArgs>
  struct is_creatable {
    static constexpr auto value = concepts::creatable<TInitialization, T, TArgs...>::value;
  };
  template <class T, class TMemory, class... TArgs>
  auto get(const type_traits::direct&, const TMemory&, TArgs&&... args) const {
    return new T(static_cast<TArgs&&>(args)...);
  }
  template <class T, class TMemory, class... TArgs>
  auto get(const type_traits::uniform&, const TMemory&, TArgs&&... args) const {
    return new T{static_cast<TArgs&&>(args)...};
  }
};
}
#define __BOOST_DI_IF(cond, t, f) __BOOST_DI_IF_I(cond, t, f)
#define __BOOST_DI_REPEAT(i, m, ...) __BOOST_DI_REPEAT_N(i, m, __VA_ARGS__)
#define __BOOST_DI_CAT(a, ...) __BOOST_DI_PRIMITIVE_CAT(a, __VA_ARGS__)
#define __BOOST_DI_EMPTY()
#define __BOOST_DI_COMMA() ,
#define __BOOST_DI_EAT(...)
#define __BOOST_DI_EXPAND(...) __VA_ARGS__
#define __BOOST_DI_SIZE(...) __BOOST_DI_CAT(__BOOST_DI_VARIADIC_SIZE_I(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, ), )
#define __BOOST_DI_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__
#define __BOOST_DI_ELEM(n, ...) __BOOST_DI_ELEM_I(n, __VA_ARGS__)
#define __BOOST_DI_IS_EMPTY(...)                              \
  __BOOST_DI_DETAIL_IS_EMPTY_IIF(__BOOST_DI_IBP(__VA_ARGS__)) \
  (__BOOST_DI_DETAIL_IS_EMPTY_GEN_ZERO, __BOOST_DI_DETAIL_IS_EMPTY_PROCESS)(__VA_ARGS__)
#define __BOOST_DI_DETAIL_IS_EMPTY_PRIMITIVE_CAT(a, b) a##b
#define __BOOST_DI_DETAIL_IS_EMPTY_IIF(bit) __BOOST_DI_DETAIL_IS_EMPTY_PRIMITIVE_CAT(__BOOST_DI_DETAIL_IS_EMPTY_IIF_, bit)
#define __BOOST_DI_DETAIL_IS_EMPTY_NON_FUNCTION_C(...) ()
#define __BOOST_DI_DETAIL_IS_EMPTY_GEN_ZERO(...) 0
#define __BOOST_DI_VARIADIC_SIZE_I(e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, size, ...) size
#define __BOOST_DI_IF_I(cond, t, f) __BOOST_DI_IIF(cond, t, f)
#define __BOOST_DI_IIF_0(t, f) f
#define __BOOST_DI_IIF_1(t, f) t
#define __BOOST_DI_IIF_2(t, f) t
#define __BOOST_DI_IIF_3(t, f) t
#define __BOOST_DI_IIF_4(t, f) t
#define __BOOST_DI_IIF_5(t, f) t
#define __BOOST_DI_IIF_6(t, f) t
#define __BOOST_DI_IIF_7(t, f) t
#define __BOOST_DI_IIF_8(t, f) t
#define __BOOST_DI_IIF_9(t, f) t
#define __BOOST_DI_ELEM_I(n, ...) __BOOST_DI_CAT(__BOOST_DI_CAT(__BOOST_DI_ELEM, n)(__VA_ARGS__, ), )
#define __BOOST_DI_ELEM0(p1, ...) p1
#define __BOOST_DI_ELEM1(p1, p2, ...) p2
#define __BOOST_DI_ELEM2(p1, p2, p3, ...) p3
#define __BOOST_DI_ELEM3(p1, p2, p3, p4, ...) p4
#define __BOOST_DI_ELEM4(p1, p2, p3, p4, p5, ...) p5
#define __BOOST_DI_ELEM5(p1, p2, p3, p4, p5, p6, ...) p6
#define __BOOST_DI_ELEM6(p1, p2, p3, p4, p5, p6, p7, ...) p7
#define __BOOST_DI_ELEM7(p1, p2, p3, p4, p5, p6, p7, p8, ...) p8
#define __BOOST_DI_ELEM8(p1, p2, p3, p4, p5, p6, p7, p8, p9, ...) p9
#define __BOOST_DI_ELEM9(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, ...) p10
#define __BOOST_DI_REPEAT_N(i, m, ...) __BOOST_DI_REPEAT_##i(m, __VA_ARGS__)
#define __BOOST_DI_REPEAT_1(m, ...) m(0, __VA_ARGS__)
#define __BOOST_DI_REPEAT_2(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__)
#define __BOOST_DI_REPEAT_3(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__)
#define __BOOST_DI_REPEAT_4(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__)
#define __BOOST_DI_REPEAT_5(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__)
#define __BOOST_DI_REPEAT_6(m, ...) \
  m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__)
#define __BOOST_DI_REPEAT_7(m, ...) \
  m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__) m(6, __VA_ARGS__)
#define __BOOST_DI_REPEAT_8(m, ...)                                                                           \
  m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__) \
      m(6, __VA_ARGS__) m(7, __VA_ARGS__)
#define __BOOST_DI_REPEAT_9(m, ...)                                                                           \
  m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__) \
      m(6, __VA_ARGS__) m(7, __VA_ARGS__) m(8, __VA_ARGS__)
#define __BOOST_DI_REPEAT_10(m, ...)                                                                          \
  m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__) \
      m(6, __VA_ARGS__) m(7, __VA_ARGS__) m(8, __VA_ARGS__) m(9, __VA_ARGS__)
#if defined(__MSVC__)
#define __BOOST_DI_VD_IBP_CAT(a, b) __BOOST_DI_VD_IBP_CAT_I(a, b)
#define __BOOST_DI_VD_IBP_CAT_I(a, b) __BOOST_DI_VD_IBP_CAT_II(a##b)
#define __BOOST_DI_VD_IBP_CAT_II(res) res
#define __BOOST_DI_IBP_SPLIT(i, ...) \
  __BOOST_DI_VD_IBP_CAT(__BOOST_DI_IBP_PRIMITIVE_CAT(__BOOST_DI_IBP_SPLIT_, i)(__VA_ARGS__), __BOOST_DI_EMPTY())
#define __BOOST_DI_IBP_IS_VARIADIC_C(...) 1 1
#define __BOOST_DI_IBP_SPLIT_0(a, ...) a
#define __BOOST_DI_IBP_SPLIT_1(a, ...) __VA_ARGS__
#define __BOOST_DI_IBP_CAT(a, ...) __BOOST_DI_IBP_PRIMITIVE_CAT(a, __VA_ARGS__)
#define __BOOST_DI_IBP_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__
#define __BOOST_DI_IBP_IS_VARIADIC_R_1 1,
#define __BOOST_DI_IBP_IS_VARIADIC_R___BOOST_DI_IBP_IS_VARIADIC_C 0,
#define __BOOST_DI_IBP(...) \
  __BOOST_DI_IBP_SPLIT(0, __BOOST_DI_IBP_CAT(__BOOST_DI_IBP_IS_VARIADIC_R_, __BOOST_DI_IBP_IS_VARIADIC_C __VA_ARGS__))
#define __BOOST_DI_IIF(bit, t, f) __BOOST_DI_IIF_OO((bit, t, f))
#define __BOOST_DI_IIF_OO(par) __BOOST_DI_IIF_I##par
#define __BOOST_DI_IIF_I(bit, t, f) __BOOST_DI_IIF_##bit(t, f)
#define __BOOST_DI_DETAIL_IS_EMPTY_IIF_0(t, b) b
#define __BOOST_DI_DETAIL_IS_EMPTY_IIF_1(t, b) t
#define __BOOST_DI_DETAIL_IS_EMPTY_PROCESS(...) __BOOST_DI_IBP(__BOOST_DI_DETAIL_IS_EMPTY_NON_FUNCTION_C __VA_ARGS__())
#else
#define __BOOST_DI_IBP_SPLIT(i, ...) __BOOST_DI_PRIMITIVE_CAT(__BOOST_DI_IBP_SPLIT_, i)(__VA_ARGS__)
#define __BOOST_DI_IBP_SPLIT_0(a, ...) a
#define __BOOST_DI_IBP_SPLIT_1(a, ...) __VA_ARGS__
#define __BOOST_DI_IBP_IS_VARIADIC_C(...) 1
#define __BOOST_DI_IBP_IS_VARIADIC_R_1 1,
#define __BOOST_DI_IBP_IS_VARIADIC_R___BOOST_DI_IBP_IS_VARIADIC_C 0,
#define __BOOST_DI_IBP(...) \
  __BOOST_DI_IBP_SPLIT(0, __BOOST_DI_CAT(__BOOST_DI_IBP_IS_VARIADIC_R_, __BOOST_DI_IBP_IS_VARIADIC_C __VA_ARGS__))
#define __BOOST_DI_IIF(bit, t, f) __BOOST_DI_IIF_I(bit, t, f)
#define __BOOST_DI_IIF_I(bit, t, f) __BOOST_DI_IIF_II(__BOOST_DI_IIF_##bit(t, f))
#define __BOOST_DI_IIF_II(id) id
#define __BOOST_DI_DETAIL_IS_EMPTY_IIF_0(t, ...) __VA_ARGS__
#define __BOOST_DI_DETAIL_IS_EMPTY_IIF_1(t, ...) t
#define __BOOST_DI_DETAIL_IS_EMPTY_PROCESS(...) __BOOST_DI_IBP(__BOOST_DI_DETAIL_IS_EMPTY_NON_FUNCTION_C __VA_ARGS__())
#endif
template <class, class>
struct named;
namespace detail {
struct named_impl {
  template <class T>
  T operator=(const T&) const;
};
static constexpr __BOOST_DI_UNUSED named_impl named{};
template <class T, class TName>
struct combine_impl {
  using type = ::boost::di::v1_0_1::named<TName, T>;
};
template <class T>
struct combine_impl<T, aux::none_type> {
  using type = T;
};
template <class, class>
struct combine;
template <class... T1, class... T2>
struct combine<aux::type_list<T1...>, aux::type_list<T2...>> {
  using type = aux::type_list<typename combine_impl<T1, T2>::type...>;
};
template <class T1, class T2>
using combine_t = typename combine<T1, T2>::type;
}
template <class... Ts>
using inject = aux::type_list<Ts...>;
#define __BOOST_DI_HAS_NAME(i, ...) __BOOST_DI_IF(__BOOST_DI_IBP(__BOOST_DI_ELEM(i, __VA_ARGS__, )), 1, )
#define __BOOST_DI_HAS_NAMES(...) \
  __BOOST_DI_IF(__BOOST_DI_IS_EMPTY(__BOOST_DI_REPEAT(__BOOST_DI_SIZE(__VA_ARGS__), __BOOST_DI_HAS_NAME, __VA_ARGS__)), 0, 1)
#define __BOOST_DI_GEN_CTOR_IMPL(p, i) \
  __BOOST_DI_IF(i, __BOOST_DI_COMMA, __BOOST_DI_EAT)() __BOOST_DI_IF(__BOOST_DI_IBP(p), __BOOST_DI_EAT p, p)
#define __BOOST_DI_GEN_CTOR(i, ...) __BOOST_DI_GEN_CTOR_IMPL(__BOOST_DI_ELEM(i, __VA_ARGS__, ), i)
#define __BOOST_DI_GEN_ARG_NAME(p) __BOOST_DI_GEN_ARG_NAME_IMPL p )
#define __BOOST_DI_GEN_NONE_TYPE(p) ::boost::di::v1_0_1::aux::none_type
#define __BOOST_DI_GEN_ARG_NAME_IMPL(p) decltype(::boost::di::v1_0_1::detail::p) __BOOST_DI_EAT(
#define __BOOST_DI_GEN_NAME_IMPL(p, i)               \
  __BOOST_DI_IF(i, __BOOST_DI_COMMA, __BOOST_DI_EAT) \
  () __BOOST_DI_IF(__BOOST_DI_IBP(p), __BOOST_DI_GEN_ARG_NAME, __BOOST_DI_GEN_NONE_TYPE)(p)
#define __BOOST_DI_GEN_NAME(i, ...) __BOOST_DI_GEN_NAME_IMPL(__BOOST_DI_ELEM(i, __VA_ARGS__, ), i)
#define __BOOST_DI_INJECT_TRAITS_IMPL_0(...)                                                           \
  static void ctor(__BOOST_DI_REPEAT(__BOOST_DI_SIZE(__VA_ARGS__), __BOOST_DI_GEN_CTOR, __VA_ARGS__)); \
  using type __BOOST_DI_UNUSED = ::boost::di::v1_0_1::aux::function_traits_t<decltype(ctor)>;
#define __BOOST_DI_INJECT_TRAITS_IMPL_1(...)                                                              \
  static void ctor(__BOOST_DI_REPEAT(__BOOST_DI_SIZE(__VA_ARGS__), __BOOST_DI_GEN_CTOR, __VA_ARGS__));    \
  static void name(__BOOST_DI_REPEAT(__BOOST_DI_SIZE(__VA_ARGS__), __BOOST_DI_GEN_NAME, __VA_ARGS__));    \
  using type __BOOST_DI_UNUSED =                                                                          \
      ::boost::di::v1_0_1::detail::combine_t<::boost::di::v1_0_1::aux::function_traits_t<decltype(ctor)>, \
                                             ::boost::di::v1_0_1::aux::function_traits_t<decltype(name)>>;
#define __BOOST_DI_INJECT_TRAITS_EMPTY_IMPL(...) \
  using boost_di_inject__ __BOOST_DI_UNUSED = ::boost::di::v1_0_1::aux::type_list<>
#define __BOOST_DI_INJECT_TRAITS_IMPL(...)                                                                             \
  struct boost_di_inject__ {                                                                                           \
    __BOOST_DI_CAT(__BOOST_DI_INJECT_TRAITS_IMPL_, __BOOST_DI_HAS_NAMES(__VA_ARGS__))                                  \
    (__VA_ARGS__) static_assert(__BOOST_DI_SIZE(__VA_ARGS__) <= BOOST_DI_CFG_CTOR_LIMIT_SIZE,                          \
                                "Number of constructor arguments is out of range - see BOOST_DI_CFG_CTOR_LIMIT_SIZE"); \
  }
#define BOOST_DI_INJECT_TRAITS(...)                                                                                   \
  __BOOST_DI_IF(__BOOST_DI_IS_EMPTY(__VA_ARGS__), __BOOST_DI_INJECT_TRAITS_EMPTY_IMPL, __BOOST_DI_INJECT_TRAITS_IMPL) \
  (__VA_ARGS__)
#define BOOST_DI_INJECT(T, ...)        \
  BOOST_DI_INJECT_TRAITS(__VA_ARGS__); \
  T(__BOOST_DI_REPEAT(__BOOST_DI_SIZE(__VA_ARGS__), __BOOST_DI_GEN_CTOR, __VA_ARGS__))
BOOST_DI_NAMESPACE_END
#endif
#if defined(__CLANG__)
#pragma clang diagnostic pop
#elif defined(__GCC__)
#pragma GCC diagnostic pop
#endif
