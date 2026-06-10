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

namespace tmi {

template <typename Node, typename KeyFromValue, typename Hash, typename KeyEqual, bool Unique, typename Allocator>
class hash_tree
{
public:
    class iterator;

    using node_type = Node;
    using size_type = std::size_t;
    using key_from_value_type = KeyFromValue;
    using key_type = typename key_from_value_type::result_type;
    using hasher_type = Hash;
    using key_equal_type = KeyEqual;
    using ctor_args = std::tuple<size_type,key_from_value_type,hasher_type,key_equal_type>;
    using allocator_type = Allocator;
    using node_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<node_type>;
    using value_type = node_type::value_type;
    using difference_type = std::ptrdiff_t;

private:

    class hash_buckets
    {
        using bucket_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<node_type*>;
        [[no_unique_address]] bucket_allocator_type m_alloc;
        node_type** m_buckets{nullptr};
        size_t m_bucket_count{0};
        size_t m_capacity{0};

        struct allocation_result
        {
            typename std::allocator_traits<bucket_allocator_type>::pointer ptr;
            typename std::allocator_traits<bucket_allocator_type>::size_type count;
        };

        public:
        hash_buckets(const bucket_allocator_type& alloc) : m_alloc(alloc) {}

        hash_buckets(hash_buckets&& rhs) : m_alloc(std::move(rhs.m_alloc)), m_buckets(rhs.m_buckets), m_bucket_count(rhs.m_bucket_count), m_capacity(rhs.m_capacity)
        {
            rhs.m_buckets = nullptr;
            rhs.m_capacity = 0;
            rhs.m_bucket_count = 0;
        }
        hash_buckets(const hash_buckets& rhs) : m_alloc(std::allocator_traits<bucket_allocator_type>::select_on_container_copy_construction(rhs.m_alloc))
        {
            init(rhs.m_bucket_count);
        }
        hash_buckets(const bucket_allocator_type& alloc, size_t size) : m_alloc(alloc)
        {
            init(size);
        }
        ~hash_buckets()
        {
            do_deallocate(m_buckets, m_capacity);
        }
        void init(size_t requested_size)
        {
            assert(!m_bucket_count);
            size_t new_bucket_count = std::bit_ceil(requested_size);
            if (m_capacity < new_bucket_count) {
                do_deallocate(m_buckets, m_capacity);
                m_buckets = do_allocate(new_bucket_count);
                m_capacity = new_bucket_count;
            }
            m_bucket_count = new_bucket_count;
        }
        void rehash(size_t size)
        {
            size_t new_bucket_count = std::bit_ceil(size);
            if (m_capacity >= new_bucket_count && new_bucket_count > m_bucket_count) {
                do_rehash_inplace(new_bucket_count);
            } else {
                do_rehash_copy(new_bucket_count);
            }
        }
        void clear()
        {
            if (m_bucket_count) {
                std::memset(m_buckets, 0, m_bucket_count * sizeof(node_type*));
                m_bucket_count = 0;
            }
        }
        size_t size() const
        {
            return m_bucket_count;
        }
        bool empty() const
        {
            return m_bucket_count == 0;
        }
        const node_type* const* begin() const
        {
            return &m_buckets[0];
        }
        const node_type* const* end() const
        {
            return &m_buckets[0] + m_bucket_count;
        }
        node_type** begin()
        {
            return &m_buckets[0];
        }
        node_type** end()
        {
            return &m_buckets[0] + m_bucket_count;
        }
        const node_type* const& at(size_t index) const
        {
            return m_buckets[index];
        }
        node_type*& at(size_t index)
        {
            return m_buckets[index];
        }
        size_type bucket_count() const
        {
            return m_bucket_count;
        }
        size_type max_bucket_count() const
        {
            return std::numeric_limits<size_t>::max();
        }
        size_type bucket_size(size_type bucket) const
        {
            size_t ret = 0;
            node_type* node = m_buckets[bucket];
            while(node) {
                node = node->next_hash();
                ret++;
            }
            return ret;
        }

        [[nodiscard]] node_allocator_type get_allocator() const noexcept
        {
            return m_alloc;
        }

        private:

        void do_rehash_copy(size_t new_bucket_count)
        {
            node_type** new_buckets = do_allocate(new_bucket_count);
            node_type** old_buckets = m_buckets;
            size_t old_capacity = m_capacity;
            for(size_t i = 0; i < m_bucket_count; i++) {
                node_type* cur_node = old_buckets[i];
                while (cur_node) {
                    node_type* next_node = cur_node->next_hash();
                    const size_t index = cur_node->hash() % new_bucket_count;
                    node_type*& new_bucket = new_buckets[index];
                    cur_node->set_next_hashptr(new_bucket);
                    new_bucket = cur_node;
                    cur_node = next_node;
                }
            }
            m_buckets = new_buckets;
            m_bucket_count = new_bucket_count;
            m_capacity = new_bucket_count;
            do_deallocate(old_buckets, old_capacity);
        }

