/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <TpmConfiguration/TpmBuildSwitches.h>
#include <CompilerDependencies.h>
#include <platform_interface/tpm_to_platform_interface.h>
#include <platform_interface/pcrstruct.h>
#include <tpm_public/TpmAlgorithmDefines.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef NV_MEMORY_SIZE
#define NV_MEMORY_SIZE 16384U
#endif

#define PAL_TPM_MANUFACTURER 0x41435320U /* "ACS " */
#define PAL_TPM_VENDOR_1     0x41435320U /* "ACS " */
#define PAL_TPM_VENDOR_2     0x6654504dU /* "fTPM" */
#define PAL_TPM_EPS_MAX_SIZE 32
#define PAL_TPM_ENTROPY_MAX_SIZE 64
#define PAL_TPM_UNIQUE_MAX_SIZE 32
#define PAL_TPM_SECRET_MAX_SIZE 32
#define PAL_TPM_MANUFACTURE_DATA_MAX_SIZE 32

UINT16 CryptHashGetDigestSize(TPM_ALG_ID hash_alg);
BOOLEAN GetRandomNumber64(UINT64 *rand);

static uint8_t g_tpm_nv[NV_MEMORY_SIZE];
static uint8_t g_tpm_nv_written[NV_MEMORY_SIZE];
static uint32_t g_tpm_nv_ready;
static uint32_t g_tpm_nv_needs_manufacture = 1U;
static uint64_t g_tpm_time_ms;
static uint32_t g_tpm_timer_reset;
static uint32_t g_tpm_timer_stopped;
static uint64_t g_tpm_entropy_word;
static uint32_t g_tpm_entropy_offset = sizeof(g_tpm_entropy_word);
static uint32_t g_tpm_failure_code;
static uint64_t g_tpm_failure_location;
static const char *g_tpm_failure_function;
static uint32_t g_tpm_failure_line;
static uint32_t g_tpm_in_failure_mode;

static int pal_tpm_next_entropy_byte(uint8_t *entropy_byte)
{
    if (entropy_byte == NULL)
    {
        return 0;
    }

    if (g_tpm_entropy_offset >= sizeof(g_tpm_entropy_word))
    {
        if (GetRandomNumber64(&g_tpm_entropy_word) == 0)
        {
            return 0;
        }
        g_tpm_entropy_offset = 0U;
    }

    *entropy_byte = (uint8_t)(g_tpm_entropy_word >> (g_tpm_entropy_offset++ * 8U));
    return 1;
}

LIB_EXPORT void _plat__GetEPS(uint16_t size, uint8_t *endorsement_seed)
{
    uint16_t i;
    uint8_t entropy_byte;

    if (endorsement_seed == NULL)
    {
        return;
    }

    if (size > PAL_TPM_EPS_MAX_SIZE)
    {
        size = PAL_TPM_EPS_MAX_SIZE;
    }

    for (i = 0U; i < size; i++)
    {
        if (pal_tpm_next_entropy_byte(&entropy_byte) == 0)
        {
            return;
        }
        endorsement_seed[i] = (uint8_t)(0xE0U ^ (uint8_t)i ^ entropy_byte);
    }
}

LIB_EXPORT void _plat__TimerReset(void)
{
    g_tpm_time_ms = 0U;
    g_tpm_timer_reset = 1U;
    g_tpm_timer_stopped = 0U;
}

LIB_EXPORT void _plat__TimerRestart(void)
{
    g_tpm_timer_stopped = 0U;
}

LIB_EXPORT uint64_t _plat__RealTime(void)
{
    g_tpm_time_ms += 1U;
    return g_tpm_time_ms;
}

LIB_EXPORT uint64_t _plat__TimerRead(void)
{
    g_tpm_time_ms += 1U;
    return g_tpm_time_ms;
}

LIB_EXPORT int _plat__TimerWasReset(void)
{
    int was_reset = (int)g_tpm_timer_reset;

    g_tpm_timer_reset = 0U;
    return was_reset;
}

LIB_EXPORT int _plat__TimerWasStopped(void)
{
    int was_stopped = (int)g_tpm_timer_stopped;

    g_tpm_timer_stopped = 0U;
    return was_stopped;
}

LIB_EXPORT void _plat__ClockRateAdjust(_plat__ClockAdjustStep adjust)
{
    (void)adjust;
}

