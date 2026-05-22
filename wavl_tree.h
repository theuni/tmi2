// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WAVL_TREE_H_
#define WAVL_TREE_H_

#include <cassert>
#include <cstddef>
#include <iterator>
#include <utility>
#include <tuple>

namespace tmi {

template <typename Node, typename KeyFromValue, typename Comparator, bool Unique>
class wavl_tree
{
public:
    using node_type = Node;
    using base_type = node_type;
    using key_from_value_type = KeyFromValue;
    using key_compare_type = Comparator;
    using key_type = typename key_from_value_type::result_type;
    using value_type = node_type::value_type;
//private:
    static constexpr bool unique_keys() { return Unique; }

    struct insert_hints {
        base_type* m_parent{nullptr};
        bool m_inserted_left{false};
    };

    struct premodify_cache{};
    static constexpr bool requires_premodify_cache() { return false; }

    base_type* m_root{nullptr};
    [[no_unique_address]] key_from_value_type m_key_from_value;
    [[no_unique_address]] key_compare_type m_comparator;

    size_t m_size{0};

    wavl_tree(key_from_value_type key_from_value = {}, key_compare_type comparator = {}) : m_key_from_value{std::move(key_from_value)}, m_comparator(std::move(comparator)) {}
    wavl_tree(const wavl_tree& rhs)  = default;
    wavl_tree(wavl_tree&& rhs)
    {
        rhs.clear();
    }

    static base_type* tree_max(base_type* x) {
      while (x->right() != nullptr)
        x = x->right();
      return x;
    }

    static const base_type* tree_max(const base_type* x) {
      while (x->right() != nullptr)
        x = x->right();
      return x;
    }

    static bool tree_is_left_child(base_type* x)
    {
        return x == x->parent()->left();
    }

    static bool tree_is_left_child(const base_type* x)
    {
        return x == x->parent()->left();
    }

    static base_type* tree_min(base_type* x)
    {
        while (x->left() != nullptr)
            x = x->left();
        return x;
    }

    static base_type* tree_next(base_type* x)
    {
        if (x->right() != nullptr)
            return tree_min(x->right());
        while (x->parent() && !tree_is_left_child(x))
            x = x->parent();
        return x->parent();
    }

    static const base_type* tree_next(const base_type* x)
    {
        if (x->right() != nullptr)
            return tree_min(x->right());
        while (x->parent() && !tree_is_left_child(x))
            x = x->parent();
        return x->parent();
    }

    static base_type* tree_prev(base_type* x)
    {
      if (x->left() != nullptr)
        return tree_max(x->left());
      while (x->parent() && tree_is_left_child(x))
        x = x->parent();
      return x->parent();
    }

    static const base_type* tree_prev(const base_type* x)
    {
      if (x->left() != nullptr)
        return tree_max(x->left());
      while (x->parent() && tree_is_left_child(x))
        x = x->parent();
      return x->parent();
    }

    base_type *rotate(base_type *x, int d)
    {
        auto pivot = x->ch(d);
        x->set_ch(d, pivot->ch(d^1));
        if (x->ch(d)) x->ch(d)->set_parent(x);
        pivot->set_parent(x->parent());
        if (!x->parent()) m_root = pivot;
        else x->parent()->set_ch(x != x->parent()->ch(0), pivot);
        pivot->set_ch(d^1, x);
        x->set_parent(pivot);
        return pivot;
    }

    void insert_rebalance(base_type *p)
    {
        base_type *x;
        bool par_x, par_y, par_p;
        int d;
        for (;;) {
            // Promote p.
            p->flip();
            x = p;
            p = p->parent();
            if (!p) return;
            d = p->ch(1) == x;
            par_x = x->rp();
            par_y = !p->ch(d^1) || p->ch(d^1)->rp();
            par_p = p->rp();
            // Continue if p is a 0,1 node.
            if (!(par_x == par_p && par_x != par_y))
            break;
        }

        // If p is not 0,2, finish.
        if (par_x != par_p || par_x != par_y)
            return;

        base_type *y = x->ch(d^1);
        if (y && y->rp() != par_x) {
            rotate(x, d^1);
            rotate(p, d);
            y->flip();
            x->flip();
            x = y;
        } else {
            rotate(p, d);
        }
        p->flip();
    }

    static bool is_22_node(base_type *n)
    {
        if (n->rp()) {
            return (!n->ch(0) || n->ch(0)->rp()) && (!n->ch(1) || n->ch(1)->rp());
        } else {
            return n->ch(0) && !n->ch(0)->rp() && n->ch(1) && !n->ch(1)->rp();
        }
    }

