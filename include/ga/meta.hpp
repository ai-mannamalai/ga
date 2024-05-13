// Copyright (c) 2017-2018 Filipe Verri <filipeverri@gmail.com>

#ifndef GA_META_HPP
#define GA_META_HPP

#include <ga/type.hpp>

#include <array>
#include <type_traits>
#include <vector>

namespace ga
{
namespace meta
{

template <typename...> using void_t = void;

template <typename...> struct conjunction : std::true_type
{
};

template <typename B> struct conjunction<B> : B
{
};

template <typename B, typename... Bs>
struct conjunction<B, Bs...>
  : std::conditional<bool(B::value), conjunction<Bs...>, B>::type
{
};

template <typename...> struct disjunction : std::false_type
{
};

template <class B> struct disjunction<B> : B
{
};

template <class B, class... Bs>
struct disjunction<B, Bs...>
  : std::conditional<bool(B::value), B, disjunction<Bs...>>::type
{
};

template <bool B> using bool_constant = std::integral_constant<bool, B>;

template <typename B> struct negation : bool_constant<!B::value>
{
};

template <typename T> struct always_false : std::false_type
{
};

// Concepts emulation

template <typename T, template <typename> class Expression, typename = void_t<>>
struct compiles : std::false_type
{
};

template <typename T, template <typename> class Expression>
struct compiles<T, Expression, void_t<Expression<T>>> : std::true_type
{
};

template <typename... Checks>
using requirescheck = typename std::enable_if<conjunction<Checks...>::value>::type;

template <typename... Checks>
using fallbackheck = typename std::enable_if<conjunction<negation<Checks>...>::value>::type;

// based on http://stackoverflow.com/questions/13830158/check-if-a-variable-is-iterable
namespace detail
{
// To allow ADL with custom begin/end
using std::begin;
using std::end;

template <typename T> using begin_result = decltype(begin(std::declval<T&>()));
template <typename T> using end_result = decltype(end(std::declval<T&>()));

template <typename T> using has_begin = meta::compiles<T, begin_result>;
template <typename T> using has_end = meta::compiles<T, end_result>;

template <typename T> using inc_result = decltype(++std::declval<T&>());
template <typename T> using deref_result = decltype(*std::declval<T&>());

template <typename T> using has_inc = meta::compiles<T, inc_result>;
template <typename T> using has_deref = meta::compiles<T, deref_result>;
}

template <typename T, typename = void> struct Iterable : std::false_type
{
};

template <typename T>
struct Iterable<
  T, requirescheck<conjunction<
       detail::has_begin<T>, detail::has_end<T>, detail::has_inc<detail::begin_result<T>>,
       detail::has_inc<detail::end_result<T>>, detail::has_deref<detail::begin_result<T>>,
       detail::has_deref<detail::end_result<T>>,
       std::is_same<detail::deref_result<detail::begin_result<T>>,
                    detail::deref_result<detail::end_result<T>>>>>> : std::true_type
{
};

template <typename T>
using comparison_result = decltype(std::declval<const T&>() < std::declval<const T&>());

template <typename T> using has_comparison = meta::compiles<T, comparison_result>;

template <typename T>
using mutate_result =
  decltype(std::declval<T&>().mutate(std::declval<typename T::individual_type&>(),
                                     std::declval<typename T::generator_type&>()));

template <typename T> using has_mutate = meta::compiles<T, mutate_result>;

template <typename T>
using recombine_result = decltype(
  std::declval<T&>().recombine(std::declval<const typename T::individual_type&>(),
                               std::declval<const typename T::individual_type&>(),
                               std::declval<typename T::generator_type&>()));

template <typename T> using has_recombine = meta::compiles<T, recombine_result>;

template <typename T>
using evaluate_result =
  decltype(std::declval<T&>().evaluate(std::declval<const typename T::individual_type&>(),
                                       std::declval<typename T::generator_type&>()));

template <typename T> using has_evaluate = meta::compiles<T, evaluate_result>;

template <typename T>
using multi_evaluate_result = decltype(std::declval<T&>().evaluate(
  std::declval<const std::vector<typename T::individual_type>&>(),
  std::declval<std::vector<
    ::ga::solution<typename T::individual_type, typename T::fitness_type>>&>(),
  std::declval<std::size_t>(),
  std::declval<std::back_insert_iterator<std::vector<typename T::fitness_type>>>(),
  std::declval<typename T::generator_type&>()));

template <typename T> using has_multi_evaluate = meta::compiles<T, multi_evaluate_result>;

template <typename T, typename = void> struct SingleEvaluation : std::false_type
{
};

template <typename T>
struct SingleEvaluation<
  T, requirescheck<conjunction<has_evaluate<T>,
                          std::is_same<typename T::fitness_type, evaluate_result<T>>>>>
  : std::true_type
{
};

template <typename T, typename = void> struct MultiEvaluation : std::false_type
{
};

template <typename T>
struct MultiEvaluation<
  T, requirescheck<
       conjunction<has_multi_evaluate<T>, std::is_same<void, multi_evaluate_result<T>>>>>
  : std::true_type
{
};

template <typename T, typename = void> struct Problem : std::false_type
{
};

template <typename T>
struct Problem<
  T, requirescheck<conjunction<has_mutate<T>, std::is_same<mutate_result<T>, void>,
                          has_recombine<T>, Iterable<recombine_result<T>>,
                          std::is_same<typename T::individual_type,
                                       typename recombine_result<T>::value_type>,
                          has_comparison<typename T::fitness_type>,
                          disjunction<SingleEvaluation<T>, MultiEvaluation<T>>>>>
  : std::true_type
{
};

} // namespace meta
} // namespace ga

#endif // GA_META_HPP
