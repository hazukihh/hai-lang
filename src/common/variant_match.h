#ifndef VARIANT_MATCH_H_
#define VARIANT_MATCH_H_

#include <variant>
#include <type_traits>

template<class... Ts>
struct Overloaded : public Ts... {
    using Ts::operator()...;
};

template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;


/**
 * @note must handle all possible case, or compile error
 * @code
 *  MATCH(variant,
 *      [](const T1& t1) {...},
 *      [](const T2& t2) {...},
 *      ...
 *      [](const auto& others) {...}
 *  );
 * @endcode
 */

#define MATCH(variant, ...) std::visit(Overloaded{__VA_ARGS__}, variant)


// #define MATCH_WITH_DEFAULT(variant, default_lambda, ...) \
//     std::visit(Overloaded{__VA_ARGS__, default_lambda}, variant)


#endif // VARIANT_MATCH_H_
