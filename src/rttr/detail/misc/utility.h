/************************************************************************************
*                                                                                   *
*   Copyright (c) 2014, 2015 Axel Menzel <info@axelmenzel.de>                       *
*                                                                                   *
*   This file is part of RTTR (Run Time Type Reflection)                            *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining           *
*   a copy of this software and associated documentation files (the "Software"),    *
*   to deal in the Software without restriction, including without limitation       *
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,        *
*   and/or sell copies of the Software, and to permit persons to whom the           *
*   Software is furnished to do so, subject to the following conditions:            *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef RTTR_UTILITY_H_
#define RTTR_UTILITY_H_

#include "rttr/detail/base/core_prerequisites.h"

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace rttr
{
namespace detail
{

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// This will add the c++14 integer sequence to c++11

    template <class T, T... I>
    struct integer_sequence
    {
        template <T N> using append = integer_sequence<T, I..., N>;
        static std::size_t size() { return sizeof...(I); }
        using next = append<sizeof...(I)>;
        using type = T;
    };

    template <class T, T Index, std::size_t N>
    struct sequence_generator
    {
        using type = typename sequence_generator<T, Index - 1, N - 1>::type::next;
    };

    template <class T, T Index>
    struct sequence_generator<T, Index, 0ul> { using type = integer_sequence<T>; };

    template <std::size_t... I>
    using index_sequence = integer_sequence<std::size_t, I...>;

#if RTTR_COMPILER == RTTR_COMPILER_MSVC
#   if RTTR_COMP_VER <= 1800
    // workaround for a compiler bug of nested aliases (#1085630)
    template <class T, T N>
    struct make_integer_sequence_impl
    {
        typedef typename sequence_generator<T, N, N>::type type;
    };

    template <class T, T N>
    struct make_index_sequence_impl
    {
        typedef typename make_integer_sequence_impl<T, N>::type type;
    };

    template <class T, T N>
    using make_integer_sequence = typename make_integer_sequence_impl<T, N>::type;

    template <std::size_t N>
    using make_index_sequence = typename make_integer_sequence_impl<std::size_t, N>::type;
#   else
#       error "Check new MSVC Compiler!"
#   endif
#else
    template <class T, T N>
    using make_integer_sequence = typename sequence_generator<T, N, N>::type;

    template <std::size_t N>
    using make_index_sequence = make_integer_sequence<std::size_t, N>;
#endif

    template<class... T>
    using index_sequence_for = make_index_sequence<sizeof...(T)>;


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct remove_first_index_impl
{
    using type = index_sequence<>;
};

template<std::size_t First, std::size_t... I>
struct remove_first_index_impl<detail::index_sequence<First, I...>>
{
    using type = detail::index_sequence<I...>;
};

template<typename T>
using remove_first_index = typename remove_first_index_impl<T>::type;


/////////////////////////////////////////////////////////////////////////////////////////

template<typename, typename>
struct concat_index_sequence { };

template<std::size_t... Ts, std::size_t... Us>
struct concat_index_sequence<index_sequence<Ts...>, index_sequence<Us...>>
{
    using type = index_sequence<Ts..., Us...>;
};

template <class T>
struct remove_last_index_impl;

template <size_t Last>
struct remove_last_index_impl<index_sequence<Last>>
{
    using type = index_sequence<>;
};

template<std::size_t First, std::size_t... I>
struct remove_last_index_impl<index_sequence<First, I...>>
{
    using type = typename concat_index_sequence<
                index_sequence<First>,
                typename remove_last_index_impl<index_sequence<I...>>::type
                >::type;
};

template<typename T>
using remove_last_index = typename remove_last_index_impl<T>::type;



/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// This will add the c++14 integer sequence to c++11


static RTTR_FORCE_INLINE bool check_all_true() { return true; }

template<typename... BoolArgs>
static RTTR_INLINE bool check_all_true(bool arg1, BoolArgs... args) { return arg1 & check_all_true(args...); }

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// copy the content of any arbitrary array, use it like: 
// copy_array(in, out);
// works with every dimension

template<typename ElementType>
struct copy_array_helper_impl
{
    void operator()(const ElementType &in, ElementType &out)
    {
        out = in;
    }
};

template<typename ElementType, std::size_t Count>
struct copy_array_helper_impl<ElementType[Count]> {
    void operator()(const ElementType (&in)[Count], ElementType (&out)[Count])
    {
        for(std::size_t i = 0; i < Count; ++i)
            copy_array_helper_impl<ElementType>()(in[i], out[i]);
    }
};

template<typename ElementType, std::size_t Count>
auto copy_array(const ElementType (&in)[Count], ElementType (&out)[Count])
    -> ElementType (&)[Count]
{
    copy_array_helper_impl<ElementType[Count]>()(in, out);
    return out;
}


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// make_unqiue implementation for C++11

template<class T> struct _Unique_if 
{
    typedef std::unique_ptr<T> _Single_object;
};

template<class T> struct _Unique_if<T[]> 
{
    typedef std::unique_ptr<T[]> _Unknown_bound;
};

template<class T, size_t N> struct _Unique_if<T[N]> 
{
    typedef void _Known_bound;
};

template<class T, class... Args>
typename _Unique_if<T>::_Single_object
make_unique(Args&&... args) 
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class T>
typename _Unique_if<T>::_Unknown_bound
make_unique(size_t n) 
{
    typedef typename std::remove_extent<T>::type U;
    return std::unique_ptr<T>(new U[n]());
}

template<class T, class... Args>
typename _Unique_if<T>::_Known_bound
make_unique(Args&&...) = delete;
        
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template< typename T >
RTTR_INLINE const T& as_const(T& obj) { return const_cast<T&>(obj); }

template< typename T >
RTTR_INLINE const T& as_const(const T& obj) { return obj; }

template<typename T>
RTTR_INLINE const T as_const(T&& obj) 
{
    static_assert(!std::is_const<T>::value, "The given obj is already const, moving a const RValue will result in a copy!");
    return std::forward<T>(obj); 
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
RTTR_FORCE_INLINE typename std::enable_if<std::is_pointer<T>::value, void*>::type as_void_ptr(const T& obj) 
{
    return const_cast<void*>(reinterpret_cast<const void*>(obj));
}


template<typename T>
RTTR_FORCE_INLINE typename std::enable_if<!std::is_pointer<T>::value, void*>::type as_void_ptr(const T& obj) 
{
    return const_cast<void*>(reinterpret_cast<const void*>(&obj));
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/*!
 * Helper class to iterate in a ranged-based for loops backwards through a container.
 * use it like following: 
 * \code{.cpp}
 *   for(const auto& value: reverse(my_vector))
 *      std::cout << value << std::endl;
 * \endcode                
 */
