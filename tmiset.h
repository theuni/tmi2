// Copyright (c) 2026 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMISET_H_
#define TMISET_H_

#include <wavl_tree.h>
#include <tmi_index.h>
#include <tmi_nodehandle.h>

#include <cstdint>

namespace tmi
{

template <class Key, bool Unique, class Compare = std::less<Key>, class Allocator = std::allocator<Key>>
class set_base;

template <class Key, class Compare = std::less<Key>, class Allocator = std::allocator<Key>>
using set = set_base<Key, true, Compare, Allocator>;

template <class Key, class Compare = std::less<Key>, class Allocator = std::allocator<Key>>
using multiset = set_base<Key, false, Compare, Allocator>;

namespace detail
{
template <typename Key>
class set_data;

template <class Comparator, class = void>
inline const bool is_transparent_v = false;

template <class Comparator>
inline const bool is_transparent_v<Comparator, std::void_t<typename Comparator::is_transparent> > = true;

} // namespace detail

template <class Key, bool Unique, class Compare, class Allocator>
class set_base : private wavl_tree<detail::set_data<Key>, identity<Key>, Compare, Unique>
{
    using tree_type = wavl_tree<detail::set_data<Key>, identity<Key>, Compare, Unique>;
    using data_type = detail::set_data<Key>;
    using node_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<data_type>;

    template <class, bool, class, class>
    friend class set_base;

public:
    using key_type = tree_type::key_type;
    using value_type = tree_type::value_type;
    using key_compare = tree_type::key_compare_type;
    using value_compare = key_compare;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = tree_type::size_type;
    using difference_type = tree_type::difference_type;
    using pointer = std::allocator_traits<allocator_type>::pointer;
    using const_pointer = std::allocator_traits<allocator_type>::const_pointer;

    using iterator = tree_type::iterator;
    using const_iterator = tree_type::const_iterator;
    using reverse_iterator = tree_type::reverse_iterator;
    using const_reverse_iterator = tree_type::const_reverse_iterator;
    using node_type = tmi::detail::node_handle<allocator_type, data_type>;
    using insert_return_type = std::conditional_t<Unique, tmi::detail::insert_return_type<iterator, node_type>, iterator>;
    using insert_result_type = std::conditional_t<Unique, std::pair<iterator, bool>, iterator>;

    set_base()
        noexcept(
            std::is_nothrow_default_constructible<allocator_type>::value &&
            std::is_nothrow_default_constructible<key_compare>::value &&
            std::is_nothrow_copy_constructible<key_compare>::value)
    {}

    explicit set_base(const value_compare& comp) : tree_type{identity<Key>{}, comp}
    {}

    set_base(const value_compare& comp, const allocator_type& a) : tree_type{identity<Key>{}, comp}, m_alloc{a}
    {}

    template <class InputIterator>
    set_base(InputIterator first, InputIterator last, const value_compare& comp = value_compare()) : set_base(comp)
    {
        insert(first, last);
    }

    template <class InputIterator>
    set_base(InputIterator first, InputIterator last, const value_compare& comp, const allocator_type& a) : set_base(comp, a)
    {
        insert(first, last);
    }

    set_base(const set_base & s) : set_base(s.value_comp(), std::allocator_traits<allocator_type>::select_on_container_copy_construction(s.get_allocator()))
    {
        insert(s.begin(), s.end());
    }

    set_base(set_base && s) noexcept(std::is_nothrow_move_constructible<allocator_type>::value &&
                          std::is_nothrow_move_constructible<key_compare>::value) = default;

    explicit set_base(const allocator_type& a) : m_alloc{a}
    {
    }

    set_base(const set_base & s, const allocator_type& a) : m_alloc(a)
    {
        insert(s.begin(), s.end());
    }

    set_base(set_base && s, const allocator_type& a) : tree_type(std::move(static_cast<tree_type&&>(s))), m_alloc(a)
    {
    }

    set_base(std::initializer_list<value_type> il, const value_compare& comp = value_compare()) : set_base(comp)
    {
        insert(il.begin(), il.end());
    }

    set_base(std::initializer_list<value_type> il, const value_compare& comp, const allocator_type& a) : set_base(comp, a)
    {
        insert(il.begin(), il.end());
    }

    template <class InputIterator>
    set_base(InputIterator first, InputIterator last, const allocator_type& a): set_base(a)
    {
        insert(first, last);
    }

    set_base(std::initializer_list<value_type> il, const allocator_type& a): set_base(a)
    {
        insert(il.begin(), il.end());
    }