    void tree_remove(base_type *n) {
        base_type *y = n;
        if (n->ch(0) && n->ch(1))
            for (y = n->ch(1); y->ch(0); y = y->ch(0));
        base_type *p = y->parent(), *x = y->ch(0) ? y->ch(0) : y->ch(1);
        bool was_2 = p && y->rp() == p->rp();

        if (p) p->set_ch(p->ch(1) == y, x); else m_root = x;
        if (x) x->set_parent(p);

        if (y != n) {
            y->set_ch(0, n->ch(0)); y->set_ch(1, n->ch(1));
            y->set_par_and_flg(n->par_and_flg());
            if (n->parent()) {
                int child = n->parent()->ch(1) == n;
                n->parent()->set_ch(child, y);
            } else {
                m_root = y;
            }
            y->ch(0)->set_parent(y);
            if (y->ch(1)) y->ch(1)->set_parent(y);
            if (p == n) p = y;
        }

        if (p) {
            if (!was_2 && !x && !p->ch(0) && !p->ch(1) && p->rp()) {
                was_2 = p->parent() && p->rp() == p->parent()->rp();
                p->flip();
                x = p;
                p = p->parent();
            }
            for (; p && was_2; x = p, p = p->parent()) {
                int d = p->ch(1) == x;
                y = p->ch(d^1);
                bool creates_3 = p->parent() && p->rp() == p->parent()->rp();
                if (y->rp() == p->rp()) {
                    p->flip();
                } else if (is_22_node(y)) {
                    y->flip();
                    p->flip();
                } else {
                    base_type* w = y->ch(d^1);
                    if ((w ? w->rp() : true) != y->rp()) {
                        rotate(p, d^1);
                        y->flip();
                        p->flip();
                        if (!p->ch(0) && !p->ch(1)) p->flip();
                    } else {
                        rotate(y, d);
                        rotate(p, d^1);
                        y->flip();
                    }
                    break;
                }
                if (!creates_3) break;
            }
        }
    }
#ifdef DEBUG
    static int compute_rank(base_type* n)
    {
        if (!n) return -1;
        int lr = compute_rank(n->left()), rr = compute_rank(n->right());
        if (lr < -1 || rr < -1) return -2;
        int rank_l = lr + (n->rp() == (lr & 1) ? 2 : 1);
        int rank_r = rr + (n->rp() == (rr & 1) ? 2 : 1);
        assert(rank_l == rank_r);
        return rank_l;
    }
    static bool verify_tree(base_type* root) {
        int rank = compute_rank(root);
        assert(rank >= -1);
        return true;
    }
#else
    static bool verify_tree(base_type*) { return true; }
#endif

    void remove_node(base_type* base)
    {
        tree_remove(base);
        verify_tree(m_root);
        m_size--;
    }

    void insert_node_direct(base_type* base)
    {
        base_type* curr = m_root;
        const auto& key = m_key_from_value(base->value());

        insert_hints hints;
        while (curr != nullptr) {
            hints.m_parent = curr;
            const auto& curr_key = m_key_from_value(curr->value());
            if (m_comparator(key, curr_key)) {
                curr = curr->left();
                hints.m_inserted_left = true;
            } else {
                curr = curr->right();
                hints.m_inserted_left = false;
            }
        }
        insert_node(base, hints);
    }

    base_type* preinsert_node(const base_type* base, insert_hints& hints)
    {
        base_type* parent = nullptr;
        base_type* curr = m_root;
        const auto& key = m_key_from_value(base->value());

        bool inserted_left = false;
        while (curr != nullptr) {
            parent = curr;
            const auto& curr_key = m_key_from_value(curr->value());
            if constexpr (unique_keys()) {
                if (m_comparator(key, curr_key)) {
                    curr = curr->left();
                    inserted_left = true;
                } else if (m_comparator(curr_key, key)) {
                    curr = curr->right();
                    inserted_left = false;
                } else {
                    return curr;
                }
            } else {
                if (m_comparator(key, curr_key)) {
                    curr = curr->left();
                    inserted_left = true;
                } else {
                    curr = curr->right();
                    inserted_left = false;
                }
            }
        }
        hints.m_inserted_left = inserted_left;
        hints.m_parent = parent;
        return nullptr;
    }

    void insert_node(base_type* base, const insert_hints& hints)
    {
        base_type* parent = hints.m_parent;

        base->set_left(nullptr);
        base->set_right(nullptr);
        base->set_parent(nullptr);
        base->clr_flags();

        if(parent) {
            bool was_leaf;
            if (hints.m_inserted_left) {
                parent->set_left(base);
                was_leaf = !parent->right();
            } else {
                parent->set_right(base);
                was_leaf = !parent->left();
            }
            base->set_parent(parent);
            if (was_leaf) {
                insert_rebalance(parent);
            }
        } else {
            m_root = base;
        }
        m_size++;
        verify_tree(m_root);
    }

