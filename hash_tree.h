// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASH_TREE_H_
#define HASH_TREE_H_

#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <iterator>
#include <limits>
#include <tuple>
#include <utility>
#include <span>

namespace tmi {

template <typename Node, typename Value, typename KeyFromValue, typename Hash, typename KeyEqual, typename Allocator, bool Unique>
class hash_tree
{
public:
    using node_type = Node;
    using key_from_value_type = KeyFromValue;
    using key_type = typename key_from_value_type::result_type;
    using hasher_type = Hash;
    using key_equal_type = KeyEqual;
    using value_type = Value;
    using allocator_type = Allocator;

    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = std::allocator_traits<allocator_type>::size_type;
    using difference_type = std::allocator_traits<allocator_type>::difference_type;
    using pointer = std::allocator_traits<allocator_type>::pointer;
    using const_pointer = std::allocator_traits<allocator_type>::const_pointer;

private:

    using hash_buckets = std::span<node_type*>;

    hash_buckets m_buckets{};
    [[no_unique_address]] key_from_value_type m_key_from_value;
    [[no_unique_address]] hasher_type m_hasher;
    [[no_unique_address]] key_equal_type m_pred;

public:
    static constexpr bool unique_keys() { return Unique; }
    static_assert(std::is_copy_constructible_v<key_equal_type>);
    static_assert(std::is_copy_constructible_v<hasher_type>);
    struct insert_hints {
        size_t m_hash{0};
        node_type* m_prev{nullptr};
    };

    struct premodify_cache {
        size_t m_index;
        const node_type* m_prev{nullptr};
    };

    static constexpr bool requires_premodify_cache() { return true; }

    hash_tree() = default;
    hash_tree(const hasher_type& hasher) : m_hasher(hasher) {}

    hash_tree(key_from_value_type key_from_value ,hasher_type hasher, key_equal_type key_equal) : m_key_from_value(key_from_value), m_hasher(hasher), m_pred(key_equal){}

    hash_tree(const hash_tree& rhs) : m_key_from_value{rhs.m_key_from_value}, m_hasher{rhs.m_hasher}, m_pred{rhs.m_pred} {}
    hash_tree& operator=(const hash_tree& rhs)
    {
        m_key_from_value = rhs.m_key_from_value;
        m_hasher = rhs.m_hasher;
        m_pred = rhs.m_pred;
        return *this;
    }
    hash_tree& operator=(hash_tree&& rhs)
    {
        m_key_from_value = rhs.m_key_from_value;
        m_hasher = rhs.m_hasher;
        m_pred = rhs.m_pred;
        return *this;
    }

    hash_tree(hash_tree&& rhs) noexcept : m_buckets{std::move(rhs.m_buckets)}, m_key_from_value{std::move(rhs.m_key_from_value)}, m_hasher{std::move(rhs.m_hasher)}, m_pred{std::move(rhs.m_pred)}
    {
        rhs.m_buckets = {};
    }

    void remove_node(const node_type* node)
    {
        if (!node) {
            return;
        }
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return;
        }
        const size_t index = hash_to_bucket(node->hash(), bucket_count);

        node_type*& bucket = m_buckets[index];
        node_type* cur_node = bucket;
        node_type* prev_node = cur_node;
        while (cur_node) {
            if (cur_node == node) {
                if (cur_node == prev_node) {
                    // head of list
                    bucket = cur_node->next_hash();
                } else {
                    prev_node->set_next_hashptr(cur_node->next_hash());
                }
                break;
            }
            prev_node = cur_node;
            cur_node = cur_node->next_hash();
        }
        verify_tree();
    }

    void insert_node_direct(node_type* node)
    {
        if (node->hash() == 0) {
            const size_t hash = m_hasher(m_key_from_value(node->value()));
            node->set_hash(hash);
        }
        const size_t index = hash_to_bucket(node->hash(), m_buckets.size());
        node_type*& bucket = m_buckets[index];

        node->set_next_hashptr(bucket);
        bucket = node;
    }

