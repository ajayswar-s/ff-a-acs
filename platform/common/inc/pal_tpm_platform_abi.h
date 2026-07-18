/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef PAL_TPM_PLATFORM_ABI_H
#define PAL_TPM_PLATFORM_ABI_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define IN
#define OUT
#define OPTIONAL
#define CONST const
#define VOID void
#define EFIAPI
#define STATIC static
#define TRUE 1
#define FALSE 0

typedef uint8_t BOOLEAN;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;
typedef uintptr_t UINTN;
typedef intptr_t INTN;
typedef UINTN RETURN_STATUS;

#define MAX_INTN INTPTR_MAX
#define MAX_UINTN UINTPTR_MAX
#define RETURN_SUCCESS ((RETURN_STATUS)0)
#define EFI_SUCCESS RETURN_SUCCESS

void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *dst, int val, size_t count);

#ifndef CopyMem
#define CopyMem(dest, src, len) memcpy((dest), (src), (len))
#endif
#ifndef SetMem
#define SetMem(buf, len, value) memset((buf), (value), (len))
#endif
#ifndef ZeroMem
#define ZeroMem(buf, len) memset((buf), 0, (len))
#endif

#ifndef LIB_EXPORT
#define LIB_EXPORT
#endif

LIB_EXPORT void _plat__GetEPS(uint16_t size, uint8_t *endorsement_seed);
LIB_EXPORT void _plat__TimerReset(void);
LIB_EXPORT void _plat__TimerRestart(void);
LIB_EXPORT uint64_t _plat__RealTime(void);
LIB_EXPORT void _plat__NVDisable(void *plat_parameter, size_t param_size);
LIB_EXPORT void _plat__SetNvAvail(void);
LIB_EXPORT void _plat__ClearNvAvail(void);
LIB_EXPORT int _plat__NVNeedsManufacture(void);
LIB_EXPORT uint32_t _plat__GetUnique(uint32_t which, uint32_t b_size,
                                     unsigned char *buffer);
void _plat_internal_resetFailureData(void);

#endif /* PAL_TPM_PLATFORM_ABI_H */
