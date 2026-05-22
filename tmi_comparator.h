// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMI_COMPARATOR_H_
#define TMI_COMPARATOR_H_

#include "tmi_nodehandle.h"
#include "wavl_tree.h"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <utility>
#include <tuple>

namespace tmi {

template <typename IndexedNode, typename Comparator, typename Parent, typename Allocator>
class tmi_comparator
{
public:

    using node_type = IndexedNode;
    using key_from_value = typename Comparator::key_from_value_type;
    using key_compare = typename Comparator::comparator;
    using key_type = typename key_from_value::result_type;
    using ctor_args = std::tuple<key_from_value,key_compare>;
    using allocator_type = Allocator;
    using node_handle = detail::node_handle<Allocator, node_type>;
    using tree_type = wavl_tree<node_type, key_from_value, key_compare, Comparator::is_ordered_unique()>;
    using iterator = tree_type::iterator;
    using const_iterator = tree_type::const_iterator;
    using insert_return_type = detail::insert_return_type<iterator, node_handle>;
    using value_type = node_type::value_type;

private:
    friend Parent;

    using insert_hints = tree_type::insert_hints;
    using premodify_cache = tree_type::premodify_cache;
    static constexpr bool requires_premodify_cache() { return tree_type::requires_premodify_cache(); }

    Parent& m_parent;

    tree_type m_tree;

    tmi_comparator(Parent& parent, const allocator_type&) : m_parent(parent){}

    tmi_comparator(Parent& parent, const allocator_type&, const ctor_args& args) : m_parent(parent), m_tree{std::get<0>(args), std::get<1>(args)}{}
    tmi_comparator(Parent& parent, const tmi_comparator& rhs) : m_parent(parent), m_tree{rhs.m_tree}{}
    tmi_comparator(Parent& parent, tmi_comparator&& rhs) : m_parent(parent), m_tree{std::move(rhs.m_tree)}{}

    void remove_node(node_type* node)
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

    void insert_node(node_type* node, const insert_hints& hints)
    {
        m_tree.insert_node(node, hints);
    }

    bool erase_if_modified(node_type* node, const premodify_cache& cache)
    {
        return m_tree.erase_if_modified(node, cache);
    }

    void do_clear()
    {
        m_tree.clear();
    }

public:

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

    iterator begin() const
    {
        return m_tree.begin();
    }

    iterator end() const
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

    template <typename Callable>
    bool modify(iterator it, Callable&& func)
    {
        node_type* node = const_cast<node_type*>(m_tree.node_from_iterator(it));
        if (!node) return false;
        return m_parent.do_modify(node, std::forward<Callable>(func));
    }

    template<typename CompatibleKey>
    iterator find(const CompatibleKey& key) const
    {
        return m_tree.find(key);
    }

    template<typename CompatibleKey>
    iterator lower_bound(const CompatibleKey& key) const
    {
        return m_tree.lower_bound(key);
    }

    template<typename CompatibleKey>
    iterator upper_bound(const CompatibleKey& key) const
    {
        return m_tree.upper_bound(key);
    }

    template<typename CompatibleKey>
    size_t count(const CompatibleKey& key) const
    {
        return m_tree.count(key);
    }

    iterator erase(iterator it)
    {
        node_type* node = const_cast<node_type*>(m_tree.node_from_iterator(it));
        ++it;
        m_parent.do_erase(node);
        return it;
    }

    size_t erase(const key_type& key) const
    {
        size_t ret{0};
        auto [first, last] = m_tree.equal_range(key);
        for(auto it = first; it != last;)
        {
            it = erase(it);
            ret++;
        }
        return ret;
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
            return {m_tree.end(), false, {}};
        }
        auto ret = m_parent.do_insert(node);
        const auto& [new_node, inserted] = ret;
        if (!inserted) {
            return {m_tree.make_iterator(new_node), false, std::move(handle)};
        }
        handle.m_node = nullptr;
        return {m_tree.make_iterator(new_node), true, {}};
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

    const_iterator make_iterator(const node_type* node)
    {
        return m_tree.make_iterator(node);
    }

};

} // namespace tmi

#endif // TMI_COMPARATOR_H_
