// Copyright (c) 2026 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMI_UNORDERED_SET_H_
#define TMI_UNORDERED_SET_H_

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

    set_data(){}
    ~set_data(){}
private:
    set_data* m_nexthash{nullptr};
    size_t m_hash{0};
    union {
        value_type m_value;
    };
};


template <class Hash, class = void>
inline const bool is_transparent_v = false;

template <class Hash>
inline const bool is_transparent_v<Hash, std::void_t<typename Hash::is_transparent> > = true;


} // namespace detail

template <class Key, bool Unique, class Hash, class KeyEqual, class Allocator>
class unordered_set_base : private hash_tree<detail::set_data<Key>, Key, identity<Key>, Hash, KeyEqual, Allocator, Unique>
{
    using hash_table_type = hash_tree<detail::set_data<Key>, Key, identity<Key>, Hash, KeyEqual, Allocator, Unique>;
    using data_type = detail::set_data<Key>;
    using node_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<data_type>;
    using bucket_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<data_type*>;
    using node_pointer = std::allocator_traits<node_allocator_type>::pointer;
    using insert_hints_type = hash_table_type::insert_hints;
    template <class, bool, class, class, class>
    friend class unordered_set_base;

    class buckets_allocator;
    class bucket_list
    {
        using bucket_pointer_type = std::allocator_traits<bucket_allocator_type>::pointer;
        using size_type = std::allocator_traits<bucket_allocator_type>::size_type;
        using value_type = std::allocator_traits<bucket_allocator_type>::value_type;

        friend class buckets_allocator;

        bucket_pointer_type m_ptr{nullptr};
        size_type m_size{0};

        bucket_list(bucket_pointer_type ptr, size_type size) noexcept : m_ptr{ptr}, m_size{size}
        {
        }
        public:

        bucket_list(const bucket_list&) noexcept = delete;
        bucket_list& operator=(const bucket_list& rhs) noexcept = delete;

        bucket_list& operator=(bucket_list&& rhs) noexcept
        {
            m_size = rhs.m_size;
            m_ptr = rhs.m_ptr;
            rhs.m_size = 0;
            rhs.m_ptr = nullptr;
            return *this;
        }
        bucket_list(bucket_list&& rhs) noexcept
        {
            m_size = rhs.m_size;
            m_ptr = rhs.m_ptr;
            rhs.m_size = 0;
            rhs.m_ptr = nullptr;
        }

        explicit operator bool() const
        {
            return m_ptr != nullptr;
        }

        value_type* begin() const
        {
            return std::to_address(m_ptr);
        }
        value_type* end() const
        {
            assert(m_size <= std::numeric_limits<difference_type>::max());
            return std::to_address(m_ptr + static_cast<difference_type>(m_size));
        }
        bucket_pointer_type data() const
        {
            return m_ptr;
        }
        size_type size() const
        {
            return m_size;
        }
    };

    class buckets_allocator
    {
        using allocator_type = Allocator;
        using bucket_pointer_type = std::allocator_traits<bucket_allocator_type>::pointer;
        using size_type = std::allocator_traits<bucket_allocator_type>::size_type;
    public:

        buckets_allocator(const allocator_type& alloc = {}) : m_alloc{alloc}
        {
        }

        buckets_allocator(const buckets_allocator& rhs) : m_alloc{std::allocator_traits<allocator_type>::select_on_container_copy_construction(rhs.m_alloc)}
        {
        }

        bool operator==(const buckets_allocator& rhs) const
        {
            return m_alloc == rhs.m_alloc;
        }

        bucket_list allocate(size_type size)
        {
            size = std::max(size, size_type{1});
            bucket_allocator_type alloc(m_alloc);
            bucket_pointer_type ptr = std::allocator_traits<bucket_allocator_type>::allocate(alloc, size);
            bucket_list buckets(ptr, size);
            std::uninitialized_value_construct(buckets.begin(), buckets.end());
            return buckets;
        }
        void destroy(bucket_list buckets)
        {
            bucket_allocator_type alloc(m_alloc);
            if (buckets) {
                std::destroy(buckets.begin(), buckets.end());
                std::allocator_traits<bucket_allocator_type>::deallocate(alloc, buckets.data(), buckets.size());
            }
        }