LIB_EXPORT int32_t _plat__GetEntropy(unsigned char *entropy, uint32_t amount)
{
    uint32_t i;

    if ((entropy == NULL) || (amount > PAL_TPM_ENTROPY_MAX_SIZE))
    {
        return -1;
    }

    for (i = 0U; i < amount; i++)
    {
        if (pal_tpm_next_entropy_byte(&entropy[i]) == 0)
        {
            return -1;
        }
    }

    return (int32_t)amount;
}

LIB_EXPORT int _plat__NVEnable(void *plat_parameter, size_t param_size)
{
    (void)plat_parameter;
    (void)param_size;

    g_tpm_nv_ready = 1U;
    return 0;
}

LIB_EXPORT void _plat__NVDisable(void *plat_parameter, size_t param_size)
{
    (void)plat_parameter;
    (void)param_size;

    g_tpm_nv_ready = 0U;
}

LIB_EXPORT int _plat__GetNvReadyState(void)
{
    /* TCG TPM platform API uses 0 for NV_READY and non-zero for unavailable. */
    return (g_tpm_nv_ready != 0U) ? 0 : 1;
}

static int pal_tpm_nv_range_valid(unsigned int offset, unsigned int size)
{
    if (size == 0)
    {
        return 1;
    }

    if (offset >= NV_MEMORY_SIZE)
    {
        return 0;
    }

    return (size <= (NV_MEMORY_SIZE - offset)) ? 1 : 0;
}

LIB_EXPORT int _plat__NvMemoryRead(unsigned int start_offset, unsigned int size, void *data)
{
    if ((data == NULL) || (pal_tpm_nv_range_valid(start_offset, size) == 0))
    {
        return 0;
    }

    if (size == 0)
    {
        return 1;
    }

    memcpy(data, &g_tpm_nv[start_offset], size);
    return 1;
}

LIB_EXPORT int _plat__NvGetChangedStatus(unsigned int start_offset, unsigned int size, void *data)
{
    if ((data == NULL) || (pal_tpm_nv_range_valid(start_offset, size) == 0))
    {
        return -1;
    }

    if (size == 0)
    {
        return 0;
    }

    return (memcmp(&g_tpm_nv[start_offset], data, size) == 0) ? 0 : 1;
}

LIB_EXPORT int _plat__NvMemoryWrite(unsigned int start_offset, unsigned int size, void *data)
{
    if ((data == NULL) || (pal_tpm_nv_range_valid(start_offset, size) == 0))
    {
        return 0;
    }

    if (size == 0)
    {
        return 1;
    }

    memcpy(&g_tpm_nv[start_offset], data, size);
    memset(&g_tpm_nv_written[start_offset], 1, size);
    g_tpm_nv_needs_manufacture = 0U;
    return 1;
}

LIB_EXPORT int _plat__NvMemoryClear(unsigned int start_offset, unsigned int size)
{
    if (pal_tpm_nv_range_valid(start_offset, size) == 0)
    {
        return 0;
    }

    if (size == 0)
    {
        return 1;
    }

    memset(&g_tpm_nv[start_offset], 0xff, size);
    memset(&g_tpm_nv_written[start_offset], 1, size);
    g_tpm_nv_needs_manufacture = 1U;
    return 1;
}

LIB_EXPORT int _plat__NvMemoryMove(unsigned int source_offset,
                                   unsigned int dest_offset,
                                   unsigned int size)
{
    if ((pal_tpm_nv_range_valid(source_offset, size) == 0) ||
        (pal_tpm_nv_range_valid(dest_offset, size) == 0))
    {
        return 0;
    }

    if (size == 0)
    {
        return 1;
    }

    memmove(&g_tpm_nv[dest_offset], &g_tpm_nv[source_offset], size);
    memset(&g_tpm_nv_written[dest_offset], 1, size);
    g_tpm_nv_needs_manufacture = 0U;
    return 1;
}

LIB_EXPORT int _plat__NvCommit(void)
{
    /* TCG TPM NvCommit wrapper treats 0 as commit success. */
    return 0;
}

LIB_EXPORT void _plat__SetNvAvail(void)
{
    g_tpm_nv_ready = 1U;
}

LIB_EXPORT void _plat__ClearNvAvail(void)
{
    g_tpm_nv_ready = 0U;
}

LIB_EXPORT int _plat__NVNeedsManufacture(void)
{
    return (int)g_tpm_nv_needs_manufacture;
}

