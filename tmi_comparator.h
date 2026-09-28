// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMI_COMPARATOR_H_
#define TMI_COMPARATOR_H_

#include "tmi_nodehandle.h"
#include "tmi_index.h"
#include "tmi_compare.h"
#include "wavl_tree.h"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <utility>
#include <tuple>

namespace tmi {

template <typename T, typename IndexedNode, bool Unique, bool IsOnlyIndex, typename Comparator, typename KeyFromValue, typename Parent, typename Allocator>
class tmi_comparator : private wavl_tree<IndexedNode, T, KeyFromValue, Comparator, Allocator, Unique>
{
    using tree_type = wavl_tree<IndexedNode, T, KeyFromValue, Comparator, Allocator, Unique>;
    using tree_type::unique_keys;

    struct ConstructionKey { constexpr ConstructionKey() noexcept = default; };
public:

    using data_type = IndexedNode;
    using key_from_value = KeyFromValue;
    using node_type = detail::node_handle<Allocator, data_type>;

    using key_type = tree_type::key_type;
    using value_type = tree_type::value_type;
    using key_compare = tree_type::key_compare_type;
    using value_compare = key_compare;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = tree_type::size_type;
    using difference_type = tree_type::difference_type;
    using pointer = std::allocator_traits<allocator_type>::pointer;
    using const_pointer = std::allocator_traits<allocator_type>::const_pointer;

    using iterator = tree_type::iterator;
    using const_iterator = tree_type::const_iterator;
    using reverse_iterator = tree_type::reverse_iterator;
    using const_reverse_iterator = tree_type::const_reverse_iterator;
    using insert_return_type = std::conditional_t<!unique_keys() && IsOnlyIndex, iterator, tmi::detail::insert_return_type<iterator, node_type>>;

    static_assert(std::is_same<typename allocator_type::value_type, value_type>::value);

private:
    friend Parent;

    template <typename, typename, typename>
    friend class multi_index_container;

    using insert_hints = tree_type::insert_hints;
    using premodify_cache = tree_type::premodify_cache;
    using insert_result_type = std::conditional_t<!unique_keys() && IsOnlyIndex, iterator, std::pair<iterator, bool>>;
    using ctor_args = std::tuple<key_from_value,key_compare>;
    using insert_hints_type = tree_type::insert_hints;

    static constexpr bool requires_premodify_cache() { return tree_type::requires_premodify_cache(); }

    Parent& m_parent;
public:
    tmi_comparator(ConstructionKey, Parent& parent, allocator_type&, const key_from_value& kv, const key_compare& kc) : tree_type{kv, kc}, m_parent(parent){}
    tmi_comparator(ConstructionKey, Parent& parent, allocator_type&, const key_from_value& kv) : tree_type{kv, key_compare{}}, m_parent(parent){}
    tmi_comparator(ConstructionKey, Parent& parent, allocator_type&, const key_compare& kc) : tree_type{key_from_value{}, kc}, m_parent(parent){}
    tmi_comparator(ConstructionKey, Parent& parent, allocator_type&) : tree_type{key_from_value{}, key_compare{}}, m_parent(parent){}
    tmi_comparator(ConstructionKey, Parent& parent, allocator_type&, const tmi_comparator& rhs) : tree_type{rhs.key_extractor(), rhs.key_comp()}, m_parent(parent){}
    tmi_comparator(ConstructionKey, Parent& parent, tmi_comparator&& rhs) : tree_type{std::move(rhs)}, m_parent(parent)
    {
        rhs.release();
    }
    tmi_comparator(Parent& parent) : m_parent(parent){}

    tmi_comparator(const tree_type& s) = delete;
    tmi_comparator(tree_type&&) = delete;

    tmi_comparator(tmi_comparator&& s) : tree_type{std::move(s)}, m_parent{s.m_parent} {}
    tmi_comparator& operator=(const tmi_comparator& s)
    {
        if (this == std::addressof(s))
            return *this;
        tree_type::operator=(tree_type{s.key_extractor(), s.key_comp()});

        return *this;
    }

