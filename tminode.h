#ifndef TMINODE_H
#define TMINODE_H

#include "tmi_index.h"

#include <array>
#include <cstdint>
#include <limits>
#include <cstdlib>
#include <tuple>
#include <type_traits>
#include <utility>
#include <cassert>

namespace tmi {

template <typename T, typename Indices, int I>
class tmi_indexed_node;

template <typename T, typename Indices, int I>
class tmi_indexed_comparator_node;

template <typename T, typename Indices, int I>
class tmi_indexed_hash_node;

template <typename T, typename Indices>
class tminode
{
private:
    // m_val MUST BE first
    union {
        T m_val;
    };

    tminode* m_prev{nullptr};
    tminode* m_next{nullptr};
    struct rb {
        tminode* m_left{nullptr};
        tminode* m_right{nullptr};
        uintptr_t par_and_flg{};
    };
    struct hash_type {
        tminode* m_nexthash{nullptr};
        size_t m_hash{0};
    };

    template <int I>
    using comparator_indexed_node_type = tmi_indexed_comparator_node<T, Indices, I>;

    template <int I>
    using hash_indexed_node_type = tmi_indexed_hash_node<T, Indices, I>;

    using index_types = typename Indices::index_types;
    template <int I>
    struct base_index_type_helper
    {
        using index_type = std::tuple_element_t<I, index_types>;
        using data_type = std::conditional_t<std::is_base_of_v<detail::hashed_type, index_type>, hash_type, rb>;
        using indexed_node_type = std::conditional_t<std::is_base_of_v<detail::hashed_type, index_type>, hash_indexed_node_type<I>, comparator_indexed_node_type<I>>;
    };

    template <typename>
    struct base_index_helper;
    template <size_t... ints>
    struct base_index_helper<std::index_sequence<ints...>> {
        using data_types = std::tuple<typename base_index_type_helper<ints>::data_type ...>;
    };
    static constexpr size_t num_indices = std::tuple_size<index_types>();
    using data_types_tuple = typename base_index_helper<std::make_index_sequence<num_indices>>::data_types;

    data_types_tuple m_data;

public:
    template <typename, typename, int>
    friend class tmi_indexed_comparator_node;

    template <typename, typename, int>
    friend class tmi_indexed_hash_node;

    template <int I>
    using indexed_node_type = base_index_type_helper<I>::indexed_node_type;

    using value_type = T;

    ~tminode()
    {
    }

    template <typename... Args>
    tminode(Args&&... args) : m_val(std::forward<Args>(args)...)
    {
    }

    const T& value() const { return m_val; }
    T& value() { return m_val; }

    tminode* next() const { return m_next; }
    tminode* prev() const { return m_prev; }

    void link(tminode* prev)
    {
        if (prev) {
            prev->m_next = this;
        }
        m_prev = prev;
    }

    void unlink()
    {
        if (m_prev)
            m_prev->m_next = m_next;
        if (m_next)
            m_next->m_prev = m_prev;
        m_prev = nullptr;
        m_next = nullptr;
    }
};

template <typename T, typename Indices, int I>
class tmi_indexed_hash_node : private tminode<T, Indices>
{
    tmi_indexed_hash_node() = delete;
    using tminode<T, Indices>::m_data;
public:
    using typename tminode<T, Indices>::value_type;
    using tminode<T, Indices>::value;

    const tmi_indexed_hash_node* next_hash() const
    {
        return static_cast<const tmi_indexed_hash_node*>(std::get<I>(m_data).m_nexthash);
    }

    tmi_indexed_hash_node* next_hash()
    {
        return static_cast<tmi_indexed_hash_node*>(std::get<I>(m_data).m_nexthash);
    }

    size_t hash() const
    {
        return std::get<I>(m_data).m_hash;
    }

    void set_hash(size_t hash)
    {
        std::get<I>(m_data).m_hash = hash;
    }

    void set_next_hashptr(const tmi_indexed_hash_node* rhs)
    {
        std::get<I>(m_data).m_nexthash = const_cast<tmi_indexed_hash_node*>(rhs);
    }


    static consteval size_t value_offset()
    {
#if defined __has_builtin
#   if __has_builtin (__builtin_offsetof)
#           pragma GCC diagnostic push
#           pragma GCC diagnostic ignored "-Winvalid-offsetof"
        return __builtin_offsetof(tmi_indexed_hash_node, m_val);
#           pragma GCC diagnostic pop
#   endif
#endif
        return 0;
    }
};

template <typename T, typename Indices, int I>
class tmi_indexed_comparator_node : private tminode<T, Indices>
{
    tmi_indexed_comparator_node() = delete;
    using tminode<T, Indices>::m_data;
public:

    using typename tminode<T, Indices>::value_type;
    using tminode<T, Indices>::value;

    const tmi_indexed_comparator_node* parent() const
    {
        return reinterpret_cast<const tmi_indexed_comparator_node*>(std::get<I>(m_data).par_and_flg & ~1UL);
    }

    tmi_indexed_comparator_node* parent()
    {
        return reinterpret_cast<tmi_indexed_comparator_node*>(std::get<I>(m_data).par_and_flg & ~1UL);
    }

    void set_parent(const tmi_indexed_comparator_node *p)
    {
        auto& par_and_flg = std::get<I>(m_data).par_and_flg;
        par_and_flg = (par_and_flg & 1UL) | reinterpret_cast<uintptr_t>(p);
    }

    void clone(tmi_indexed_comparator_node* node)
    {
        std::get<I>(m_data) = std::get<I>(node->m_data);
    }

    bool rp() const
    {
        return std::get<I>(m_data).par_and_flg & 1UL;
    }

    void flip()
    {
        std::get<I>(m_data).par_and_flg ^= 1UL;
    }

    const tmi_indexed_comparator_node* left() const
    {
        return static_cast<const tmi_indexed_comparator_node*>(std::get<I>(m_data).m_left);
    }

    const tmi_indexed_comparator_node* right() const
    {
        return static_cast<const tmi_indexed_comparator_node*>(std::get<I>(m_data).m_right);
    }

    tmi_indexed_comparator_node* left()
    {
        return static_cast<tmi_indexed_comparator_node*>(std::get<I>(m_data).m_left);
    }

    tmi_indexed_comparator_node* right()
    {
        return static_cast<tmi_indexed_comparator_node*>(std::get<I>(m_data).m_right);
    }

    void set_right(const tmi_indexed_comparator_node* node)
    {
        std::get<I>(m_data).m_right = const_cast<tmi_indexed_comparator_node*>(node);
    }

    void set_left(const tmi_indexed_comparator_node* node)
    {
        std::get<I>(m_data).m_left = const_cast<tmi_indexed_comparator_node*>(node);
    }

    void reset()
    {
        std::get<I>(m_data) = {};
    }

    static consteval size_t value_offset()
    {
#if defined __has_builtin
#   if __has_builtin (__builtin_offsetof)
#           pragma GCC diagnostic push
#           pragma GCC diagnostic ignored "-Winvalid-offsetof"
        return __builtin_offsetof(tmi_indexed_comparator_node, m_val);
#           pragma GCC diagnostic pop
#   endif
#endif
        return 0;
    }
};


} // namespace tmi
#endif // TMINODE_H
