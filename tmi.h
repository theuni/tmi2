// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMI_H_
#define TMI_H_

#include "tminode.h"
#include "tmi_comparator.h"
#include "tmi_hasher.h"
#include "tmi_index.h"
#include "tmi_nodehandle.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace tmi {
namespace detail {

template <typename T, typename Indices, typename Allocator, typename Parent, int I>
struct index_type_helper
{
    using index_types = typename Indices::index_types;
    using index_type = std::tuple_element_t<I, index_types>;
    using indexed_node_type = typename tminode_helper<T, Indices>::template indexed_node_type<I>;
    static constexpr size_t num_indices = std::tuple_size<index_types>();

    template <typename IndexType, typename BaseType = typename IndexType::base_type>
    struct index_base_type
    {
        static_assert(false, "must specialize base type");
    };

    template <typename IndexType>
    struct index_base_type<IndexType, detail::hashed_type>
    {
        using key_from_value_type = typename index_type::key_from_value_type;
        using hash = typename index_type::hasher_type;
        using key_equal = typename index_type::pred_type;
        static constexpr bool Unique = index_type::is_unique();

        using type = tmi_hasher<T, indexed_node_type, Parent, Unique, num_indices == 1, hash, key_equal, key_from_value_type, Allocator>;
    };

    template <typename IndexType>
    struct index_base_type<IndexType, detail::ordered_type>
    {
        using type = tmi_comparator<T, indexed_node_type, index_type::is_unique(), num_indices == 1, typename index_type::comparator, typename index_type::key_from_value_type, Parent, Allocator>;
    };

    using type = index_base_type<index_type>::type;

    using allocator_type = Allocator;
};

    template<typename T>
    concept HasValueCompare = requires {
        typename T::value_compare;
    };

    template<typename T>
    concept HasHasher = requires {
        typename T::hasher;
    };

    struct tag_hash_unique{};
    struct tag_hash_non_unique{};
    struct tag_ordered_unique{};
    struct tag_ordered_non_unique{};

} // namespace detail

template <typename T, typename Indices = indexed_by<ordered_unique<identity<T>>>, typename Allocator = std::allocator<T>>
class multi_index_container : public detail::index_type_helper<T, Indices, Allocator, multi_index_container<T, Indices, Allocator>, 0>::type
{
public:
    using parent_type = multi_index_container<T, Indices, Allocator>;
    using allocator_type = Allocator;
    using index_types = typename Indices::index_types;
    using tmi_node_data_tuples = tminode_helper<T, Indices>::data_types_tuple;
    using tmi_node_type = tminode<T, tmi_node_data_tuples>;
    using node_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<tmi_node_type>;
    using inherited_index = typename detail::index_type_helper<T, Indices, Allocator, multi_index_container<T, Indices, Allocator>, 0>::type;
    using node_pointer = std::allocator_traits<node_allocator_type>::pointer;
    using value_type = T;

    template <int I>
    using indexed_node_type = tminode_helper<T, Indices>::template indexed_node_type<I>;

    template <typename IndexedNode>
    using node_handle = detail::node_handle<allocator_type, IndexedNode>;

    static constexpr size_t num_indices = std::tuple_size<index_types>();

    template <int I>
    struct nth_index
    {
        using type = typename detail::index_type_helper<T, Indices, Allocator, multi_index_container<T, Indices, Allocator>, I>::type;
    };

    template <int I>
    using nth_index_t = typename nth_index<I>::type;

    template <typename Tag>
    struct index
    {
        template <size_t I = 0, size_t J = 0>
        static constexpr size_t get_index_for_tag()
        {
            using tags = typename std::tuple_element_t<I, index_types>::tags;
            constexpr size_t tag_count = std::tuple_size<tags>();
            using tag = typename std::tuple_element_t<J, tags>;

            if constexpr (std::is_same_v<tag, Tag>) return I;
            else if constexpr (J + 1 < tag_count) return get_index_for_tag<I, J + 1>();
            else if constexpr (I + 1 < num_indices) return get_index_for_tag<I + 1, 0>();
            else return num_indices;
        }
        static constexpr size_t value = get_index_for_tag();
        static_assert(value < num_indices, "tag not found");
        using type = nth_index_t<value>;
    };

    template <typename Tag>
    using index_t = typename index<Tag>::type;

    template <typename Tag>
    static constexpr size_t index_v = index<Tag>::value;


    template <typename Iterator>
    struct index_iterator
    {
        template <size_t I = 0>
        static constexpr size_t get_index_for_iterator()
        {
            using nth_iterator = typename nth_index_t<I>::iterator;
            if constexpr (std::is_same_v<Iterator, nth_iterator>) return I;
            else if constexpr (I + 1 < num_indices) return get_index_for_iterator<I + 1>();
            else return num_indices;
        }
        static constexpr size_t value = get_index_for_iterator();
        static_assert(value < num_indices, "iterator");
        using type = typename nth_index_t<value>::iterator;
    };

    template <typename Iterator>
    static constexpr size_t index_iterator_v = index_iterator<Iterator>::value;


