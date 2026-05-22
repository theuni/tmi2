// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASH_TREE_H_
#define HASH_TREE_H_

#include <algorithm>
#include <bit>
#include <cassert>
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
    using base_type = node_type;
    using size_type = size_t;
    using key_from_value_type = KeyFromValue;
    using key_type = typename key_from_value_type::result_type;
    using hasher_type = Hash;
    using key_equal_type = KeyEqual;
    using ctor_args = std::tuple<size_type,key_from_value_type,hasher_type,key_equal_type>;
    using allocator_type = Allocator;
    using node_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<node_type>;
    using value_type = node_type::value_type;

//private:
    static constexpr bool unique_keys() { return Unique; }

    struct insert_hints {
        size_t m_hash{0};
        base_type** m_bucket{nullptr};
    };

    struct premodify_cache {
        base_type** m_bucket{nullptr};
        base_type* m_prev{nullptr};
    };

    class hash_buckets
    {
        using bucket_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<base_type*>;
        [[no_unique_address]] bucket_allocator_type m_alloc;
        base_type** m_buckets{nullptr};
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
                std::memset(m_buckets, 0, m_bucket_count);
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
        const base_type* const* begin() const
        {
            return &m_buckets[0];
        }
        const base_type* const* end() const
        {
            return &m_buckets[0] + m_bucket_count;
        }
        base_type** begin()
        {
            return &m_buckets[0];
        }
        base_type** end()
        {
            return &m_buckets[0] + m_bucket_count;
        }
        const base_type* const& at(size_t index) const
        {
            return m_buckets[index];
        }
        base_type*& at(size_t index)
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
            base_type* node = m_buckets[bucket];
            while(node) {
                node = node->next_hash();
                ret++;
            }
            return ret;
        }

        private:

        void do_rehash_copy(size_t new_bucket_count)
        {
            base_type** new_buckets = do_allocate(new_bucket_count);
            base_type** old_buckets = m_buckets;
            size_t old_capacity = m_capacity;
            for(size_t i = 0; i < m_bucket_count; i++) {
                base_type* cur_node = old_buckets[i];
                while (cur_node) {
                    base_type* next_node = cur_node->next_hash();
                    const size_t index = cur_node->hash() % new_bucket_count;
                    base_type*& new_bucket = new_buckets[index];
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
                base_type* cur_node = m_buckets[i];
                base_type* prev_node = nullptr;
                while (cur_node) {
                    base_type* next_node = cur_node->next_hash();
                    const size_t index = cur_node->hash() % new_bucket_count;
                    base_type*& new_bucket = m_buckets[index];
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
        base_type** do_allocate(size_t new_capacity)
        {
            if (!new_capacity)
            {
                return nullptr;
            }
            base_type** ret = std::allocator_traits<bucket_allocator_type>::allocate(m_alloc, new_capacity);
            std::memset(ret, 0, new_capacity * sizeof(*ret));
            return ret;
        }
        void do_deallocate(base_type** buckets, size_t capacity)
        {
            if (capacity) {
                std::allocator_traits<bucket_allocator_type>::deallocate(m_alloc, buckets, capacity);
            }
        }


    };

    static constexpr bool requires_premodify_cache() { return true; }

    static constexpr size_t first_hashes_resize = 2048;

    hash_buckets m_buckets;
    size_t m_size{0};
    [[no_unique_address]] key_from_value_type m_key_from_value;
    [[no_unique_address]] hasher_type m_hasher;
    [[no_unique_address]] key_equal_type m_pred;


    hash_tree(const allocator_type& alloc) : m_buckets(alloc) {}

    hash_tree(const allocator_type& alloc, size_t bucket_count, key_from_value_type key_from_value ,hasher_type hasher, key_equal_type key_equal) : m_buckets(alloc, bucket_count), m_key_from_value(key_from_value), m_hasher(hasher), m_pred(key_equal){}

    hash_tree(const hash_tree& rhs) : m_buckets(rhs.m_buckets.size(), nullptr), m_key_from_value(rhs.m_key_from_value), m_hasher(rhs.m_hasher), m_pred(rhs.m_pred){}
    hash_tree(hash_tree&& rhs) : m_buckets(std::move(rhs.m_buckets)), m_key_from_value(std::move(rhs.m_key_from_value)), m_hasher(std::move(rhs.m_hasher)), m_pred(std::move(rhs.m_pred))
    {
        rhs.m_size = 0;
    }

    static base_type* get_next_hash(base_type* base)
    {
        return base->next_hash();
    }

    static size_t get_hash(base_type* base)
    {
        return base->hash();
    }

    static void set_hash(base_type* base, size_t hash)
    {
        base->set_hash(hash);
    }

    static void set_next_hashptr(base_type* lhs, base_type* rhs)
    {
        lhs->set_next_hashptr(rhs);
    }

    void remove_node(const base_type* base)
    {
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return;
        }
        const size_t index = base->hash() % bucket_count;

        base_type*& bucket = m_buckets.at(index);
        base_type* cur_node = bucket;
        base_type* prev_node = cur_node;
        while (cur_node) {
            if (cur_node == base) {
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

    void insert_node_direct(base_type* base)
    {
        if (base->hash() == 0) {
            const size_t hash = m_hasher(m_key_from_value(base->value()));
            base->set_hash(hash);
        }
        const size_t index = base->hash() % m_buckets.size();
        base_type*& bucket = m_buckets.at(index);

        base->set_next_hashptr(bucket);
        bucket = base;
        m_size++;
    }

    /*
        First rehash if necessary, using first_hashes_resize as the initial
        size if empty. Then find calculate the bucket and insert there.
    */
    base_type* preinsert_node(const base_type* base, insert_hints& hints)
    {
        size_t bucket_count = m_buckets.size();
        const auto& key = m_key_from_value(base->value());
        const size_t hash = m_hasher(key);

        if (!bucket_count) {
            m_buckets.init(first_hashes_resize);
            bucket_count = first_hashes_resize;
        } else if (static_cast<double>(m_size) / static_cast<double>(bucket_count) >= 0.8) {
            bucket_count *= 2;
            m_buckets.rehash(bucket_count);
        }

        const size_t index = hash % m_buckets.size();
        base_type*& bucket = m_buckets.at(index);

        if constexpr (unique_keys()) {
            base_type* curr = bucket;
            base_type* prev = curr;
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
        hints.m_bucket = &bucket;
        hints.m_hash = hash;
        return nullptr;
    }

    void create_premodify_cache(const base_type* base, premodify_cache& cache)
    {
        const size_t bucket_count = m_buckets.size();
        if (!bucket_count) {
            return;
        }
        const size_t index = base->hash() % bucket_count;

        base_type*& bucket = m_buckets.at(index);
        base_type* cur_node = bucket;
        base_type* prev_node = cur_node;
        while (cur_node) {
            if (cur_node == base) {
                if (cur_node == prev_node) {
                    cache.m_prev = nullptr;
                    cache.m_bucket = &bucket;
                } else {
                    cache.m_prev = prev_node;
                    cache.m_bucket = nullptr;
                }
                break;
            }
            prev_node = cur_node;
            cur_node = cur_node->next_hash();
        }
    }

    bool erase_if_modified(const base_type* base, const premodify_cache& cache)
    {
        if (m_hasher(m_key_from_value(base->value())) != base->hash()) {
            if (cache.m_prev) {
                cache.m_prev->set_next_hashptr(base->next_hash());
            } else {
                *cache.m_bucket = base->next_hash();
            }
            m_size--;
            return true;
        }
        return false;
    }

    void insert_node(base_type* base, const insert_hints& hints)
    {
        base->set_hash(hints.m_hash);
        base->set_next_hashptr(*hints.m_bucket);
        *hints.m_bucket = base;
        m_size++;
    }


    const node_type* find_hash(const key_type& hash_key) const
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

public:

    class iterator
    {
        const base_type* m_node{};
        const hash_buckets* m_buckets{nullptr};

        iterator(const base_type* node, const hash_buckets* buckets) : m_node(node), m_buckets(buckets) {}
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
            const base_type* next = m_node->next_hash();
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
    }
;
    using const_iterator = iterator;

    iterator begin()
    {
        for (const auto& bucket : m_buckets) {
            if (bucket) {
                return make_iterator(bucket);
            }
        }
        return end();
    }

    const_iterator begin() const
    {
        for (const auto& bucket : m_buckets) {
            if (bucket) {
                return make_iterator(bucket);
            }
        }
        return end();
    }

    iterator end()
    {
        return make_iterator(nullptr);
    }

    const_iterator end() const
    {
        return make_iterator(nullptr);
    }

    template <typename CompatibleKey>
    iterator find(const CompatibleKey& key) const
    {
        const node_type* node = find_hash(key);
        return make_iterator(node);
    }

    iterator erase(iterator it)
    {
        base_type* node = const_cast<base_type*>(it.m_node);
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

    void clear()
    {
        m_size = 0;
        m_buckets.clear();
    }

    size_t size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return !m_size;
    }

//private:

    const base_type* node_from_iterator(const_iterator it) const
    {
        return it.m_node;
    }

    const_iterator make_iterator(const base_type* node) const
    {
        return const_iterator(node, &m_buckets);
    }
};

} // namespace tmi

#endif // HASH_TREE_H_