/* Production TPM integrations must replace this with provisioned per-device
 * unique data or a value derived from a protected hardware identity.
 */
LIB_EXPORT uint32_t _plat__GetUnique(uint32_t which, uint32_t b_size, unsigned char *buffer)
{
    uint32_t i;

    (void)which;

    if ((buffer == NULL) || (b_size > PAL_TPM_UNIQUE_MAX_SIZE))
    {
        return 0U;
    }

    for (i = 0U; i < b_size; i++)
    {
        buffer[i] = (uint8_t)(0xA5U ^ (uint8_t)i);
    }

    return b_size;
}

LIB_EXPORT uint32_t _plat__GetManufacturerCapabilityCode(void)
{
    return PAL_TPM_MANUFACTURER;
}

LIB_EXPORT uint32_t _plat__GetVendorCapabilityCode(int index)
{
    if (index == 1)
    {
        return PAL_TPM_VENDOR_1;
    }

    if (index == 2)
    {
        return PAL_TPM_VENDOR_2;
    }

    return 0U;
}

LIB_EXPORT uint32_t _plat__GetTpmFirmwareVersionHigh(void)
{
    return 0x20260624U;
}

LIB_EXPORT uint32_t _plat__GetTpmFirmwareVersionLow(void)
{
    return 0U;
}

LIB_EXPORT uint16_t _plat__GetTpmFirmwareSvn(void)
{
    return 1U;
}

LIB_EXPORT uint16_t _plat__GetTpmFirmwareMaxSvn(void)
{
    return 255U;
}

/* Production TPM integrations must replace these firmware secret providers with
 * protected secret material or a KDF rooted in a non-exportable device secret.
 */
LIB_EXPORT int _plat__GetTpmFirmwareSvnSecret(uint16_t svn,
                                              uint16_t secret_buf_size,
                                              uint8_t *secret_buf,
                                              uint16_t *secret_size)
{
    uint16_t i;

    if ((secret_buf == NULL) || (secret_size == NULL) ||
        (secret_buf_size > PAL_TPM_SECRET_MAX_SIZE))
    {
        return -1;
    }

    for (i = 0U; i < secret_buf_size; i++)
    {
        secret_buf[i] = (uint8_t)(svn ^ i);
    }

    *secret_size = secret_buf_size;
    return 0;
}

LIB_EXPORT int _plat__GetTpmFirmwareSecret(uint16_t secret_buf_size,
                                           uint8_t *secret_buf,
                                           uint16_t *secret_size)
{
    uint16_t i;

    if ((secret_buf == NULL) || (secret_size == NULL) ||
        (secret_buf_size > PAL_TPM_SECRET_MAX_SIZE))
    {
        return -1;
    }

    for (i = 0U; i < secret_buf_size; i++)
    {
        secret_buf[i] = (uint8_t)(0x5AU ^ i);
    }

    *secret_size = secret_buf_size;
    return 0;
}

LIB_EXPORT void _plat__GetPlatformManufactureData(uint8_t *data, uint32_t size)
{
    uint32_t i;

    if (data == NULL)
    {
        return;
    }

    if (size > PAL_TPM_MANUFACTURE_DATA_MAX_SIZE)
    {
        size = PAL_TPM_MANUFACTURE_DATA_MAX_SIZE;
    }

    for (i = 0U; i < size; i++)
    {
        data[i] = (uint8_t)(0xC3U ^ (uint8_t)i);
    }
}

void _plat_internal_resetFailureData(void)
{
    g_tpm_failure_code = 0U;
    g_tpm_failure_location = 0U;
    g_tpm_failure_function = NULL;
    g_tpm_failure_line = 0U;
    g_tpm_in_failure_mode = 0U;
}

LIB_EXPORT BOOL _plat__InFailureMode(void)
{
    return (BOOL)g_tpm_in_failure_mode;
}

LIB_EXPORT NORETURN void _plat__Fail(const char *function,
                                     int line,
                                     uint64_t location_code,
                                     int failure_code)
{
    g_tpm_failure_function = function;
    g_tpm_failure_line = (uint32_t)line;
    g_tpm_failure_location = location_code;
    g_tpm_failure_code = (uint32_t)failure_code;
    g_tpm_in_failure_mode = 1U;

    while (1)
    {
    }
}

