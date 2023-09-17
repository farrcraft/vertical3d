/**
* Voxel Engine
*
* (c) Joshua Farr <j.wgasa@gmail.com>
*
*/

#pragma once

#include <memory>

/**
 * Source: http://stackoverflow.com/questions/16376942/best-cross-platform-method-to-get-aligned-memory
 */
template<std::size_t Alignment>
class Aligned
{
public:
    void* operator new(std::size_t size)
    {
        std::size_t space = size + (Alignment - 1);
        void *ptr = malloc(space + sizeof(void*));
        void *original_ptr = ptr;

        char *ptr_bytes = static_cast<char*>(ptr);
        ptr_bytes += sizeof(void*);
        ptr = static_cast<void*>(ptr_bytes);

        ptr = std::align(Alignment, size, ptr, space);

        ptr_bytes = static_cast<char*>(ptr);
        ptr_bytes -= sizeof(void*);
        std::memcpy(ptr_bytes, &original_ptr, sizeof(void*));

        return ptr;
    }

    void operator delete(void* ptr)
    {
        char *ptr_bytes = static_cast<char*>(ptr);
        ptr_bytes -= sizeof(void*);

        void *original_ptr;
        std::memcpy(&original_ptr, ptr_bytes, sizeof(void*));

        std::free(original_ptr);
    }
};