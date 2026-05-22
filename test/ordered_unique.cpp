// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tmi.h>

#include <test/ordered_unique.h>

#include <set>

namespace {

struct my_class
{
    int m_val{0};
    my_class() = default;
    bool operator==(const my_class& rhs) const { return m_val == rhs.m_val;}
    bool operator<(const my_class& rhs) const { return m_val < rhs.m_val;}
};

struct comp_less;

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

    std::set<T> std_set;
    tmi::multi_index_container<T,tmi::indexed_by<tmi::ordered_unique<tmi::identity<T>>>> tmi_set;
    typename decltype(tmi_set)::template nth_index_t<0>& tmi_set_view{tmi_set.template get<0>()};
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
    const auto& [it, inserted] = std_set.insert(T{});
    const auto& [it2, inserted2]  = tmi_set_view.insert(foo);

    assert(*it == *it2);
    assert(inserted == inserted2);
}

template <typename T>
void CompileTest<T>::test_find()
{
        auto it = std_set.find(T{});
        auto it2 = tmi_set_view.find(T{});
        assert(*it == *it2);
}

template <typename T>
void CompileTest<T>::test_erase()
{
        auto it = std_set.erase(std_set.begin());
        auto it2 = tmi_set_view.erase(tmi_set_view.begin());
        assert(it == std_set.end());
        assert(it2 == tmi_set_view.end());
}

template <typename T>
void CompileTest<T>::test_emplace()
{
        const auto& [it, inserted] = std_set.emplace(T{});
        const auto& [it2, inserted2]  = tmi_set_view.emplace(T{});

        assert(*it == *it2);
        assert(inserted == inserted2);
}

template <typename T>
void CompileTest<T>::test_emplace_hint()
{
        //auto it = std_set.emplace_hint(std_set.begin(), T{});
        //auto it2 = tmi_set_view.emplace_hint(tmi_set_view.begin());

        //assert(*it != *it2);
}

template <typename T>
void CompileTest<T>::test_extract()
{
        auto nh = std_set.extract(std_set.begin());
        auto nh2 = tmi_set_view.extract(tmi_set_view.begin());
        assert(nh.value() == nh2.value());
        assert(nh.empty() == nh2.empty());
        assert(nh.get_allocator() == std_set.get_allocator());
        assert(nh2.get_allocator() == tmi_set_view.get_allocator());

        auto ret = std_set.insert(std::move(nh));
        auto ret2 = tmi_set_view.insert(std::move(nh2));
        assert(ret.inserted == ret2.inserted);
        assert(*ret.position == *ret2.position);
        assert(ret.node.empty() == ret2.node.empty());

/*
        auto nh = std_set.extract(T{});
        auto nh2 = tmi_set_view.extract(T{});
        assert(nh.value() == nh2.value());
        assert(nh.empty() == nh2.empty());
        assert(nh.get_allocator() == std_set.get_allocator());
        assert(nh2.get_allocator() == tmi_set_view.get_allocator());

        auto it = std_set.insert(std_set.begin(), std::move(nh));
        auto ret2 = tmi_set_view.insert(tmi_set_view.begin(), std::move(nh2));
        assert(ret.inserted == ret2.inserted);
        assert(*ret.position == *ret2.position);
        assert(ret.node.empty() == ret2.node.empty());
*/

}

template <typename T>
void CompileTest<T>::test_empty()
{
        assert(std_set.empty() == tmi_set_view.empty());
}

template <typename T>
void CompileTest<T>::test_size()
{
        assert(std_set.size() == tmi_set_view.size());
}

template <typename T>
void CompileTest<T>::test_count()
{
        assert(std_set.count(T{}) == tmi_set_view.count(T{}));
}

} // anonymous namespace

void test_ordered_unique()
{
    CompileTest<int> ordered_unique_tester;
    CompileTest<my_class> ordered_unique_tester2;
}