    template <typename>
    struct index_tuple_helper;
    template <size_t First, size_t... ints>
    struct index_tuple_helper<std::index_sequence<First, ints...>> {
        using index_types = std::tuple<inherited_index&, nth_index_t<ints> ...>;
        using index_types_nonref = std::tuple<inherited_index, nth_index_t<ints> ...>;
        using hints_types =  std::tuple<typename nth_index_t<First>::insert_hints_type, typename nth_index_t<ints>::insert_hints_type ...>;
        using ctor_args_types =  std::tuple<typename nth_index_t<First>::ctor_args, typename nth_index_t<ints>::ctor_args ...>;
        using premodify_cache_types = std::tuple<typename nth_index_t<First>::premodify_cache, typename nth_index_t<ints>::premodify_cache ...>;

        static index_types make_index_types(parent_type& parent, const ctor_args_types& args, Allocator& alloc) {
            return std::make_tuple(std::ref(parent), std::make_from_tuple<nth_index_t<ints>>(std::tuple_cat(std::make_tuple(typename nth_index_t<ints>::ConstructionKey{}, std::ref(parent), std::ref(alloc)), std::get<ints>(args)))...);
        }
        static index_types make_index_types(parent_type& parent, const index_types& rhs, Allocator& alloc) {
            return std::make_tuple(std::ref(parent), nth_index_t<ints>(typename nth_index_t<ints>::ConstructionKey{}, parent, alloc, std::get<ints>(rhs)) ...);
        }
        static index_types make_index_types(parent_type& parent, index_types&& rhs, Allocator& alloc) {
            return std::make_tuple(std::ref(parent), nth_index_t<ints>(typename nth_index_t<ints>::ConstructionKey{}, parent, alloc, std::move(std::get<ints>(rhs))) ...);
        }
        static index_types make_index_types(parent_type& parent, Allocator& alloc) {
            return std::make_tuple(std::ref(parent), nth_index_t<ints>(typename nth_index_t<ints>::ConstructionKey{}, parent, alloc) ...);
        }
    };

    using indices_tuple = typename index_tuple_helper<std::make_index_sequence<num_indices>>::index_types;
    using indices_tuple_nonref = typename index_tuple_helper<std::make_index_sequence<num_indices>>::index_types_nonref;
    using indices_hints_tuple = typename index_tuple_helper<std::make_index_sequence<num_indices>>::hints_types;
    using indices_premodify_cache_tuple = typename index_tuple_helper<std::make_index_sequence<num_indices>>::premodify_cache_types;
    using ctor_args_list = typename index_tuple_helper<std::make_index_sequence<num_indices>>::ctor_args_types;

    template <typename, typename, typename, bool, bool, class, class, class, typename>
    friend class tmi_hasher;

    template <typename, typename, bool, bool, typename, typename, typename, typename>
    friend class tmi_comparator;

    template <typename, typename, typename>
    friend class multi_index_container;

private:
    tmi_node_type* m_begin{nullptr};
    tmi_node_type* m_end{nullptr};

    indices_tuple m_index_instances;

    [[no_unique_address]] allocator_type m_alloc;
    size_t m_size{0};

    template <int I = 0, class Callable, typename... Args>
    static void foreach_index(Callable&& func, std::nullptr_t, Args&&... args)
    {
        func.template operator()<I>(std::get<I>(args)...);
        if constexpr (I + 1 < num_indices) {
            foreach_index<I + 1>(std::forward<Callable>(func), nullptr, std::forward<Args>(args)...);
        }
    }

    template <int I = 0, class Callable, typename Node, typename... Args>
    static void foreach_index(Callable&& func, Node node, Args&&... args)
    {
        func.template operator()<I>(indexed_node_cast<indexed_node_type<I>>(node), std::get<I>(args)...);
        if constexpr (I + 1 < num_indices) {
            foreach_index<I + 1>(std::forward<Callable>(func), node, std::forward<Args>(args)...);
        }
    }

    template <int I = 0, class Callable, typename Node, typename... Args>
    static bool get_foreach_index(Callable&& func, Node node, Args&&... args)
    {
        if (!func.template operator()<I>(indexed_node_cast<indexed_node_type<I>>(node), std::get<I>(args)...)) {
            return false;
        }
        else if constexpr (I + 1 < num_indices) {
            return get_foreach_index<I + 1>(std::forward<Callable>(func), node, std::forward<Args>(args)...);
        } else {
            return true;
        }
    }

    template <int I = 0, class Callable, typename... Args>
    static bool get_foreach_index(Callable&& func, std::nullptr_t, Args&&... args)
    {
        if (!func.template operator()<I>(std::get<I>(args)...)) {
            return false;
        }
        else if constexpr (I + 1 < num_indices) {
            return get_foreach_index<I + 1>(std::forward<Callable>(func), nullptr, std::forward<Args>(args)...);
        } else {
            return true;
        }
    }

