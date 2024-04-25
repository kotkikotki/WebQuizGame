#ifndef UTILITY_HPP
#define UTILITY_HPP

#include<type_traits>

template<bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type; //where

template<typename Base, typename Derived>
inline constexpr bool is_base_of_v = std::is_base_of<Base, Derived>::value;


#endif // UTILITY_HPP
