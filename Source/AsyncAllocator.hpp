/*
 * Author: Landon Fuller <landonf@plausible.coop>
 *
 * Copyright (c) 2013 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef PLCRASH_ASYNC_ALLOCATOR_H
#define PLCRASH_ASYNC_ALLOCATOR_H


#include <unistd.h>
#include <stdint.h>

#include "PLCrashAsync.h"

/**
 * @internal
 * @ingroup plcrash_async
 *
 * Implements async-safe memory allocation
 *
 * @{
 */



namespace plcrash { namespace async {

/* Forward declaration */
class AsyncAllocator;

/**
 * @internal
 *
 * Provides new and delete operators that may only be used in combination with
 * an async-safe allocator.
 */
class AsyncAllocatable {
public:
    void *operator new (size_t size, AsyncAllocator &allocator);
    void *operator new[] (size_t size, AsyncAllocator &allocator);
    
    void operator delete (void *ptr, size_t size);
    void operator delete[] (void *ptr, size_t size);

private:
    /* Disallow non-async-safe allocation entirely. */
    void *operator new (size_t);
    void *operator new[] (size_t);
};


/**
 * @internal
 *
 * An async-safe page-guarded and locking memory pool allocator. The allocator
 * will allocate itself within the target pages, ensuring that its own metadata is
 * guarded.
 */
class AsyncAllocator {
public:
    /**
     * Initialization options for AsyncAllocator.
     */
    typedef enum {
        /**
         * Enable a low guard page. This will insert a PROT_NONE page prior to the
         * allocatable region, helping to ensure that a buffer overflow that occurs elsewhere
         * in the code will not overwrite the allocatable space.
         */
        GuardLowPage = 1 << 0,
        
        /**
         * Enable a high guard page. This will insert a PROT_NONE page after to the
         * allocatable region, helping to ensure that a buffer overflow (including a stack overflow)
         * will be immediately detected. */
        GuardHighPage = 1 << 1,
    } AsyncAllocatorOption;
    
    static plcrash_error_t Create (AsyncAllocator **allocator, size_t initial_size, uint32_t options);
    ~AsyncAllocator ();
    void operator delete (void *ptr, size_t size);

    plcrash_error_t alloc (void **allocated, size_t size);
    void dealloc (void *ptr);
    
    /* An allocation instance may not be copied or moved; all access must be performed through the pointer returned
     * via Create(). */
    AsyncAllocator (const AsyncAllocator &other) = delete;
    AsyncAllocator (AsyncAllocator &&other) = delete;
    AsyncAllocator operator= (const AsyncAllocator &other) = delete;
    AsyncAllocator &operator= (AsyncAllocator &&other) = delete;

private:
    AsyncAllocator (vm_address_t base_page, vm_size_t total_size, vm_address_t usable_page, vm_size_t usable_size, vm_address_t next_addr);

    /** The address base of the allocation. */
    vm_address_t _base_page;
    
    /** The total size of the allocation, including guard pages. */
    vm_size_t _total_size;
    
    /** The base address of the first usable page of the allocation */
    vm_address_t _usable_page;
    
    /** The usable size of the allocation (ie, total size, minus guard pages.) */
    vm_size_t _usable_size;
    
    /** The next valid allocation address. Note that this must be kept page-aligned. */
    vm_address_t _next_addr;
};
    
}}

/**
 * @}
 */

#endif /* PLCRASH_ASYNC_ALLOCATOR_H */
