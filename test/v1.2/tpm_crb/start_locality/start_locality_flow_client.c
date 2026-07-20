/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"
#include "val_tpm_crb.h"

uint32_t start_locality_flow_client(uint32_t test_run_data)
{
    uint16_t sender_id;
    uint16_t tpm_sp_id;
    uint64_t loc_ctrl;
    uint64_t loc_sts;
    uint32_t status;
    uint32_t locality_granted;

    (void)test_run_data;

    /* Get TPM endpoint IDs */
    status = val_tpm_get_endpoint_ids(&sender_id, &tpm_sp_id);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(1);
    }

    /* Get CRB locality registers */
    loc_ctrl = val_tpm_crb_loc_ctrl(TPM_TEST_LOCALITY);
    loc_sts = val_tpm_crb_loc_sts(TPM_TEST_LOCALITY);

    /* Start from an inactive locality state */
    val_tpm_crb_write32(loc_ctrl, 0);
    locality_granted = 0;
    status = val_tpm_relinquish_locality(sender_id, tpm_sp_id, TPM_TEST_LOCALITY);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    /* Verify requestAccess is tested from inactive state */
    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "Locality %u was granted before requestAccess, status=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        return VAL_ERROR_POINT(3);
    }

    /* Request locality access */
    LOG(DBG, "Requesting TPM locality %u access\n", TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_REQUEST_ACCESS);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(4);
        goto cleanup;
    }
    locality_granted = 1;

    /* Verify locality was granted */
    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) == 0)
    {
        LOG(ERROR, "Locality %u was not granted, status=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        status = VAL_ERROR_POINT(5);
        goto cleanup;
    }

    /* Request already granted locality again */
    LOG(DBG, "Requesting already granted TPM locality %u again\n", TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_REQUEST_ACCESS);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(6);
        goto cleanup;
    }

    /* Verify repeated request kept locality granted */
    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) == 0)
    {
        LOG(ERROR, "Repeated locality %u request cleared granted status=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        status = VAL_ERROR_POINT(7);
        goto cleanup;
    }

    /* Relinquish locality */
    LOG(DBG, "Relinquishing TPM locality %u\n", TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(8);
        goto cleanup;
    }
    locality_granted = 0;

    /* Verify locality was relinquished */
    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "Locality %u was not relinquished, status=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        status = VAL_ERROR_POINT(9);
        goto cleanup;
    }

    LOG(TEST, "TPM locality %u request was granted, repeated, and relinquished\n",
        TPM_TEST_LOCALITY);

cleanup:
    if ((locality_granted != 0) && ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0))
    {
        val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);
        if (val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                    CRB_START_TYPE_LOCALITY_REQUEST,
                                    TPM_TEST_LOCALITY, 0, CRB_OK) != VAL_SUCCESS)
        {
            if (status == VAL_SUCCESS)
            {
                status = VAL_ERROR_POINT(10);
            }
        }
    }

    return status;
}