template<class T>
class reverse_wrapper
{
    public:
        reverse_wrapper(T& container) : m_container(container) { }
        decltype(std::declval<T>().rbegin()) begin() { return m_container.rbegin(); }
        decltype(std::declval<T>().rend()) end() { return m_container.rend(); }

        decltype(std::declval<T>().crbegin()) begin() const { return m_container.crbegin(); }
        decltype(std::declval<T>().crend()) end() const { return m_container.crend(); }

    private:
        T& m_container;
};

template<class T>
class reverse_move_wrapper
{
    public:
        reverse_move_wrapper(T&& container) : m_container(std::move(container)) { }
        decltype(std::declval<T>().rbegin()) begin() { return m_container.rbegin(); }
        decltype(std::declval<T>().rend()) end() { return m_container.rend(); }
        decltype(std::declval<T>().crbegin()) begin() const { return m_container.crbegin(); }
        decltype(std::declval<T>().crend()) end() const { return m_container.crend(); }

    private:
        T m_container;
};


template<class T>
reverse_move_wrapper<T> reverse(T&& container) 
{
    return reverse_move_wrapper<T>(std::forward<T>(container));
}

template<class T>
const reverse_move_wrapper<const T> reverse(const T&& container) 
{
    return reverse_move_wrapper<const T>(std::forward<const T>(container));
}

template<class T>
reverse_wrapper<T> reverse(T& container) 
{
     return reverse_wrapper<T>(container);
}

template<class T>
const reverse_wrapper<const T> reverse(const T& container) 
{
     return reverse_wrapper<const T>(container);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
        
template<typename T>
using return_type_normal = typename std::add_pointer<typename remove_pointers<T>::type>::type;

template<typename T>
using raw_addressof_return_type = typename std::conditional<is_function_ptr<typename remove_pointers_except_one<T>::type>::value,
                                                            typename std::add_pointer<typename remove_pointers_except_one<T>::type>::type,
                                                            return_type_normal<T> >;
template<typename T>
using raw_addressof_return_type_t = typename raw_addressof_return_type<T>::type;

template<typename T, typename Enable = void>
struct raw_addressof_impl
{
    static RTTR_INLINE raw_addressof_return_type_t<T> get(T& data)
    {
        return std::addressof(data);
    }
};

template<typename T>
using is_raw_void_pointer = std::is_same<void*, typename std::add_pointer<typename raw_type<T>::type>::type>;

template<typename T>
using is_void_pointer = std::integral_constant<bool, std::is_pointer<T>::value && is_raw_void_pointer<T>::value && pointer_count<T>::value == 1>;

template<typename T>
struct raw_addressof_impl<T, typename std::enable_if<(std::is_pointer<T>::value && pointer_count<T>::value >= 1 && !is_void_pointer<T>::value) ||
                                                      (pointer_count<T>::value == 1 && std::is_member_pointer<typename remove_pointer<T>::type>::value)
                                                     >::type >
{
    static RTTR_INLINE raw_addressof_return_type_t<T> get(T& data)
    {
        return raw_addressof_impl<typename remove_pointer<T>::type>::get(*data);
    }
};

template<typename T>
struct raw_addressof_impl<T, typename std::enable_if<is_void_pointer<T>::value>::type >
{
    static RTTR_INLINE raw_addressof_return_type_t<T> get(T& data)
    {
        return data; // void pointer cannot be dereferenced to type "void"
    }
};

/*!
 * \brief This function will return from its raw type \p T
 *        its address as pointer.
 *
 * \see detail::raw_type
 *
 * \return The address of the raw type from the given object \p data as pointer.
 */
template<typename T>
static RTTR_INLINE raw_addressof_return_type_t<T> raw_addressof(T& data)
{
    return raw_addressof_impl<T>::get(data);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
 

} // end namespace detail
} // end namespace rttr

#endif //RTTR_UTILITY_H_