    template <typename CompatibleKey>
    node_type* preinsert_node(const CompatibleKey& val, insert_hints& hints) const
    {
        const auto& key = m_key_from_value(val);
        const size_t hash = m_hasher(key);

        const size_t index = hash_to_bucket(hash, m_buckets.size());
        node_type* node = m_buckets[index];

        hints.m_hash = hash;
        hints.m_prev = nullptr;
        while (node) {
            hints.m_prev = node;
            if (node->hash() == hash) {
                if (m_pred(m_key_from_value(node->value()), key)) {
                    if constexpr (unique_keys()) {
                        return node;
                    } else {
                        return nullptr;
                    }
                }
            }
            node = node->next_hash();
        }
        return nullptr;
    }

    void create_premodify_cache(const node_type* node, premodify_cache& cache) const
    {
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return;
        }
        const size_t index = hash_to_bucket(node->hash(), bucket_count);
        const node_type* cur_node = m_buckets[index];
        const node_type* prev_node = cur_node;
        while (cur_node) {
            if (cur_node == node) {
                if (cur_node == prev_node) {
                    cache.m_prev = nullptr;
                    cache.m_index = index;
                } else {
                    cache.m_prev = prev_node;
                    cache.m_index = 0;
                }
                break;
            }
            prev_node = cur_node;
            cur_node = cur_node->next_hash();
        }
    }

    bool erase_if_modified(node_type* node, const premodify_cache& cache)
    {
        if (m_hasher(m_key_from_value(node->value())) != node->hash()) {
            if (cache.m_prev) {
                const_cast<node_type*>(cache.m_prev)->set_next_hashptr(node->next_hash());
            } else {
                m_buckets[cache.m_index] = node->next_hash();
            }
            return true;
        }
        return false;
    }

    void insert_node(node_type* node, const insert_hints& hints)
    {
        node->set_hash(hints.m_hash);
        if(!hints.m_prev) {
            const size_t bucket_count = m_buckets.size();
            const size_t index = hash_to_bucket(node->hash(), bucket_count);
            node->set_next_hashptr(nullptr);
            m_buckets[index] = node;
        } else {
            node->set_next_hashptr(hints.m_prev->next_hash());
            hints.m_prev->set_next_hashptr(node);
        }
        verify_tree();
    }


    template <typename CompatibleKey>
    const node_type* find_hash(const CompatibleKey& hash_key) const
    {
        const size_t hash = m_hasher(hash_key);
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return nullptr;
        }
        auto* node = m_buckets[hash_to_bucket(hash, bucket_count)];
        while (node) {
            if (node->hash() == hash) {
                if (m_pred(m_key_from_value(node->value()), hash_key)) {
                    return node;
                }
            }
            node = node->next_hash();
        }
        return nullptr;
    }

    class iterator
    {
        const node_type* m_node{};
        const hash_tree* m_tree{nullptr};

        iterator(const node_type* node, const hash_tree* tree) : m_node(node), m_tree(tree) {}
        friend class hash_tree;
    public:

        using value_type = hash_tree::value_type;
        using pointer = hash_tree::const_pointer;
        using reference = hash_tree::const_reference;
        using difference_type = hash_tree::difference_type;
        using element_type = const value_type;
        using iterator_category = std::forward_iterator_tag;
        iterator() = default;
        reference operator*() const { return m_node->value(); }
        pointer operator->() const { return std::pointer_traits<pointer>::pointer_to(m_node->value()); }
        iterator& operator++()
        {
            const node_type* next = m_node->next_hash();
            if (!next) {
                const size_t bucket_count = m_tree->m_buckets.size();
                size_t bucket_start = hash_to_bucket(m_node->hash(), bucket_count);
                for (size_t bucket = bucket_start + 1; bucket < bucket_count; ++bucket) {
                    next = m_tree->m_buckets[bucket];
                    if (next) {
                        break;
                    }
                }
            }
            m_node = next;
            return *this;
        }
        iterator operator++(int)
        {
            iterator copy(m_node, m_tree);
            ++(*this);
            return copy;
        }
        bool operator==(iterator rhs) const { return m_node == rhs.m_node; }
        bool operator!=(iterator rhs) const { return m_node != rhs.m_node; }
    };
    using const_iterator = iterator;

    class local_iterator
    {
        const node_type* m_node{};
        local_iterator(const node_type* node) : m_node(node) {}
        friend class hash_tree;
    public:

        using value_type = hash_tree::value_type;
        using pointer = hash_tree::const_pointer;
        using reference = hash_tree::const_reference;
        using difference_type = hash_tree::difference_type;
        using element_type = const value_type;
        using local_iterator_category = std::forward_iterator_tag;
        local_iterator() = default;
        reference operator*() const { return m_node->value(); }
        pointer operator->() const { return std::pointer_traits<pointer>::pointer_to(m_node->value()); }
        local_iterator& operator++()
        {
            m_node = m_node->next_hash();
            return *this;
        }
        local_iterator operator++(int)
        {
            local_iterator copy(m_node);
            m_node = m_node->next_hash();
            return copy;
        }
        bool operator==(local_iterator rhs) const { return m_node == rhs.m_node; }
        bool operator!=(local_iterator rhs) const { return m_node != rhs.m_node; }

    };

    using const_local_iterator = local_iterator;

    iterator begin() noexcept
    {
        for (const auto& bucket : m_buckets) {
            if (bucket) {
                return make_iterator(bucket);
            }
        }
        return end();
    }

    const_iterator begin() const noexcept
    {
        for (const auto& bucket : m_buckets) {
            if (bucket) {
                return make_iterator(bucket);
            }
        }
        return end();
    }

    iterator end() noexcept
    {
        return make_iterator(nullptr);
    }

    const_iterator end() const noexcept
    {
        return make_iterator(nullptr);
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    local_iterator begin(size_type n)
    {
        return local_iterator{m_buckets[n]};
    }

    local_iterator end(size_type)
    {
        return local_iterator{nullptr};
    }

    const_local_iterator begin(size_type n) const
    {
        return const_local_iterator{m_buckets[n]};
    }

    const_local_iterator end(size_type) const
    {
        return const_local_iterator{nullptr};
    }

    const_local_iterator cbegin(size_type n) const
    {
        return begin(n);
    }

    const_local_iterator cend(size_type n) const
    {
        return end(n);
    }

    template<typename CompatibleKey>
    std::pair<iterator, iterator> equal_range( const CompatibleKey& key) const
    {
        const size_t hash = m_hasher(key);
        const size_t bucket_count = m_buckets.size();
        auto* node = m_buckets[hash_to_bucket(hash, bucket_count)];
        while (node) {
            if((node->hash() == hash) && m_pred(m_key_from_value(node->value()), key)) {
                break;
            }
            node = node->next_hash();
        }
        if (!node) return {end(), end()};
        iterator first = make_iterator(node);
        node_type* last = node;

        if constexpr (!unique_keys()) {
            while (node && (node->hash() == hash) && m_pred(m_key_from_value(node->value()), key)) {
                last = node;
                node = node->next_hash();
            }
        }
        return {first, ++make_iterator(last)};
    }

    template <typename CompatibleKey>
    iterator find(const CompatibleKey& key) const
    {
        const node_type* node = find_hash(key);
        return make_iterator(node);
    }

    iterator erase(iterator it)
    {
        node_type* node = const_cast<node_type*>(it.m_node);
        iterator next = it++;
        remove_node(node);
        return next;
    }

    template <typename CompatibleKey>
    size_t count(const CompatibleKey& key) const
    {
        size_t ret = 0;
        const size_t hash = m_hasher(key);
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return 0;
        }
        auto* node = m_buckets[hash_to_bucket(hash, bucket_count)];
        while (node) {
            if (node->hash() == hash) {
                if (m_pred(m_key_from_value(node->value()), key)) {
                    ret++;
                    if constexpr (unique_keys()) break;
                }
            }
            node = node->next_hash();
        }
        return ret;
    }

    void clear() noexcept
    {
        m_buckets = {};
    }

    void swap(hash_tree& rhs) noexcept(std::is_nothrow_swappable_v<hasher_type> && std::is_nothrow_swappable_v<key_equal_type> && std::is_nothrow_swappable_v<key_from_value_type>)
    {
        using std::swap;
        swap(m_key_from_value, rhs.m_key_from_value);
        swap(m_hasher, rhs.m_hasher);
        swap(m_pred, rhs.m_pred);
    }

    template <typename CompatibleKey>
    bool contains(const CompatibleKey& key) const
    {
        const size_t hash = m_hasher(key);
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return false;
        }
        auto* node = m_buckets[hash_to_bucket(hash, bucket_count)];
        while (node) {
            if (node->hash() == hash) {
                if (m_pred(m_key_from_value(node->value()), key)) {
                    return true;
                }
            }
            node = node->next_hash();
        }
        return false;
    }

    size_type bucket_size(size_type n) const
    {
        size_type ret = 0;
        const node_type* node = m_buckets[n];
        while(node) {
            ret++;
            node = node->next_hash();
        }
        return ret;
    }

    size_type bucket_count() const
    {
        return m_buckets.size();
    }

    template <typename CompatibleKey>
    size_type bucket( const CompatibleKey& key ) const
    {
        const size_t hash = m_hasher(key);
        const size_t bucket_count = m_buckets.size();
        return hash_to_bucket(hash, bucket_count);
    }

    void set_buckets(hash_buckets new_buckets)
    {
        m_buckets = new_buckets;
        verify_tree();
    }

    void rehash(hash_buckets new_buckets)
    {
        for(size_t i = 0; i < m_buckets.size(); i++) {
            node_type* cur_node = m_buckets[i];
            while (cur_node) {
                node_type* next_node = cur_node->next_hash();
                const size_t index = hash_to_bucket(cur_node->hash(), new_buckets.size());
                node_type*& new_bucket = new_buckets[index];
                cur_node->set_next_hashptr(new_bucket);
                new_bucket = cur_node;
                cur_node = next_node;
            }
        }
        set_buckets(new_buckets);
    }

    hasher_type hash_function() const
    {
        return m_hasher;
    }

    key_equal_type key_eq() const
    {
        return m_pred;
    }

