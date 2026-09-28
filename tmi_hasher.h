// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMI_HASHER_H_
#define TMI_HASHER_H_

#include <hash_tree.h>
#include <tmi_index.h>
#include <tmi_nodehandle.h>
#include <bucket_list.h>

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstring>
#include <cstddef>
#include <iterator>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace tmi {


template <typename T, typename IndexedNode, typename Parent, bool Unique, bool IsOnlyIndex, class Hash, class KeyEqual, class KeyFromValue, typename Allocator>
class tmi_hasher

{

    using data_type = IndexedNode;
    using BucketList = tmi::bucket_list<data_type, Allocator>;
    using key_from_value = KeyFromValue;
    using key_type = typename key_from_value::result_type;
    using hash_table_type = hash_tree<data_type, T, key_from_value, Hash, KeyEqual, Allocator, BucketList, Unique>;
    using node_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<data_type>;
    using bucket_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<data_type*>;
    using node_pointer = std::allocator_traits<node_allocator_type>::pointer;
    using insert_hints_type = hash_table_type::insert_hints;
    using buckets_type = bucket_list<data_type, Allocator>;
    struct ConstructionKey { constexpr ConstructionKey() noexcept = default; };

    template <typename, typename, typename, bool, bool, class, class, class, typename>
    friend class tmi_hasher;
    friend Parent;

    template <typename, typename, typename>
    friend class multi_index_container;

    class buckets_allocator;

    using premodify_cache = hash_table_type::premodify_cache;
    static constexpr bool requires_premodify_cache() { return hash_table_type::requires_premodify_cache(); }
public:
    using value_type = hash_table_type::value_type;
    using hasher = hash_table_type::hasher_type;
    using key_equal = hash_table_type::key_equal_type;
    using allocator_type = hash_table_type::allocator_type;
    using reference = hash_table_type::reference;
    using const_reference = hash_table_type::const_reference;
    using size_type = hash_table_type::size_type;
    using difference_type = hash_table_type::difference_type;
    using pointer = hash_table_type::pointer;
    using const_pointer = hash_table_type::const_pointer;
    using iterator = hash_table_type::iterator;
    using const_iterator = hash_table_type::const_iterator;
    using local_iterator = hash_table_type::local_iterator;
    using const_local_iterator = hash_table_type::const_local_iterator;

    using node_type = tmi::detail::node_handle<allocator_type, data_type>;
    using insert_return_type = std::conditional_t<!Unique && IsOnlyIndex, iterator, tmi::detail::insert_return_type<iterator, node_type>>;
    using insert_result_type = std::conditional_t<!Unique && IsOnlyIndex, iterator, std::pair<iterator, bool>>;
    using ctor_args = std::tuple<size_type,key_from_value,hasher,key_equal>;

    static_assert(std::is_same_v<value_type, typename allocator_type::value_type>);

    explicit tmi_hasher(ConstructionKey, Parent& parent, allocator_type& alloc, const key_from_value& kv = key_from_value{}, const hasher& hf = hasher(), const key_equal& eql = key_equal()) : m_parent{parent}, m_buckets{alloc}, m_hash_table{m_buckets, kv, hf, eql}
    {
    }

    explicit tmi_hasher(ConstructionKey, Parent& parent, allocator_type& alloc, size_type, const key_from_value& kv = key_from_value{}, const hasher& hf = hasher(), const key_equal& eql = key_equal()) : m_parent{parent}, m_buckets{alloc}, m_hash_table{m_buckets, kv,
hf, eql}
    {
    }

    tmi_hasher(ConstructionKey, Parent& parent, Allocator& a, const tmi_hasher& u) : m_parent{parent}, m_buckets{a}, m_hash_table{u.m_hash_table, m_buckets}
    {
    }

