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
 * Test START(COMMAND) notification flow. A valid TPM2_GetCapability command is
 * deferred after notification is signaled, then finish_notified completes the
 * command and the test validates the TPM response before cleanup.
 */
uint32_t notification_start_command_client(uint32_t test_run_data)
{
    val_tpm_notification_context_t notification;
    uint8_t cmd[TPM_GET_CAPABILITY_CMD_SIZE];
    uint8_t rsp[VAL_TPM_CRB_DATA_BUFFER_SIZE];
    uint32_t rsp_size;
    uint32_t rc;
    uint32_t status;
    uint32_t cleanup_status;
    uint32_t locality_granted;
    uint64_t loc_ctrl;
    uint64_t loc_sts;
    uint64_t ctrl_start;
    uint64_t data_buffer;

    (void)test_run_data;

    /* Initialize notification context */
    status = val_tpm_notification_init(&notification);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(1);
    }

    /* Check TPM notification support */
    status = val_tpm_notification_feature_supported(notification.client_id,
                                                    notification.tpm_sp_id);
    if (status == VAL_SKIP_CHECK)
    {
        (void)val_tpm_notification_cleanup(&notification);
        return status;
    }
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_notification_cleanup(&notification);
        return VAL_ERROR_POINT(2);
    }

    /* Get CRB locality registers */
    loc_ctrl = val_tpm_crb_loc_ctrl(TPM_TEST_LOCALITY);
    loc_sts = val_tpm_crb_loc_sts(TPM_TEST_LOCALITY);
    /* Get CRB command registers */
    ctrl_start = val_tpm_crb_ctrl_start(TPM_TEST_LOCALITY);
    data_buffer = val_tpm_crb_data_buffer(TPM_TEST_LOCALITY);
    locality_granted = 0;

    LOG(DBG, "Requesting TPM locality %u before notification setup\n",
        TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_REQUEST_ACCESS);
    status = val_tpm_send_and_expect(notification.client_id, notification.tpm_sp_id,
                                     CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_notification_cleanup(&notification);
        return VAL_ERROR_POINT(3);
    }
    locality_granted = 1;

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) == 0)
    {
        LOG(ERROR, "TPM locality %u was not granted, loc_sts=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        (void)val_tpm_relinquish_locality(notification.client_id,
                                         notification.tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        (void)val_tpm_notification_cleanup(&notification);
        return VAL_ERROR_POINT(4);
    }

    /* Set up notification interrupt */
    status = val_tpm_notification_setup_interrupt(&notification);
    if (status == VAL_SKIP_CHECK)
    {
        (void)val_tpm_relinquish_locality(notification.client_id,
                                         notification.tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        (void)val_tpm_notification_cleanup(&notification);
        return status;
    }
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_relinquish_locality(notification.client_id,
                                         notification.tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        (void)val_tpm_notification_cleanup(&notification);
        return VAL_ERROR_POINT(5);
    }

    /* Create notification bitmap */
    status = val_tpm_notification_create_bitmap(&notification);
    if (status == VAL_SKIP_CHECK)
    {
        (void)val_tpm_relinquish_locality(notification.client_id,
                                         notification.tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        (void)val_tpm_notification_cleanup(&notification);
        return status;
    }
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_relinquish_locality(notification.client_id,
                                         notification.tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        (void)val_tpm_notification_cleanup(&notification);
        return VAL_ERROR_POINT(6);
    }

    /* Bind notification bitmap */
    status = val_tpm_notification_bind(&notification);
    if (status == VAL_SKIP_CHECK)
    {
        (void)val_tpm_relinquish_locality(notification.client_id,
                                         notification.tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        (void)val_tpm_notification_cleanup(&notification);
        return status;
    }
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_relinquish_locality(notification.client_id,
                                         notification.tpm_sp_id,
                                         TPM_TEST_LOCALITY);
        (void)val_tpm_notification_cleanup(&notification);
        return VAL_ERROR_POINT(7);
    }

    LOG(DBG, "Registering TPM notification id=%u\n",
        VAL_TPM_NOTIFICATION_ID);
    /* Register TPM notification */
    status = val_tpm_notification_register(&notification,
                                           CRB_NOTIFICATION_TYPE_GLOBAL,
                                           0, VAL_TPM_NOTIFICATION_ID,
                                           CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(8);
        goto cleanup;
    }

    /* Enable notification interrupt */
    status = val_tpm_notification_prepare_interrupt(&notification);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(9);
        goto cleanup;
    }

    LOG(DBG, "Preparing TPM2_GetCapability command for notification flow\n");
    /*
     * Prepare TPM2_GetCapability command buffer:
     * tag = TPM2_ST_NO_SESSIONS, commandSize = 22,
     * commandCode = TPM2_CC_GET_CAPABILITY, capability = TPM2_CAP_TPM_PROPERTIES,
     * property = TPM2_PT_FIXED, propertyCount = 1.
     */
    val_tpm_prepare_get_capability(cmd, TPM_GET_CAPABILITY_CMD_SIZE);
    val_memset(rsp, 0, sizeof(rsp));
    val_tpm_crb_copy_to_mmio(data_buffer, cmd, TPM_GET_CAPABILITY_CMD_SIZE);

    val_tpm_crb_write32(ctrl_start, TPM_CRB_CTRL_START);
    LOG(DBG, "Triggering TPM notification for TPM2_GetCapability\n");
    status = val_tpm_send_and_expect(notification.client_id, notification.tpm_sp_id,
                                     CRB_START,
                                     CRB_START_TYPE_COMMAND,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(10);
        goto cleanup;
    }

    if ((val_tpm_crb_read32(ctrl_start) & TPM_CRB_CTRL_START) == 0)
    {
        LOG(ERROR, "TPM CRB Start bit cleared before finish_notified\n");
        status = VAL_ERROR_POINT(11);
        goto cleanup;
    }

    /* Complete notification interrupt */
    status = val_tpm_notification_complete_interrupt(&notification);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(12);
        goto cleanup;
    }

    /* Check pending notification bitmap */
    status = val_tpm_check_pending_notification(&notification,
                                                VAL_TPM_NOTIFICATION_PRESENT);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(13);
        goto cleanup;
    }

    LOG(DBG, "Calling finish_notified for TPM2_GetCapability notification\n");
    /* Finish notified TPM work */
    status = val_tpm_finish_notified(notification.client_id, notification.tpm_sp_id,
                                     CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(14);
        goto cleanup;
    }

    if ((val_tpm_crb_read32(ctrl_start) & TPM_CRB_CTRL_START) != 0)
    {
        LOG(ERROR, "TPM CRB Start bit was not cleared after finish_notified\n");
        status = VAL_ERROR_POINT(15);
        goto cleanup;
    }

    val_tpm_crb_copy_from_mmio(rsp, sizeof(rsp), data_buffer, sizeof(rsp));
    /* Validate TPM response */
    status = val_tpm_validate_response_header(rsp, sizeof(rsp), &rsp_size, &rc);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(16);
        goto cleanup;
    }

    if (rc != TPM2_RC_SUCCESS)
    {
        LOG(ERROR, "TPM2_GetCapability failed rc=0x%x response_size=%u\n",
            rc, rsp_size);
        status = VAL_ERROR_POINT(17);
        goto cleanup;
    }

    LOG(DBG, "Unregistering TPM notification\n");
    /* Unregister TPM notification */
    status = val_tpm_notification_unregister(&notification, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(18);
        goto cleanup;
    }

