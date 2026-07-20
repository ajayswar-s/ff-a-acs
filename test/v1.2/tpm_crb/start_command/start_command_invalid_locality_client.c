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
 * Test START(COMMAND) with invalid ABI locality. A valid TPM2_GetCapability
 * command is prepared, but the FF-A locality argument is outside 0..4, so the
 * TPM service must reject the request with CRB_INVARG before execution.
 */
uint32_t start_command_invalid_locality_client(uint32_t test_run_data)
{
    uint8_t cmd[TPM_GET_CAPABILITY_CMD_SIZE];
    uint16_t sender_id;
    uint16_t tpm_sp_id;
    uint32_t status;
    uint64_t ctrl_start;
    uint64_t data_buffer;

    (void)test_run_data;

    /* Get TPM endpoint IDs */
    status = val_tpm_get_endpoint_ids(&sender_id, &tpm_sp_id);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(1);
    }

    /* Get CRB command registers */
    ctrl_start = val_tpm_crb_ctrl_start(TPM_TEST_LOCALITY);
    data_buffer = val_tpm_crb_data_buffer(TPM_TEST_LOCALITY);

    LOG(DBG, "Preparing TPM2_GetCapability command for invalid locality check\n");
    /*
     * Prepare TPM2_GetCapability command buffer:
     * tag = TPM2_ST_NO_SESSIONS, commandSize = 22,
     * commandCode = TPM2_CC_GET_CAPABILITY, capability = TPM2_CAP_TPM_PROPERTIES,
     * property = TPM2_PT_FIXED, propertyCount = 1.
     */
    val_tpm_prepare_get_capability(cmd, TPM_GET_CAPABILITY_CMD_SIZE);
    /* Write valid command to CRB buffer */
    val_tpm_crb_copy_to_mmio(data_buffer, cmd, TPM_GET_CAPABILITY_CMD_SIZE);
    /* Set CRB Start bit before sending invalid locality */
    val_tpm_crb_write32(ctrl_start, TPM_CRB_CTRL_START);

    /* Send START(COMMAND) with locality outside 0..4 */
    LOG(DBG, "Sending CRB_START COMMAND with invalid locality %u\n",
        TPM_INVALID_LOCALITY);
    status = val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                     CRB_START_TYPE_COMMAND,
                                     TPM_INVALID_LOCALITY, 0, CRB_INVARG);

    val_tpm_crb_write32(ctrl_start, 0);

    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    LOG(TEST, "TPM command start with invalid locality returned CRB_INVARG\n");

    return VAL_SUCCESS;
}
