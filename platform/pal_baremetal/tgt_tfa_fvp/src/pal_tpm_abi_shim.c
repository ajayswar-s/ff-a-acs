/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#undef SetMem
#undef ZeroMem
#undef CopyMem


#define TPM_ABI_HIDDEN __attribute__((visibility("hidden")))

TPM_ABI_HIDDEN const uint64_t _gPcd_FixedAtBuild_PcdTpmBaseAddress = 0ULL;
TPM_ABI_HIDDEN const uint64_t _gPcd_FixedAtBuild_PcdTpmSecureCrbBase = 0ULL;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PcdControlFlowEnforcementPropertyMask = 0U;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PcdVerifyNodeInList = 0U;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PcdMaximumLinkedListLength = 1000000U;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PcdMaximumAsciiStringLength = 1000000U;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PcdMaximumUnicodeStringLength = 1000000U;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PcdSpeculationBarrierType = 1U;
TPM_ABI_HIDDEN const uint16_t _gPcd_FixedAtBuild_PcdUartDefaultReceiveFifoDepth = 1U;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PcdSerialBaudRate = 115200U;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PL011UartInteger = 0U;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PL011UartFractional = 0U;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PL011UartRegOffsetVariant = 0U;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PL011UartClkInHz = 24000000U;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PcdDebugPrintErrorLevel = 0x8000008FU;
TPM_ABI_HIDDEN const uint64_t _gPcd_FixedAtBuild_PcdSerialRegisterBase = 0x1c090000ULL;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PcdSerialUseHardwareFlowControl = 0U;
TPM_ABI_HIDDEN const uint64_t _gPcd_FixedAtBuild_PcdUartDefaultBaudRate = 115200ULL;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PcdUartDefaultDataBits = 8U;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PcdUartDefaultParity = 1U;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PcdUartDefaultStopBits = 1U;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PcdDebugClearMemoryValue = 0xAFU;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PcdDebugPropertyMask = 0x2FU;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PcdFixedDebugPrintErrorLevel = 0xFFFFFFFFU;
TPM_ABI_HIDDEN const uint32_t _gPcd_FixedAtBuild_PcdSpinLockTimeout = 10000000U;
TPM_ABI_HIDDEN const uint64_t _gPcd_FixedAtBuild_PcdTpmNvMemoryBase = 0x0FF80000ULL;
TPM_ABI_HIDDEN const uint64_t _gPcd_FixedAtBuild_PcdTpmNvMemorySize = 0x4000ULL;
TPM_ABI_HIDDEN const uint8_t _gPcd_FixedAtBuild_PcdTpmEmuNvMemory = 0U;
TPM_ABI_HIDDEN volatile uint64_t _gPcd_BinaryPatch_PcdTpmUniqueValue = 0x12345678ULL;
TPM_ABI_HIDDEN uintptr_t _gPcd_BinaryPatch_Size_PcdTpmUniqueValue = 8U;

TPM_ABI_HIDDEN char *gEfiCallerBaseName = "AcsTpmSp";

__attribute__((visibility("hidden"))) uintptr_t __stack_chk_guard =
    (uintptr_t)0x41435354504d5350ULL;

void __attribute__((noreturn)) __stack_chk_fail(void)
{
    for (;;)
    {
    }
}

void *SetMem(void *buffer, size_t length, uint8_t value)
{
    return memset(buffer, (int)value, length);
}

void *ZeroMem(void *buffer, size_t length)
{
    return memset(buffer, 0, length);
}

void *CopyMem(void *destination, const void *source, size_t length)
{
    return memcpy(destination, source, length);
}

int CompareMem(const void *first, const void *second, size_t length)
{
    return memcmp(first, second, length);
}

void *memmove(void *destination, const void *source, size_t length)
{
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;

    if ((dst == src) || (length == 0U))
    {
        return destination;
    }

    if (dst < src)
    {
        for (size_t index = 0U; index < length; index++)
        {
            dst[index] = src[index];
        }
    }
    else
    {
        for (size_t index = length; index > 0U; index--)
        {
            dst[index - 1U] = src[index - 1U];
        }
    }

    return destination;
}


uint8_t DebugPrintEnabled(void)
{
    return 0U;
}

uint8_t DebugPrintLevelEnabled(uint64_t error_level)
{
    (void)error_level;
    return 0U;
}

void DebugPrint(uint64_t error_level, const char *format, ...)
{
    (void)error_level;
    (void)format;
}

uint8_t DebugAssertEnabled(void)
{
    return 0U;
}

void DebugAssert(const char *file_name, uint32_t line_number, const char *description)
{
    (void)file_name;
    (void)line_number;
    (void)description;
}

void CpuDeadLoop(void)
{
    for (;;)
    {
    }
}

void *ScanMem8(const void *buffer, size_t length, uint8_t value)
{
    const uint8_t *bytes = (const uint8_t *)buffer;

    for (size_t index = 0U; index < length; index++)
    {
        if (bytes[index] == value)
        {
            return (void *)&bytes[index];
        }
    }

    return NULL;
}


size_t SerialPortWrite(uint8_t *buffer, size_t number_of_bytes)
{
    if (buffer == NULL)
    {
        return 0U;
    }

    return number_of_bytes;
}


uint32_t __aarch64_ldadd4_relax(uint32_t value, uint32_t *ptr)
{
    uint32_t old;
    uint32_t new_value;
    uint32_t status;

    if (ptr == NULL)
    {
        return 0U;
    }

    __asm__ volatile (
        "1: ldxr %w0, [%3]\n"
        "add %w1, %w0, %w4\n"
        "stxr %w2, %w1, [%3]\n"
        "cbnz %w2, 1b\n"
        : "=&r" (old), "=&r" (new_value), "=&r" (status)
        : "r" (ptr), "r" (value)
        : "memory");

    return old;
}

__int128 __ashlti3(__int128 value, int shift)
{
    if ((shift < 0) || (shift >= 128))
    {
        return 0;
    }

    return (__int128)(((unsigned __int128)value) << shift);
}