    ~set_base()
    {
        clear();
    }

    set_base & operator=(const set_base & s)
    {
        clear();
        insert(s.begin(), s.end());
        return *this;
    }

    set_base & operator=(set_base && s) noexcept(allocator_type::propagate_on_container_move_assignment::value &&
                                     std::is_nothrow_move_assignable<allocator_type>::value &&
                                     std::is_nothrow_move_assignable<key_compare>::value) = default;

    set_base & operator=(std::initializer_list<value_type> il)
    {
        clear();
        insert(il.begin(), il.end());
        return *this;
    }

    using tree_type::begin;
    using tree_type::end;
    using tree_type::rbegin;
    using tree_type::rend;
    using tree_type::cbegin;
    using tree_type::cend;
    using tree_type::crbegin;
    using tree_type::crend;

    using tree_type::empty;
    using tree_type::size;
    using tree_type::max_size;


    insert_result_type make_insert_result(std::pair<data_type*, bool> result)
    {
        if constexpr(Unique) return std::make_pair(tree_type::make_iterator(result.first), result.second);
        else return tree_type::make_iterator(result.first);
    }

    template <class... Args>
    insert_result_type emplace(Args&&... args)
    {
        data_type* node = construct_impl(std::forward<Args>(args)...);
        std::pair<data_type*,bool> ret = insert_impl(node);
        if(!ret.second) {
            destroy_impl(node);
        }
        return make_insert_result(ret);
    }

    template <class... Args>
    iterator emplace_hint(const_iterator, Args&&... args)
    {
        //TODO: hint optimization
        data_type* node = construct_impl(std::forward<Args>(args)...);
        return tree_type::make_iterator(node);
    }

    insert_result_type insert(const value_type& v)
    {
        return emplace(v);
    }

    insert_result_type insert(value_type&& v)
    {
        return emplace(std::move(v));
    }

    iterator insert(const_iterator, const value_type& v)
    {
        //TODO: hint optimization
        data_type* node = construct_impl(v);
        return tree_type::make_iterator(node);
    }

    iterator insert(const_iterator, value_type&& v)
    {
        //TODO: hint optimization
        data_type* node = construct_impl(v);
        return tree_type::make_iterator(node);
    }

