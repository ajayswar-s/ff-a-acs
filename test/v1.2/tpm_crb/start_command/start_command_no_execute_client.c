/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"
#include "val_tpm_crb.h"


static uint32_t tpm_buffer_matches(uint64_t buffer, const uint8_t *expected,
                                   uint32_t size)
{
    uint32_t i;

    for (i = 0; i < size; i++)
    {
        if (val_tpm_crb_read8(buffer + i) != expected[i])
        {
            LOG(ERROR, "TPM CRB buffer changed at offset %u got=0x%x expected=0x%x\n",
                i, (uint32_t)val_tpm_crb_read8(buffer + i), (uint32_t)expected[i]);
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

/*
 * Test START(COMMAND) states that must not execute the TPM command. The test
 * first leaves CRB Start clear for a granted locality, then sends START(COMMAND)
 * with no granted locality. Both cases return CRB_OK and leave command
 * state unchanged.
 */
uint32_t start_command_no_execute_client(uint32_t test_run_data)
{
    uint8_t cmd[TPM_GET_CAPABILITY_CMD_SIZE];
    uint16_t sender_id;
    uint16_t tpm_sp_id;
    uint32_t status;
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

    /*
     * Prepare TPM2_GetCapability command buffer:
     * tag = TPM2_ST_NO_SESSIONS, commandSize = 22,
     * commandCode = TPM2_CC_GET_CAPABILITY, capability = TPM2_CAP_TPM_PROPERTIES,
     * property = TPM2_PT_FIXED, propertyCount = 1.
     */
    val_tpm_prepare_get_capability(cmd, TPM_GET_CAPABILITY_CMD_SIZE);

    LOG(DBG, "Checking START(COMMAND) with Start bit clear\n");
    /* Request TPM locality */
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_REQUEST_ACCESS);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) == 0)
    {
        LOG(ERROR, "TPM locality %u was not granted, loc_sts=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(3);
    }

    /* Write valid command but keep CRB Start bit clear */
    val_tpm_crb_copy_to_mmio(data_buffer, cmd, TPM_GET_CAPABILITY_CMD_SIZE);
    val_tpm_crb_write32(ctrl_start, 0);
    /* Send START(COMMAND) and expect no TPM execution */
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_COMMAND,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(4);
    }

    if (val_tpm_crb_read32(ctrl_start) != 0)
    {
        LOG(ERROR, "TPM CRB Start bit changed unexpectedly, ctrl_start=0x%x\n",
            val_tpm_crb_read32(ctrl_start));
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(5);
    }

    /* Check CRB command buffer was not replaced by response */
    if (tpm_buffer_matches(data_buffer, cmd, TPM_GET_CAPABILITY_CMD_SIZE) != VAL_SUCCESS)
    {
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(6);
    }

    /* Relinquish TPM locality */
    status = val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(7);
    }

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "TPM locality %u still granted after relinquish, loc_sts=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        return VAL_ERROR_POINT(8);
    }

    LOG(DBG, "Checking START(COMMAND) with locality not granted\n");
    val_tpm_crb_copy_to_mmio(data_buffer, cmd, TPM_GET_CAPABILITY_CMD_SIZE);
    val_tpm_crb_write32(ctrl_start, TPM_CRB_CTRL_START);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_COMMAND,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        val_tpm_crb_write32(ctrl_start, 0);
        return VAL_ERROR_POINT(9);
    }

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "TPM locality %u became granted unexpectedly, loc_sts=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        val_tpm_crb_write32(ctrl_start, 0);
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(10);
    }

    if ((val_tpm_crb_read32(ctrl_start) & TPM_CRB_CTRL_START) == 0)
    {
        LOG(ERROR, "TPM CRB Start bit was cleared even though locality was not granted\n");
        val_tpm_crb_write32(ctrl_start, 0);
        return VAL_ERROR_POINT(11);
    }

    if (tpm_buffer_matches(data_buffer, cmd, TPM_GET_CAPABILITY_CMD_SIZE) != VAL_SUCCESS)
    {
        val_tpm_crb_write32(ctrl_start, 0);
        return VAL_ERROR_POINT(12);
    }

    val_tpm_crb_write32(ctrl_start, 0);

    LOG(TEST, "TPM command start returned OK without execution when CRB state disallowed it\n");

    return VAL_SUCCESS;
}
