/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb_ffa.h"
#include "val_tpm_crb.h"

#define TPM_UNSUPPORTED_CC 0x000001ff

static void tpm_prepare_unsupported_command(uint8_t *cmd)
{
    val_memset(cmd, 0, TPM_CMD_HEADER_SIZE);
    val_tpm_be16_write(&cmd[TPM_CMD_TAG_OFFSET], TPM2_ST_NO_SESSIONS);
    val_tpm_be32_write(&cmd[TPM_CMD_SIZE_OFFSET], TPM_CMD_HEADER_SIZE);
    val_tpm_be32_write(&cmd[TPM_CMD_CODE_OFFSET], TPM_UNSUPPORTED_CC);
}

static void tpm_clear_crb_buffer(uint64_t buffer)
{
    uint32_t i;

    for (i = 0; i < VAL_TPM_CRB_DATA_BUFFER_SIZE; i++)
    {
        val_tpm_crb_write8(buffer + i, 0);
    }
}

static uint32_t tpm_run_invalid_command(uint16_t sender_id, uint16_t tpm_sp_id,
                                        const uint8_t *cmd, uint32_t cmd_size,
                                        uint32_t expected_rc,
                                        const char *expected_rc_name,
                                        const char *label)
{
    uint8_t rsp[VAL_TPM_CRB_DATA_BUFFER_SIZE];
    uint32_t rsp_size;
    uint32_t rc;
    uint32_t status;
    uint64_t ctrl_start;
    uint64_t data_buffer;

    /* Get CRB command registers */
    ctrl_start = val_tpm_crb_ctrl_start(TPM_TEST_LOCALITY);
    data_buffer = val_tpm_crb_data_buffer(TPM_TEST_LOCALITY);

    LOG(DBG, "Preparing invalid TPM command buffer: %s\n", label);
    /* Write invalid TPM command bytes to CRB buffer */
    val_memset(rsp, 0, sizeof(rsp));
    tpm_clear_crb_buffer(data_buffer);
    val_tpm_crb_copy_to_mmio(data_buffer, cmd, cmd_size);

    /* Set CRB Start bit to pass invalid command to TPM backend */
    val_tpm_crb_write32(ctrl_start, TPM_CRB_CTRL_START);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_COMMAND,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        val_tpm_crb_write32(ctrl_start, 0);
        return VAL_ERROR;
    }

    if ((val_tpm_crb_read32(ctrl_start) & TPM_CRB_CTRL_START) != 0)
    {
        LOG(ERROR, "TPM CRB Start bit was not cleared for %s\n", label);
        val_tpm_crb_write32(ctrl_start, 0);
        return VAL_ERROR;
    }

    val_tpm_crb_copy_from_mmio(rsp, sizeof(rsp), data_buffer, sizeof(rsp));
    /* Validate TPM backend error response */
    status = val_tpm_validate_response_header(rsp, sizeof(rsp), &rsp_size, &rc);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR;
    }

    if (rc != expected_rc)
    {
        LOG(ERROR, "Invalid TPM command %s expected %s rc=0x%x got=0x%x\n",
            label, expected_rc_name, expected_rc, rc);
        return VAL_ERROR;
    }

    LOG(TEST, "Invalid TPM command %s returned %s\n",
        label, expected_rc_name);

    return VAL_SUCCESS;
}

/*
 * Test START(COMMAND) with invalid TPM command buffers. The service must
 * return CRB_OK because the FF-A ABI arguments are valid, while the TPM
 * backend returns an error response for an unsupported commandCode and for a
 * malformed TPM2_GetCapability command missing propertyCount.
 */
uint32_t start_command_invalid_buffer_client(uint32_t test_run_data)
{
    uint8_t cmd[TPM_GET_CAPABILITY_CMD_SIZE];
    uint16_t sender_id;
    uint16_t tpm_sp_id;
    uint32_t status;
    uint64_t loc_ctrl;
    uint64_t loc_sts;

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

    LOG(DBG, "Requesting TPM locality %u before invalid command checks\n",
        TPM_TEST_LOCALITY);
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

    /* Prepare command with unsupported commandCode field */
    tpm_prepare_unsupported_command(cmd);
    status = tpm_run_invalid_command(sender_id, tpm_sp_id, cmd, TPM_CMD_HEADER_SIZE,
                                     TPM2_RC_COMMAND_CODE, "TPM_RC_COMMAND_CODE",
                                     "unsupported command code");
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(4);
    }

    /* Prepare malformed TPM2_GetCapability with missing propertyCount field */
    val_tpm_prepare_get_capability(cmd, TPM_MALFORMED_GET_CAPABILITY_CMD_SIZE);
    status = tpm_run_invalid_command(sender_id, tpm_sp_id, cmd,
                                     TPM_MALFORMED_GET_CAPABILITY_CMD_SIZE,
                                     TPM2_RC_GET_CAP_COUNT,
                                     "TPM_RC_INSUFFICIENT + RC_GetCapability_propertyCount",
                                     "malformed GetCapability");
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        return VAL_ERROR_POINT(5);
    }

    /* Relinquish TPM locality */
    status = val_tpm_relinquish_locality(sender_id, tpm_sp_id,
                                         TPM_TEST_LOCALITY);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(6);
    }

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "TPM locality %u still granted after relinquish, loc_sts=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        return VAL_ERROR_POINT(7);
    }

    LOG(TEST, "Invalid TPM command buffers completed with TPM error responses\n");

    return VAL_SUCCESS;
}