    tmi_comparator& operator=(tmi_comparator&& s) noexcept(std::allocator_traits<Allocator>::is_always_equal::value
                                        && std::is_nothrow_move_assignable_v<allocator_type> && std::is_nothrow_move_assignable_v<tree_type>)
    {
        if (this == std::addressof(s))
            return *this;

        tree_type::operator=(std::move(s));
        s.release(); 

        return *this;
    }
/*
    void assign(tmi_comparator&& s)
    {
        tree_type::operator=(std::move(s));
        s.release();
    }
*/
    void assign_release(tmi_comparator&& s)
    {
        tree_type::operator=(std::move(s));
        s.release(); 
        tree_type::release();
    }
private:
/*
    insert_hints_type tmi_set_hint(const data_type* hint)
    {
        return tree_type::set_hint(hint);
    }
*/
    void tmi_remove_node(data_type* node)
    {
        iterator it = tree_type::make_iterator(node);
        tree_type::erase(it);
    }

    void tmi_insert_node_direct(data_type* node)
    {
        tree_type::insert_node_direct(node);
    }

    data_type* tmi_preinsert_node_hint(const data_type* supplied_hint, const value_type& value, insert_hints& hints)
    {
        return tree_type::preinsert_node_hint(supplied_hint, value, hints);
    }

    data_type* tmi_preinsert_node(const value_type& value, insert_hints& hints)
    {
        return tree_type::preinsert_node(value, hints);
    }

    void tmi_insert_node(data_type* node, const insert_hints& hints)
    {
        tree_type::insert_node(node, hints);
    }

    bool tmi_erase_if_modified(data_type* node, const premodify_cache& cache)
    {
        return tree_type::erase_if_modified(node, cache);
    }

    void tmi_clear()
    {
        tree_type::release();
    }

    insert_result_type make_insert_result(std::pair<data_type*, bool> result)
    {
        if constexpr(!unique_keys() && IsOnlyIndex) {
            return tree_type::make_iterator(result.first);
        } else {
            return std::make_pair(tree_type::make_iterator(result.first), result.second);
        }
    }

    insert_return_type make_insert_return(data_type* node, bool inserted, node_type&& nh)
    {
        if constexpr(!unique_keys() && IsOnlyIndex) {
            return tree_type::make_iterator(node);
        } else {
            return {tree_type::make_iterator(node), inserted, std::move(nh)};
        }
    }

    void unlink_node(data_type* node)
    {
        m_parent.do_unlink(node);
    }

    const data_type* node_from_iterator(const_iterator it) const
    {
        return tree_type::node_from_iterator(it);
    }

    data_type* node_from_iterator(iterator it)
    {
        return tree_type::node_from_iterator(it);
    }

public:

    insert_result_type insert(const value_type& v)
    {
        auto result = m_parent.template do_insert<data_type>(v);
        return make_insert_result(std::move(result));
    }

    insert_result_type insert(value_type&& v)
    {
        auto result = m_parent.template do_insert<data_type>(std::move(v));
        return make_insert_result(std::move(result));
    }

    iterator insert(const_iterator position, const value_type& v)
    {
        const data_type* node_hint = tree_type::node_from_iterator(position);
        const auto& [node, success] = m_parent.template do_insert_hint<data_type>(node_hint, v);
        return make_iterator(node);
    }

    iterator insert(const_iterator position, value_type&& v)
    {
        const data_type* node_hint = tree_type::node_from_iterator(position);
        const auto& [node, success] = m_parent.template do_insert_hint<data_type>(node_hint, std::move(v));
        return make_iterator(node);
    }

    template <class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        for(auto it = first; it != last; ++it) {
            m_parent.template do_emplace<data_type>(*it);
        }
    }

    void insert(std::initializer_list<value_type> il)
    {
        insert(il.begin(), il.end());
    }

    template <class... Args>
    insert_result_type emplace(Args&&... args)
    {
        auto result = m_parent.template do_emplace<data_type>(std::forward<Args>(args)...);
        return make_insert_result(std::move(result));
    }

    template <class... Args>
    iterator emplace_hint(const_iterator position, Args&&... args)
    {
        const data_type* node_hint = tree_type::node_from_iterator(position);
        const auto& [node, inserted] = m_parent.template do_emplace_hint<data_type>(node_hint, std::forward<Args>(args)...);
        return make_iterator(node);
    }

    node_type extract(const_iterator position)
    {
        const data_type* node = tree_type::node_from_iterator(position);
        return m_parent.do_extract(const_cast<data_type*>(node));
    }

