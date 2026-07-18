/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"

uint32_t tpm_crb_unknown_function_id_client(uint32_t test_run_data)
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

    /* Send unsupported TPM function request */
    LOG(DBG, "Sending unsupported TPM function ID 0x%x\n",
        CRB_UNKNOWN_FUNCTION_ID);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id,
                                     CRB_UNKNOWN_FUNCTION_ID,
                                     0, 0, 0, CRB_NOFUNC);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    LOG(TEST, "Unsupported TPM service function ID 0x%x returned CRB_NOFUNC\n",
        CRB_UNKNOWN_FUNCTION_ID);

    return VAL_SUCCESS;
}
