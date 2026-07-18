/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PAL_TPM_H_
#define _PAL_TPM_H_

#include <stdint.h>

#define PAL_TPM_SUCCESS           0U
#define PAL_TPM_INVALID_PARAMETER 1U

/*
 * Mirrors the TCG/EDK2 ExecuteCommand ABI:
 *   ExecuteCommand(requestSize, request, responseSize, response)
 * so the TPM SP service can route CRB traffic through PAL and PAL can own
 * the actual fTPM backend.
 */
uint32_t pal_tpm_execute_command(uint32_t request_size,
                                 uint8_t *request,
                                 uint32_t *response_size,
                                 uint8_t **response);

uint32_t pal_tpm_init(void);

#endif /* _PAL_TPM_H_ */
