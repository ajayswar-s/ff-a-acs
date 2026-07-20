/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pal_tpm.h"

#include <TpmConfiguration/TpmBuildSwitches.h>
#include <CompilerDependencies.h>
#include <ExecCommand_fp.h>
#include <Manufacture_fp.h>
#include <_TPM_Init_fp.h>
#include <platform_interface/tpm_to_platform_interface.h>
#include <stddef.h>
#include <stdint.h>

extern int _plat__NVNeedsManufacture(void);

static uint32_t g_pal_tpm_initialized;

static uint32_t pal_tpm_startup(void)
{
    static uint8_t startup_clear_cmd[] = {
        0x80U, 0x01U,             /* TPM_ST_NO_SESSIONS */
        0x00U, 0x00U, 0x00U, 0x0cU,
        0x00U, 0x00U, 0x01U, 0x44U, /* TPM_CC_Startup */
        0x00U, 0x00U              /* TPM_SU_CLEAR */
    };
    static uint8_t startup_rsp[10];
    uint8_t *response;
    uint32_t response_size;

    response = startup_rsp;
    response_size = sizeof(startup_rsp);
    ExecuteCommand((uint32_t)sizeof(startup_clear_cmd),
                   startup_clear_cmd,
                   &response_size,
                   &response);

    if ((response == NULL) || (response_size != 10U) ||
        (response[0] != 0x80U) || (response[1] != 0x01U) ||
        (response[2] != 0x00U) || (response[3] != 0x00U) ||
        (response[4] != 0x00U) || (response[5] != 0x0aU) ||
        (response[6] != 0x00U) || (response[7] != 0x00U) ||
        (response[8] != 0x00U) || (response[9] != 0x00U))
    {
        return PAL_TPM_INVALID_PARAMETER;
    }

    return PAL_TPM_SUCCESS;
}

uint32_t pal_tpm_init(void)
{
    int manufacture_status;

    if (g_pal_tpm_initialized != 0U)
    {
        return PAL_TPM_SUCCESS;
    }

    if (_plat__NVEnable(NULL, 0U) < 0)
    {
        return PAL_TPM_INVALID_PARAMETER;
    }

    _TPM_Init();

    if (_plat__NVNeedsManufacture() != 0)
    {
        manufacture_status = TPM_Manufacture(1);
        if (manufacture_status != 0)
        {
            return PAL_TPM_INVALID_PARAMETER;
        }
    }

    if (pal_tpm_startup() != PAL_TPM_SUCCESS)
    {
        return PAL_TPM_INVALID_PARAMETER;
    }

    g_pal_tpm_initialized = 1U;
    return PAL_TPM_SUCCESS;
}

uint32_t pal_tpm_execute_command(uint32_t request_size,
                                 uint8_t *request,
                                 uint32_t *response_size,
                                 uint8_t **response)
{
    if ((request == NULL) || (request_size == 0U) ||
        (response_size == NULL) || (response == NULL) ||
        (*response == NULL) || (*response_size == 0U))
    {
        return PAL_TPM_INVALID_PARAMETER;
    }

    if (pal_tpm_init() != PAL_TPM_SUCCESS)
    {
        return PAL_TPM_INVALID_PARAMETER;
    }

    ExecuteCommand(request_size, request, response_size, response);
    if ((*response == NULL) || (*response_size == 0U))
    {
        return PAL_TPM_INVALID_PARAMETER;
    }

    return PAL_TPM_SUCCESS;
}
