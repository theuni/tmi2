// Copyright (c) 2026 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMISET_H_
#define TMISET_H_

#include <hash_tree.h>
#include <tmi_index.h>
#include <tmi_nodehandle.h>

#include <cstdint>

namespace tmi
{

template <class Key, bool Unique, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = std::allocator<Key>>
class unordered_set_base;

template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = std::allocator<Key>>
using unordered_set = unordered_set_base<Key, true, Hash, KeyEqual, Allocator>;

template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = std::allocator<Key>>
using unordered_multiset = unordered_set_base<Key, false, Hash, KeyEqual, Allocator>;

namespace detail
{
template <typename Key>
class set_data;

template <class Hash, class = void>
inline const bool is_transparent_v = false;

template <class Hash>
inline const bool is_transparent_v<Hash, std::void_t<typename Hash::is_transparent> > = true;

} // namespace detail

template <class Key, bool Unique, class Hash, class KeyEqual, class Allocator>
class unordered_set_base : private hash_tree<detail::set_data<Key>, identity<Key>, Hash, KeyEqual, Unique, Allocator>
{
    using hash_table_type = hash_tree<detail::set_data<Key>, identity<Key>, Hash, KeyEqual, Unique, Allocator>;
    using data_type = detail::set_data<Key>;
    using node_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<data_type>;

    template <class, bool, class, class, class>
    friend class unordered_set_base;

public:
    using key_type = hash_table_type::key_type;
    using value_type = hash_table_type::value_type;
    using hasher = hash_table_type::hasher_type;
    using key_equal = hash_table_type::key_equal_type;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = hash_table_type::size_type;
    using difference_type = hash_table_type::difference_type;
    using pointer = std::allocator_traits<allocator_type>::pointer;
    using const_pointer = std::allocator_traits<allocator_type>::const_pointer;

    using iterator = hash_table_type::iterator;
    using const_iterator = hash_table_type::const_iterator;
    using local_iterator = hash_table_type::local_iterator;
    using const_local_iterator = hash_table_type::const_local_iterator;
    using node_type = tmi::detail::node_handle<allocator_type, data_type>;
    using insert_return_type = std::conditional_t<Unique, tmi::detail::insert_return_type<iterator, node_type>, iterator>;
    using insert_result_type = std::conditional_t<Unique, std::pair<iterator, bool>, iterator>;

    using buckets_type = std::unique_ptr<data_type*[]>;

    unordered_set_base()
    {
    }

    explicit unordered_set_base(size_type n, const hasher& hf = hasher(), const key_equal& eql = key_equal(), const allocator_type& a = allocator_type()) : hash_table_type{identity<Key>{}, hf, eql}, m_alloc{a}
    {
        rehash(n);
    }

    template <class InputIterator>
    unordered_set_base(InputIterator f, InputIterator l, size_type n = 0, const hasher& hf = hasher(), const key_equal& eql = key_equal(), const allocator_type& a = allocator_type()) : unordered_set_base(n, hf, eql, a)
    {
        insert(f, l);
    }

    explicit unordered_set_base(const allocator_type& a) : m_alloc{a}
    {
    }

    unordered_set_base(const unordered_set_base& u) : hash_table_type{}
    {
        insert(u.begin(), u.end());
    }

    unordered_set_base(const unordered_set_base& u, const Allocator& a) : m_alloc{a}
    {
        insert(u.begin(), u.end());
    }

    unordered_set_base(unordered_set_base&&) = default;
    unordered_set_base(unordered_set_base&& u, const Allocator& a) : hash_table_type{std::move(u)}, m_alloc{a}
    {
    }

    unordered_set_base(std::initializer_list<value_type> il, size_type n = 0, const hasher& hf = hasher(), const key_equal& eql = key_equal(), const allocator_type& a = allocator_type()) : unordered_set_base(n, hf, eql, a)
    {
        insert(il.begin(), il.end());
    }

    unordered_set_base(size_type n, const allocator_type& a) : m_alloc{a}
    {
        rehash(n);
    }

    unordered_set_base(size_type n, const hasher& hf, const allocator_type& a) : hash_table_type(hf), m_alloc{a}
    {
        rehash(n);
    }

    template <class InputIterator>
    unordered_set_base(InputIterator f, InputIterator l, size_type n, const allocator_type& a) : unordered_set_base(n, a)
    {
        insert(f, l);
    }

    template <class InputIterator>
    unordered_set_base(InputIterator f, InputIterator l, size_type n, const hasher& hf,  const allocator_type& a) : unordered_set_base(n, hf, a)
    {
        insert(f, l);
    }

    unordered_set_base(std::initializer_list<value_type> il, size_type n, const allocator_type& a) : unordered_set_base(il.begin(), il.end(), n, a)
    {
    }

    unordered_set_base(std::initializer_list<value_type> il, size_type n, const hasher& hf,  const allocator_type& a) : unordered_set_base(il.begin(), il.end(), n, hf, a)
    {
    }

    ~unordered_set_base()
    {
        clear();
    }

