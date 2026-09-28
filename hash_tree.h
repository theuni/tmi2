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

template <typename Node, typename Value, typename KeyFromValue, typename Hash, typename KeyEqual, typename Allocator, typename BucketList, bool Unique>
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
    using size_type = size_t;
    using difference_type =  ptrdiff_t;
    using pointer = std::allocator_traits<allocator_type>::pointer;
    using const_pointer = std::allocator_traits<allocator_type>::const_pointer;
    using bucket_list_type = BucketList;

private:
    using hash_buckets = std::span<node_type*>;
    hash_buckets m_buckets;
    node_type* m_begin{nullptr};

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

    const hash_buckets& get_buckets() const
    {
        return m_buckets;
    }


    hash_tree() noexcept(std::is_nothrow_default_constructible<hasher_type>::value &&
                         std::is_nothrow_default_constructible<key_from_value_type>::value &&
                         std::is_nothrow_default_constructible<key_equal_type>::value)
    {}

    hash_tree(key_from_value_type key_from_value ,hasher_type hasher, key_equal_type key_equal) : m_key_from_value(key_from_value), m_hasher(hasher), m_pred(key_equal){}
    hash_tree(hash_buckets buckets, key_from_value_type key_from_value = key_from_value_type{}, hasher_type hasher = hasher_type{}, key_equal_type key_equal = key_equal_type{}) : m_buckets{buckets}, m_key_from_value(key_from_value), m_hasher(hasher), m_pred(key_equal){}

    hash_tree(const hash_tree& rhs, hash_buckets buckets) : m_buckets{buckets}, m_key_from_value{rhs.m_key_from_value}, m_hasher{rhs.m_hasher}, m_pred{rhs.m_pred}
    {
    }

    hash_tree(hash_tree&& rhs, hash_buckets buckets) : m_buckets{buckets}, m_key_from_value{std::move(rhs.m_key_from_value)}, m_hasher{std::move(rhs.m_hasher)}, m_pred{std::move(rhs.m_pred)}
    {
        rhs.m_begin = nullptr;
        rhs.m_buckets = {};
    }

    hash_tree(const hash_tree& rhs) = delete;
    hash_tree& operator=(const hash_tree&) = delete;

    hash_tree& operator=(hash_tree&& rhs) noexcept(std::is_nothrow_move_assignable<key_equal_type>::value &&
                                                   std::is_nothrow_move_assignable<key_from_value_type>::value &&
                                                   std::is_nothrow_move_assignable<hasher_type>::value)
    {
        m_key_from_value = std::move(rhs.m_key_from_value);
        m_hasher = std::move(rhs.m_hasher);
        m_pred = std::move(rhs.m_pred);

        m_begin = rhs.m_begin;
        m_buckets = rhs.m_buckets;
        rhs.m_begin = nullptr;
        rhs.m_buckets = {};
        return *this;
    }

    hash_tree(hash_tree&& rhs) noexcept(std::is_nothrow_move_constructible<hasher_type>::value &&
                                        std::is_nothrow_move_constructible<key_from_value_type>::value &&
                                        std::is_nothrow_move_constructible<key_equal_type>::value)
    : m_buckets{rhs.m_buckets}, m_begin{rhs.m_begin}, m_key_from_value{std::move(rhs.m_key_from_value)}, m_hasher{std::move(rhs.m_hasher)}, m_pred{std::move(rhs.m_pred)}
    {
        rhs.m_begin = nullptr;
        rhs.m_buckets = {};
    }

    void remove_node(const node_type* node)
    {
        verify_tree();
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
                    node_type* prev = node_before_bucket(index);
                    if (prev) {
                        assert(prev->next_hash() == cur_node);
                        prev->set_next_hashptr(cur_node->next_hash());
                    }
                    if (cur_node == m_begin) {
                        m_begin = cur_node->next_hash();
                    }
                    bucket = bucket_next_node(cur_node, index, bucket_count);
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

        const size_t bucket_count = m_buckets.size();
        const size_t bucket = hash_to_bucket(hash, bucket_count);

        node_type* node = m_buckets[bucket];
        assert(bucket_count);
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
            node = bucket_next_node(node, bucket, bucket_count);
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
                if (node == m_begin) {
                    m_begin = node->next_hash();
                }
                m_buckets[cache.m_index] = bucket_next_node(node, cache.m_index, m_buckets.size());
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
            const size_t bucket = hash_to_bucket(node->hash(), bucket_count);
            m_buckets[bucket] = node;

            node_type* prev = node_before_bucket(bucket);
            if (prev) {
                node->set_next_hashptr(prev->next_hash());
                prev->set_next_hashptr(node);
            } else {
                node->set_next_hashptr(node_after_bucket(bucket));
                m_begin = node;
            }
        } else {
            node->set_next_hashptr(hints.m_prev->next_hash());
            hints.m_prev->set_next_hashptr(node);
        }
        verify_tree();
    }

    insert_hints set_hint(const node_type* hint)
    {
        return {0, const_cast<node_type*>(hint)};
    }

    template <typename CompatibleKey>
    const node_type* find_key(const CompatibleKey& hash_key) const
    {
        const size_t hash = m_hasher(hash_key);
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return nullptr;
        }
        size_type bucket = hash_to_bucket(hash, bucket_count);
        const auto* node = m_buckets[bucket];
        while (node) {
            if (node->hash() == hash) {
                if (m_pred(m_key_from_value(node->value()), hash_key)) {
                    return node;
                }
            }
            node = bucket_next_node(node, bucket, bucket_count);
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
            m_node = m_node->next_hash();
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
        size_type m_bucket;
        size_type m_bucket_count;
        local_iterator(node_type* node, size_type bucket, size_type bucket_count) : m_node(node), m_bucket(bucket), m_bucket_count(bucket_count) {}
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
            m_node = bucket_next_node(m_node, m_bucket, m_bucket_count);
            return *this;
        }
        local_iterator operator++(int)
        {
            local_iterator copy(m_node);
            ++(*this);
            return copy;
        }
        bool operator==(local_iterator rhs) const { return m_node == rhs.m_node; }
        bool operator!=(local_iterator rhs) const { return m_node != rhs.m_node; }

    };

    using const_local_iterator = local_iterator;

    iterator begin() noexcept
    {
        return make_iterator(m_begin);
    }

    const_iterator begin() const noexcept
    {
        return make_iterator(m_begin);
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
        return local_iterator{m_buckets[n], n, m_buckets.size()};
    }

    local_iterator end(size_type n)
    {
        return local_iterator{nullptr, n, m_buckets.size()};
    }

    const_local_iterator begin(size_type n) const
    {
        return const_local_iterator{m_buckets[n], n, m_buckets.size()};
    }

    const_local_iterator end(size_type n) const
    {
        return const_local_iterator{nullptr, n, m_buckets.size()};
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
        if (!bucket_count) return {end(), end()};
        const size_t bucket = hash_to_bucket(hash, bucket_count);
        const auto* node = m_buckets[bucket];
        while (node) {
            if((node->hash() == hash) && m_pred(m_key_from_value(node->value()), key)) {
                break;
            }
            node = bucket_next_node(node, bucket, bucket_count);
        }
        if (!node) return {end(), end()};
        iterator first = make_iterator(node);
        const node_type* last = node;

        if constexpr (!unique_keys()) {
            while (node && (node->hash() == hash) && m_pred(m_key_from_value(node->value()), key)) {
                last = node;
                node = bucket_next_node(node, bucket, bucket_count);
            }
        }
        return {first, ++make_iterator(last)};
    }

    template <typename CompatibleKey>
    iterator find(const CompatibleKey& key) const
    {
        const node_type* node = find_key(key);
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
        size_type bucket = hash_to_bucket(hash, bucket_count);
        const auto* node = m_buckets[bucket];
        while (node) {
            if (node->hash() == hash) {
                if (m_pred(m_key_from_value(node->value()), key)) {
                    ret++;
                    if constexpr (unique_keys()) break;
                }
            }
            node = bucket_next_node(node, bucket, bucket_count);
        }
        return ret;
    }

    void clear() noexcept
    {
        for(auto& bucket : m_buckets) {
            bucket = nullptr;
        }
        m_begin = nullptr;
    }

    void swap(hash_tree& rhs) noexcept(std::is_nothrow_swappable_v<hasher_type> && std::is_nothrow_swappable_v<key_equal_type> && std::is_nothrow_swappable_v<key_from_value_type>)
    {
        using std::swap;
        swap(m_key_from_value, rhs.m_key_from_value);
        swap(m_hasher, rhs.m_hasher);
        swap(m_pred, rhs.m_pred);
        swap(m_begin, rhs.m_begin);
        swap(m_buckets, rhs.m_buckets);
    }

    key_from_value_type key_extractor() const
    {
        return m_key_from_value;
    }

    template <typename CompatibleKey>
    bool contains(const CompatibleKey& key) const
    {
        const size_t hash = m_hasher(key);
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return false;
        }
        size_type bucket = hash_to_bucket(hash, bucket_count);
        auto* node = m_buckets[bucket];
        while (node) {
            if (node->hash() == hash) {
                if (m_pred(m_key_from_value(node->value()), key)) {
                    return true;
                }
            }
            node = bucket_next_node(node, bucket, bucket_count);
        }
        return false;
    }

    size_type bucket_size(size_type n) const
    {
        size_type ret = 0;
        const size_t bucket_count = m_buckets.size();
        const node_type* node = m_buckets[n];
        while(node) {
            ret++;
            node = bucket_next_node(node, n, bucket_count);
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
    }
/*
    hash_tree clone(hash_buckets new_buckets)
    {
        hash_tree ret(m_key_from_value, m_hasher, m_pred);
    }
*/
    void rehash(hash_buckets new_buckets)
    {
        //verify_tree();
        node_type* cur_node = m_begin;
        m_buckets = new_buckets;
        size_type new_bucket_count = new_buckets.size();
        while (cur_node) {
            node_type* next_node = cur_node->next_hash();
            const size_t index = hash_to_bucket(cur_node->hash(), new_bucket_count);
            node_type*& new_bucket = m_buckets[index];
            if(!new_bucket) {
                new_bucket = cur_node;
            } else {
                node_type* last_bucket_node = new_bucket;
                while (last_bucket_node->next_hash()) {
                    last_bucket_node = last_bucket_node->next_hash();
                }
                last_bucket_node->set_next_hashptr(cur_node);
            }
            cur_node->set_next_hashptr(nullptr);
            cur_node = next_node;
        }

        node_type* new_begin = nullptr;
        size_t bucket = 0;
        while (bucket < new_bucket_count) {
            node_type* lhs = m_buckets[bucket];
            bucket++;
            if (lhs) {
                if (!new_begin) {
                    new_begin = lhs;
                }
                node_type* rhs = nullptr;
                while (bucket < new_bucket_count) {
                    rhs = m_buckets[bucket];
                    if (rhs) {
                        while(lhs->next_hash()) {
                            lhs = lhs->next_hash();
                        }
                        lhs->set_next_hashptr(rhs);
                        break;
                    }
                    bucket++;
                }
            }
        }
        m_begin = new_begin;
        verify_tree();
    }

    void reset(hash_buckets new_buckets)
    {
        m_buckets = new_buckets;
        for(node_type* bucket : new_buckets) {
            if(bucket) {
                m_begin = bucket;
                break;
            }
        }
    }

    hasher_type hash_function() const
    {
        return m_hasher;
    }

    key_equal_type key_eq() const
    {
        return m_pred;
    }

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

    static constexpr const node_type* next_node(const node_type* node)
    {
        assert(node);
        return node->next_hash();
    }

    static constexpr node_type* next_node(node_type* node)
    {
        assert(node);
        return node->next_hash();
    }


    constexpr node_type* bucket_first_node(size_type bucket) const
    {
        return m_buckets[bucket];
    }

    static constexpr node_type* bucket_next_node(node_type* node, size_type bucket, size_type bucket_count)
    {
        assert(node);
        node_type* ret = next_node(node);
        if (ret && hash_to_bucket(ret->hash(), bucket_count) != bucket) {
            ret = nullptr;
        }
        return ret;
    }

    static constexpr const node_type* bucket_next_node(const node_type* node, size_type bucket, size_type bucket_count)
    {
        assert(node);
        const node_type* ret = next_node(node);
        if (ret && hash_to_bucket(ret->hash(), bucket_count) != bucket) {
            ret = nullptr;
        }
        return ret;
    }

    constexpr node_type* node_after_bucket(size_type after_bucket) const
    {
        size_type bucket_count = m_buckets.size();
        if (after_bucket + 1 >= bucket_count) {
            return nullptr;
        }
        size_type bucket = after_bucket + 1;
        node_type* node = nullptr;
        while(!node && bucket < bucket_count) {
            node = m_buckets[bucket];
            bucket++;
        }
        return node;
    }

    constexpr node_type* node_before_bucket(size_type before_bucket) const
    {
        if (!before_bucket) {
            return nullptr;
        }
        size_type bucket_count = m_buckets.size();
        size_type bucket = before_bucket;
        node_type* node = nullptr;
        while(bucket > 0 && !node) {
            bucket--;
            node =  bucket_first_node(bucket);
        }
        node_type* last = node;
        while(node) {
            last = node;
            node = bucket_next_node(node, bucket, bucket_count);
        }
        return last;
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
        bool found_first = false;
        for (size_t index = 0; index < bucket_count; index++) {
            const node_type* node = m_buckets[index];
            if (!found_first && node) {
                assert(node == m_begin);
                found_first = true;
            }
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
                node = bucket_next_node(node, index, bucket_count);
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

template <typename Node, typename Value, typename KeyFromValue, typename Hash, typename KeyEqual, typename Allocator, typename BucketList, bool Unique>
void swap(hash_tree<Node, Value, KeyFromValue, Hash, KeyEqual, Allocator, BucketList, Unique>& x, hash_tree<Node, Value, KeyFromValue, Hash, KeyEqual, Allocator, BucketList, Unique>& y) noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

} // namespace tmi

#endif // HASH_TREE_H_