protected:

    const node_type* node_from_iterator(const_iterator it) const
    {
        return it.m_node;
    }

    node_type* node_from_iterator(iterator it)
    {
        return const_cast<node_type*>(it.m_node);
    }

    const_iterator make_iterator(const node_type* node) const
    {
        return const_iterator(node, this);
    }

    static constexpr size_type hash_to_bucket(size_type hash, size_type bucket_count)
    {
        return hash % bucket_count;
    }

    static constexpr size_type increase_bucket_count(size_type from, size_type new_minimum)
    {
        size_type new_bucket_count = std::max(new_minimum, size_type{1});
        new_bucket_count = std::max(new_bucket_count, from);
        if (new_minimum > 1) {
            new_bucket_count = std::bit_ceil(new_bucket_count);
        }
        return new_bucket_count;
    }

private:

#ifdef TMI_VERIFY_CHECK
    bool verify_unique_key(const node_type* node, size_t index) const
    {
        const node_type* other = m_buckets[index];
        const key_type& key = m_key_from_value(node->value());
        while(other) {
            if (other != node && m_pred(m_key_from_value(other->value()), key)) {
                return false;
            }
            other = other->next_hash();
        }
        return true;
    }

    bool verify_hashes() const {
        size_t bucket_count = m_buckets.size();
        for (size_t index = 0; index < bucket_count; index++) {
            const node_type* node = m_buckets[index];
            while(node) {
                size_t hash = m_hasher(m_key_from_value(node->value()));
                if (hash != node->hash())
                {
                    return false;
                }
                if (index != hash_to_bucket(hash, bucket_count)) {
                    return false;
                }
                if constexpr (unique_keys()) {
                    if (!verify_unique_key(node, index)) {
                        return false;
                    }
                }
                node = node->next_hash();
            }
        }
        return true;
    }

    bool verify_tree() const {
        assert(verify_hashes());
        return true;
    }
#else
    static constexpr bool verify_tree() { return true; }
#endif

};

template <typename Node, typename Value, typename KeyFromValue, typename Hash, typename KeyEqual, typename Allocator, bool Unique>
void swap(hash_tree<Node, Value, KeyFromValue, Hash, KeyEqual, Allocator, Unique>& x, hash_tree<Node, Value, KeyFromValue, Hash, KeyEqual, Allocator, Unique>& y) noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

} // namespace tmi

#endif // HASH_TREE_H_
