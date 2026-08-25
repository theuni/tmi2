// Copyright (c) 2026 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMI_BUCKET_LIST_H_
#define TMI_BUCKET_LIST_H_

#include <memory>
#include <algorithm>

#include <cassert>

namespace tmi
{
    template <typename Node, typename Allocator>
    class bucket_list
    {
    public:
        using allocator_type = Allocator;
        using bucket_allocator_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<Node*>;
        using bucket_pointer_type = std::allocator_traits<bucket_allocator_type>::pointer;
        using size_type = std::allocator_traits<bucket_allocator_type>::size_type;
        using value_type = std::allocator_traits<bucket_allocator_type>::value_type;
        using difference_type = std::allocator_traits<bucket_allocator_type>::difference_type;

    private:
        friend class buckets_allocator;

        allocator_type& m_alloc;
        bucket_pointer_type m_ptr{nullptr};
        size_type m_size{0};

        void allocate(size_type size)
        {
            assert(!m_size);
            if (size) {
                bucket_allocator_type alloc{m_alloc};
                m_ptr = std::allocator_traits<bucket_allocator_type>::allocate(alloc, size);
                m_size = size;
                std::uninitialized_value_construct(begin(), end());
            }
        }

    public:

        ~bucket_list() noexcept
        {
            clear();
        }

        bucket_list(size_type buckets, allocator_type& alloc) noexcept : m_alloc{alloc}
        {
            allocate(buckets);
        }

        bucket_list(allocator_type& alloc) noexcept : m_alloc{alloc}
        {
        }
        bucket_list(bucket_list&& rhs) noexcept : m_alloc{rhs.m_alloc}, m_ptr{rhs.m_ptr}, m_size{rhs.m_size}
        {
            rhs.m_size = 0;
            rhs.m_ptr = nullptr;
        }
        bucket_list& operator=(bucket_list&& rhs) noexcept
        {
            clear();
            m_ptr = rhs.m_ptr;
            m_size = rhs.m_size;
            rhs.m_size = 0;
            rhs.m_ptr = nullptr;
            return *this;
        }
        void clear()
        {
            if (m_size) {
                bucket_allocator_type alloc{m_alloc};
                std::destroy(begin(), end());
                std::allocator_traits<bucket_allocator_type>::deallocate(alloc, m_ptr, m_size);
                m_size = 0;
            }
            m_ptr = nullptr;
        }
        void resize(size_type buckets)
        {
            clear();
            allocate(buckets);
        }
        void swap(bucket_list& rhs)
        {
            std::swap(m_size, rhs.m_size);
            std::swap(m_ptr, rhs.m_ptr);
        }
        value_type* begin() const
        {
            return std::to_address(m_ptr);
        }
        value_type* end() const
        {
            assert(m_size <= std::numeric_limits<difference_type>::max());
            return std::to_address(m_ptr + static_cast<difference_type>(m_size));
        }
        size_type size() const
        {
            return m_size;
        }
    };

} // namespace tmi
#endif // TMI_BUCKET_LIST_H_
