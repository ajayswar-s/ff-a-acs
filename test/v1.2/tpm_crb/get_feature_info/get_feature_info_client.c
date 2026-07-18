/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"

uint32_t get_feature_info_client(uint32_t test_run_data)
{
    uint16_t sender_id;
    uint16_t tpm_sp_id;
    ffa_args_t payload;
    uint32_t status;

    (void)test_run_data;

    /* Get TPM endpoint IDs */
    status = val_tpm_get_endpoint_ids(&sender_id, &tpm_sp_id);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(1);
    }

    /* Query notification feature */
    LOG(DBG, "Sending get_feature_info notification selector=0x%x\n",
        CRB_FEATURE_NOTIFICATION);
    status = val_tpm_send_abi(sender_id, tpm_sp_id,
                              CRB_GET_FEATURE_INFO,
                              CRB_FEATURE_NOTIFICATION, 0, 0, &payload);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    if (payload.fid != FFA_MSG_SEND_DIRECT_RESP_64)
    {
        LOG(ERROR, "Expected direct response for get_feature_info, fid=0x%x\n",
            (uint32_t)payload.fid);
        return VAL_ERROR_POINT(3);
    }

    status = (uint32_t)payload.arg4;
    if ((status != CRB_OK) && (status != CRB_NOTSUP))
    {
        LOG(ERROR, "Unexpected notification feature status=0x%x\n", status);
        return VAL_ERROR_POINT(4);
    }

    if (status == CRB_OK)
    {
        LOG(TEST, "TPM notification feature is supported\n");
    }
    else
    {
        LOG(TEST, "TPM notification feature is not supported\n");
    }

    return VAL_SUCCESS;
}