        void do_rehash_inplace(size_t new_bucket_count)
        {
            for(size_t i = 0; i < m_bucket_count; i++) {
                node_type* cur_node = m_buckets[i];
                node_type* prev_node = nullptr;
                while (cur_node) {
                    node_type* next_node = cur_node->next_hash();
                    const size_t index = cur_node->hash() % new_bucket_count;
                    node_type*& new_bucket = m_buckets[index];
                    if (index != i) {
                        if (prev_node == nullptr) {
                            m_buckets[i] = cur_node->next_hash();
                        } else {
                            prev_node->set_next_hashptr(cur_node->next_hash());
                        }
                        cur_node->set_next_hashptr(new_bucket);
                        new_bucket = cur_node;
                    }
                    prev_node = cur_node;
                    cur_node = next_node;
                }
            }
            m_bucket_count = new_bucket_count;
        }
        node_type** do_allocate(size_t new_capacity)
        {
            if (!new_capacity)
            {
                return nullptr;
            }
            node_type** ret = std::allocator_traits<bucket_allocator_type>::allocate(m_alloc, new_capacity);
            std::memset(ret, 0, new_capacity * sizeof(*ret));
            return ret;
        }
        void do_deallocate(node_type** buckets, size_t capacity)
        {
            if (capacity) {
                std::allocator_traits<bucket_allocator_type>::deallocate(m_alloc, buckets, capacity);
            }
        }


    };

    static constexpr size_t first_hashes_resize = 2048;

    hash_buckets m_buckets;
    size_t m_size{0};
    [[no_unique_address]] key_from_value_type m_key_from_value;
    [[no_unique_address]] hasher_type m_hasher;
    [[no_unique_address]] key_equal_type m_pred;
    float m_max_load_factor{0.8f};

public:
    static constexpr bool unique_keys() { return Unique; }

    struct insert_hints {
        size_t m_hash{0};
        size_t m_index{0};
    };

    struct premodify_cache {
        size_t m_index;
        const node_type* m_prev{nullptr};
    };

    static constexpr bool requires_premodify_cache() { return true; }

    hash_tree(const allocator_type& alloc = allocator_type{}) : m_buckets(alloc) {}
    hash_tree(size_t bucket_count, const allocator_type& alloc) : m_buckets(alloc, bucket_count) {}
    hash_tree(size_t bucket_count, const hasher_type& hasher, const allocator_type& alloc) : m_buckets(alloc, bucket_count), m_hasher(hasher) {}

    hash_tree(size_t bucket_count, key_from_value_type key_from_value ,hasher_type hasher, key_equal_type key_equal, const allocator_type& alloc) : m_buckets(alloc, bucket_count), m_key_from_value(key_from_value), m_hasher(hasher), m_pred(key_equal){}

    hash_tree(const hash_tree& rhs) = delete;
    hash_tree(hash_tree&& rhs) = default;

    void remove_node(const node_type* node)
    {
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return;
        }
        const size_t index = node->hash() % bucket_count;

        node_type*& bucket = m_buckets.at(index);
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
        m_size--;
    }

    void insert_node_direct(node_type* node)
    {
        if (node->hash() == 0) {
            const size_t hash = m_hasher(m_key_from_value(node->value()));
            node->set_hash(hash);
        }
        const size_t index = node->hash() % m_buckets.size();
        node_type*& bucket = m_buckets.at(index);

        node->set_next_hashptr(bucket);
        bucket = node;
        m_size++;
    }

    /*
        First rehash if necessary, using first_hashes_resize as the initial
        size if empty. Then find calculate the bucket and insert there.
    */
    //TODO: instead, add a "m_needs_rehash" to hints to keep this const
    node_type* preinsert_node(const node_type* node, insert_hints& hints)
    {
        size_t bucket_count = m_buckets.size();
        const auto& key = m_key_from_value(node->value());
        const size_t hash = m_hasher(key);

        if (!bucket_count) {
            m_buckets.init(first_hashes_resize);
            bucket_count = first_hashes_resize;
        } else if (static_cast<double>(m_size) / static_cast<double>(bucket_count) >= m_max_load_factor) {
            bucket_count *= 2;
            m_buckets.rehash(bucket_count);
        }

        const size_t index = hash % m_buckets.size();
        node_type*& bucket = m_buckets.at(index);

        if constexpr (unique_keys()) {
            node_type* curr = bucket;
            node_type* prev = curr;
            while (curr) {
                if (curr->hash() == hash) {
                    if (m_pred(m_key_from_value(curr->value()), key)) {
                        return curr;
                    }
                }
                prev = curr;
                curr = curr->next_hash();
            }
        }
        hints.m_index = index;
        hints.m_hash = hash;
        return nullptr;
    }

