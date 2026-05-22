#ifndef TMINODE_BASE_H
#define TMINODE_BASE_H

#include "tmi_index.h"

#include <array>
#include <cstdint>
#include <limits>
#include <cstdlib>
#include <tuple>
#include <type_traits>
#include <utility>

namespace tmi {

template <typename T, typename Indices>
class tminode;

template <typename T, typename Indices>
class tminode_base {
    struct rb {
        tminode_base* m_left{nullptr};
        tminode_base* m_right{nullptr};
        uintptr_t par_and_flg{};
    };
    struct hash {
        tminode_base* m_nexthash{nullptr};
        size_t m_hash{0};
    };

    /* Pointer back to self. This is a hack which enables the tminode_base
       structure to be instantiated as required by the red-black-tree
       implementation. See note towards the top of manysortfind.h for
       more tminode_base.

       libc++ uses inheritance to create a base class which contains only
       pointer/color tminode_base. That doesn't work here as we require T to be the
       first member to enable casting T* to node*. See note in
       manysortfind::iterator_to.

       Ideally this will be removed once the libc++ red-black-tree
       functions have been replaced.

    */
    tminode<T, Indices>* m_node{nullptr};

    using index_types = typename Indices::index_types;
    template <int I>
    struct base_index_type_helper
    {
        using index_type = std::tuple_element_t<I, index_types>;
        using data_type = std::conditional_t<std::is_base_of_v<detail::hashed_type, index_type>, hash, rb>;
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
    friend class tminode<T, Indices>;

    template <int I>
    tminode_base* parent() const
    {
        return reinterpret_cast<tminode_base*>(std::get<I>(m_data).par_and_flg & ~1UL);
    }
    template <int I>
    void set_parent(tminode_base *p)
    {
        auto& par_and_flg = std::get<I>(m_data).par_and_flg;
        par_and_flg = (par_and_flg & 1) | reinterpret_cast<uintptr_t>(p);
    }

    template <int I>
    uintptr_t flags() const
    {
        return std::get<I>(m_data).par_and_flg & 1;
    }

    template <int I>
    bool rp() const
    {
        return std::get<I>(m_data).par_and_flg & 1;
    }
    template <int I>
    void flip()
    {
        std::get<I>(m_data).par_and_flg ^= 1;
    }
    template <int I>
    void clr_flags()
    {
        std::get<I>(m_data).par_and_flg &= ~1UL;
    }

    template <int I>
    tminode_base* left() const
    {
        return std::get<I>(m_data).m_left;
    }

    template <int I>
    tminode_base* right() const
    {
        return std::get<I>(m_data).m_right;
    }

    template <int I>
    void set_right(tminode_base* base)
    {
        std::get<I>(m_data).m_right = base;
    }

    template <int I>
    void set_left(tminode_base* base)
    {
        std::get<I>(m_data).m_left = base;
    }

    template <int I>
    void set_ch(int d, tminode_base* rhs)
    {
        tminode_base*& child = d ? std::get<I>(m_data).m_right : std::get<I>(m_data).m_left;
        child = rhs;
    }

    template <int I>
    tminode_base* ch(int d)
    {
        return d ? std::get<I>(m_data).m_right : std::get<I>(m_data).m_left;
    }

    template <int I>
    const tminode_base* ch(int d) const
    {
        return d ? std::get<I>(m_data).m_right : std::get<I>(m_data).m_left;
    }

    template <int I>
    uintptr_t par_and_flg() const
    {
        return std::get<I>(m_data).par_and_flg;
    }

    template <int I>
    void set_par_and_flg(uintptr_t rhs)
    {
        std::get<I>(m_data).par_and_flg = rhs;
    }

    tminode<T, Indices>* node() const
    {
        return m_node;
    }

    template <int I>
    tminode_base* next_hash() const
    {
        return std::get<I>(m_data).m_nexthash;
    }

    template <int I>
    size_t hash() const
    {
        return std::get<I>(m_data).m_hash;
    }

    template <int I>
    void set_hash(size_t hash)
    {
        std::get<I>(m_data).m_hash = hash;
    }

    template <int I>
    void set_next_hashptr(tminode_base* rhs)
    {
        std::get<I>(m_data).m_nexthash = rhs;
    }
};

} // namespace tmi
#endif // TMINODE_BASE_H
