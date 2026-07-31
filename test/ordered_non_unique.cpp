// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tmi.h>

#include <test/ordered_non_unique.h>

#include <set>

namespace
{

struct my_class
{
    int m_val{0};
    my_class() = default;
    bool operator==(const my_class& rhs) const { return m_val == rhs.m_val;}
    bool operator<(const my_class& rhs) const { return m_val < rhs.m_val;}
};

struct comp_less;

template <typename T>
T modified_value();

template <>
int modified_value<int>()
{
    return 1;
}

template <>
my_class modified_value<my_class>()
{
    my_class ret;
    ret.m_val = 1;
    return ret;
}

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
    void test_modify();
    void test_clear();

    std::multiset<T> std_multiset;
    tmi::multi_index_container<T,tmi::indexed_by<tmi::ordered_non_unique<tmi::identity<T>>>> tmi_multiset;
    typename decltype(tmi_multiset)::template nth_index_t<0>& tmi_multiset_view{tmi_multiset.template get<0>()};
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
    test_modify();
    test_clear();
}

template <typename T>
void CompileTest<T>::test_insert()
{
    T foo{};
    auto it = std_multiset.insert(T{});
    auto it2 = tmi_multiset_view.insert(foo);

    assert(it != std_multiset.end());
    assert(it2 != tmi_multiset_view.end());
}

template <typename T>
void CompileTest<T>::test_find()
{
        auto it = std_multiset.find(T{});
        auto it2 = tmi_multiset_view.find(T{});
        assert(*it == *it2);
}

template <typename T>
void CompileTest<T>::test_erase()
{
        auto it = std_multiset.erase(std_multiset.begin());
        auto it2 = tmi_multiset_view.erase(tmi_multiset_view.begin());
        assert(it == std_multiset.end());
        assert(it2 == tmi_multiset_view.end());
}

template <typename T>
void CompileTest<T>::test_emplace()
{
        auto it = std_multiset.emplace(T{});
        const auto it2 = tmi_multiset_view.emplace(T{});

        assert(it != std_multiset.end());
        assert(*it2 == T{});
        assert(it2 != tmi_multiset_view.end());
}

template <typename T>
void CompileTest<T>::test_emplace_hint()
{
        //auto it = std_multiset.emplace_hint(std_multiset.begin(), T{});
        //auto it2 = tmi_multiset_view.emplace_hint(tmi_multiset_view.begin());

        //assert(*it != *it2);
}

template <typename T>
void CompileTest<T>::test_extract()
{
        auto nh = std_multiset.extract(std_multiset.begin());
        auto nh2 = tmi_multiset_view.extract(tmi_multiset_view.begin());
        assert(nh.value() == nh2.value());
        assert(nh.empty() == nh2.empty());
        assert(nh.get_allocator() == std_multiset.get_allocator());
        assert(nh2.get_allocator() == tmi_multiset_view.get_allocator());

        auto it = std_multiset.insert(std::move(nh));
        auto it2 = tmi_multiset_view.insert(std::move(nh2));
        assert(it != std_multiset.end());
        assert(it2 != tmi_multiset_view.end());

/*
        auto nh = std_multiset.extract(T{});
        auto nh2 = tmi_multiset_view.extract(T{});
        assert(nh.value() == nh2.value());
        assert(nh.empty() == nh2.empty());
        assert(nh.get_allocator() == std_multiset.get_allocator());
        assert(nh2.get_allocator() == tmi_multiset_view.get_allocator());

        auto it = std_multiset.insert(std_multiset.begin(), std::move(nh));
        auto ret2 = tmi_multiset_view.insert(tmi_multiset_view.begin(), std::move(nh2));
        assert(it != std_multiset.end());
        assert(ret2.inserted);
        assert(*ret2.position == *it);
        assert(ret2.node.empty());
*/

}

template <typename T>
void CompileTest<T>::test_empty()
{
        assert(std_multiset.empty() == tmi_multiset_view.empty());
}

template <typename T>
void CompileTest<T>::test_size()
{
        assert(std_multiset.size() == tmi_multiset_view.size());
}

template <typename T>
void CompileTest<T>::test_count()
{
        assert(std_multiset.count(T{}) == tmi_multiset_view.count(T{}));
}

template <typename T>
void CompileTest<T>::test_modify()
{
        tmi::multi_index_container<T,tmi::indexed_by<tmi::ordered_non_unique<tmi::identity<T>>>> container;
        auto& view = container.template get<0>();
        const T old_value{};
        const T new_value{modified_value<T>()};
        view.insert(old_value);

        auto it = view.find(old_value);
        assert(it != view.end());
        assert(view.modify(it, [&new_value](T& value) { value = new_value; }));
        assert(view.size() == 1);
        assert(view.find(old_value) == view.end());
        assert(view.find(new_value) != view.end());
}

template <typename T>
void CompileTest<T>::test_clear()
{
        std_multiset.clear();
        tmi_multiset_view.clear();
        assert(std_multiset.size() == tmi_multiset_view.size());
        assert(std_multiset.empty() == tmi_multiset_view.empty());
        assert(tmi_multiset_view.begin() == tmi_multiset_view.end());
}

} // anonymous namespace

void test_ordered_non_unique()
{
    CompileTest<int> tester;
    CompileTest<my_class> tester2;
}