    void create_premodify_cache(const node_type* node, premodify_cache& cache) const
    {
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return;
        }
        const size_t index = node->hash() % bucket_count;
        const node_type* cur_node = m_buckets.at(index);
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
                m_buckets.at(cache.m_index) = node->next_hash();
            }
            m_size--;
            return true;
        }
        return false;
    }

    void insert_node(node_type* node, const insert_hints& hints)
    {
        node->set_hash(hints.m_hash);
        node_type*& bucket = m_buckets.at(hints.m_index);
        node->set_next_hashptr(bucket);
        bucket = node;
        m_size++;
    }


    template <typename CompatibleKey>
    const node_type* find_hash(const CompatibleKey& hash_key) const
    {
        const size_t hash = m_hasher(hash_key);
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return nullptr;
        }
        auto* node = m_buckets.at(hash % bucket_count);
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

    void do_clear()
    {
        m_buckets.clear();
    }

    class iterator
    {
        const node_type* m_node{};
        const hash_buckets* m_buckets{nullptr};

        iterator(const node_type* node, const hash_buckets* buckets) : m_node(node), m_buckets(buckets) {}
        friend class hash_tree;
    public:

        typedef const hash_tree::value_type value_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        using difference_type = std::ptrdiff_t;
        using element_type = const value_type;
        using iterator_category = std::forward_iterator_tag;
        iterator() = default;
        reference operator*() const { return m_node->value(); }
        pointer operator->() const { return &m_node->value(); }
        iterator& operator++()
        {
            const node_type* next = m_node->next_hash();
            if (!next) {
                const size_t bucket_size = m_buckets->size();
                size_t bucket = m_node->hash() % bucket_size;
                bucket++;
                for (; bucket < bucket_size; ++bucket) {
                    next = m_buckets->at(bucket);
                    if (next) {
                        break;
                    }
                }
            }
            if (next == nullptr) {
                m_node = nullptr;
            } else {
                m_node = next;
            }
            return *this;
        }
        iterator operator++(int)
        {
            iterator copy(m_node, m_buckets);
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

        typedef const hash_tree::value_type value_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        using difference_type = std::ptrdiff_t;
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
        return local_iterator{m_buckets.at(n)};
    }

    local_iterator end(size_type)
    {
        return local_iterator{nullptr};
    }

    const_local_iterator begin(size_type n) const
    {
        return const_local_iterator{m_buckets.at(n)};
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
    iterator lower_bound(const CompatibleKey& key) const
    {
        //TODO: Fix insert to group equal values
        return end();
    }

    template<typename CompatibleKey>
    iterator upper_bound(const CompatibleKey& key) const
    {
        //TODO: Fix insert to group equal values
        return end();
    }

    template<typename CompatibleKey>
    std::pair<iterator, iterator> equal_range( const CompatibleKey& key) const
    {
        return { lower_bound(key), upper_bound(key) };
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
        auto* node = m_buckets.at(hash % bucket_count);
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
        m_size = 0;
        m_buckets.clear();
    }

    size_t size() const noexcept
    {
        return m_size;
    }

    bool empty() const noexcept
    {
        return !size();
    }

    void swap(hash_tree& rhs)
    {
        // TODO
    }

    template <typename CompatibleKey>
    bool contains(const CompatibleKey& key) const
    {
        const size_t hash = m_hasher(key);
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return false;
        }
        auto* node = m_buckets.at(hash % bucket_count);
        while (node) {
            if (node->hash() == hash) {
                if (m_pred(m_key_from_value(node->value()), key)) {
                    return true;
                }
            }
        }
        return false;
    }

    size_type bucket_size(size_type n) const
    {
        return std::distance(begin(n), end(n));
    }

    size_type bucket_count() const
    {
        return m_buckets.size();
    }

    float load_factor() const
    {
        return size() / bucket_count();
    }

    float max_load_factor() const
    {
        return m_max_load_factor;
    }

    void max_load_factor(float n)
    {
        m_max_load_factor = std::max(n, m_max_load_factor);
    }

    void rehash(size_type buckets) {
        m_buckets.rehash(buckets);
    }

    void reserve(size_type count)
    {
        rehash(std::ceil(count / max_load_factor()));
    }

    hasher_type hash_function() const
    {
        return m_hasher;
    }

    key_equal_type key_eq() const
    {
        return m_pred;
    }

    [[nodiscard]] allocator_type get_allocator() const noexcept
    {
        return m_buckets.get_allocator();
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
        return const_iterator(node, &m_buckets);
    }
};

} // namespace tmi

#endif // HASH_TREE_H_
