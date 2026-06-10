// Copyright (c) 2026 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tmi.h>

#include <test/ordered_unique.h>

#include <tmi_unordered_set.h>
#include <set>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-variable"

namespace {

struct my_type
{
    int m_val{0};
    my_type() = default;
    bool operator==(const my_type& rhs) const { return m_val == rhs.m_val;}
    bool operator<(const my_type& rhs) const { return m_val < rhs.m_val;}
};

struct my_type_hasher
{
    size_t operator()(const my_type&) const
    {
        return 0;
    }
};

template <typename T, typename Hash = std::hash<T>, typename KeyEqual = std::equal_to<T>, typename... Args>
void unordered_set_test(Args&&... ctor_args)
{
    using unordered_set_type = tmi::unordered_set<T, Hash, KeyEqual>;

    using key_type = unordered_set_type::key_type;
    using value_type = unordered_set_type::value_type;
    using hasher = unordered_set_type::hasher;
    using key_equal = unordered_set_type::key_equal;
    using allocator_type = unordered_set_type::allocator_type;
    using reference = unordered_set_type::reference;
    using const_reference = unordered_set_type::const_reference;
    using pointer = unordered_set_type::pointer;
    using const_pointer = unordered_set_type::const_pointer;
    using size_type = unordered_set_type::size_type;
    using difference_type = unordered_set_type::difference_type;

    using iterator = unordered_set_type::iterator;
    using const_iterator = unordered_set_type::const_iterator;
    using local_iterator = unordered_set_type::local_iterator;
    using const_local_iterator = unordered_set_type::const_local_iterator;

    using node_type = unordered_set_type::node_type;
    using insert_return_type = unordered_set_type::insert_return_type;


    const allocator_type alloc;
    const key_equal eq;
    const hasher hf;
    unordered_set_type s;
    tmi::unordered_multiset<T, Hash, KeyEqual> ms;
    const unordered_set_type cs;
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
    local_iterator local_it;
    const_local_iterator const_local_it;
    std::pair<iterator, bool> iter_bool_pair;

    { unordered_set_type y(0, hf); }
    { unordered_set_type y(0, hf, eq); }
    { unordered_set_type y(0, hf, eq, alloc); }

    { unordered_set_type y(0, alloc); }

    { unordered_set_type y(0, hf, alloc); }

    { unordered_set_type y(alloc); }

    { unordered_set_type y(cs.begin(), cs.end()); }
    { unordered_set_type y(cs.begin(), cs.end(), 0); }
    { unordered_set_type y(cs.begin(), cs.end(), 0, hf); }
    { unordered_set_type y(cs.begin(), cs.end(), 0, hf, eq); }
    { unordered_set_type y(cs.begin(), cs.end(), 0, hf, eq, alloc); }

    { unordered_set_type y(cs.begin(), cs.end(), 0, alloc); }

    { unordered_set_type y(cs.begin(), cs.end(), 0, hf, alloc); }

    { unordered_set_type y(cs); }

//    { unordered_set_type y(cs, alloc); }

    { unordered_set_type y(std::move(s)); }

//    { unordered_set_type y(std::move(s), alloc); }


    { unordered_set_type y(std::initializer_list<T>{}); }
    { unordered_set_type y(std::initializer_list<T>{}, 0); }
    { unordered_set_type y(std::initializer_list<T>{}, 0, hf); }
    { unordered_set_type y(std::initializer_list<T>{}, 0, hf, eq); }
    { unordered_set_type y(std::initializer_list<T>{}, 0, hf, eq, alloc); }

    { unordered_set_type y(std::initializer_list<T>{}, 0, alloc); }

    { unordered_set_type y(std::initializer_list<T>{}, 0, hf, alloc); }

    { it = s.begin(); }
    { cit = cs.begin(); }
    { it = s.end(); }
    { cit = cs.end(); }
    { local_it = s.begin(0); }
    { const_local_it = cs.begin(0); }
    { local_it = s.end(0); }
    { const_local_it = cs.end(0); }
    { cit = cs.cbegin(); }
    { cit = cs.cend(); }
    { const_local_it = cs.cbegin(0); }
    { const_local_it = cs.cend(0); }
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
    { key_equal ret = cs.key_eq(); }
    { hasher ret = cs.hash_function(); }
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
    { size_type ret = cs.bucket_size(0); }
    { size_type ret = cs.bucket_count(); }
    { size_type ret = cs.max_bucket_count(); }
    { float ret = cs.load_factor(); }
    { float ret = cs.max_load_factor(); }
    { s.max_load_factor(1.0); }
    { s.rehash(0); }
    { s.reserve(0); }

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
    //{ tmi::unordered_set y(s.begin(), s.end()); }
    //{ tmi::unordered_set y(std::initializer_list<T>{}); }
    //{ tmi::unordered_set y(s.begin(), s.end(), alloc); }
    //{ tmi::unordered_set y(std::initializer_list<T>{}, alloc); }

}

void unordered_set_test_transparent()
{
struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view str) const
    {
        return std::hash<std::string_view>{}(str);
    }
};

    using unordered_set_type = tmi::unordered_set<std::string, StringHash, std::equal_to<>>;

    using size_type = unordered_set_type::size_type;
    using iterator = unordered_set_type::iterator;
    using const_iterator = unordered_set_type::const_iterator;

    iterator it;
    const_iterator cit;
    bool bool_ret;
    size_type size_ret;
    std::pair<iterator, iterator> iter_pair_ret;
    std::pair<const_iterator, const_iterator> const_iter_pair_ret;
    std::string_view ck = "test";

    unordered_set_type s;
    const unordered_set_type cs;
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

void unordered_compile_only_test()
{
    unordered_set_test<int>();
    unordered_set_test<my_type, my_type_hasher>();
    unordered_set_test<std::string>();
    unordered_set_test_transparent();
}


#pragma GCC diagnostic pop