    tmi_node_type* do_preinsert_value(const T& value, indices_hints_tuple& hints)
    {
        tmi_node_type* conflict = nullptr;
        get_foreach_index([&conflict, &value]<int I>(nth_index_t<I>& instance, auto& indexed_hints) {
            auto* ret = instance.tmi_preinsert_node(value, indexed_hints);
            if (ret) {
                conflict = node_cast(ret);
                return false;
            }
            return true;
        }, nullptr, m_index_instances, hints);
        return conflict;
    }

    template <typename IndexedNode>
    tmi_node_type* do_preinsert_value_hint(const IndexedNode* supplied_hint, const T& value, indices_hints_tuple& hints)
    {
        tmi_node_type* conflict = nullptr;
        get_foreach_index([&conflict, &value, supplied_hint]<int I>(nth_index_t<I>& instance, auto& indexed_hints) {
            indexed_node_type<I>* ret = nullptr;
            if constexpr(I == IndexedNode::index) {
                ret = instance.tmi_preinsert_node_hint(supplied_hint, value, indexed_hints);
            } else {
                ret = instance.tmi_preinsert_node(value, indexed_hints);
            }
            if (ret) {
                conflict = node_cast(ret);
                return false;
            }
            return true;
        }, nullptr, m_index_instances, hints);
        return conflict;
    }

    void do_insert_node(tmi_node_type* node, const indices_hints_tuple& hints)
    {

        foreach_index([]<int I>(indexed_node_type<I>* indexed_node, nth_index_t<I>& instance, const auto& indexed_hints) TMI_CPP23_STATIC {
            instance.tmi_insert_node(indexed_node, indexed_hints);
        }, node, m_index_instances,  hints);

        node->link(m_end);

        if (m_begin == nullptr) {
            assert(m_end == nullptr);
            m_begin = m_end = node;
        } else {
            m_end = node;
        }

        m_size++;
    }

    template <typename IndexedNode>
    std::pair<IndexedNode*, bool> do_reinsert_node(IndexedNode* indexed)
    {
        indices_hints_tuple hints;
        tmi_node_type* conflict = do_preinsert_value(indexed->value(), hints);
        if (conflict) {
            return {indexed_node_cast<IndexedNode>(conflict), false};
        }
        do_insert_node(node_cast(indexed), hints);
        return {indexed, true};
    }

    template <typename IndexedNode, typename Index>
    void do_merge_index(Index& source)
    {
        for(auto it = source.begin(); it != source.end();)
        {
            auto* node = source.node_from_iterator(it++);
            indices_hints_tuple hints;
            tmi_node_type* conflict = do_preinsert_value(node->value(), hints);
            if (!conflict) {
                source.m_parent.do_unlink(node);
                do_insert_node(node_cast_other(node), hints);
            }
        }
    }

    void do_erase_cleanup(tmi_node_type* node)
    {
        if (node == m_end) {
            m_end = node->prev();
        }
        if (node == m_begin) {
            m_begin = node->next();
        }
        node->unlink();
        m_size--;
    }

    template <typename... Args>
    tmi_node_type* construct_impl(Args&&... args)
    {
        node_allocator_type alloc{m_alloc};
        node_pointer node = std::allocator_traits<node_allocator_type>::allocate(alloc, 1);
        std::construct_at(std::to_address(node));
        std::allocator_traits<allocator_type>::construct(m_alloc, std::addressof(node->value()), std::forward<Args>(args)...);
        return std::to_address(node);
    }

