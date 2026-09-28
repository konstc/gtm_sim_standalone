// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - zero-initializing global operator new.
//
// The prebuilt GTM libraries contain heap objects with members that are never
// initialized, e.g. the internal state of the hres8_delay module in the ATOM
// output path. Whether the outputs take their reset level then depends on
// leftover heap contents, and results vary with the environment size,
// parallel runs, the platform, etc. (see docs/ABI_NOTES.md).
//
// Replacing the global allocation functions with zero-filling versions makes
// every such object start from zero - the behaviour of fresh memory and of
// the configured reset levels - and the simulation deterministic. The default
// operator delete (free) matches calloc.
//
// This file is compiled into every executable that links gtm::gtm (option
// GTM_ZERO_INIT_HEAP), so the replacement is always part of the link.
#include <cstdlib>
#include <new>

namespace
{
void* zero_initialized_alloc(std::size_t size)
{
    if (size == 0) size = 1;
    for (;;)
    {
        if (void* p = std::calloc(1, size)) return p;
        std::new_handler handler = std::get_new_handler();
        if (!handler) throw std::bad_alloc();
        handler();
    }
}

void* zero_initialized_alloc_nothrow(std::size_t size) noexcept
{
    try
    {
        return zero_initialized_alloc(size);
    }
    catch (...)
    {
        return nullptr;
    }
}
} // namespace

void* operator new(std::size_t size) { return zero_initialized_alloc(size); }
void* operator new[](std::size_t size) { return zero_initialized_alloc(size); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept { return zero_initialized_alloc_nothrow(size); }
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept { return zero_initialized_alloc_nothrow(size); }
