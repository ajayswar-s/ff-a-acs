/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"
#include "val_tpm_crb.h"

uint32_t start_locality_relinquish_inactive_client(uint32_t test_run_data)
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

    /* Ensure locality is inactive */
    LOG(DBG, "Ensuring TPM locality %u is inactive\n", TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    /* Verify locality is not granted */
    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "Locality %u unexpectedly granted before inactive relinquish\n",
            TPM_TEST_LOCALITY);
        val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);
        (void)val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                      CRB_START_TYPE_LOCALITY_REQUEST,
                                      TPM_TEST_LOCALITY, 0, CRB_OK);
        return VAL_ERROR_POINT(3);
    }

    /* Relinquish inactive locality */
    LOG(DBG, "Relinquishing inactive TPM locality %u\n", TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(4);
    }

    /* Verify inactive relinquish left locality ungranted */
    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "Inactive relinquish changed locality grant status=0x%x\n",
            val_tpm_crb_read32(loc_sts));
        val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);
        (void)val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                      CRB_START_TYPE_LOCALITY_REQUEST,
                                      TPM_TEST_LOCALITY, 0, CRB_OK);
        return VAL_ERROR_POINT(5);
    }

    LOG(TEST, "Inactive TPM locality relinquish returned OK and left Granted clear\n");

    return VAL_SUCCESS;
}