    void do_destroy_node(tmi_node_type* node)
    {
        node_allocator_type alloc{m_alloc};
        node_pointer ptr = std::pointer_traits<node_pointer>::pointer_to(*node);
        std::allocator_traits<node_allocator_type>::destroy(alloc, std::addressof(node->value()));
        std::destroy_at(std::to_address(ptr));
        std::allocator_traits<node_allocator_type>::deallocate(alloc, ptr, 1);
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

    template <typename IndexedNode, typename... Args>
    std::pair<IndexedNode*,bool> emplace_impl_hint(const IndexedNode* node_hint, Args&&... args)
    {
        indices_hints_tuple hints;
        if constexpr(sizeof...(Args) == 1) {
            if constexpr(is_value_arg<Args...>()) {
                if (tmi_node_type* conflict = do_preinsert_value_hint(node_hint, args..., hints)) {
                    return {indexed_node_cast<IndexedNode>(conflict), false};
                }
                tmi_node_type* node = construct_impl(std::forward<Args>(args)...);
                do_insert_node(node, hints);
                return {indexed_node_cast<IndexedNode>(node), true};
            }
        }
        tmi_node_type* node = construct_impl(std::forward<Args>(args)...);
        tmi_node_type* conflict = do_preinsert_value_hint(node_hint, node->value(), hints);
        if (conflict) {
            do_destroy_node(node);
            return {indexed_node_cast<IndexedNode>(conflict), false};
        }
        do_insert_node(node, hints);
        return {indexed_node_cast<IndexedNode>(node), true};
    }

    template <typename IndexedNode, typename... Args>
    std::pair<IndexedNode*,bool> emplace_impl(Args&&... args)
    {
        indices_hints_tuple hints;
        if constexpr(sizeof...(Args) == 1) {
            if constexpr(is_value_arg<Args...>()) {
                if (tmi_node_type* conflict = do_preinsert_value(args..., hints)) {
                    return {indexed_node_cast<IndexedNode>(conflict), false};
                }
                tmi_node_type* node = construct_impl(std::forward<Args>(args)...);
                do_insert_node(node, hints);
                return {indexed_node_cast<IndexedNode>(node), true};
            }
        }
        tmi_node_type* node = construct_impl(std::forward<Args>(args)...);
        tmi_node_type* conflict = do_preinsert_value(node->value(), hints);
        if (conflict) {
            do_destroy_node(node);
            return {indexed_node_cast<IndexedNode>(conflict), false};
        }
        do_insert_node(node, hints);
        return {indexed_node_cast<IndexedNode>(node), true};
    }

    template <typename IndexedNode, typename... Args>
    std::pair<IndexedNode*, bool> do_emplace(Args&&... args)
    {
        return emplace_impl<IndexedNode>(std::forward<Args>(args)...);
    }

    template <typename IndexedNode, typename... Args>
    std::pair<IndexedNode*, bool> do_emplace_hint(const IndexedNode* node_hint, Args&&... args)
    {
        return emplace_impl_hint<IndexedNode>(node_hint, std::forward<Args>(args)...);
    }

    template <typename IndexedNode>
    std::pair<IndexedNode*, bool> do_insert(const T& value)
    {
        return emplace_impl<IndexedNode>(value);
    }

    template <typename IndexedNode>
    std::pair<IndexedNode*, bool> do_insert(T&& value)
    {
        return emplace_impl<IndexedNode>(std::move(value));
    }

    template <typename IndexedNode>
    std::pair<IndexedNode*, bool> do_insert_hint(const IndexedNode* node_hint, const T& value)
    {
        return emplace_impl_hint<IndexedNode>(node_hint, value);
    }

    template <typename IndexedNode>
    std::pair<IndexedNode*, bool> do_insert_hint(const IndexedNode* node_hint, T&& value)
    {
        return emplace_impl_hint<IndexedNode>(node_hint, std::move(value));
    }

    template <typename IndexedNode>
    void do_unlink(IndexedNode* indexed)
    {
        tmi_node_type* node = node_cast(indexed);
        foreach_index([]<int I>(indexed_node_type<I>* indexed_node, nth_index_t<I>& instance) TMI_CPP23_STATIC {
            instance.tmi_remove_node(indexed_node);
        }, node, m_index_instances);
        do_erase_cleanup(node);
    }

    template <typename IndexedNode>
    void do_erase(IndexedNode* indexed)
    {
        do_unlink(indexed);
        tmi_node_type* node = node_cast(indexed);
        do_destroy_node(node);
    }

    template <typename IndexedNode, typename Callable>
    bool do_modify(IndexedNode* indexed, Callable&& func)
    {
        tmi_node_type* node = node_cast(indexed);
        indices_premodify_cache_tuple index_cache;

        foreach_index([]<int I>(const indexed_node_type<I>* indexed_node, nth_index_t<I>& instance, auto& cache) TMI_CPP23_STATIC {
            if constexpr (nth_index_t<I>::requires_premodify_cache()) {
                instance.tmi_create_premodify_cache(indexed_node, cache);
            }
        }, node, m_index_instances,  index_cache);


        func(node->value());

        std::array<bool, num_indices> indicies_to_modify{};

        foreach_index([]<int I>(indexed_node_type<I>* indexed_node, nth_index_t<I>& instance, auto& modify, const auto& cache) TMI_CPP23_STATIC {
            modify = instance.tmi_erase_if_modified(indexed_node, cache);
         }, node, m_index_instances,  indicies_to_modify, index_cache);


        indices_hints_tuple index_hints;

        bool insertable = get_foreach_index([]<int I>(const indexed_node_type<I>* indexed_node, nth_index_t<I>& instance, const auto& modify, auto& indexed_hints) TMI_CPP23_STATIC {
            if (modify) return instance.tmi_preinsert_node(indexed_node->value(), indexed_hints) == nullptr;
            return true;
        }, node, m_index_instances,  indicies_to_modify, index_hints);

        if (insertable) {
            foreach_index([]<int I>(indexed_node_type<I>* indexed_node, nth_index_t<I>& instance, const auto& modify, const auto& indexed_hints) TMI_CPP23_STATIC {
                if (modify) instance.tmi_insert_node(indexed_node, indexed_hints);
            }, node, m_index_instances,  indicies_to_modify, index_hints);
            return true;
        } else {
            foreach_index([]<int I>(indexed_node_type<I>* indexed_node, nth_index_t<I>& instance, const auto& modify) TMI_CPP23_STATIC {
                if (!modify) instance.tmi_remove_node(indexed_node);
            }, node, m_index_instances,  indicies_to_modify);
            do_erase_cleanup(node);
            do_destroy_node(node);
            return false;
        }
    }

    void do_clear()
    {
        foreach_index([]<int I>(nth_index_t<I>& instance) TMI_CPP23_STATIC {
            instance.tmi_clear();
         }, nullptr, m_index_instances);

        auto* node = m_begin;
        while (node) {
            auto* to_delete = node;
            node = node->next();
            do_destroy_node(to_delete);
        }
        m_begin = m_end = nullptr;
        m_size = 0;
    }

    template <typename IndexedNode>
    node_handle<IndexedNode> do_extract(IndexedNode* indexed)
    {
        if(!indexed) {
            return {};
        }
        tmi_node_type* node = node_cast(indexed);
        foreach_index([]<int I>(indexed_node_type<I>* indexed_node, nth_index_t<I>& instance) TMI_CPP23_STATIC {
             instance.tmi_remove_node(indexed_node);
         }, node, m_index_instances);
        do_erase_cleanup(node);
        return {m_alloc, indexed};
    }

    template <typename IndexedNode>
    static constexpr IndexedNode* indexed_node_cast(tmi_node_type* node)
    {
        static_assert(std::is_base_of_v<tmi_node_type, IndexedNode>);
        static_assert(sizeof(IndexedNode) - sizeof(tmi_node_type) == 0);
        return reinterpret_cast<IndexedNode*>(node);
    }

    template <typename IndexedNode>
    static constexpr const IndexedNode* indexed_node_cast(const tmi_node_type* node)
    {
        static_assert(std::is_base_of_v<tmi_node_type, IndexedNode>);
        static_assert(sizeof(IndexedNode) - sizeof(tmi_node_type) == 0);
        return reinterpret_cast<const IndexedNode*>(node);
    }

    template <typename IndexedNode>
    static constexpr tmi_node_type* node_cast(IndexedNode* node)
    {
        static_assert(std::is_base_of_v<tmi_node_type, IndexedNode>);
        static_assert(sizeof(IndexedNode) - sizeof(tmi_node_type) == 0);
        return reinterpret_cast<tmi_node_type*>(node);
    }

    template <typename IndexedNode>
    static constexpr const tmi_node_type* node_cast(const IndexedNode* node)
    {
        static_assert(std::is_base_of_v<tmi_node_type, IndexedNode>);
        static_assert(sizeof(IndexedNode) - sizeof(tmi_node_type) == 0);
        return reinterpret_cast<const tmi_node_type*>(node);
    }

    template <typename IndexedNode>
    static constexpr tmi_node_type* node_cast_other(IndexedNode* node)
    {
        static_assert(sizeof(IndexedNode) == sizeof(tmi_node_type));
        return reinterpret_cast<tmi_node_type*>(node);
    }

    template <typename IndexedNode>
    static constexpr IndexedNode* value_cast(T& elem)
    {
        static_assert(IndexedNode::value_offset() == 0);
        return indexed_node_cast<IndexedNode>(reinterpret_cast<tmi_node_type*>(&elem));
    }

    template <typename IndexedNode>
    static constexpr const IndexedNode* value_cast(const T& elem)
    {
        static_assert(IndexedNode::value_offset() == 0);
        return indexed_node_cast<const IndexedNode>(reinterpret_cast<const tmi_node_type*>(&elem));
    }



public:

    using inherited_construction_key = inherited_index::ConstructionKey;

/* Unordered */
    using key_type = std::conditional_t<num_indices == 1, typename inherited_index::key_type, std::void_t<>>;

    template <detail::HasHasher U = inherited_index>
    explicit multi_index_container(U::size_type size, const U::hasher& hf = typename U::hasher(), const U::key_equal& eql = typename U::key_equal(), const allocator_type& a = allocator_type()) requires(num_indices == 1)
        : inherited_index(inherited_construction_key{}, *this, m_alloc, typename U::key_from_value{}, hf, eql)
        , m_index_instances(*this)
        , m_alloc(a)
    {
        if (size) {
            inherited_index::rehash(size);
        }
    }

    template <class InputIterator, detail::HasHasher U = inherited_index>
    multi_index_container(InputIterator f, InputIterator l, U::size_type n = 0, const U::hasher& hf = typename U::hasher(), const U::key_equal& eql = typename U::key_equal(), const allocator_type& a = allocator_type()) : multi_index_container(n, hf, eql, a)
    {
        inherited_index::insert(f, l);
    }

    template <detail::HasHasher U = inherited_index>
    multi_index_container(std::initializer_list<value_type> il, U::size_type n = 0, const U::hasher& hf = typename U::hasher(), const U::key_equal& eql = typename U::key_equal(), const allocator_type& a = allocator_type()) : multi_index_container(n, hf, eql, a)
    {
        inherited_index::insert(il);
    }

    template <detail::HasHasher U = inherited_index>
    multi_index_container(U::size_type n, const allocator_type& a) : multi_index_container(n, typename U::hasher(), typename U::key_equal(), a)
    {
    }

    template <detail::HasHasher U = inherited_index>
    multi_index_container(U::size_type n, const U::hasher& hf, const allocator_type& a) : multi_index_container(n, hf, typename U::key_equal(), a)
    {
    }

    template <class InputIterator, detail::HasHasher U = inherited_index>
    multi_index_container(InputIterator f, InputIterator l, U::size_type n, const allocator_type& a) : multi_index_container(f, l, n, typename U::hasher(), typename U::key_equal(), a)
    {
    }

    template <class InputIterator, detail::HasHasher U = inherited_index>
    multi_index_container(InputIterator f, InputIterator l, U::size_type n, const U::hasher& hf,  const allocator_type& a) : multi_index_container(f, l, n, hf, typename U::key_equal(), a)
    {
    }

    template <detail::HasHasher U = inherited_index>
    multi_index_container(std::initializer_list<value_type> il, U::size_type n, const allocator_type& a) : multi_index_container(il, n, typename U::hasher(), typename U::key_equal(), a)
    {
    }

    template <detail::HasHasher U = inherited_index>
    multi_index_container(std::initializer_list<value_type> il, U::size_type n, const U::hasher& hf,  const allocator_type& a) : multi_index_container(il, n, hf, typename U::key_equal(), a)
    {
    }

    template<class Hash2, class KeyEqual2>
    using other_unordered_set = tmi::multi_index_container<T, tmi::indexed_by<tmi::hashed_unique<tmi::tag<detail::tag_hash_unique>, tmi::identity<T>, Hash2, KeyEqual2>>, Allocator>;

    template<class Hash2, class KeyEqual2>
    using other_unordered_multiset = tmi::multi_index_container<T, tmi::indexed_by<tmi::hashed_non_unique<tmi::tag<detail::tag_hash_non_unique>, tmi::identity<T>, Hash2, KeyEqual2>>, Allocator>;

/* ordered */
    template <detail::HasValueCompare U = inherited_index>
    explicit multi_index_container(const typename U::value_compare& comp) requires(num_indices == 1)
        : inherited_index(inherited_construction_key{}, *this, m_alloc, comp)
        , m_index_instances(*this)
    {}

    template <detail::HasValueCompare U = inherited_index>
    multi_index_container(const typename U::value_compare& comp, const allocator_type& a) requires(num_indices == 1)
        : inherited_index(inherited_construction_key{}, *this, m_alloc, comp)
        , m_index_instances(*this)
        , m_alloc(a)
    {}

    template <class InputIterator, detail::HasValueCompare U = inherited_index>
    multi_index_container(InputIterator first, InputIterator last, const typename U::value_compare& comp = typename U::value_compare()) requires(num_indices == 1) : multi_index_container(comp)
    {
        inherited_index::insert(first, last);
    }

    template <class InputIterator, detail::HasValueCompare U = inherited_index>
    multi_index_container(InputIterator first, InputIterator last, const typename U::value_compare& comp, const allocator_type& a) requires(num_indices == 1) : multi_index_container(comp, a)
    {
        inherited_index::insert(first, last);
    }

    template <class InputIterator, detail::HasValueCompare U = inherited_index>
    multi_index_container(InputIterator first, InputIterator last, const allocator_type& a) requires(num_indices == 1) : multi_index_container(a)
    {
        inherited_index::insert(first, last);
    }

    template <detail::HasValueCompare U = inherited_index>
    multi_index_container(std::initializer_list<value_type> il, const typename U::value_compare& comp = typename U::value_compare()) requires(num_indices == 1) : multi_index_container(comp)
    {
        inherited_index::insert(il.begin(), il.end());
    }

    template <detail::HasValueCompare U = inherited_index>
    multi_index_container(std::initializer_list<value_type> il, const typename U::value_compare& comp, const allocator_type& a) requires(num_indices == 1) : multi_index_container(comp, a)
    {
        inherited_index::insert(il.begin(), il.end());
    }

    template <detail::HasValueCompare U = inherited_index>
    multi_index_container(std::initializer_list<value_type> il, const allocator_type& a) requires(num_indices == 1) : multi_index_container(a)
    {
        inherited_index::insert(il.begin(), il.end());
    }




    multi_index_container(const allocator_type& alloc = {})
          : inherited_index(typename inherited_index::ConstructionKey{}, *this, m_alloc),
          m_index_instances(index_tuple_helper<std::make_index_sequence<num_indices>>::make_index_types(*this, m_alloc)),
          m_alloc(alloc)

    {
    }

    multi_index_container(const multi_index_container & s, const allocator_type& a)
        : inherited_index(inherited_construction_key{}, *this, m_alloc, s)
        , m_index_instances(index_tuple_helper<std::make_index_sequence<num_indices>>::make_index_types(*this, s.m_index_instances, m_alloc))
        , m_alloc(a)
    {
        inherited_index::insert(s.begin(), s.end());
    }

    multi_index_container(multi_index_container && rhs, const allocator_type& a)
        : inherited_index(inherited_construction_key{}, *this, m_alloc)
        , m_index_instances(index_tuple_helper<std::make_index_sequence<num_indices>>::make_index_types(*this, m_alloc))
        , m_alloc(a)
    {
        if (a != rhs.get_allocator()) {
            auto begin_it = rhs.begin();
            auto end_it = rhs.end();

            foreach_index([]<int I>(nth_index_t<I>& to, nth_index_t<I>& from) TMI_CPP23_STATIC {
                to.assign_release(std::move(from));
            }, nullptr, m_index_instances, rhs.m_index_instances);
            for(auto it = begin_it; it != end_it; ++it) {
                indexed_node_type<0>* node = inherited_index::node_from_iterator(it);
                do_emplace<indexed_node_type<0>>(std::move(node->value()));
            }
            rhs.do_clear();
        } else {
            foreach_index([]<int I>(nth_index_t<I>& to, nth_index_t<I>& from) TMI_CPP23_STATIC {
                to = std::move(from);
            }, nullptr, m_index_instances, rhs.m_index_instances);
            m_size = rhs.m_size;
            m_begin = rhs.m_begin;
            m_end = rhs.m_end;
            rhs.m_begin = nullptr;
            rhs.m_end = nullptr;
            rhs.m_size = 0;
        }
    }

    multi_index_container(const ctor_args_list& args, const allocator_type& alloc = {})
        : inherited_index(std::make_from_tuple<inherited_index>(std::tuple_cat(std::make_tuple(typename inherited_index::ConstructionKey{}, std::ref(*this), std::ref(m_alloc)), std::get<0>(args)))),
          m_index_instances(index_tuple_helper<std::make_index_sequence<num_indices>>::make_index_types(*this, args, m_alloc)),
          m_alloc(alloc)
    {
    }

    ~multi_index_container()
    {
        do_clear();
    }

    multi_index_container(const multi_index_container& rhs)
        : inherited_index(inherited_construction_key{}, *this, m_alloc, rhs),
          m_index_instances(index_tuple_helper<std::make_index_sequence<num_indices>>::make_index_types(*this, rhs.m_index_instances, m_alloc)),
          m_alloc(std::allocator_traits<allocator_type>::select_on_container_copy_construction(rhs.m_alloc))
    {
        if (!rhs.m_size) {
            return;
        }
        inherited_index::insert(rhs.begin(), rhs.end());
    }

    multi_index_container(multi_index_container&& rhs)
        : inherited_index(inherited_construction_key{}, *this, std::move(rhs)),
          m_index_instances(index_tuple_helper<std::make_index_sequence<num_indices>>::make_index_types(*this, std::move(rhs.m_index_instances), m_alloc)),
          m_alloc(std::move(rhs.m_alloc))
    {
        m_size = rhs.m_size;
        m_begin = rhs.m_begin;
        m_end = rhs.m_end;
        rhs.m_begin = nullptr;
        rhs.m_end = nullptr;
        rhs.m_size = 0;
    }

    multi_index_container& operator=(const multi_index_container& s)
    {
        if (this == std::addressof(s))
            return *this;

        do_clear();
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value) {
            if (m_alloc != s.m_alloc) {
                m_alloc = s.m_alloc;
            }
        }
        foreach_index([]<int I>(nth_index_t<I>& to, const nth_index_t<I>& from) TMI_CPP23_STATIC {
            to = from;
        }, nullptr, m_index_instances, s.m_index_instances);
        inherited_index::insert(s.begin(), s.end());
        return *this;
    }