    template <class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        for(auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    void insert(std::initializer_list<value_type> il)
    {
        insert(il.begin(), il.end());
    }

    node_type extract(const_iterator position)
    {
        data_type* node = tree_type::node_from_iterator(position);
        tree_type::remove_node(node);
        return {m_alloc, node};
    }

    node_type extract(const key_type& x)
    {
        const_iterator position = find(x);
        return extract(position);
    }
    
    insert_return_type insert(node_type&& nh)
    {
        if constexpr(!Unique) {
            if(!nh) {
                return tree_type::end();
            }
            auto [new_node, _] = insert_impl(nh.get());
            nh.release();
            return tree_type::make_iterator(new_node);
        } else {
            if(!nh) {
                return {tree_type::end(), false, {}};
            }
            auto ret = insert_impl(nh.get());
            auto [new_node, inserted] = ret;
            if (!inserted) {
                return {tree_type::make_iterator(new_node), false, std::move(nh)};
            }
            nh.release();
            return {tree_type::make_iterator(new_node), true, {}};
        }
    }
    
    iterator insert(const_iterator, node_type&& nh)
    {
        //TODO: hint optimization
        if constexpr(!Unique) {
            return insert(std::move(nh));
        } else {
            return insert(std::move(nh)).position;
        }
    }

    iterator erase(const_iterator position)
    {
        data_type* node = tree_type::node_from_iterator(position++);
        tree_type::remove_node(node);
        destroy_impl(node);
        return position;
    }
    
    size_type erase(const key_type& k)
    {
        size_t ret = 0;
        auto [first, last] = tree_type::equal_range(k);
        for(auto it = first; it != last; it = erase(it)) {
            ++ret;
        }
        return ret;
    }
    
    iterator erase(const_iterator first, const_iterator last)
    {
        auto it = first;
        while(it != last) it = erase(it);
        return it;
    }
    
    void clear() noexcept
    {
        erase(begin(), end());
    }

    template<class C2>
    void merge(set_base<Key, Unique, C2, Allocator>& source)
    {
        merge_impl(source);
    }
    
    template<class C2>
    void merge(set_base<Key, Unique, C2, Allocator>&& source)
    {
        merge_impl(std::move(source));
    }
    
    template<class C2>
    void merge(set_base<Key, !Unique, C2, Allocator>& source)
    {
        merge_impl(source);
    }
   
    template<class C2>
    void merge(set_base<Key, !Unique, C2, Allocator>&& source)
    {
        merge_impl(std::move(source));
    }

    void swap(set_base & s) noexcept(std::allocator_traits<Allocator>::is_always_equal::value &&
                               std::is_nothrow_swappable_v<tree_type>)
    {
        return tree_type::swap(s);
    }

    [[nodiscard]] allocator_type get_allocator() const noexcept
    {
        return m_alloc;
    }
   
    using tree_type::key_comp;
    using tree_type::value_comp;
    
    [[nodiscard]] iterator find(const key_type& k)
    {
        return tree_type::find(k);
    }
    
    [[nodiscard]] const_iterator find(const key_type& k) const
    {
        return tree_type::find(k);
    }
    
    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] iterator find(const K& k)
    {
        return tree_type::find(k);
    }

    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] const_iterator find(const K& k) const
    {
        return tree_type::find(k);
    }

    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] size_type count(const K& x) const
    {
        return tree_type::count(x);
    }
    
    [[nodiscard]] size_type count(const key_type& k) const
    {
        return tree_type::count(k);
    }

    [[nodiscard]] bool contains(const key_type& x) const
    {
        return tree_type::contains(x);
    }

    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] bool contains(const K& x) const
    {
        return tree_type::contains(x);
    }

    [[nodiscard]] iterator lower_bound(const key_type& k)
    {
        return tree_type::lower_bound(k);
    }
    
    [[nodiscard]] const_iterator lower_bound(const key_type& k) const
    {
        return tree_type::lower_bound(k);
    }
    
    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] iterator lower_bound(const K& x)
    {
        return tree_type::lower_bound(x);
    }
    
    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] const_iterator lower_bound(const K& x) const
    {
        return tree_type::lower_bound(x);
    }

    [[nodiscard]] iterator upper_bound(const key_type& k)
    {
        return tree_type::upper_bound(k);
    }
    
    [[nodiscard]] const_iterator upper_bound(const key_type& k) const
    {
        return tree_type::upper_bound(k);
    }
    
    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] iterator upper_bound(const K& x)
    {
        return tree_type::upper_bound(x);
    }
    
    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] const_iterator upper_bound(const K& x) const
    {
        return tree_type::upper_bound(x);
    }
    
    [[nodiscard]] std::pair<iterator,iterator> equal_range(const key_type& k)
    {
        return tree_type::equal_range(k);
    }
    
    [[nodiscard]] std::pair<const_iterator,const_iterator> equal_range(const key_type& k) const
    {
        return tree_type::equal_range(k);
    }
    
    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] std::pair<iterator,iterator> equal_range(const K& x)
    {
        return tree_type::equal_range(x);
    }
    
    template <typename K, class Comp = Compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] std::pair<const_iterator,const_iterator> equal_range(const K& x) const
    {
        return tree_type::equal_range(x);
    }
private:

    template<class S2>
    void merge_impl(S2&& source)
    {
        typename tree_type::insert_hints hints;
        for(auto it = source.begin(); it != source.end();)
        {
            data_type* node = source.node_from_iterator(it++);
            std::pair<data_type*,bool> ret = insert_impl(node);
            if (ret.second) {
                source.remove_node_impl(node);
            }
        }
    }

    template <typename... Args>
    data_type* construct_impl(Args&&... args)
    {
        data_type* node = m_alloc.allocate(1);
        return std::uninitialized_construct_using_allocator<data_type>(node, m_alloc, std::forward<Args>(args)...);
    }

    std::pair<data_type*,bool> insert_impl(data_type* node)
    {
        typename tree_type::insert_hints hints;
        data_type* conflict = tree_type::preinsert_node(node, hints);
        if (conflict) {
            return std::make_pair(conflict, false);
        }
        tree_type::insert_node(node, hints);
        return std::make_pair(node, true);
    }

    void destroy_impl(data_type* node)
    {
        std::allocator_traits<node_allocator_type>::destroy(m_alloc, node);
        std::allocator_traits<node_allocator_type>::deallocate(m_alloc, node, 1);
    }

    [[no_unique_address]] node_allocator_type m_alloc;
};