    tmi_hasher(ConstructionKey, Parent& parent, tmi_hasher&& s) : m_parent{parent}, m_max_load_factor{s.m_max_load_factor}, m_buckets{std::move(s.m_buckets)}, m_hash_table{std::move(s.m_hash_table)}
    {
    }

    tmi_hasher(tmi_hasher&& s) : tmi_hasher{ConstructionKey{}, s.m_parent, std::move(s)}
    {
    }

    tmi_hasher& operator=(const tmi_hasher & s)
    {
        if (this == std::addressof(s))
            return *this;

        m_max_load_factor = s.max_load_factor();
        m_buckets.resize(s.bucket_count());
        m_hash_table = hash_table_type(s.m_hash_table, m_buckets);
        return *this;
    }

    tmi_hasher & operator=(tmi_hasher && s) noexcept(std::allocator_traits<Allocator>::is_always_equal::value
                                        && std::is_nothrow_move_assignable_v<Hash> && std::is_nothrow_move_assignable_v<KeyEqual>)
    {
        if (this == std::addressof(s))
            return *this;

        m_max_load_factor = s.max_load_factor();
        m_buckets = std::move(s.m_buckets);
        m_hash_table = std::move(s.m_hash_table);

        return *this;
    }

    void assign_release(tmi_hasher && s)
    {
        m_max_load_factor = s.max_load_factor();
        m_buckets.resize(s.bucket_count());
        m_hash_table = hash_table_type(std::move(s.m_hash_table), m_buckets);
    }

    iterator begin() noexcept
    {
        return m_hash_table.begin();
    }

    const_iterator begin() const noexcept
    {
        return m_hash_table.begin();
    }

    iterator end() noexcept
    {
        return m_hash_table.end();
    }

    const_iterator end() const noexcept
    {
        return m_hash_table.end();
    }

    const_iterator cbegin() const noexcept
    {
        return m_hash_table.cbegin();
    }

    const_iterator cend() const noexcept
    {
        return m_hash_table.cend();
    }

    local_iterator begin(size_type n)
    {
        return m_hash_table.begin(n);
    }

    local_iterator end(size_type n)
    {
        return m_hash_table.end(n);
    }

    const_local_iterator begin(size_type n) const
    {
        return m_hash_table.begin(n);
    }

    const_local_iterator end(size_type n) const
    {
        return m_hash_table.end(n);
    }

    const_local_iterator cbegin(size_type n) const
    {
        return m_hash_table.cbegin(n);
    }

    const_local_iterator cend(size_type n) const
    {
        return m_hash_table.cend(n);
    }

    size_type size() const noexcept
    {
        return m_parent.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return size() == 0;
    }

    size_type max_size() const noexcept
    {
        return std::min<size_type>(std::allocator_traits<allocator_type>::max_size(get_allocator()), std::numeric_limits<difference_type >::max());
    }


    insert_result_type make_insert_result(std::pair<data_type*, bool> result)
    {
        if constexpr(!Unique && IsOnlyIndex) {
            return m_hash_table.make_iterator(result.first);
        } else {
            return std::make_pair(m_hash_table.make_iterator(result.first), result.second);
        }
    }

    insert_return_type make_insert_return(data_type* node, bool inserted, node_type&& nh)
    {
        if constexpr(!Unique && IsOnlyIndex) {
            return m_hash_table.make_iterator(node);
        } else {
            return {m_hash_table.make_iterator(node), inserted, std::move(nh)};
        }
    }

    template <class... Args>
    insert_result_type emplace(Args&&... args)
    {
        auto result = m_parent.template do_emplace<data_type>(std::forward<Args>(args)...);
        return make_insert_result(result);
    }

    template <class... Args>
    iterator emplace_hint(const_iterator, Args&&... args)
    {
        //TODO: hint optimization
        const auto& [node, inserted] = m_parent.template do_emplace<data_type>(std::forward<Args>(args)...);
        return m_hash_table.make_iterator(node);
    }