        allocator_type get_allocator() const noexcept
        {
            return m_alloc;
        }

        void swap(buckets_allocator& rhs)
        {
            if constexpr(std::allocator_traits<allocator_type>::propagate_on_container_swap::value)
            {
                using std::swap;
                swap(m_alloc, rhs.m_alloc);
            }
        }

        [[no_unique_address]] allocator_type m_alloc;
    };

public:
    using key_type = hash_table_type::key_type;
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
    using insert_return_type = std::conditional_t<Unique, tmi::detail::insert_return_type<iterator, node_type>, iterator>;
    using insert_result_type = std::conditional_t<Unique, std::pair<iterator, bool>, iterator>;

    static_assert(std::is_same_v<value_type, typename allocator_type::value_type>);

    unordered_set_base() : m_bucket_list{m_alloc.allocate(1)}
    {
        hash_table_type::set_buckets(m_bucket_list);
    }

    explicit unordered_set_base(size_type n, const hasher& hf = hasher(), const key_equal& eql = key_equal(), const allocator_type& a = allocator_type()) : hash_table_type{identity<Key>{}, hf, eql}, m_alloc{a}, m_bucket_list{m_alloc.allocate(n)}
    {
        hash_table_type::set_buckets(m_bucket_list);
    }

    template <class InputIterator>
    unordered_set_base(InputIterator f, InputIterator l, size_type n = 0, const hasher& hf = hasher(), const key_equal& eql = key_equal(), const allocator_type& a = allocator_type()) : unordered_set_base(n, hf, eql, a)
    {
        insert(f, l);
    }

    explicit unordered_set_base(const allocator_type& a) : m_alloc{a}, m_bucket_list{m_alloc.allocate(1)}
    {
        hash_table_type::set_buckets(m_bucket_list);
    }

    unordered_set_base(const unordered_set_base& u) :  hash_table_type{u}, m_max_load_factor{u.m_max_load_factor}, m_alloc{u.m_alloc}, m_bucket_list{m_alloc.allocate(u.bucket_count())}
    {
        hash_table_type::set_buckets(m_bucket_list);
        insert(u.begin(), u.end());
    }

    unordered_set_base(const unordered_set_base& u, const Allocator& a) : unordered_set_base(u.bucket_count(), u.hash_function(), u.key_eq(), a)
    {
        insert(u.begin(), u.end());
    }

    unordered_set_base(unordered_set_base&& u) : hash_table_type{std::move(u)}, m_size(u.m_size), m_max_load_factor{u.m_max_load_factor}, m_alloc{std::move(u.m_alloc)}, m_bucket_list{std::move(u.m_bucket_list)}
    {
        u.m_size = 0;
        hash_table_type::set_buckets(m_bucket_list);
    }

    unordered_set_base(unordered_set_base&& u, const Allocator& a) : hash_table_type{std::move(u)}, m_size(u.m_size), m_max_load_factor{u.m_max_load_factor}, m_alloc{a}, m_bucket_list{std::move(u.m_bucket_list)}
    {
        u.m_size = 0;
        hash_table_type::set_buckets(m_bucket_list);
    }

    unordered_set_base(std::initializer_list<value_type> il, size_type n = 0, const hasher& hf = hasher(), const key_equal& eql = key_equal(), const allocator_type& a = allocator_type()) : unordered_set_base(n, hf, eql, a)
    {
        insert(il.begin(), il.end());
    }

    unordered_set_base(size_type n, const allocator_type& a) : unordered_set_base(n, hasher(), key_equal(), a)
    {
    }

    unordered_set_base(size_type n, const hasher& hf, const allocator_type& a) : unordered_set_base(n, hf, key_equal(), a)
    {
    }