    unordered_set_base & operator=(const unordered_set_base & s)
    {
        clear();
        insert(s.begin(), s.end());
        return *this;
    }

    unordered_set_base & operator=(unordered_set_base && s) noexcept(std::allocator_traits<Allocator>::is_always_equal_v
                                        && std::is_nothrow_move_assignable_v<Hash> && std::is_nothrow_move_assignable_v<KeyEqual>) = default;

    unordered_set_base & operator=(std::initializer_list<value_type> il)
    {
        clear();
        insert(il.begin(), il.end());
        return *this;
    }

    using hash_table_type::begin;
    using hash_table_type::end;
    using hash_table_type::cbegin;
    using hash_table_type::cend;

    size_type size() const noexcept
    {
        return m_size;
    }

    bool empty() const noexcept
    {
        return !size();
    }

    size_type max_size() const noexcept
    {
        return std::min<size_type>(std::allocator_traits<node_allocator_type>::max_size(m_alloc), std::numeric_limits<difference_type >::max());
    }


    insert_result_type make_insert_result(std::pair<data_type*, bool> result)
    {
        if constexpr(Unique) return std::make_pair(hash_table_type::make_iterator(result.first), result.second);
        else return hash_table_type::make_iterator(result.first);
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
        return hash_table_type::make_iterator(node);
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
        return hash_table_type::make_iterator(node);
    }

    iterator insert(const_iterator, value_type&& v)
    {
        //TODO: hint optimization
        data_type* node = construct_impl(v);
        return hash_table_type::make_iterator(node);
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
        data_type* node = hash_table_type::node_from_iterator(position);
        remove_node_impl(node);
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
                return hash_table_type::end();
            }
            auto [new_node, _] = insert_impl(nh.get());
            nh.release();
            return hash_table_type::make_iterator(new_node);
        } else {
            if(!nh) {
                return {hash_table_type::end(), false, {}};
            }
            auto ret = insert_impl(nh.get());
            auto [new_node, inserted] = ret;
            if (!inserted) {
                return {hash_table_type::make_iterator(new_node), false, std::move(nh)};
            }
            nh.release();
            return {hash_table_type::make_iterator(new_node), true, {}};
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
        data_type* node = hash_table_type::node_from_iterator(position++);
        remove_node_impl(node);
        destroy_impl(node);
        return position;
    }

