/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _VAL_TPM_CRB_H_
#define _VAL_TPM_CRB_H_

#include "pal_mmio.h"

#include <stdint.h>

/* TPM CRB MMIO base address. */
#ifndef VAL_TPM_CRB_BASE
#define VAL_TPM_CRB_BASE 0x88f00000
#endif

/* TPM CRB locality layout. */
#define VAL_TPM_CRB_LOCALITY_SIZE               0x1000
#define VAL_TPM_CRB_DATA_BUFFER_SIZE           0x400

/* TPM CRB register offsets used by the tests. */
#define VAL_TPM_CRB_LOC_CTRL_OFFSET    0x008
#define VAL_TPM_CRB_LOC_STS_OFFSET     0x00c
#define VAL_TPM_CRB_CTRL_START_OFFSET  0x04c
#define VAL_TPM_CRB_DATA_BUFFER_OFFSET 0x080

/* TPM CRB locality control and status bits. */
#define TPM_LOC_CTRL_REQUEST_ACCESS 0x1
#define TPM_LOC_CTRL_RELINQUISH    0x2
#define TPM_LOC_CTRL_VALID_MASK    0x3
#define TPM_LOC_STS_GRANTED        0x1
#define TPM_CRB_CTRL_START         0x1

/* TPM command and response header offsets. */
#define TPM_CMD_TAG_OFFSET   0
#define TPM_CMD_SIZE_OFFSET  2
#define TPM_CMD_CODE_OFFSET  6
#define TPM_RSP_TAG_OFFSET   0
#define TPM_RSP_SIZE_OFFSET  2
#define TPM_RSP_RC_OFFSET    6
#define TPM_CMD_HEADER_SIZE     10
#define TPM_RSP_HEADER_SIZE     10

/* TPM response codes and command constants used by the tests. */
#define TPM2_ST_NO_SESSIONS       0x8001
#define TPM2_RC_SUCCESS           0x00000000
#define TPM2_RC_COMMAND_CODE      0x00000143
#define TPM2_RC_INSUFFICIENT      0x0000009a
#define TPM2_RC_P                 0x00000040
#define TPM2_RC_3                 0x00000300
#define TPM2_RC_GET_CAP_COUNT     (TPM2_RC_INSUFFICIENT + TPM2_RC_P + TPM2_RC_3)
#define TPM2_CC_GET_CAPABILITY    0x0000017a
#define TPM2_CAP_TPM_PROPERTIES   0x00000006
#define TPM2_PT_FIXED             0x00000100

/* TPM command buffers are big-endian. */
static inline uint16_t val_tpm_be16_read(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static inline uint32_t val_tpm_be32_read(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static inline void val_tpm_be16_write(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static inline void val_tpm_be32_write(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* TPM CRB register access helpers. */
static inline uint64_t val_tpm_crb_locality_base(uint32_t locality)
{
    return VAL_TPM_CRB_BASE + ((uint64_t)locality * VAL_TPM_CRB_LOCALITY_SIZE);
}

static inline uint64_t val_tpm_crb_loc_ctrl(uint32_t locality)
{
    return val_tpm_crb_locality_base(locality) + VAL_TPM_CRB_LOC_CTRL_OFFSET;
}

static inline uint64_t val_tpm_crb_loc_sts(uint32_t locality)
{
    return val_tpm_crb_locality_base(locality) + VAL_TPM_CRB_LOC_STS_OFFSET;
}

static inline uint64_t val_tpm_crb_ctrl_start(uint32_t locality)
{
    return val_tpm_crb_locality_base(locality) + VAL_TPM_CRB_CTRL_START_OFFSET;
}

static inline uint64_t val_tpm_crb_data_buffer(uint32_t locality)
{
    return val_tpm_crb_locality_base(locality) + VAL_TPM_CRB_DATA_BUFFER_OFFSET;
}

/* TPM CRB MMIO access helpers. */
static inline uint8_t val_tpm_crb_read8(uint64_t addr)
{
    return pal_mmio_read8(addr);
}

static inline uint32_t val_tpm_crb_read32(uint64_t addr)
{
    return pal_mmio_read32(addr);
}

static inline void val_tpm_crb_write8(uint64_t addr, uint8_t value)
{
    pal_mmio_write8(addr, value);
}

static inline void val_tpm_crb_write32(uint64_t addr, uint32_t value)
{
    pal_mmio_write32(addr, value);
}

#endif /* _VAL_TPM_CRB_H_ */
