/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"

uint32_t start_locality_invalid_args_client(uint32_t test_run_data)
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

    /* Check invalid locality argument */
    LOG(DBG, "Checking invalid TPM locality argument\n");
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     CRB_LOCALITY_MAX + 1, 0, CRB_INVARG);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    /* Check invalid start qualifier */
    LOG(DBG, "Checking invalid TPM start qualifier\n");
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     TPM_INVALID_START_QUALIFIER,
                                     TPM_TEST_LOCALITY, 0, CRB_INVARG);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(3);
    }

    /* Check high bits in start qualifier */
    LOG(DBG, "Checking non-zero high bits in TPM start qualifier\n");
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST |
                                     TPM_START_ARG_HIGH_BIT,
                                     TPM_TEST_LOCALITY, 0, CRB_INVARG);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(4);
    }

    /* Check high bits in locality argument */
    LOG(DBG, "Checking non-zero high bits in TPM locality argument\n");
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY |
                                     TPM_START_ARG_HIGH_BIT, 0, CRB_INVARG);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(5);
    }

    LOG(TEST, "Invalid TPM start locality ABI arguments returned CRB_INVARG\n");

    return VAL_SUCCESS;
}
