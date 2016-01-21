/*!
    \file spsc_ring_buffer.h
    \brief Single producer / single consumer wait-free ring buffer class definition
    \author Ivan Shynkarenka
    \date 16.01.2016
    \copyright MIT License
*/

#ifndef CPPCOMMON_SPSC_RING_BUFFER_H
#define CPPCOMMON_SPSC_RING_BUFFER_H

#include <atomic>
#include <cassert>

namespace CppCommon {

//! Single producer / single consumer wait-free ring buffer
/*!
    Single producer / single consumer wait-free ring buffer use only atomic operations to provide thread-safe enqueue
    and dequeue operations. Ring buffer is bounded to the fixed capacity provided in the constructor.

    A combination of the algorithms described by the circular buffers documentation found in the Linux kernel, and the
    bounded MPMC queue by Dmitry Vyukov. Implemented in pure C++11. Should work across most CPU architectures.
    http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
*/
class SPSCRingBuffer
{
public:
    //! Default class constructor
    /*!
        \param capacity - ring buffer capacity (must be a power of two)
    */
    explicit SPSCRingBuffer(int64_t capacity);
    SPSCRingBuffer(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer(SPSCRingBuffer&&) = default;
    ~SPSCRingBuffer() { delete[] _buffer; }

    SPSCRingBuffer& operator=(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer& operator=(SPSCRingBuffer&&) = default;

    //! Get ring buffer capacity in bytes
    int64_t capacity() const { return _capacity; }
    //! Get ring buffer size in bytes
    int64_t size() const;

    //! Enqueue a chunk of bytes into the ring buffer (single producer thread method)
    /*!
        The chunk of bytes will be copied into the ring buffer using 'memcpy()' function.

        \param chunk - chunk buffer to enqueue
        \param size - chunk buffer size
        \return 'true' if the chunk of bytes was successfully enqueue, 'false' if the ring buffer is full
    */
    bool Enqueue(const void* chunk, int64_t size);

    //! Dequeue a chunk of bytes from the ring buffer (single consumer thread method)
    /*!
        The chunk of bytes will be copied from the ring buffer using 'memcpy()' function.

        \param chunk - chunk buffer to dequeue
        \param size - chunk buffer size
        \return 'true' if the chunk of bytes was successfully dequeue, 'false' if the ring buffer is empty
    */
    bool Dequeue(void* chunk, int64_t& size);

private:
    typedef char cache_line_pad[64];

    cache_line_pad _pad0;
    const int64_t _capacity;
    const int64_t _mask;
    char* const _buffer;

    cache_line_pad _pad1;
    std::atomic<int64_t> _head;

    cache_line_pad _pad2;
    std::atomic<int64_t> _tail;
};

} // namespace CppCommon

#include "spsc_ring_buffer.inl"

#endif //CPPCOMMON_SPSC_RING_BUFFER_H