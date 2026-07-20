// Copyright (c) 2026 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tmi.h>

#include <test/iterator_to.h>

#include <set>

namespace
{

struct ordered_unique;
struct ordered_nonunique;
struct hash_unique;
struct hash_nonunique;

void test_iterator_to_compile()
{

        tmi::multi_index_container<std::string,
            tmi::indexed_by<
                tmi::hashed_unique<tmi::tag<hash_unique>, tmi::identity<std::string>>,
                tmi::hashed_non_unique<tmi::tag<hash_nonunique>, tmi::identity<std::string>>,
                tmi::ordered_unique<tmi::tag<ordered_unique>, tmi::identity<std::string>>,
                tmi::ordered_non_unique<tmi::tag<ordered_nonunique>, tmi::identity<std::string>>
            >
        > foo;

        [[maybe_unused]] auto it = foo.get<ordered_nonunique>().iterator_to(*foo.get<hash_unique>().begin());
}

} // anonymous namespace

void test_iterator_to()
{
    test_iterator_to_compile();
}