LIB_EXPORT uint32_t _plat__GetFailureCode(void)
{
    return g_tpm_failure_code;
}

LIB_EXPORT uint64_t _plat__GetFailureLocation(void)
{
    return g_tpm_failure_location;
}

LIB_EXPORT const char *_plat__GetFailureFunctionName(void)
{
    return g_tpm_failure_function;
}

LIB_EXPORT uint32_t _plat__GetFailureLine(void)
{
    return g_tpm_failure_line;
}

LIB_EXPORT uint32_t _plat__GetVendorTpmType(void)
{
    return 1U;
}

LIB_EXPORT void _plat_GetSpecCapabilityValue(SPEC_CAPABILITY_VALUE *return_data)
{
    if (return_data == NULL)
    {
        return;
    }

    return_data->tpmSpecLevel = 0U;
    return_data->tpmSpecVersion = 184U;
    return_data->tpmSpecYear = 2025U;
    return_data->tpmSpecDayOfYear = 79U;
    return_data->platformFamily = 1U;
    return_data->platfromLevel = 0U;
    return_data->platformRevision = 0x105U;
    return_data->platformYear = 0U;
    return_data->platformDayOfYear = 0U;
}


static const PCR_Attributes g_tpm_pcr_init_attributes[IMPLEMENTATION_PCR] = {
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {1, 0, 0, 0, 0, 0x1F},
    {0, 0, 0, 0, 0x0F, 0x1F},
    {0, 0, 0, 0, 0x10, 0x1C},
    {0, 0, 0, 0, 0x10, 0x1C},
    {0, 0, 0, 0, 0x10, 0x0C},
    {0, 1, 1, 1, 0x14, 0x0E},
    {0, 1, 1, 1, 0x14, 0x04},
    {0, 1, 1, 1, 0x14, 0x04},
    {0, 0, 0, 0, 0x0F, 0x1F},
};

LIB_EXPORT UINT32 _platPcr__NumberOfPcrs(void)
{
    return IMPLEMENTATION_PCR;
}

LIB_EXPORT PCR_Attributes _platPcr__GetPcrInitializationAttributes(UINT32 pcr_number)
{
    if (pcr_number >= IMPLEMENTATION_PCR)
    {
        pcr_number = 0U;
    }

    return g_tpm_pcr_init_attributes[pcr_number];
}

LIB_EXPORT BOOL _platPcr_IsPcrBankDefaultActive(TPM_ALG_ID pcr_alg)
{
#if ALG_SHA256 == YES
    if (pcr_alg == TPM_ALG_SHA256)
    {
        return TRUE;
    }
#endif
#if ALG_SHA384 == YES
    if (pcr_alg == TPM_ALG_SHA384)
    {
        return TRUE;
    }
#endif
    return FALSE;
}

LIB_EXPORT TPM_RC _platPcr__GetInitialValueForPcr(UINT32 pcr_number,
                                                 TPM_ALG_ID pcr_alg,
                                                 BYTE startup_locality,
                                                 BYTE *pcr_buffer,
                                                 uint16_t buffer_size,
                                                 uint16_t *pcr_length)
{
    uint16_t pcr_size = CryptHashGetDigestSize(pcr_alg);
    BYTE default_value = 0U;
    PCR_Attributes attrs;

    if ((pcr_buffer == NULL) || (pcr_length == NULL) ||
        (pcr_number >= IMPLEMENTATION_PCR) || (pcr_size == 0U) ||
        (buffer_size < pcr_size))
    {
        return TPM_RC_FAILURE;
    }

    attrs = _platPcr__GetPcrInitializationAttributes(pcr_number);
    if ((attrs.resetLocality & 0x10U) != 0U)
    {
        default_value = 0xFFU;
    }

    memset(pcr_buffer, default_value, pcr_size);
    if (pcr_number == HCRTM_PCR)
    {
        pcr_buffer[pcr_size - 1U] = startup_locality;
    }

    *pcr_length = pcr_size;
    return TPM_RC_SUCCESS;
}

LIB_EXPORT void _plat_GetEnabledSelfTest(uint8_t full_test,
                                         uint8_t *to_test_vector,
                                         size_t to_test_vector_size)
{
    (void)full_test;
    (void)to_test_vector;
    (void)to_test_vector_size;
}


LIB_EXPORT int _plat__WasPowerLost(void)
{
    return 0;
}
