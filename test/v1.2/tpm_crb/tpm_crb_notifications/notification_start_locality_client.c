/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb.h"


uint32_t notification_start_locality_client(uint32_t test_run_data)
{
    val_tpm_notification_context_t notification;
    uint64_t loc_ctrl;
    uint64_t loc_sts;
    uint32_t status;
    uint32_t cleanup_status;

    (void)test_run_data;

    /* Get CRB locality registers */
    loc_ctrl = val_tpm_crb_loc_ctrl(TPM_TEST_LOCALITY);
    loc_sts = val_tpm_crb_loc_sts(TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, 0);

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

    /* Set up notification interrupt */
    status = val_tpm_notification_setup_interrupt(&notification);
    if (status == VAL_SKIP_CHECK)
    {
        (void)val_tpm_notification_cleanup(&notification);
        return status;
    }
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_notification_cleanup(&notification);
        return VAL_ERROR_POINT(3);
    }

    /* Create notification bitmap */
    status = val_tpm_notification_create_bitmap(&notification);
    if (status == VAL_SKIP_CHECK)
    {
        (void)val_tpm_notification_cleanup(&notification);
        return status;
    }
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_notification_cleanup(&notification);
        return VAL_ERROR_POINT(4);
    }

    /* Bind notification bitmap */
    status = val_tpm_notification_bind(&notification);
    if (status == VAL_SKIP_CHECK)
    {
        (void)val_tpm_notification_cleanup(&notification);
        return status;
    }
    if (status != VAL_SUCCESS)
    {
        (void)val_tpm_notification_cleanup(&notification);
        return VAL_ERROR_POINT(5);
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
        status = VAL_ERROR_POINT(6);
        goto cleanup;
    }

    /* Enable notification interrupt */
    status = val_tpm_notification_prepare_interrupt(&notification);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(7);
        goto cleanup;
    }

    LOG(DBG, "Triggering TPM notification for locality %u\n",
        TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_REQUEST_ACCESS);
    status = val_tpm_send_and_expect(notification.client_id, notification.tpm_sp_id,
                                     CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(8);
        goto cleanup;
    }

    /* Complete notification interrupt */
    status = val_tpm_notification_complete_interrupt(&notification);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(9);
        goto cleanup;
    }

    /* Check pending notification bitmap */
    status = val_tpm_check_pending_notification(&notification,
                                                VAL_TPM_NOTIFICATION_PRESENT);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(10);
        goto cleanup;
    }

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "Locality %u was granted before finish_notified, status=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        status = VAL_ERROR_POINT(11);
        goto cleanup;
    }

    LOG(DBG, "Calling finish_notified for locality notification\n");
    /* Finish notified TPM work */
    status = val_tpm_finish_notified(notification.client_id, notification.tpm_sp_id,
                                     CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(12);
        goto cleanup;
    }

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) == 0)
    {
        LOG(ERROR, "Locality %u was not granted, status=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        status = VAL_ERROR_POINT(13);
        goto cleanup;
    }

    LOG(DBG, "Unregistering TPM notification\n");
    /* Unregister TPM notification */
    status = val_tpm_notification_unregister(&notification, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(14);
        goto cleanup;
    }

    LOG(DBG, "Relinquishing TPM locality %u\n",
        TPM_TEST_LOCALITY);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);
    status = val_tpm_send_and_expect(notification.client_id, notification.tpm_sp_id,
                                     CRB_START,
                                     CRB_START_TYPE_LOCALITY_REQUEST,
                                     TPM_TEST_LOCALITY, 0, CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(15);
        goto cleanup;
    }

    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        LOG(ERROR, "Locality %u was not relinquished, status=0x%x\n",
            TPM_TEST_LOCALITY, val_tpm_crb_read32(loc_sts));
        status = VAL_ERROR_POINT(16);
    }

cleanup:
    /* Clean up notification state */
    cleanup_status = val_tpm_notification_cleanup(&notification);
    if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
    {
        val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);
        if (val_tpm_send_and_expect(notification.client_id, notification.tpm_sp_id,
                                    CRB_START,
                                    CRB_START_TYPE_LOCALITY_REQUEST,
                                    TPM_TEST_LOCALITY, 0, CRB_OK) != VAL_SUCCESS)
        {
            if (status == VAL_SUCCESS)
            {
                status = VAL_ERROR_POINT(17);
            }
        }
        else if ((val_tpm_crb_read32(loc_sts) & TPM_LOC_STS_GRANTED) != 0)
        {
            if (status == VAL_SUCCESS)
            {
                status = VAL_ERROR_POINT(18);
            }
        }
    }

    if (status != VAL_SUCCESS)
    {
        return status;
    }

    if (cleanup_status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(19);
    }

    LOG(TEST, "TPM notification register, finish_notified, unregister completed for locality %u\n",
        TPM_TEST_LOCALITY);
    return VAL_SUCCESS;
}