    node_type extract(const key_type& x)
    {
        const_iterator position = find(x);
        return extract(position);
    }

    insert_return_type insert(node_type&& handle)
    {

        if(!handle) {
            return make_insert_return(nullptr, false, {});
        }
        auto ret = m_parent.do_reinsert_node(handle.get());
        const auto& [new_node, inserted] = ret;
        if (!inserted) {
            return make_insert_return(new_node, false, std::move(handle));
        }
        handle.release();
        return make_insert_return(new_node, true, {});
    }

    iterator insert(const_iterator, node_type&& nh)
    {
        //TODO: hint optimization
        if constexpr(!unique_keys()) {
            return insert(std::move(nh));
        } else {
            return insert(std::move(nh)).position;
        }
    }

    using tree_type::begin;
    using tree_type::end;
    using tree_type::rbegin;
    using tree_type::rend;
    using tree_type::cbegin;
    using tree_type::cend;
    using tree_type::crbegin;
    using tree_type::crend;

    size_type size() const noexcept
    {
        return m_parent.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_parent.empty();
    }

    size_type max_size() const noexcept
    {
        return std::min<size_type>(std::allocator_traits<allocator_type>::max_size(get_allocator()), std::numeric_limits<difference_type >::max());
    }

    iterator erase(const_iterator it)
    {
        data_type* node = const_cast<data_type*>(tree_type::node_from_iterator(it));
        ++it;
        m_parent.do_erase(node);
        return it;
    }

