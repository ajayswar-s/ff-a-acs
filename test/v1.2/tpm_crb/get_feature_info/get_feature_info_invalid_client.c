/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"

uint32_t get_feature_info_invalid_client(uint32_t test_run_data)
{
    uint16_t sender_id;
    uint16_t tpm_sp_id;
    uint32_t status;

    (void)test_run_data;

    /* Get TPM endpoint IDs */
    status = val_tpm_get_endpoint_ids(&sender_id, &tpm_sp_id);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(1);
    }

    /* Query invalid feature selector */
    LOG(DBG, "Sending get_feature_info invalid selector=0x%x\n",
        CRB_INVALID_FEATURE_ID);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id,
                                     CRB_GET_FEATURE_INFO,
                                     CRB_INVALID_FEATURE_ID, 0, 0, CRB_INVARG);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    LOG(TEST, "Invalid TPM feature selector returned CRB_INVARG\n");

    return VAL_SUCCESS;
}
