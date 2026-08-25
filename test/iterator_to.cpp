// Copyright (c) 2026 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tmi.h>

#include <test/iterator_to.h>

#include <set>

struct myclass {
    std::string val;
    myclass(size_t rhs) : val(std::to_string(rhs)) {}
};

class compare_myclass_less
{
public:
    TMI_CPP23_STATIC bool operator()(const myclass& a, const myclass& b) TMI_CONST_IF_NOT_CPP23_STATIC
    {
        return std::stol(a.val) < std::stol(b.val);
    }
    TMI_CPP23_STATIC bool operator()(const myclass* a, const myclass* b) TMI_CONST_IF_NOT_CPP23_STATIC
    {
        return std::stol(a->val) < std::stol(b->val);
    }
};

class compare_myclass_greater
{
public:
    TMI_CPP23_STATIC bool operator()(const myclass& a, const myclass& b) TMI_CONST_IF_NOT_CPP23_STATIC
    {
        return std::stol(a.val) > std::stol(b.val);
    }
    TMI_CPP23_STATIC bool operator()(const myclass* a, const myclass* b) TMI_CONST_IF_NOT_CPP23_STATIC
    {
        return std::stol(a->val) > std::stol(b->val);
    }
};

class myclass_key_from_value
{
public:
    using result_type = std::string;
    TMI_CPP23_STATIC constexpr const result_type& operator()(const myclass& a) TMI_CONST_IF_NOT_CPP23_STATIC noexcept
    {
        return a.val;
    }
    TMI_CPP23_STATIC constexpr const result_type& operator()(const myclass* a) TMI_CONST_IF_NOT_CPP23_STATIC noexcept
    {
        return a->val;
    }
};

class myclass_hash
{
public:

    TMI_CPP23_STATIC size_t operator()(const myclass_key_from_value::result_type& a) TMI_CONST_IF_NOT_CPP23_STATIC noexcept
    {
        return std::hash<std::string>{}(a);
    }
};

class myclass_pred
{
public:

    TMI_CPP23_STATIC constexpr bool operator()(const myclass_key_from_value::result_type& a, const myclass_key_from_value::result_type& b) TMI_CONST_IF_NOT_CPP23_STATIC noexcept
    {
        return a == b;
    }

};

namespace {

struct hash_unique;
struct hash_nonunique;
struct comp_less;
struct comp_greater;

void test_iterator_to_compile()
{

        tmi::multi_index_container<myclass,tmi::indexed_by<tmi::hashed_unique<tmi::tag<hash_unique>, myclass_key_from_value, myclass_hash, myclass_pred>,
                         tmi::ordered_unique<tmi::tag<comp_less>, tmi::identity<myclass>, compare_myclass_less>,
                         tmi::ordered_unique<tmi::tag<comp_greater>, tmi::identity<myclass>, compare_myclass_greater>>,
                         std::allocator<myclass>> foo{std::make_tuple(std::make_tuple(32UL, myclass_key_from_value(), myclass_hash(), myclass_pred()), std::make_tuple(tmi::identity<myclass>(), compare_myclass_less()), std::make_tuple(tmi::identity<myclass>
(), compare_myclass_greater()))};

        [[maybe_unused]] auto it = foo.get<comp_less>().iterator_to(*foo.get<hash_unique>().begin());
}

} // anonymous namespace

void test_iterator_to()
{
    test_iterator_to_compile();
}