    multi_index_container& operator=(multi_index_container&& s) noexcept(std::allocator_traits<Allocator>::is_always_equal::value
                                        && std::is_nothrow_move_assignable_v<indices_tuple_nonref>)
    {

        do_clear();
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value) {
            foreach_index([]<int I>(nth_index_t<I>& to, nth_index_t<I>& from) TMI_CPP23_STATIC {
                to = std::move(from);
            }, nullptr, m_index_instances, s.m_index_instances);
            m_alloc = s.m_alloc;
            m_size = s.m_size;
            m_begin = s.m_begin;
            m_end = s.m_end;
            s.m_begin = nullptr;
            s.m_end = nullptr;
            s.m_size = 0;
        } else {
            if (m_alloc != s.m_alloc) {
                auto begin_it = s.begin();
                auto end_it = s.end();
                foreach_index([]<int I>(nth_index_t<I>& to, nth_index_t<I>& from) TMI_CPP23_STATIC {
                    to.assign_release(std::move(from));
                }, nullptr, m_index_instances, s.m_index_instances);
                for(auto it = begin_it; it != end_it; ++it) {
                    indexed_node_type<0>* node = inherited_index::node_from_iterator(it);
                    do_emplace<indexed_node_type<0>>(std::move(node->value()));
                }
                s.do_clear();
            } else {
                foreach_index([]<int I>(nth_index_t<I>& to, nth_index_t<I>& from) TMI_CPP23_STATIC {
                    to = std::move(from);
                }, nullptr, m_index_instances, s.m_index_instances);
                m_size = s.m_size;
                m_begin = s.m_begin;
                m_end = s.m_end;
                s.m_begin = nullptr;
                s.m_end = nullptr;
                s.m_size = 0;
            }
        }

