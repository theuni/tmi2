// Copyright (c) 2026 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tmi.h>

#include <test/ordered_unique.h>

#include <tmiset.h>
#include <set>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

namespace {

struct my_type
{
    int m_val{0};
    my_type() = default;
    bool operator==(const my_type& rhs) const { return m_val == rhs.m_val;}
    bool operator<(const my_type& rhs) const { return m_val < rhs.m_val;}
};

template <typename T, typename Compare = std::less<T>, typename... Args>
void set_test(Args&&... ctor_args)
{
    using set_type = tmi::set<T, Compare>;

    using key_type = set_type::key_type;
    using value_type = set_type::value_type;
    using key_compare = set_type::key_compare;
    using value_compare = set_type::value_compare;
    using allocator_type = set_type::allocator_type;
    using reference = set_type::reference;
    using const_reference = set_type::const_reference;
    using size_type = set_type::size_type;
    using difference_type = set_type::difference_type;
    using pointer = set_type::pointer;
    using const_pointer = set_type::const_pointer;
    using iterator = set_type::iterator;
    using const_iterator = set_type::const_iterator;
    using reverse_iterator = set_type::reverse_iterator;
    using const_reverse_iterator = set_type::const_reverse_iterator;
    using node_type = set_type::node_type;
    using insert_return_type = set_type::insert_return_type;


    const allocator_type alloc;
    const value_compare comp;
    set_type s;
    tmi::multiset<T> ms;
    const set_type cs;
    const value_type cv{};
    value_type v{};
    const key_type ck{};
    node_type nh;

    iterator it;
    const_iterator cit;
    bool bool_ret;
    size_type size_ret;
    std::pair<iterator, iterator> iter_pair_ret;
    std::pair<const_iterator, const_iterator> const_iter_pair_ret;
    reverse_iterator rev_it;
    const_reverse_iterator const_rev_it;
    std::pair<iterator, bool> iter_bool_pair;

    { set_type y(comp); }
    { set_type y(comp, alloc); }
    { set_type y(cs.begin(), cs.end(), comp); }
    { set_type y(cs.begin(), cs.end(), comp, alloc); }
    { set_type y(cs); }
    { set_type y(std::move(s)); }
    { set_type y(alloc); }
    { set_type y(cs, alloc); }
    { set_type y(std::move(s), alloc); }
    { set_type y(std::initializer_list<T>{}, comp); }
    { set_type y(std::initializer_list<T>{}, comp, alloc); }
    { set_type y(cs.begin(), cs.end(), alloc); }
    { set_type y(std::initializer_list<T>{}, alloc); }
    { set_type y = cs; }
    { set_type y = std::move(s); }
    { set_type y = std::initializer_list<T>{}; }
    { it = s.begin(); }
    { cit = cs.begin(); }
    { it = s.end(); }
    { cit = cs.end(); }
    { rev_it = s.rbegin(); }
    { const_rev_it = cs.rbegin(); }
    { rev_it = s.rend(); }
    { const_rev_it = cs.rend(); }
    { cit = cs.cbegin(); }
    { cit = cs.cend(); }
    { const_rev_it = cs.crbegin(); }
    { const_rev_it = cs.crend(); }
    { bool_ret = cs.empty(); }
    { size_ret = cs.size(); }
    { size_ret = cs.max_size(); }
    { iter_bool_pair = s.emplace(std::forward<Args>(ctor_args)...); }
    { it = s.emplace_hint(s.cbegin(), std::forward<Args>(ctor_args)...); }
    { iter_bool_pair = s.insert(cv); }
    { iter_bool_pair = s.insert(std::move(v)); }
    { it = s.insert(s.cbegin(), cv); }
    { it = s.insert(s.cbegin(), std::move(v)); }
    { s.insert(cs.cbegin(), cs.cend()); }
    { s.insert(std::initializer_list<T>{}); }
    { node_type ret = s.extract(s.cbegin()); }
    { node_type ret = s.extract(ck); }
    { insert_return_type ret = s.insert(std::move(nh)); }
    { it = s.insert(s.cbegin(), std::move(nh)); }
    { it = s.erase(s.cbegin()); }
    { it = s.erase(s.begin()); }
    { size_ret = s.erase(ck); }
    { it = s.erase(s.cbegin(), s.cend()); }
    { s.clear(); }
    { s.merge(s); }
    { s.merge(std::move(s)); }
    { s.merge(ms); }
    { s.merge(std::move(ms)); }
    { s.swap(s); }
    { allocator_type ret = cs.get_allocator(); }
    { key_compare ret = cs.key_comp(); }
    { value_compare ret = cs.value_comp(); }
    { it = s.find(ck); }
    { cit = cs.find(ck); }
    { size_ret = cs.count(ck); }
    { bool_ret = cs.contains(ck); }
    { it = s.lower_bound(ck); }
    { cit = cs.lower_bound(ck); }
    { it = s.upper_bound(ck); }
    { cit = cs.upper_bound(ck); }
    { iter_pair_ret = s.equal_range(ck); }
    { const_iter_pair_ret = cs.equal_range(ck); }

    { swap(s, s); }
    { size_ret = erase_if(s, [](const T&){return true;}); }
    { bool_ret = cs == cs; }
    { bool_ret = cs < cs; }
    { bool_ret = cs != cs; }
    { bool_ret = cs > cs; }
    { bool_ret = cs >= cs; }
    { bool_ret = cs <= cs; }

    // deduction guides are a nightmare to implement
    //
    //{ tmi::set y(s.begin(), s.end()); }
    //{ tmi::set y(std::initializer_list<T>{}); }
    //{ tmi::set y(s.begin(), s.end(), alloc); }
    //{ tmi::set y(std::initializer_list<T>{}, alloc); }
}


void set_test_transparent()
{
    using set_type = tmi::set<std::string, std::less<>>;

    using size_type = set_type::size_type;
    using iterator = set_type::iterator;
    using const_iterator = set_type::const_iterator;

    iterator it;
    const_iterator cit;
    bool bool_ret;
    size_type size_ret;
    std::pair<iterator, iterator> iter_pair_ret;
    std::pair<const_iterator, const_iterator> const_iter_pair_ret;
    const char* ck = "test";

    set_type s;
    const set_type cs;
    { it = s.find(ck); }
    { cit = cs.find(ck); }
    { size_ret = cs.count(ck); }
    { bool_ret = cs.contains(ck); }
    { it = s.lower_bound(ck); }
    { cit = cs.lower_bound(ck); }
    { it = s.upper_bound(ck); }
    { cit = cs.upper_bound(ck); }
    { iter_pair_ret = s.equal_range(ck); }
    { const_iter_pair_ret = cs.equal_range(ck); }
}


} // anonymous namespace

void compile_only_test()
{
    set_test<int>();
    set_test<my_type>();
    set_test<std::string, std::less<>>();
    set_test_transparent();
}

#pragma GCC diagnostic pop