    insert_result_type insert(const value_type& v)
    {
        auto result = m_parent.template do_insert<data_type>(v);
        return make_insert_result(result);
    }

    insert_result_type insert(value_type&& v)
    {
        auto result = m_parent.template do_insert<data_type>(std::move(v));
        return make_insert_result(result);
    }

    iterator insert(const_iterator, const value_type& v)
    {
        //TODO: hint optimization
        auto [node, success] = m_parent.template do_insert<data_type>(v);
        return m_hash_table.make_iterator(node);
    }

    iterator insert(const_iterator, value_type&& v)
    {
        //TODO: hint optimization
        auto [node, success] = m_parent.template do_insert<data_type>(std::move(v));
        return m_hash_table.make_iterator(node);
    }

    template <class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        for(auto it = first; it != last; ++it) {
            m_parent.template do_emplace<data_type>(*it);
        }
    }

    void insert(std::initializer_list<value_type> il)
    {
        insert(il.begin(), il.end());
    }

    node_type extract(const_iterator position)
    {
        data_type* node = m_hash_table.node_from_iterator(position);
        return m_parent.do_extract(const_cast<data_type*>(node));
    }

    node_type extract(const key_type& x)
    {
        const_iterator position = find(x);
        return extract(position);
    }

    insert_return_type insert(node_type&& handle)
    {
        if(!handle) {
            return make_insert_return(nullptr, false, {});
        }
        auto ret = m_parent.do_reinsert_node(handle.get());
        const auto& [new_node, inserted] = ret;
        if (!inserted) {
            return make_insert_return(new_node, false, std::move(handle));
        }
        handle.release();
        return make_insert_return(new_node, true, {});
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

    iterator erase(const_iterator it)
    {
        data_type* node = const_cast<data_type*>(m_hash_table.node_from_iterator(it));
        ++it;
        m_parent.do_erase(node);
        return it;
    }

    size_type erase(const key_type& k)
    {
        size_t ret = 0;
        auto [first, last] = m_hash_table.equal_range(k);
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
        m_parent.do_clear();
    }

    void unlink_node(data_type* node)
    {
        m_parent.template do_unlink<data_type>(node);
    }

    template<class IndexedNode2, class Parent2, class Hash2, class KeyEqual2>
    void merge(tmi_hasher<T, IndexedNode2, Parent2, Unique, IsOnlyIndex, Hash2, KeyEqual2, KeyFromValue, Allocator>& source)
    {
        m_parent.template do_merge_index<data_type>(source);
    }

    template<class IndexedNode2, class Parent2, class Hash2, class KeyEqual2>
    void merge(tmi_hasher<T, IndexedNode2, Parent2, Unique, IsOnlyIndex, Hash2, KeyEqual2, KeyFromValue, Allocator>&& source)
    {
        m_parent.template do_merge_index<data_type>(source);
    }

    template<class IndexedNode2, class Parent2, class Hash2, class KeyEqual2>
    void merge(tmi_hasher<T, IndexedNode2, Parent2, !Unique, IsOnlyIndex, Hash2, KeyEqual2, KeyFromValue, Allocator>& source)
    {
        m_parent.template do_merge_index<data_type>(source);
    }

    template<class IndexedNode2, class Parent2, class Hash2, class KeyEqual2>
    void merge(tmi_hasher<T, IndexedNode2, Parent2, !Unique, IsOnlyIndex, Hash2, KeyEqual2, KeyFromValue, Allocator>&& source)
    {
        m_parent.template do_merge_index<data_type>(source);
    }

    void swap(tmi_hasher & s)
        noexcept(std::allocator_traits<Allocator>::is_always_equal::value && std::is_nothrow_swappable_v<hash_table_type>)
    {
        m_buckets.swap(s.m_buckets);
        m_hash_table.swap(s.m_hash_table);
        std::swap(m_max_load_factor, s.m_max_load_factor);
    }

    [[nodiscard]] allocator_type get_allocator() const noexcept
    {
        return m_parent.get_allocator();
    }

    [[nodiscard]] iterator find(const key_type& k)
    {
        return m_hash_table.find(k);
    }

    [[nodiscard]] const_iterator find(const key_type& k) const
    {
        return m_hash_table.find(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] iterator find(const K& k)
    {
        return m_hash_table.find(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] const_iterator find(const K& k) const
    {
        return m_hash_table.find(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] size_type count(const K& x) const
    {
        return m_hash_table.count(x);
    }

    [[nodiscard]] size_type count(const key_type& k) const
    {
        return m_hash_table.count(k);
    }

    [[nodiscard]] bool contains(const key_type& x) const
    {
        return m_hash_table.contains(x);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] bool contains(const K& x) const
    {
        return m_hash_table.contains(x);
    }

    [[nodiscard]] std::pair<iterator,iterator> equal_range(const key_type& k)
    {
        return m_hash_table.equal_range(k);
    }

    [[nodiscard]] std::pair<const_iterator,const_iterator> equal_range(const key_type& k) const
    {
        return m_hash_table.equal_range(k);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] std::pair<iterator,iterator> equal_range(const K& x)
    {
        return m_hash_table.equal_range(x);
    }

    template <class K, class Hasher2 = hasher, class KeyEqual2 = key_equal, std::enable_if_t<detail::is_transparent_v<Hasher2> && detail::is_transparent_v<KeyEqual2>>* = nullptr>
    [[nodiscard]] std::pair<const_iterator,const_iterator> equal_range(const K& x) const
    {
        return m_hash_table.equal_range(x);
    }

    size_type bucket( const key_type& key ) const
    {
        return m_hash_table.bucket(key);
    }

    void rehash(size_type buckets) {
        rehash_impl(buckets);
    }

    void reserve(size_type count)
    {
        rehash(static_cast<size_type>(std::ceil(static_cast<float>(count) / max_load_factor())));
    }

    size_type bucket_size(size_type bucket) const
    {
        return m_hash_table.bucket_size(bucket);
    }

    size_type bucket_count() const
    {
        return m_hash_table.bucket_count();
    }

    size_type max_bucket_count() const noexcept
    {
        return max_size();
    }

    float load_factor() const
    {
        size_type bucket_count = m_hash_table.bucket_count();
        return bucket_count ? static_cast<float>(size()) / static_cast<float>(bucket_count) : 0.0f;
    }

    float max_load_factor() const
    {
        return m_max_load_factor;
    }

    void max_load_factor(float n)
    {
        m_max_load_factor = std::max(n, m_max_load_factor);
    }

    hasher hash_function() const
    {
        return m_hash_table.hash_function();
    }

    key_equal key_eq() const
    {
        return m_hash_table.key_eq();
    }

    key_from_value key_extractor() const
    {
        return m_hash_table.key_extractor();
    }

    template <typename Callable>
    bool modify(iterator it, Callable&& func)
    {
        data_type* node = const_cast<data_type*>(m_hash_table.node_from_iterator(it));
        if (!node) return false;
        return m_parent.do_modify(node, std::forward<Callable>(func));
    }

private:

    Parent& m_parent;
    float m_max_load_factor{1.0f};
    buckets_type m_buckets;
    hash_table_type m_hash_table;

    void rehash_impl(size_type to)
    {
        size_type new_bucket_count = m_hash_table.increase_bucket_count(bucket_count(), to);
        if (new_bucket_count > bucket_count()) {
            m_buckets.resize(new_bucket_count);
            m_hash_table.rehash(m_buckets);
        }
    }

    const data_type* node_from_iterator(const_iterator it) const
    {
        return m_hash_table.node_from_iterator(it);
    }

    data_type* node_from_iterator(iterator it)
    {
        return m_hash_table.node_from_iterator(it);
    }

    insert_hints_type tmi_set_hint(const data_type* hint)
    {
        return m_hash_table.set_hint(hint);
    }

    void tmi_remove_node(const data_type* node)
    {
        iterator it = m_hash_table.make_iterator(node);
        m_hash_table.erase(it);
    }

    void tmi_insert_node_direct(data_type* node)
    {
        m_hash_table.insert_node_direct(node);
    }

    data_type* tmi_preinsert_node(const value_type& value, insert_hints_type& hints)
    {
        data_type* ret = nullptr;
        size_t buckets = m_hash_table.bucket_count();
        if (buckets) {
            ret = m_hash_table.preinsert_node(value, hints);
            if (ret) {
                return ret;
            }
        }
        if (size() + 1 >= static_cast<size_type>(std::ceil(static_cast<float>(buckets) * max_load_factor()))) {
            rehash_impl(buckets + 1);
            m_hash_table.preinsert_node(value, hints);
        }
        return nullptr;
    }

    data_type* tmi_preinsert_node_hint(const data_type*, const value_type& value, insert_hints_type& hints)
    {
        return tmi_preinsert_node(value, hints);
    }

    void tmi_create_premodify_cache(const data_type* node, premodify_cache& cache) const
    {
        m_hash_table.create_premodify_cache(node, cache);
    }

    bool tmi_erase_if_modified(data_type* node, const premodify_cache& cache)
    {
        return m_hash_table.erase_if_modified(node, cache);
    }

    void tmi_insert_node(data_type* node, const insert_hints_type& hints)
    {
        m_hash_table.insert_node(node, hints);
    }

    void tmi_clear()
    {
        m_hash_table.clear();
        m_buckets.clear();
        m_hash_table.rehash(m_buckets);
    }
};

template <typename T, typename IndexedNode, typename Parent, bool Unique, bool IsOnlyIndex, class Hash, class KeyEqual, class KeyFromValue, typename Allocator>
inline bool swap(tmi_hasher<T, IndexedNode, Parent, Unique, IsOnlyIndex, Hash, KeyEqual, KeyFromValue, Allocator>& x, tmi_hasher<T, IndexedNode, Parent, Unique, IsOnlyIndex, Hash, KeyEqual, KeyFromValue, Allocator>& y) noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

template <typename T, typename IndexedNode, typename Parent, bool Unique, bool IsOnlyIndex, class Hash, class KeyEqual, class KeyFromValue, typename Allocator>
inline bool operator==(const tmi_hasher<T, IndexedNode, Parent, Unique, IsOnlyIndex, Hash, KeyEqual, KeyFromValue, Allocator>& x, const tmi_hasher<T, IndexedNode, Parent, Unique, IsOnlyIndex, Hash, KeyEqual, KeyFromValue, Allocator>& y)
{
    if (x.size() != y.size()) {
        return false;
    }
    for (auto it = x.begin(); it != x.end();) {
        const auto& [lhs_eq1, lhs_eq2] = x.equal_range(*it);
        const auto& [rhs_eq1, rhs_eq2] = y.equal_range(*it);
        if (std::distance(lhs_eq1, lhs_eq2) != std::distance(rhs_eq1, rhs_eq2) || !std::is_permutation(lhs_eq1, lhs_eq2, rhs_eq1)) {
            return false;
        }
        it = lhs_eq2;
    }
    return true;
}

template <typename T, typename IndexedNode, typename Parent, bool Unique, bool IsOnlyIndex, class Hash, class KeyEqual, class KeyFromValue, typename Allocator, class Pred>
tmi_hasher<T, IndexedNode, Parent, Unique, IsOnlyIndex, Hash, KeyEqual, KeyFromValue, Allocator>::size_type erase_if(tmi_hasher<T, IndexedNode, Parent, Unique, IsOnlyIndex, Hash, KeyEqual, KeyFromValue, Allocator>& c, Pred pred)
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

} // namespace tmi

#endif // TMI_HASHER_H_