        return *this;
    }
    static constexpr size_t node_size()
    {
        return sizeof(tmi_node_type);
    }

    template <int I, typename IteratorType>
    class IteratorProject
    {
        static constexpr size_t from_iterator_index = index_iterator_v<IteratorType>;
        using from_const_iterator_type = nth_index_t<from_iterator_index>::const_iterator;
        using from_iterator_type = nth_index_t<from_iterator_index>::iterator;
        using to_const_iterator_type = typename nth_index_t<I>::const_iterator;
        using to_iterator_type = typename nth_index_t<I>::iterator;

        static_assert(std::is_same_v<IteratorType, from_const_iterator_type> || std::is_same_v<IteratorType, from_iterator_type>);

        public:
        using projected_iterator_type = std::conditional_t<std::is_same_v<IteratorType, from_const_iterator_type>, to_const_iterator_type, to_iterator_type>;

        static projected_iterator_type convert(const indices_tuple& index_instances, IteratorType it)
        {
            auto* indexed_node = std::get<from_iterator_index>(index_instances).node_from_iterator(it);
            auto* node = node_cast(indexed_node);
            return std::get<I>(index_instances).make_iterator(indexed_node_cast<indexed_node_type<I>>(node));
        }
    };

    template<size_t I, typename IteratorType>
    typename IteratorProject<I, IteratorType>::projected_iterator_type project(IteratorType it)
    {
        return IteratorProject<I, IteratorType>::convert(m_index_instances, it);
    }

    template<size_t I, typename IteratorType>
    typename IteratorProject<I, IteratorType>::projected_iterator_type project(IteratorType it) const
    {
        return IteratorProject<I, IteratorType>::convert(m_index_instances, it);
    }

    template<typename Tag, typename IteratorType>
    typename IteratorProject<index_v<Tag>, IteratorType>::projected_iterator_type project(IteratorType it)
    {
        return IteratorProject<index_v<Tag>, IteratorType>::convert(m_index_instances, it);
    }

    template<typename Tag,typename IteratorType>
    typename IteratorProject<index_v<Tag>, IteratorType>::projected_iterator_type project(IteratorType it) const
    {
        return IteratorProject<index_v<Tag>, IteratorType>::convert(m_index_instances, it);
    }

    template<size_t I>
    nth_index_t<I>& get() noexcept
    {
        return std::get<I>(m_index_instances);
    }

    template<int I>
    const nth_index_t<I>& get() const noexcept
    {
        return std::get<I>(m_index_instances);
    }

    template<typename Tag>
    index_t<Tag>& get() noexcept
    {
        return std::get<index_v<Tag>>(m_index_instances);
    }

    template<typename Tag>
    const index_t<Tag>& get() const noexcept
    {
        return std::get<index_v<Tag>>(m_index_instances);
    }

    allocator_type get_allocator() const noexcept
    {
        return allocator_type(m_alloc);
    }

    size_t size() const noexcept
    {
        return m_size;
    }

    bool empty() const noexcept
    {
        return !size();
    }

    void swap(multi_index_container & s) noexcept(std::allocator_traits<Allocator>::is_always_equal::value && std::is_nothrow_swappable_v<indices_tuple_nonref>)
    {
        if constexpr(std::allocator_traits<allocator_type>::propagate_on_container_swap::value)
        {
            using std::swap;
            swap(m_alloc, s.m_alloc);
        }
        std::swap(m_begin, s.m_begin);
        std::swap(m_end, s.m_end);
        std::swap(m_size, s.m_size);

        foreach_index([]<int I>(nth_index_t<I>& lhs, nth_index_t<I>& rhs) TMI_CPP23_STATIC {
            lhs.swap(rhs);
        }, nullptr, m_index_instances, s.m_index_instances);
    }
};