    template <class InputIterator>
    unordered_set_base(InputIterator f, InputIterator l, size_type n, const allocator_type& a) : unordered_set_base(f, l, n, hasher(), key_equal(), a)
    {
    }

    template <class InputIterator>
    unordered_set_base(InputIterator f, InputIterator l, size_type n, const hasher& hf,  const allocator_type& a) : unordered_set_base(f, l, n, hf, key_equal(), a)
    {
    }

    unordered_set_base(std::initializer_list<value_type> il, size_type n, const allocator_type& a) : unordered_set_base(il, n, hasher(), key_equal(), a)
    {
    }

    unordered_set_base(std::initializer_list<value_type> il, size_type n, const hasher& hf,  const allocator_type& a) : unordered_set_base(il, n, hf, key_equal(), a)
    {
    }

    ~unordered_set_base()
    {
        clear();
        m_alloc.destroy(std::move(m_bucket_list));
    }

    unordered_set_base& operator=(const unordered_set_base & s)
    {
        if (this == std::addressof(s))
            return *this;

        size_type old_bucket_count = bucket_count();
        size_type new_bucket_count = s.bucket_count();
        hash_table_type::operator=(s);
        clear();
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value) {
            if (m_alloc != s.m_alloc) {
                m_alloc.destroy(std::move(m_bucket_list));
                m_alloc = s.m_alloc;
                m_bucket_list = m_alloc.allocate(new_bucket_count);
                hash_table_type::set_buckets(m_bucket_list);
            } else {
                m_alloc = s.m_alloc;
                if (new_bucket_count > old_bucket_count) {
                    reset_buckets(new_bucket_count);
                }
            }
        } else {
            if (new_bucket_count > old_bucket_count) {
                reset_buckets(new_bucket_count);
            }
        }
        m_max_load_factor = s.max_load_factor();
        insert(s.begin(), s.end());
        return *this;
    }

    unordered_set_base & operator=(unordered_set_base && s) noexcept(std::allocator_traits<Allocator>::is_always_equal::value
                                        && std::is_nothrow_move_assignable_v<Hash> && std::is_nothrow_move_assignable_v<KeyEqual>)
    {
        if (this == std::addressof(s))
            return *this;

        size_type old_bucket_count = bucket_count();
        size_type new_bucket_count = s.bucket_count();
        clear();
        m_max_load_factor = s.max_load_factor();
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value) {
            m_alloc = s.m_alloc;
            hash_table_type::operator=(std::move(s));
            m_bucket_list = std::move(s.m_bucket_list);
            hash_table_type::set_buckets(m_bucket_list);
            m_size = s.m_size;
            s.set_buckets({});
        } else {
            if (m_alloc != s.m_alloc) {
                m_alloc.destroy(std::move(m_bucket_list));
                m_bucket_list = m_alloc.allocate(new_bucket_count);
                hash_table_type::set_buckets(m_bucket_list);
                hash_table_type::operator=(std::move(s));
                for(auto it = s.begin(); it != s.end(); ++it) {
                    emplace_impl(std::move(*it));
                }
                s.set_buckets({});
            } else {
                hash_table_type::operator=(std::move(s));
                m_bucket_list = std::move(s.m_bucket_list);
                hash_table_type::set_buckets(m_bucket_list);
                m_size = s.m_size;
                s.set_buckets({});
            }
        }
        return *this;
    }

    unordered_set_base & operator=(std::initializer_list<value_type> il)
    {
        clear();
        rehash_impl(il.size());
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
        if constexpr(Unique) return std::make_pair(hash_table_type::make_iterator(result.first), result.second);
        else return hash_table_type::make_iterator(result.first);
    }

    template <typename Arg>
    static constexpr bool is_value_arg()
    {
        return std::is_same_v<value_type, std::remove_cvref_t<Arg>>;
    }

    template <typename... Args, typename Arg>
    static constexpr bool is_value_arg()
    {
        return false;
    }

    static constexpr bool is_value_arg()
    {
        return false;
    }

    template <class... Args>
    std::pair<data_type*,bool> emplace_impl(Args&&... args)
    {
        insert_hints_type hints;
        if constexpr(sizeof...(Args) == 1) {
            if constexpr(is_value_arg<Args...>()) {
            data_type* conflict = preinsert_impl(args..., hints);
            if (conflict) {
                return {conflict, false};
            }
            data_type* node = construct_impl(std::forward<Args>(args)...);
            insert_impl(node, hints);
            return {node, true};
        }
        }
        data_type* node = construct_impl(std::forward<Args>(args)...);
        data_type* conflict = preinsert_impl(node->value(), hints);
        if(conflict) {
            destroy_impl(node);
            return {conflict, false};
        }
        insert_impl(node, hints);
        return {node, true};
    }


    template <class... Args>
    insert_result_type emplace(Args&&... args)
    {
        return make_insert_result(emplace_impl(std::forward<Args>(args)...));
    }

    template <class... Args>
    iterator emplace_hint(const_iterator, Args&&... args)
    {
        //TODO: hint optimization
        auto [node, inserted] = emplace_impl(std::forward<Args>(args)...);
        return hash_table_type::make_iterator(node);
    }

    insert_result_type insert(const value_type& v)
    {
        return make_insert_result(emplace_impl(v));
    }

    insert_result_type insert(value_type&& v)
    {
        return make_insert_result(emplace_impl(std::move(v)));
    }

    iterator insert(const_iterator, const value_type& v)
    {
        //TODO: hint optimization
        return hash_table_type::make_iterator(emplace_impl(v).first);
    }

    iterator insert(const_iterator, value_type&& v)
    {
        //TODO: hint optimization
        return hash_table_type::make_iterator(emplace_impl(std::move(v)).first);
    }

    template <class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        for(auto it = first; it != last; ++it) {
            emplace_impl(*it);
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
        return {get_allocator(), node};
    }

    node_type extract(const key_type& x)
    {
        const_iterator position = find(x);
        return extract(position);
    }

    insert_return_type insert(node_type&& nh)
    {
        insert_hints_type hints;
        if constexpr(!Unique) {
            if(!nh) {
                return end();
            }
            data_type* node = nh.get();
            preinsert_impl(node->value(), hints);
            insert_impl(node, hints);
            nh.release();
            return hash_table_type::make_iterator(node);
        } else {
            if(!nh) {
                return {hash_table_type::end(), false, {}};
            }
            data_type* node = nh.get();
            data_type* conflict = preinsert_impl(node->value(), hints);
            if (conflict) {
                return {hash_table_type::make_iterator(conflict), false, std::move(nh)};
            }
            insert_impl(node, hints);
            nh.release();
            return {hash_table_type::make_iterator(node), true, {}};
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
        m_alloc.swap(s.m_alloc);
        hash_table_type::swap(s);
        std::swap(m_size, s.m_size);
        std::swap(m_max_load_factor, s.m_max_load_factor);
        std::swap(m_bucket_list, s.m_bucket_list);
        hash_table_type::set_buckets(m_bucket_list);
        s.set_buckets(s.m_bucket_list);
    }

    [[nodiscard]] allocator_type get_allocator() const noexcept
    {
        return m_alloc.get_allocator();
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
        rehash(static_cast<size_type>(std::ceil(static_cast<float>(count) / max_load_factor())));
    }

    using hash_table_type::bucket_size;
    using hash_table_type::bucket_count;

    size_type max_bucket_count() const noexcept
    {
        return max_size();
    }

    float load_factor() const
    {
        size_type bucket_count = hash_table_type::bucket_count();
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

    using hash_table_type::hash_function;
    using hash_table_type::key_eq;

private:

    size_type m_size{0};
    float m_max_load_factor{1.0f};
    buckets_allocator m_alloc;
    bucket_list m_bucket_list;


    template<class S2>
    void merge_impl(S2&& source)
    {
        for(auto it = source.begin(); it != source.end();)
        {
            data_type* node = source.node_from_iterator(it++);
            insert_hints_type hints;
            data_type* conflict = preinsert_impl(node->value(), hints);
            if (!conflict) {
                source.remove_node_impl(node);
                insert_impl(node, hints);
            }
        }
    }

    template <typename... Args>
    data_type* construct_impl(Args&&... args)
    {
        node_allocator_type node_alloc(get_allocator());
        node_pointer node = std::allocator_traits<node_allocator_type>::allocate(node_alloc, 1);
        std::construct_at(std::to_address(node));
        allocator_type alloc(node_alloc);
        std::allocator_traits<allocator_type>::construct(alloc, std::addressof(node->value()), std::forward<Args>(args)...);
        return std::to_address(node);
    }

    void remove_node_impl(data_type* node)
    {
        hash_table_type::remove_node(node);
        m_size--;
    }

    std::pair<data_type*,bool> insert_impl(data_type* node, const insert_hints_type& hints)
    {
        hash_table_type::insert_node(node, hints);
        m_size++;
        return std::make_pair(node, true);
    }

    data_type* preinsert_impl(const value_type& val, insert_hints_type& hints)
    {
        data_type* ret = nullptr;
        size_t buckets = bucket_count();
        assert(buckets);
        ret = hash_table_type::preinsert_node(val, hints);
        if (ret) {
            return ret;
        }
        if (size() + 1 >= static_cast<size_type>(std::ceil(static_cast<float>(buckets) * max_load_factor()))) {
            rehash_impl(buckets + 1);
            hash_table_type::preinsert_node(val, hints);
        }
        return nullptr;
    }

    void reset_buckets(size_t new_bucket_count)
    {
        m_alloc.destroy(std::move(m_bucket_list));
        m_bucket_list = m_alloc.allocate(new_bucket_count);
        hash_table_type::set_buckets(m_bucket_list);
    }

    void rehash_impl(size_type to)
    {
        size_type new_bucket_count = hash_table_type::increase_bucket_count(bucket_count(), to);
        if (new_bucket_count > bucket_count()) {
            auto new_buckets = m_alloc.allocate(new_bucket_count);
            hash_table_type::rehash(new_buckets);
            m_alloc.destroy(std::move(m_bucket_list));
            m_bucket_list = std::move(new_buckets);
        }
    }

    void destroy_impl(data_type* node)
    {
        node_allocator_type alloc(get_allocator());
        node_pointer ptr = std::pointer_traits<node_pointer>::pointer_to(*node);
        std::allocator_traits<node_allocator_type>::destroy(alloc, std::addressof(node->value()));
        std::destroy_at(std::to_address(ptr));
        std::allocator_traits<node_allocator_type>::deallocate(alloc, ptr, 1);
    }
};

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

template <class Key, bool Unique, class Hash, class KeyEqual, class Allocator>
inline bool operator==(const unordered_set_base<Key, Unique, Hash, KeyEqual, Allocator>& x, const unordered_set_base<Key, Unique, Hash, KeyEqual, Allocator>& y)
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

template <class Key, bool Unique, class Hash, class KeyEqual, class Allocator>
void swap(unordered_set_base<Key, Unique, Hash, KeyEqual, Allocator>& x, unordered_set_base<Key, Unique, Hash, KeyEqual, Allocator>& y) noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

template <class Key, bool Unique, class Hash, class KeyEqual, class Allocator, class Pred>
unordered_set_base<Key, Unique, Hash, KeyEqual, Allocator>::size_type erase_if(unordered_set_base<Key, Unique, Hash, KeyEqual, Allocator>& c,Pred pred )
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



} // namespace detail


} // namespace tmi

#ifdef CLANGD
#include <test/std_unordered_set.cpp>
#endif

#endif // TMI_UNORDERED_SET_H_