namespace detail
{
template <class T>
concept boolean_testable_impl = std::convertible_to<T, bool>;

template <class T>
concept boolean_testable = boolean_testable_impl<T> && requires(T&& t) {
    { !std::forward<T>(t) } -> boolean_testable_impl;
};

inline constexpr auto synth_three_way = []<class T, class U>(const T& t, const U& u)
requires requires {
    { t < u } -> boolean_testable;
    { u < t } -> boolean_testable;
}
{
    if constexpr (std::three_way_comparable_with<T, U>) {
        return t <=> u;
    } else {
        if (t < u)
            return std::weak_ordering::less;
        if (u < t)
            return std::weak_ordering::greater;
        return std::weak_ordering::equivalent;
    }
};

template <class T, class U = T>
using synth_three_way_result = decltype(synth_three_way(std::declval<T&>(), std::declval<U&>()));

}

/*

template <class InputIterator,
      class Compare = std::less<typename std::iterator_traits<InputIterator>::value_type>,
      class Allocator = std::allocator<typename std::iterator_traits<InputIterator>::value_type>>
set(InputIterator, InputIterator,
    Compare = Compare(), Allocator = Allocator())
  -> set<typename std::iterator_traits<InputIterator>::value_type, Compare, Allocator>;

template<class Key, class Compare = std::less<Key>, class Allocator = std::allocator<Key>>
set(std::initializer_list<Key>, Compare = Compare(), Allocator = Allocator())
  -> set<Key, Compare, Allocator>;

template<class InputIterator, class Allocator>
set(InputIterator, InputIterator, Allocator)
  -> set<typename std::iterator_traits<InputIterator>::value_type,
          std::less<typename std::iterator_traits<InputIterator>::value_type>, Allocator>;

template<class Key, class Allocator>
set(std::initializer_list<Key>, Allocator) -> set<Key, std::less<Key>, Allocator>;

*/


template <class Key, bool Unique, class Compare, class Allocator>
inline bool operator==(const set_base<Key, Unique, Compare, Allocator>& x, const set_base<Key, Unique, Compare, Allocator>& y) {
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}

template <class Key, bool Unique, class Compare, class Allocator>
detail::synth_three_way_result<Key> operator<=>(const set_base<Key, Unique, Compare, Allocator>& x, const set_base<Key, Unique, Compare, Allocator>& y) {
    return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(), detail::synth_three_way);
}

template <class Key, bool Unique, class Compare, class Allocator>
void swap(set_base<Key, Unique, Compare, Allocator>& x, set_base<Key, Unique, Compare, Allocator>& y) noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

template <class Key, bool Unique, class Compare, class Allocator, class Predicate>
typename set_base<Key, Unique, Compare, Allocator>::size_type erase_if(set_base<Key, Unique, Compare, Allocator>& c, Predicate pred)
{
    auto old_size = c.size();
    for (auto first = c.begin(), last = c.end(); first != last;)
    {
        if (pred(*first))
            first = c.erase(first);
        else
            ++first;
    }
    return old_size - c.size();
}

namespace detail
{
template <typename Key>
class set_data
{
public:
    using value_type = Key;

    const set_data* parent() const
    {
        return reinterpret_cast<const set_data*>(m_par_and_flag & ~1UL);
    }

    set_data* parent()
    {
        return reinterpret_cast<set_data*>(m_par_and_flag & ~1UL);
    }

    void set_parent(const set_data *p)
    {
        m_par_and_flag = (m_par_and_flag & 1UL) | reinterpret_cast<uintptr_t>(p);
    }

    bool rp() const
    {
        return m_par_and_flag & 1UL;
    }

    void flip()
    {
        m_par_and_flag ^= 1UL;
    }

    const set_data* left() const
    {
        return m_left;
    }

    set_data* left()
    {
        return m_left;
    }

    const set_data* right() const
    {
        return m_right;
    }

    set_data* right()
    {
        return m_right;
    }

    void set_right(set_data* node)
    {
        m_right = node;
    }

    void set_left(set_data* node)
    {
        m_left = node;
    }

    void reset()
    {
        m_left = nullptr;
        m_right = nullptr;
        m_par_and_flag = 0;
    }

    void clone(set_data* node)
    {
        m_left = node->m_left;
        m_right = node->m_right;
        m_par_and_flag = node->m_par_and_flag;
    }

    const value_type& value() const { return m_value; }
    value_type& value() { return m_value; }

    template <typename... Args>
    set_data(Args&&... args) : m_value(std::forward<Args>(args)...){}
private:
    set_data* m_left{nullptr};
    set_data* m_right{nullptr};
    uintptr_t m_par_and_flag{};
    value_type m_value;
};
} // namespace detail


} // namespace tmi

#endif // TMISET_H_