    size_type erase(const key_type& k)
    {
        size_t ret = 0;
        auto [first, last] = hash_table_type::equal_range(k);
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

    template<class Hash2, class KeyEqual2>
    void merge(unordered_set_base<Key, Unique, Hash2, KeyEqual2, Allocator>& source)
    {
        merge_impl(source);
    }

    template<class Hash2, class KeyEqual2>
    void merge(unordered_set_base<Key, Unique, Hash2, KeyEqual2, Allocator>&& source)
    {
        merge_impl(std::move(source));
    }

    template<class Hash2, class KeyEqual2>
    void merge(unordered_set_base<Key, !Unique, Hash2, KeyEqual2, Allocator>& source)
    {
        merge_impl(source);
    }

    template<class Hash2, class KeyEqual2>
    void merge(unordered_set_base<Key, !Unique, Hash2, KeyEqual2, Allocator>&& source)
    {
        merge_impl(std::move(source));
    }

    void swap(unordered_set_base & s) noexcept(std::allocator_traits<Allocator>::is_always_equal::value &&
                               std::is_nothrow_swappable_v<hash_table_type>)
    {
        return hash_table_type::swap(s);
    }

    [[nodiscard]] allocator_type get_allocator() const noexcept
    {
        return m_alloc;
    }

    [[nodiscard]] iterator find(const key_type& k)
    {
        return hash_table_type::find(k);
    }

    [[nodiscard]] const_iterator find(const key_type& k) const
    {
        return hash_table_type::find(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] iterator find(const K& k)
    {
        return hash_table_type::find(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] const_iterator find(const K& k) const
    {
        return hash_table_type::find(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] size_type count(const K& x) const
    {
        return hash_table_type::count(x);
    }

    [[nodiscard]] size_type count(const key_type& k) const
    {
        return hash_table_type::count(k);
    }

    [[nodiscard]] bool contains(const key_type& x) const
    {
        return hash_table_type::contains(x);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] bool contains(const K& x) const
    {
        return hash_table_type::contains(x);
    }

    [[nodiscard]] iterator lower_bound(const key_type& k)
    {
        return hash_table_type::lower_bound(k);
    }

    [[nodiscard]] const_iterator lower_bound(const key_type& k) const
    {
        return hash_table_type::lower_bound(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] iterator lower_bound(const K& x)
    {
        return hash_table_type::lower_bound(x);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] const_iterator lower_bound(const K& x) const
    {
        return hash_table_type::lower_bound(x);
    }

    [[nodiscard]] iterator upper_bound(const key_type& k)
    {
        return hash_table_type::upper_bound(k);
    }

    [[nodiscard]] const_iterator upper_bound(const key_type& k) const
    {
        return hash_table_type::upper_bound(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] iterator upper_bound(const K& x)
    {
        return hash_table_type::upper_bound(x);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] const_iterator upper_bound(const K& x) const
    {
        return hash_table_type::upper_bound(x);
    }

    [[nodiscard]] std::pair<iterator,iterator> equal_range(const key_type& k)
    {
        return hash_table_type::equal_range(k);
    }

    [[nodiscard]] std::pair<const_iterator,const_iterator> equal_range(const key_type& k) const
    {
        return hash_table_type::equal_range(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] std::pair<iterator,iterator> equal_range(const K& x)
    {
        return hash_table_type::equal_range(x);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] std::pair<const_iterator,const_iterator> equal_range(const K& x) const
    {
        return hash_table_type::equal_range(x);
    }

    size_type bucket( const Key& key ) const
    {
        return hash_table_type::bucket(key);
    }

    void rehash(size_type buckets) {
        rehash_impl(buckets);
    }

    void reserve(size_type count)
    {
        rehash(std::ceil(count / max_load_factor()));
    }

    using hash_table_type::bucket_size;
    using hash_table_type::bucket_count;

    size_type max_bucket_count() const noexcept
    {
        return max_size();
    }

    float load_factor() const
    {
        return size() / hash_table_type::bucket_count();
    }

    float max_load_factor() const
    {
        return m_max_load_factor;
    }

    void max_load_factor(float n)
    {
        m_max_load_factor = std::max(n, m_max_load_factor);
    }

    using hash_table_type::hash_function;
    using hash_table_type::key_eq;

private:

    [[no_unique_address]] node_allocator_type m_alloc;
    size_type m_size{0};
    float m_max_load_factor{0.8f};
    buckets_type m_buckets;

    template<class S2>
    void merge_impl(S2&& source)
    {
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

    void remove_node_impl(data_type* node)
    {
        hash_table_type::remove_node(node);
        m_size--;
    }

    std::pair<data_type*,bool> insert_impl(data_type* node)
    {
        typename hash_table_type::insert_hints hints;
        size_t buckets = bucket_count();
        if (!buckets) {
            rehash_impl(1);
        } else if (size() + 1 > buckets * max_load_factor()) {
            rehash_impl(buckets * 2);
        }
        data_type* conflict = hash_table_type::preinsert_node(node, hints);
        if (conflict) {
            return std::make_pair(conflict, false);
        }
        hash_table_type::insert_node(node, hints);
        m_size++;
        return std::make_pair(node, true);
    }

    void rehash_impl(size_t new_bucket_count)
    {
        auto new_buckets = allocate_buckets_impl(new_bucket_count);
        hash_table_type::rehash(std::span<data_type*>{new_buckets.get(), new_bucket_count});
        m_buckets = std::move(new_buckets);
    }

    void destroy_impl(data_type* node)
    {
        std::allocator_traits<node_allocator_type>::destroy(m_alloc, node);
        std::allocator_traits<node_allocator_type>::deallocate(m_alloc, node, 1);
    }

    static buckets_type allocate_buckets_impl(size_t new_capacity)
    {
        return std::make_unique<data_type*[]>(new_capacity);
    }
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
inline bool operator==(const unordered_set_base<Key, Unique, Compare, Allocator>& x, const unordered_set_base<Key, Unique, Compare, Allocator>& y) {
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}

template <class Key, bool Unique, class Compare, class Allocator>
detail::synth_three_way_result<Key> operator<=>(const unordered_set_base<Key, Unique, Compare, Allocator>& x, const unordered_set_base<Key, Unique, Compare, Allocator>& y) {
    return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(), detail::synth_three_way);
}

template <class Key, bool Unique, class Compare, class Allocator>
void swap(unordered_set_base<Key, Unique, Compare, Allocator>& x, unordered_set_base<Key, Unique, Compare, Allocator>& y) noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

template <class Key, bool Unique, class Compare, class Allocator, class Predicate>
typename unordered_set_base<Key, Unique, Compare, Allocator>::size_type erase_if(unordered_set_base<Key, Unique, Compare, Allocator>& c, Predicate pred)
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

    const set_data* next_hash() const
    {
        return static_cast<const set_data*>(m_nexthash);
    }

    set_data* next_hash()
    {
        return static_cast<set_data*>(m_nexthash);
    }

    size_t hash() const
    {
        return m_hash;
    }

    void set_hash(size_t hash)
    {
        m_hash = hash;
    }

    void set_next_hashptr(const set_data* rhs)
    {
        m_nexthash = const_cast<set_data*>(rhs);
    }
    const value_type& value() const { return m_value; }
    value_type& value() { return m_value; }

    template <typename... Args>
    set_data(Args&&... args) : m_value(std::forward<Args>(args)...){}
private:
    set_data* m_nexthash{nullptr};
    size_t m_hash{0};
    value_type m_value;
};
} // namespace detail


} // namespace tmi

#endif // TMISET_H_
