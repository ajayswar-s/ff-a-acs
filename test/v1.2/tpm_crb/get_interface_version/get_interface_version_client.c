/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"

#define TPM_SERVICE_CLIENT_VERSION_MAJOR 1
#define TPM_SERVICE_CLIENT_VERSION_MINOR 0

uint32_t get_interface_version_client(uint32_t test_run_data)
{
    ffa_args_t payload;
    uint16_t sender_id;
    uint16_t tpm_sp_id;
    uint32_t status;
    uint32_t version;
    uint32_t client_major;
    uint32_t client_minor;
    uint32_t server_major;
    uint32_t server_minor;

    (void)test_run_data;

    /* Get TPM endpoint IDs */
    status = val_tpm_get_endpoint_ids(&sender_id, &tpm_sp_id);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(1);
    }

    /* Send get_interface_version request */
    LOG(DBG, "Sending get_interface_version to TPM SP 0x%x\n", tpm_sp_id);
    status = val_tpm_send_abi(sender_id, tpm_sp_id, CRB_GET_INTERFACE_VERSION,
                              0, 0, 0, &payload);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    /* Check direct response */
    if (payload.fid != FFA_MSG_SEND_DIRECT_RESP_64)
    {
        LOG(ERROR, "Expected direct response, fid=0x%lx\n", payload.fid);
        return VAL_ERROR_POINT(3);
    }

    if ((uint32_t)payload.arg4 != CRB_OK_RESULTS_RETURNED)
    {
        LOG(ERROR, "Unexpected TPM service status=0x%lx\n", payload.arg4);
        return VAL_ERROR_POINT(4);
    }

    /* Decode returned ABI version */
    client_major = TPM_SERVICE_CLIENT_VERSION_MAJOR;
    client_minor = TPM_SERVICE_CLIENT_VERSION_MINOR;
    version = (uint32_t)payload.arg5;
    server_major = version >> CRB_VERSION_MAJOR_SHIFT;
    server_minor = version & CRB_VERSION_MINOR_MASK;

    /* Check reserved response registers */
    if ((uint32_t)payload.arg6 != 0 || (uint32_t)payload.arg7 != 0)
    {
        LOG(ERROR, "Reserved response registers are non-zero: w6=0x%lx w7=0x%lx\n",
            payload.arg6, payload.arg7);
        return VAL_ERROR_POINT(5);
    }

    /* Check ABI compatibility */
    if (server_major != client_major)
    {
        LOG(ERROR, "Incompatible TPM service ABI major version: client=%u server=%u\n",
            client_major, server_major);
        return VAL_ERROR_POINT(6);
    }

    if (server_minor < client_minor)
    {
        LOG(ERROR, "Incompatible TPM service ABI minor version: client=%u server=%u\n",
            client_minor, server_minor);
        return VAL_ERROR_POINT(7);
    }

    LOG(TEST, "TPM service ABI version %u.%u is compatible with client %u.%u\n",
        server_major, server_minor, client_major, client_minor);

    return VAL_SUCCESS;
}