    bool erase_if_modified(base_type* base, const premodify_cache&)
    {
        base_type* next_ptr = nullptr;
        base_type* prev_ptr = nullptr;

        if (base != tree_min(m_root))
            prev_ptr = tree_prev(base);
        if (base != tree_max(m_root))
            next_ptr = tree_next(base);

        const auto& key = m_key_from_value(base->value());

        bool needs_resort = ((next_ptr != nullptr && m_comparator(m_key_from_value(next_ptr->value()), key)) ||
                             (prev_ptr != nullptr && m_comparator(key, m_key_from_value(prev_ptr->value()))));
        if (needs_resort) {

            remove_node(base);
            base->set_parent(nullptr);
            base->set_left(nullptr);
            base->set_right(nullptr);
            base->clr_flags();
            return true;
        }
        return false;
    }

public:

    void clear()
    {
        m_root = nullptr;
        m_size = 0;
    }
 
    size_t size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return !size();
    }
   
    class iterator
    {
        const base_type* m_node{};
        const base_type* m_root{};
        iterator(const base_type* node, const base_type* root) : m_node(node), m_root(root){}
        friend wavl_tree;
    public:
        typedef const wavl_tree::value_type value_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        typedef std::ptrdiff_t difference_type;
        using iterator_category = std::bidirectional_iterator_tag;
        using element_type = const value_type;
        iterator() = default;
        reference operator*() const { return m_node->value(); }
        pointer operator->() const { return &m_node->value(); }
        iterator& operator++()
        {
            const base_type* next = tree_next(m_node);
            if (next) {
                m_node = next;
            } else {
                m_node = nullptr;
            }
            return *this;
        }
        iterator& operator--()
        {
            if (m_node) {
                const base_type* prev = tree_prev(m_node);
                if (prev) {
                    m_node = prev;
                } else {
                    m_node = nullptr;
                }
            } else {
                base_type* root = m_root->left();
                assert(root);
                m_node = tree_max(root);
            }
            return *this;
        }
        iterator operator++(int)
        {
            iterator copy(m_node, m_root);
            ++(*this);
            return copy;
        }
        iterator operator--(int)
        {
            iterator copy(m_node, m_root);
            --(*this);
            return copy;
        }
        bool operator==(iterator rhs) const { return m_node == rhs.m_node; }
        bool operator!=(iterator rhs) const { return m_node != rhs.m_node; }
    };
    using const_iterator = iterator;

    iterator begin() const
    {
        base_type* root = m_root;
        if (root == nullptr)
            return end();
        return make_iterator(tree_min(m_root));
    }

    iterator end() const
    {
        return make_iterator(nullptr);
    }

    const_iterator make_iterator(const base_type* node) const
    {
        return const_iterator(node, m_root);
    }
    const base_type* node_from_iterator(const_iterator it) const
    {
        return it.m_node;
    }


    template<typename CompatibleKey>
    iterator lower_bound(const CompatibleKey& key) const
    {
        base_type* curr = m_root;
        base_type* ret = nullptr;
        while (curr != nullptr) {
            const auto& curr_key = m_key_from_value(curr->value());
            if (!m_comparator(curr_key, key)) {
                ret = curr;
                curr = curr->left();
            } else {
                curr = curr->right();
            }
        }
        if (ret) {
            return make_iterator(ret);
        } else {
            return end();
        }
    }

    template<typename CompatibleKey>
    iterator upper_bound(const CompatibleKey& key) const
    {
        base_type* curr = m_root;
        base_type* ret = nullptr;
        while (curr != nullptr) {
            const auto& curr_key = m_key_from_value(curr->value());
            if (m_comparator(key, curr_key)) {
                ret = curr;
                curr = curr->left();
            } else {
                curr = curr->right();
            }
        }
        if (ret) {
            return make_iterator(ret);
        } else {
            return end();
        }
    }

    template<typename CompatibleKey>
    std::pair<iterator, iterator> equal_range( const CompatibleKey& key) const
    {
        return { lower_bound(key), upper_bound(key) };
    }

    template<typename CompatibleKey>
    size_t count(const CompatibleKey& key) const
    {
        size_t ret = 0;
        auto [first, last] = equal_range(key);
        for(auto it = first; it != last; ++it) {
            ++ret;
        }
        return ret;
    }

    template<typename CompatibleKey>
    iterator find(const CompatibleKey& key) const
    {
        base_type* curr = m_root;
        while (curr != nullptr) {
            const auto& curr_key = m_key_from_value(curr->value());
            if (m_comparator(key, curr_key)) {
                curr = curr->left();
            } else if (m_comparator(curr_key, key)) {
                curr = curr->right();
            } else {
                return make_iterator(curr);
            }
        }
        return end();
    }

    iterator erase(iterator it)
    {
        base_type* node = const_cast<base_type*>(it.m_node);
        iterator next = it++;
        remove_node(node);
        return next;
    }

    iterator erase(iterator first, iterator last)
    {
        auto it = first;
        while (it != last) {
            it = erase(it);
        }
        return it;
    }
    
    size_t erase(const key_type& key) const
    {
        size_t ret = 0;
        auto [first, last] = equal_range(key);
        for(auto it = first; it != last; it = erase(it)) {
            ++ret;
        }
        return ret;
    }
};

} // namespace tmi

#endif // wavl_tree_H_
