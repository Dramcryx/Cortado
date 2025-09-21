/// @file UniquePtrOverArch.h
/// A shared_ptr-like structure for sharing an object
/// based on atomic refcount.
///

#ifndef CORTADO_DETAIL_UNIQUE_PTR_OVER_ARC_H
#define CORTADO_DETAIL_UNIQUE_PTR_OVER_ARC_H

// Cortado
//
#include <Cortado/Concepts/CoroutineAllocator.h>
#include <Cortado/Detail/AtomicRefCount.h>

// STL
//
#include <memory>

namespace Cortado::Detail
{

/// @brief Composition of refcount and object to hold.
/// @tparam HeldT Type that we want to store.
/// @tparam AtomicT Implementation of atomic primitive.
///
template <typename HeldT, Concepts::Atomic AtomicT>
struct RefCountedObject : AtomicRefCount<AtomicT>
{
    /// @brief unique_ptr contract. Call delete only if refcount reaches zero.
    ///
    struct Deleter
    {
        void operator()(RefCountedObject *self)
        {
            if (self->Release() == 0)
            {
                ::delete self;
            }
        }
    };

    HeldT Object;
};

/// @brief Copyable unique_ptr holder which exposes HeldT.
/// @tparam HeldT Type that we want to store.
/// @tparam AtomicT Implementation of atomic primitive.
///
template <typename HeldT, Concepts::Atomic AtomicT>
struct UniquePtrOverArc
{
    /// @brief Construct HeldT from args, allocating unique_ptr with given
    /// allcoator.
    /// @tparam Alloc An allocator to use.
    /// @tparam Args HeldT's constructor args.
    ///
    template <Concepts::CoroutineAllocator Alloc, typename... Args>
    UniquePtrOverArc(Alloc &alloc, Args &&...args)
    {
        std::unique_ptr<std::byte> bytes{reinterpret_cast<std::byte *>(
            alloc.allocate(sizeof(RefCountedObjectT)))};
        ::new (bytes.get()) RefCountedObjectT{};
        m_ptr.reset(reinterpret_cast<RefCountedObjectT *>(bytes.release()));
    }

    /// @brief Copy constructor. Take other's pointer and increment refcount.
    ///
    UniquePtrOverArc(const UniquePtrOverArc &other)
    {
        m_ptr.reset(other.m_ptr.get());
        m_ptr->AddRef();
    }

    /// @brief Copy assignment oeprator. Take other's pointer and increment
    /// refcount.
    ///
    UniquePtrOverArc &operator=(const UniquePtrOverArc &other)
    {
        m_ptr.reset(other.m_ptr.get());
        m_ptr->AddRef();
        return *this;
    }

    /// @brief Default-movable.
    ///
    UniquePtrOverArc(UniquePtrOverArc &&other) noexcept = default;

    /// @brief Default-movable.
    ///
    UniquePtrOverArc &operator=(UniquePtrOverArc &&other) noexcept = default;

    /// @brief Default destructor.
    ///
    ~UniquePtrOverArc() = default;

    /// @brief Operator to expose HeldT.
    ///
    HeldT *operator->() const
    {
        return &m_ptr->Object;
    }

private:
    using RefCountedObjectT = RefCountedObject<HeldT, AtomicT>;
    std::unique_ptr<RefCountedObjectT, typename RefCountedObjectT::Deleter>
        m_ptr{nullptr};
};

} // namespace Cortado::Detail

#endif // CORTADO_DETAIL_UNIQUE_PTR_OVER_ARC_H
