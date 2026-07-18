/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"
#include "val_tpm_crb.h"

static uint32_t tpm_relinquish_if_granted(uint16_t sender_id, uint16_t tpm_sp_id,
                                          uint64_t loc_ctrl,
                                          uint64_t loc_sts)
{
    uint32_t status;

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) == 0)
    {
        return VAL_SUCCESS;
    }

    LOG(DBG, "Cleaning up granted TPM locality %u\n", TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR;
    }

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "Locality %u remained granted after cleanup status=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

uint32_t start_locality_ctrl_data_client(uint32_t test_run_data)
{
    uint16_t sender_id;
    uint16_t tpm_sp_id;
    uint64_t loc_ctrl;
    uint64_t loc_sts;
    uint32_t status;

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
    LOG(DBG, "Preparing inactive locality %u for ctrl data checks\n",
        TPM_TEST_LOCALITY);
    status = tpm_relinquish_if_granted(sender_id, tpm_sp_id, loc_ctrl, loc_sts);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    /* Check no-op locality control data */
    LOG(DBG, "Checking no-op TPM locality control data\n");
    val_tpm_crb_write32(loc_ctrl, 0);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(3);
    }

    /* Verify no-op did not grant locality */
    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "No-op locality control data granted locality, status=0x%x\n",
            val_tpm_crb_read32(loc_sts));
        (void)tpm_relinquish_if_granted(sender_id, tpm_sp_id, loc_ctrl, loc_sts);
        return VAL_ERROR_POINT(4);
    }

    /* Check requestAccess and Relinquish set together */
    LOG(DBG, "Checking requestAccess and Relinquish set together\n");
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_REQUEST_ACCESS | TPM_LOC_CTRL_RELINQUISH);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        (void)tpm_relinquish_if_granted(sender_id, tpm_sp_id, loc_ctrl, loc_sts);
        return VAL_ERROR_POINT(5);
    }

    /* Clean up any granted locality */
    status = tpm_relinquish_if_granted(sender_id, tpm_sp_id, loc_ctrl, loc_sts);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(6);
    }

    LOG(TEST, "TPM locality control data states did not return control data errors\n");

    return VAL_SUCCESS;
}
