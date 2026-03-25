#pragma once
// Compatibility shim for building PostgreSQL extensions with MSVC.
//
// VS2017's intrin.h only declares _Interlocked*64 functions with signed
// (volatile __int64 *) pointer arguments. PostgreSQL's port/atomics/generic-msvc.h
// passes volatile uint64 * (i.e. volatile unsigned __int64 *) without a cast,
// which causes C2664 in C++ strict mode.
//
// VS2019+ added unsigned overloads. This file back-fills them for VS2017 and
// earlier (_MSC_VER < 1920). We provide C++ overloads rather than macros to
// avoid interfering with the intrinsic pragma mechanism.

#if defined(_MSC_VER) && _MSC_VER < 1920 && defined(__cplusplus)

#include <intrin.h>

static __forceinline __int64
_InterlockedCompareExchange64(volatile unsigned __int64 *Dest,
                              unsigned __int64 ExChange,
                              unsigned __int64 Comperand)
{
    return ::_InterlockedCompareExchange64(
        reinterpret_cast<volatile __int64 *>(Dest),
        static_cast<__int64>(ExChange),
        static_cast<__int64>(Comperand));
}

static __forceinline __int64
_InterlockedExchange64(volatile unsigned __int64 *Target, unsigned __int64 Value)
{
    return ::_InterlockedExchange64(
        reinterpret_cast<volatile __int64 *>(Target),
        static_cast<__int64>(Value));
}

static __forceinline __int64
_InterlockedExchangeAdd64(volatile unsigned __int64 *Addend, __int64 Value)
{
    return ::_InterlockedExchangeAdd64(
        reinterpret_cast<volatile __int64 *>(Addend), Value);
}

#endif // _MSC_VER < 1920

// When building with -D_CRT_DECLARE_NONSTDC_NAMES=0 (required to prevent
// struct stat redefinition between UCRT and PostgreSQL's win32_port.h),
// UCRT's <sys/types.h> omits the POSIX-name typedef for off_t.
// PostgreSQL's win32_port.h uses off_t unconditionally in extern declarations.
// Include <sys/types.h> here to get _off_t, then provide the off_t alias.
#if defined(_MSC_VER) && defined(_CRT_DECLARE_NONSTDC_NAMES) && !_CRT_DECLARE_NONSTDC_NAMES
#include <sys/types.h>
typedef _off_t off_t;

// Similarly, strdup is not declared as 'strdup' when NONSTDC names are
// suppressed; map it to the underscore-prefixed MSVC intrinsic.
#ifndef strdup
#define strdup _strdup
#endif
#endif

#ifdef _MSC_VER
// Pre-define FD_H to prevent the real storage/fd.h from being compiled.
// PostgreSQL 17's storage/fd.h defines FileRead() and FileWrite() as static
// inline functions using C99 designated initializers ({.field = value}),
// which MSVC rejects in C++17 mode.  Because this file is force-included
// (via -FI) before any source-level #include directives are processed,
// defining FD_H here means that any later #include "storage/fd.h" — even
// one resolved via MSVC's "search the directory of the including header"
// rule that would find the real fd.h before our include/storage/fd.h shim —
// becomes a no-op.
//
// The only type that PG headers (fileset.h etc.) need from fd.h is:
//   typedef int File;
// We provide it here.  The full shim at include/storage/fd.h adds the
// remaining declarations when found via the normal include-path search.
#ifndef FD_H
#define FD_H
#ifndef PGDUCKDB_FD_FILE_TYPEDEF
#define PGDUCKDB_FD_FILE_TYPEDEF
typedef int File;
#endif
#endif
#endif // _MSC_VER
