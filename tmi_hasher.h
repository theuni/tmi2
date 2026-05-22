// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMI_HASHER_H_
#define TMI_HASHER_H_

#include "tmi_nodehandle.h"
#include "hash_tree.h"

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

template <typename IndexedNode, typename Hasher, typename Parent, typename Allocator>
class tmi_hasher
{
public:
    using node_type = IndexedNode;
    using size_type = size_t;
    using key_from_value = typename Hasher::key_from_value_type;
    using key_type = typename key_from_value::result_type;
    using hasher = typename Hasher::hasher_type;
    using key_equal = typename Hasher::pred_type;
    using tree_type = hash_tree<node_type, key_from_value, hasher, key_equal, Hasher::is_hashed_unique(), Allocator>;
    using ctor_args = std::tuple<size_type,key_from_value,hasher,key_equal>;
    using allocator_type = Allocator;
    using node_handle = detail::node_handle<Allocator, node_type>;
    using iterator = tree_type::iterator;
    using const_iterator = tree_type::const_iterator;
    using insert_return_type = detail::insert_return_type<iterator, node_handle>;
    using value_type = node_type::value_type;
    friend Parent;

private:

    using insert_hints = tree_type::insert_hints;
    using premodify_cache = tree_type::premodify_cache;
    static constexpr bool requires_premodify_cache() { return tree_type::requires_premodify_cache(); }

    Parent& m_parent;
    tree_type m_tree;

    tmi_hasher(Parent& parent, const allocator_type& alloc) : m_parent(parent), m_tree(alloc) {}

    tmi_hasher(Parent& parent, const allocator_type& alloc, const ctor_args& args) : m_parent(parent), m_tree(alloc, std::get<0>(args), std::get<1>(args), std::get<2>(args), std::get<3>(args)){}

    tmi_hasher(Parent& parent, const tmi_hasher& rhs) : m_parent(parent), m_tree(rhs.m_tree){}
    tmi_hasher(Parent& parent, tmi_hasher&& rhs) : m_parent(parent), m_tree(std::move(rhs.m_tree))
    {
    }
    void remove_node(const node_type* node)
    {
        iterator it = m_tree.make_iterator(node);
        m_tree.erase(it);
    }

    void insert_node_direct(node_type* node)
    {
        m_tree.insert_node_direct(node);
    }

    node_type* preinsert_node(const node_type* node, insert_hints& hints)
    {
        return m_tree.preinsert_node(node, hints);
    }

    void create_premodify_cache(const node_type* node, premodify_cache& cache)
    {
        m_tree.create_premodify_cache(node, cache);
    }

    bool erase_if_modified(const node_type* node, const premodify_cache& cache)
    {
        return m_tree.erase_if_modified(node, cache);
    }

    void insert_node(node_type* node, const insert_hints& hints)
    {
        m_tree.insert_node(node, hints);
    }

    void do_clear()
    {
        m_tree.clear();
    }

public:

    iterator begin()
    {
        return m_tree.begin();
    }

    const_iterator begin() const
    {
        return m_tree.begin();
    }

    iterator end()
    {
        return m_tree.end();
    }

    const_iterator end() const
    {
        return m_tree.end();
    }

    const_iterator iterator_to(const value_type& entry) const
    {
        const node_type* node = Parent::template value_cast<node_type>(entry);
        return m_tree.make_iterator(node);
    }

    iterator iterator_to(const value_type& entry)
    {
        node_type* node = Parent::template value_cast<node_type>(const_cast<value_type&>(entry));
        return m_tree.make_iterator(node);
    }

    template <typename... Args>
    std::pair<iterator,bool> emplace(Args&&... args)
    {
        auto [node, success] = m_parent.template do_emplace<node_type>(std::forward<Args>(args)...);
        return std::make_pair(m_tree.make_iterator(node), success);
    }

    std::pair<iterator,bool> insert(const value_type& value)
    {
        auto [node, success] = m_parent.template do_insert<node_type>(value);
        return std::make_pair(m_tree.make_iterator(node), success);
    }

    std::pair<iterator,bool> insert(value_type&& value)
    {
        auto [node, success] = m_parent.template do_insert<node_type>(std::move(value));
        return std::make_pair(m_tree.make_iterator(node), success);
    }

    template <typename Callable>
    bool modify(iterator it, Callable&& func)
    {
        node_type* node = const_cast<node_type*>(m_tree.node_from_iterator(it));
        if (!node) return false;
        return m_parent.do_modify(node, std::forward<Callable>(func));
    }

    template <typename CompatibleKey>
    iterator find(const CompatibleKey& key) const
    {
        return m_tree.find(key);
    }

    iterator erase(iterator it)
    {
        node_type* node = const_cast<node_type*>(m_tree.node_from_iterator(it));
        ++it;
        m_parent.do_erase(node);
        return it;
    }

    template <typename CompatibleKey>
    size_t count(const CompatibleKey& key) const
    {
        return m_tree.count(key);
    }

    void clear()
    {
        m_parent.do_clear();
    }

    size_t size() const
    {
        return m_tree.size();
    }

    bool empty() const
    {
        return m_tree.empty();
    }

    insert_return_type insert(node_handle&& handle)
    {
        node_type* node = handle.m_node;
        if(!node) {
            return {end(), false, {}};
        }
        auto ret = m_parent.do_insert(node);
        const auto& [new_node, inserted] = ret;
        if (!inserted) {
            return {m_tree.make_iterator(new_node), false, std::move(handle)};
        }
        handle.m_node = nullptr;
        return {m_tree.make_iterator(new_node), true, {}};
    }

    const_iterator make_iterator(const node_type* node)
    {
        return m_tree.make_iterator(node);
    }

    node_handle extract(const_iterator it)
    {
        const node_type* node = m_tree.node_from_iterator(it);
        return m_parent.do_extract(const_cast<node_type*>(node));
    }

    allocator_type get_allocator() const noexcept
    {
        return m_parent.get_allocator();
    }
};

} // namespace tmi

#endif // TMI_HASHER_H_
