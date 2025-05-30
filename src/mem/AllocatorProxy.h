/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_MEM_ALLOCATORPROXY_H
#define SQUID_SRC_MEM_ALLOCATORPROXY_H

// XXX: remove AllocatorProxy.h include from mem/forward.h
namespace Mem {
class Allocator;
class PoolStats;
}

/**
 * \hideinitializer
 *
 * Pool and account the memory used for the CLASS object.
 * This macro is intended for use within the declaration of a class.
 *
 * The memory block allocated by operator new is not zeroed; it is the
 * responsibility of users to ensure that constructors correctly
 * initialize all data members.
 */
#define MEMPROXY_CLASS(CLASS) \
    private: \
    static inline Mem::AllocatorProxy &Pool() { \
        static Mem::AllocatorProxy thePool(#CLASS, sizeof(CLASS), false); \
        return thePool; \
    } \
    public: \
    void *operator new(size_t byteCount) { \
        /* derived classes with different sizes must implement their own new */ \
        assert(byteCount == sizeof(CLASS)); \
        return Pool().alloc(); \
    } \
    void operator delete(void *address) { \
        if (address) \
            Pool().freeOne(address); \
    } \
    static int UseCount() { return Pool().inUseCount(); } \
    private:

namespace Mem
{

class PoolMeter;

/**
 * Support late binding of pool type for allocator agnostic classes
 */
class AllocatorProxy
{
public:
    AllocatorProxy(char const *aLabel, size_t const &aSize, bool doZeroBlocks = true):
        label(aLabel),
        size(aSize),
        theAllocator(nullptr),
        doZero(doZeroBlocks)
    {}

    /// Allocate one element from the pool
    void *alloc();

    /// Free a element allocated by Mem::AllocatorProxy::alloc()
    void freeOne(void *);

    int inUseCount() const;
    size_t objectSize() const {return size;}
    char const * objectType() const {return label;}

    PoolMeter const &getMeter() const;

    /**
     * \param stats Object to be filled with statistical data about pool.
     * \retval      Number of objects in use, ie. allocated.
     */
    size_t getStats(PoolStats &stats);

    void zeroBlocks(bool doIt);

    /// \copydoc Mem::Allocator::relabel()
    void relabel(const char * const aLabel);

private:
    Allocator *getAllocator() const;

    const char *label;
    size_t size;
    mutable Allocator *theAllocator;
    bool doZero;
};

} // namespace Mem

#endif /* SQUID_SRC_MEM_ALLOCATORPROXY_H */

