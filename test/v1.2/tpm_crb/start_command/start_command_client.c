/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"
#include "val_tpm_crb.h"


/*
 * Test START(COMMAND) positive flow. The client owns locality 0, writes a
 * valid TPM2_GetCapability command structure to the CRB data buffer, sets
 * CRB Start, and expects CRB_OK with a successful TPM response. The test
 * fails if locality is not granted, Start is not cleared, the response header
 * is invalid, or the TPM response code is not TPM2_RC_SUCCESS.
 */
uint32_t start_command_client(uint32_t test_run_data)
{
    uint8_t cmd[TPM_GET_CAPABILITY_CMD_SIZE];
    uint8_t rsp[VAL_TPM_CRB_DATA_BUFFER_SIZE];
    uint16_t sender_id;
    uint16_t tpm_sp_id;
    uint32_t rsp_size;
    uint32_t rc;
    uint32_t status;
    uint32_t locality_granted;
    uint64_t loc_ctrl;
    uint64_t loc_sts;
    uint64_t ctrl_start;
    uint64_t data_buffer;

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
    /* Get CRB command registers */
    ctrl_start = val_tpm_crb_ctrl_start(TPM_TEST_LOCALITY);
    data_buffer = val_tpm_crb_data_buffer(TPM_TEST_LOCALITY);
    locality_granted = 0;

    LOG(DBG, "Requesting TPM locality %u before command start\n", TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_REQUEST_ACCESS);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }
    locality_granted = 1;

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) == 0)
    {
        LOG(ERROR, "TPM locality %u was not granted, loc_sts=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(3);
    }

    LOG(DBG, "Preparing TPM2_GetCapability command in CRB data buffer\n");
    /*
     * Prepare TPM2_GetCapability command buffer:
     * tag = TPM2_ST_NO_SESSIONS, commandSize = 22,
     * commandCode = TPM2_CC_GET_CAPABILITY, capability = TPM2_CAP_TPM_PROPERTIES,
     * property = TPM2_PT_FIXED, propertyCount = 1.
     */
    val_tpm_prepare_get_capability(cmd, TPM_GET_CAPABILITY_CMD_SIZE);
    val_memset(rsp, 0, sizeof(rsp));
    /* Write TPM command to CRB data buffer */
    val_tpm_crb_copy_to_mmio(data_buffer, cmd, TPM_GET_CAPABILITY_CMD_SIZE);

    /* Set CRB Start bit to request command execution */
    val_tpm_crb_write32(ctrl_start, TPM_CRB_CTRL_START);
    LOG(DBG, "Sending CRB_START COMMAND for locality %u\n", TPM_TEST_LOCALITY);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_COMMAND,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        val_tpm_crb_write32(ctrl_start, 0);
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(4);
    }

    if ((val_tpm_crb_read32(ctrl_start) & TPM_CRB_CTRL_START) != 0)
    {
        LOG(ERROR, "TPM CRB Start bit was not cleared, ctrl_start=0x%x\n",
            val_tpm_crb_read32(ctrl_start));
        val_tpm_crb_write32(ctrl_start, 0);
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(5);
    }

    /* Read TPM response from CRB data buffer */
    val_tpm_crb_copy_from_mmio(rsp, sizeof(rsp), data_buffer, sizeof(rsp));
    /* Validate TPM response header and success code */
    status = val_tpm_validate_response_header(rsp, sizeof(rsp), &rsp_size, &rc);
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(6);
    }

    if (rc != TPM2_RC_SUCCESS)
    {
        LOG(ERROR, "TPM2_GetCapability failed rc=0x%x response_size=%u\n",
            rc, rsp_size);
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(7);
    }

    LOG(TEST, "TPM2_GetCapability completed with TPM_RC_SUCCESS\n");

    if (locality_granted != 0)
    {
        /* Relinquish TPM locality */
        status = val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        if (status != VAL_SUCCESS)
        {
            return VAL_ERROR_POINT(8);
        }

        if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
        {
            LOG(ERROR, "TPM locality %u still granted after relinquish, loc_sts=0x%x\n",
                TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
            return VAL_ERROR_POINT(9);
        }
    }

    return VAL_SUCCESS;
}