cleanup:
    /* Clean up notification state */
    cleanup_status = val_tpm_notification_cleanup(&notification);
    if ((ctrl_start != 0) && ((val_tpm_crb_read32(ctrl_start) & TPM_CRB_CTRL_START) != 0))
    {
        val_tpm_crb_write32(ctrl_start, 0);
    }

    if (locality_granted != 0)
    {
        LOG(DBG, "Relinquishing TPM locality %u\n", TPM_TEST_LOCALITY);
        if (val_tpm_relinquish_locality(notification.client_id,
                                         notification.tpm_sp_id,
                                         TPM_TEST_LOCALITY) != VAL_SUCCESS)
        {
            if (status == VAL_SUCCESS)
            {
                status = VAL_ERROR_POINT(19);
            }
        }
        else if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
        {
            LOG(ERROR, "TPM locality %u still granted after relinquish, loc_sts=0x%x\n",
                TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
            if (status == VAL_SUCCESS)
            {
                status = VAL_ERROR_POINT(20);
            }
        }
    }

    if (status != VAL_SUCCESS)
    {
        return status;
    }

    if (cleanup_status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(21);
    }

    LOG(TEST, "TPM notification register, finish_notified, unregister completed "
        "for TPM2_GetCapability\n");
    return VAL_SUCCESS;
}
