// Copyright (c) 2024 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TMI_NODEHANDLE_H_
#define TMI_NODEHANDLE_H_

#include <memory>
#include <optional>

namespace tmi {
namespace detail {

template <typename Allocator, typename Node>
class node_handle
{
    using node_type = Node;
    using allocator_type = Allocator;
    using node_allocator_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<node_type>;
    using value_type = typename Node::value_type;
    using node_pointer = std::allocator_traits<node_allocator_type>::pointer;
    std::optional<node_allocator_type> m_alloc{};
    node_type* m_node = nullptr;

    void destroy()
    {
        if (m_node) {
            node_pointer ptr = std::pointer_traits<node_pointer>::pointer_to(*m_node);
            std::allocator_traits<node_allocator_type>::destroy(*m_alloc, std::addressof(m_node->value()));
            std::destroy_at(std::to_address(ptr));
            std::allocator_traits<node_allocator_type>::deallocate(*m_alloc, ptr, 1);
            m_node = nullptr;
        }
    }

public:
    constexpr node_handle(const node_allocator_type& alloc, node_type* node) noexcept : m_alloc(alloc), m_node(node){}
    constexpr node_handle() noexcept = default;
    node_handle(node_handle&& rhs) noexcept : m_alloc(std::move(rhs.m_alloc)), m_node(rhs.m_node)
    {
        rhs.m_node = nullptr;
        rhs.m_alloc.reset();
    }
    node_handle& operator=(node_handle&& rhs)
    {
        destroy();
        m_node = rhs.m_node;
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_move_assignment)
        {
            m_alloc = std::move(rhs.m_alloc);
        } else if (!m_alloc) {
            m_alloc = std::move(rhs.m_alloc);
        }
        rhs.m_node = nullptr;
        rhs.m_alloc.reset();
        return *this;
    }
    ~node_handle()
    {
        destroy();
    }
    bool empty() const noexcept
    {
        return m_node == nullptr;
    }
    explicit operator bool() const noexcept
    {
        return m_node != nullptr;
    }
    allocator_type get_allocator() const
    {
        return *m_alloc;
    }
    value_type& value() const
    {
        return m_node->value();
    }
    void swap(node_handle& rhs) noexcept(std::allocator_traits<allocator_type>::propagate_on_container_swap::value ||
                                std::allocator_traits<allocator_type>::is_always_equal::value)
    {
        std::swap(m_node, rhs.m_node);
        if constexpr(std::allocator_traits<allocator_type>::propagate_on_container_swap)
        {
            if (!empty() || !rhs.empty()) {
                std::swap(m_alloc, rhs.m_alloc);
            }
        } else if (empty() != rhs.empty()) {
            std::swap(m_alloc, rhs.m_alloc);
        }
    }
    /* extra function for tmi */
    node_type* get() const
    {
        return m_node;
    }
    /* extra function for tmi */
    void release()
    {
        m_node = nullptr;
    }
};

template<typename Iter, typename NodeType>
struct insert_return_type
{
    Iter position;
    bool inserted;
    NodeType node;
};

} // namespace detail
} // namespace tmi

#endif // TMI_NODEHANDLE_H_
