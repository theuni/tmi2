// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tmi.h>

#include <test/hashed_non_unique.h>

#include <unordered_set>

namespace {

template <typename T>
class CompileTest
{
    void test_insert();
    void test_find();
    void test_erase();
    void test_emplace();
    void test_emplace_hint();
    void test_extract();
    void test_empty();
    void test_size();
    void test_count();

    tmi::multi_index_container<T,tmi::indexed_by<tmi::hashed_non_unique<tmi::identity<T>>>> tmi_unordered_multiset;
    typename decltype(tmi_unordered_multiset)::template nth_index_t<0>& tmi_unordered_multiset_view{tmi_unordered_multiset.template get<0>()};
    std::unordered_multiset<T> std_unordered_multiset;
public:
    CompileTest();
};

template <typename T>
CompileTest<T>::CompileTest()
{

    test_insert();
    test_find();
    test_erase();
    test_emplace();
    test_emplace_hint();
    test_extract();
    test_empty();
    test_size();
    test_count();

}

template <typename T>
void CompileTest<T>::test_insert()
{
    T foo{};
    const auto it = std_unordered_multiset.insert(T{});
    const auto& [it2, inserted2]  = tmi_unordered_multiset_view.insert(foo);

    assert(inserted2);
    assert(*it == *it2);
}

template <typename T>
void CompileTest<T>::test_find()
{
        auto it = std_unordered_multiset.find(T{});
        auto it2 = tmi_unordered_multiset_view.find(T{});
        assert(it == std_unordered_multiset.begin());
        assert(it2 == tmi_unordered_multiset_view.begin());
        assert(*it == *it2);
}

template <typename T>
void CompileTest<T>::test_erase()
{
        auto it = std_unordered_multiset.erase(std_unordered_multiset.begin());
        auto it2 = tmi_unordered_multiset_view.erase(tmi_unordered_multiset_view.begin());
        assert(it == std_unordered_multiset.end());
        assert(it2 == tmi_unordered_multiset_view.end());
}

template <typename T>
void CompileTest<T>::test_emplace()
{
        const auto it = std_unordered_multiset.emplace(T{});
        const auto& [it2, inserted2]  = tmi_unordered_multiset_view.emplace(T{});

        assert(*it == *it2);
        assert(inserted2);
}

template <typename T>
void CompileTest<T>::test_emplace_hint()
{
        //auto it = std_unordered_multiset.emplace_hint(std_unordered_multiset.begin(), T{});
        //auto it2 = tmi_unordered_multiset_view.emplace_hint(tmi_unordered_multiset_view.begin());

        //assert(*it != *it2);
}

template <typename T>
void CompileTest<T>::test_extract()
{
        auto nh = std_unordered_multiset.extract(std_unordered_multiset.begin());
        auto nh2 = tmi_unordered_multiset_view.extract(tmi_unordered_multiset_view.begin());
        assert(nh.value() == nh2.value());
        assert(nh.empty() == nh2.empty());
        assert(nh.get_allocator() == std_unordered_multiset.get_allocator());
        assert(nh2.get_allocator() == tmi_unordered_multiset_view.get_allocator());

        auto it = std_unordered_multiset.insert(std::move(nh));
        auto ret2 = tmi_unordered_multiset_view.insert(std::move(nh2));
        assert(ret2.inserted);
        assert(*it == *ret2.position);
        assert(ret2.node.empty());

/*
        auto nh = std_unordered_multiset.extract(T{});
        auto nh2 = tmi_unordered_multiset_view.extract(T{});
        assert(nh.value() == nh2.value());
        assert(nh.empty() == nh2.empty());
        assert(nh.get_allocator() == std_unordered_multiset.get_allocator());
        assert(nh2.get_allocator() == tmi_unordered_multiset_view.get_allocator());

        auto it = std_unordered_multiset.insert(std_unordered_multiset.begin(), std::move(nh));
        auto ret2 = tmi_unordered_multiset_view.insert(tmi_unordered_multiset_view.begin(), std::move(nh2));
        assert(ret.inserted == ret2.inserted);
        assert(*ret.position == *ret2.position);
        assert(ret.node.empty() == ret2.node.empty());
*/

}

template <typename T>
void CompileTest<T>::test_empty()
{
        assert(std_unordered_multiset.empty() == tmi_unordered_multiset_view.empty());
}

template <typename T>
void CompileTest<T>::test_size()
{
        assert(std_unordered_multiset.size() == tmi_unordered_multiset_view.size());
}

template <typename T>
void CompileTest<T>::test_count()
{
        assert(std_unordered_multiset.count(T{}) == tmi_unordered_multiset_view.count(T{}));
}

} // anonymous namespace

void test_hashed_non_unique()
{
    CompileTest<int> hashed_non_unique_tester;
}