    size_type erase(const key_type& key)
    {
        size_t ret{0};
        auto [first, last] = tree_type::equal_range(key);
        for(auto it = first; it != last;)
        {
            it = erase(it);
            ret++;
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
        m_parent.do_clear();
    }

    using tree_type::key_comp;
    using tree_type::value_comp;

    [[nodiscard]] iterator find(const key_type& k)
    {
        return tree_type::find(k);
    }

    [[nodiscard]] const_iterator find(const key_type& k) const
    {
        return tree_type::find(k);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] iterator find(const K& k)
    {
        return tree_type::find(k);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] const_iterator find(const K& k) const
    {
        return tree_type::find(k);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] size_type count(const K& x) const
    {
        return tree_type::count(x);
    }

    [[nodiscard]] size_type count(const key_type& k) const
    {
        return tree_type::count(k);
    }

    [[nodiscard]] bool contains(const key_type& x) const
    {
        return tree_type::contains(x);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] bool contains(const K& x) const
    {
        return tree_type::contains(x);
    }

    [[nodiscard]] iterator lower_bound(const key_type& k)
    {
        return tree_type::lower_bound(k);
    }

    [[nodiscard]] const_iterator lower_bound(const key_type& k) const
    {
        return tree_type::lower_bound(k);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] iterator lower_bound(const K& x)
    {
        return tree_type::lower_bound(x);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] const_iterator lower_bound(const K& x) const
    {
        return tree_type::lower_bound(x);
    }

    [[nodiscard]] iterator upper_bound(const key_type& k)
    {
        return tree_type::upper_bound(k);
    }

    [[nodiscard]] const_iterator upper_bound(const key_type& k) const
    {
        return tree_type::upper_bound(k);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] iterator upper_bound(const K& x)
    {
        return tree_type::upper_bound(x);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] const_iterator upper_bound(const K& x) const
    {
        return tree_type::upper_bound(x);
    }

    [[nodiscard]] std::pair<iterator,iterator> equal_range(const key_type& k)
    {
        return tree_type::equal_range(k);
    }

    [[nodiscard]] std::pair<const_iterator,const_iterator> equal_range(const key_type& k) const
    {
        return tree_type::equal_range(k);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] std::pair<iterator,iterator> equal_range(const K& x)
    {
        return tree_type::equal_range(x);
    }

    template <typename K, class Comp = key_compare, std::enable_if_t<detail::is_transparent_v<Comp>, int> = 0>
    [[nodiscard]] std::pair<const_iterator,const_iterator> equal_range(const K& x) const
    {
        return tree_type::equal_range(x);
    }

    template<class IndexedNode2, class Parent2, class C2>
    void merge(tmi_comparator<T, IndexedNode2, Unique, IsOnlyIndex, C2, KeyFromValue, Parent2, Allocator>& source)
    {
        m_parent.template do_merge_index<data_type>(source);
    }

    template<class IndexedNode2, class Parent2, class C2>
    void merge(tmi_comparator<T, IndexedNode2, Unique, IsOnlyIndex, C2, KeyFromValue, Parent2, Allocator>&& source)
    {
        m_parent.template do_merge_index<data_type>(source);
    }

    template<class IndexedNode2, class Parent2, class C2>
    void merge(tmi_comparator<T, IndexedNode2, !Unique, IsOnlyIndex, C2, KeyFromValue, Parent2, Allocator>& source)
    {
        m_parent.template do_merge_index<data_type>(source);
    }

    template<class IndexedNode2, class Parent2, class C2>
    void merge(tmi_comparator<T, IndexedNode2, !Unique, IsOnlyIndex, C2, KeyFromValue, Parent2, Allocator>&& source)
    {
        m_parent.template do_merge_index<data_type>(source);
    }

    void swap(tmi_comparator& s) noexcept(std::allocator_traits<Allocator>::is_always_equal::value &&
                               std::is_nothrow_swappable_v<tree_type>)
    {
        tree_type::swap(s);
    }

    [[nodiscard]] allocator_type get_allocator() const noexcept
    {
        return m_parent.get_allocator();
    }

    const_iterator make_iterator(const data_type* node) const
    {
        return tree_type::make_iterator(node);
    }

    const_iterator iterator_to(const value_type& entry) const
    {
        const data_type* node = Parent::template value_cast<data_type>(entry);
        return tree_type::make_iterator(node);
    }

    iterator iterator_to(const value_type& entry)
    {
        data_type* node = Parent::template value_cast<data_type>(const_cast<value_type&>(entry));
        return tree_type::make_iterator(node);
    }

    template <typename Callable>
    bool modify(iterator it, Callable&& func)
    {
        data_type* node = const_cast<data_type*>(tree_type::node_from_iterator(it));
        if (!node) return false;
        return m_parent.do_modify(node, std::forward<Callable>(func));
    }

};

template <typename T, typename IndexedNode, bool Unique, bool IsOnlyIndex, typename Comparator, typename KeyFromValue, typename Parent, typename Allocator, typename Predicate>
typename tmi::tmi_comparator<T, IndexedNode, Unique, IsOnlyIndex, Comparator, KeyFromValue, Parent, Allocator>::size_type erase_if(tmi::tmi_comparator<T, IndexedNode, Unique, IsOnlyIndex, Comparator, KeyFromValue, Parent, Allocator>& c, Predicate pred)
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

template <typename T, typename IndexedNode, bool Unique, bool IsOnlyIndex, typename Comparator, typename KeyFromValue, typename Parent, typename Allocator>
inline bool operator==(const tmi::tmi_comparator<T, IndexedNode, Unique, IsOnlyIndex, Comparator, KeyFromValue, Parent, Allocator>& x, const tmi::tmi_comparator<T, IndexedNode, Unique, IsOnlyIndex, Comparator, KeyFromValue, Parent, Allocator>& y) {
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}

template <typename T, typename IndexedNode, bool Unique, bool IsOnlyIndex, typename Comparator, typename KeyFromValue, typename Parent, typename Allocator>
detail::synth_three_way_result<typename IndexedNode::value_type> operator<=>(const tmi::tmi_comparator<T, IndexedNode, Unique, IsOnlyIndex, Comparator, KeyFromValue, Parent, Allocator>& x, const tmi::tmi_comparator<T, IndexedNode, Unique, IsOnlyIndex, Comparator, KeyFromValue, Parent, Allocator>& y) {
    return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(), detail::synth_three_way);
}

template <typename T, typename IndexedNode, bool Unique, bool IsOnlyIndex, typename Comparator, typename KeyFromValue, typename Parent, typename Allocator>
void swap(tmi::tmi_comparator<T, IndexedNode, Unique, IsOnlyIndex, Comparator, KeyFromValue, Parent, Allocator>& x, tmi::tmi_comparator<T, IndexedNode, Unique, IsOnlyIndex, Comparator, KeyFromValue, Parent, Allocator>& y) noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

} // namespace tmi

#endif // TMI_COMPARATOR_H_