template <typename T, typename Indices, typename Allocator>
void swap(multi_index_container<T, Indices, Allocator>& x, multi_index_container<T, Indices, Allocator>& y) noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

template <typename T, typename Indices, typename Allocator, typename Predicate>
typename multi_index_container<T, Indices, Allocator>::size_type erase_if(multi_index_container<T, Indices, Allocator>& c, Predicate pred)
{
    return erase_if(c.template get<0>(), std::move(pred));
}

template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = std::allocator<Key>>
using unordered_set = tmi::multi_index_container<Key, tmi::indexed_by<tmi::hashed_unique<tmi::tag<detail::tag_hash_unique>, tmi::identity<Key>, Hash, KeyEqual>>, Allocator>;

template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = std::allocator<Key>>
using unordered_multiset = tmi::multi_index_container<Key, tmi::indexed_by<tmi::hashed_non_unique<tmi::tag<detail::tag_hash_non_unique>, tmi::identity<Key>, Hash, KeyEqual>>, Allocator>;

template <class Key, class Compare = std::less<Key>, class Allocator = std::allocator<Key>>
using set = tmi::multi_index_container<Key, tmi::indexed_by<tmi::ordered_unique<tmi::tag<detail::tag_ordered_unique>, tmi::identity<Key>, Compare>>, Allocator>;

template <class Key, class Compare = std::less<Key>, class Allocator = std::allocator<Key>>
using multiset = tmi::multi_index_container<Key, tmi::indexed_by<tmi::ordered_non_unique<tmi::tag<detail::tag_ordered_non_unique>, tmi::identity<Key>, Compare>>, Allocator>;

} // namespace tmi

#endif // TMI_H_
