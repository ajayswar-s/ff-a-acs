/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb.h"

uint32_t finish_notified_no_notification_client(uint32_t test_run_data)
{
    val_tpm_notification_context_t notification;
    uint32_t status;
    uint32_t cleanup_status;

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
        return status;
    }
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(2);
    }

    /* Create notification bitmap */
    status = val_tpm_notification_create_bitmap(&notification);
    if (status == VAL_SKIP_CHECK)
    {
        return status;
    }
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(3);
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
        return VAL_ERROR_POINT(4);
    }

    /* Register TPM notification */
    status = val_tpm_notification_register(&notification,
                                           CRB_NOTIFICATION_TYPE_GLOBAL,
                                           0, VAL_TPM_NOTIFICATION_ID,
                                           CRB_OK);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(5);
        goto cleanup;
    }

    LOG(DBG, "Calling finish_notified without an outstanding TPM notification\n");
    /* Finish notified TPM work */
    status = val_tpm_finish_notified(notification.client_id, notification.tpm_sp_id,
                                     CRB_DENIED);
    if (status != VAL_SUCCESS)
    {
        status = VAL_ERROR_POINT(6);
        goto cleanup;
    }

cleanup:
    /* Clean up notification state */
    cleanup_status = val_tpm_notification_cleanup(&notification);
    if (status != VAL_SUCCESS)
    {
        return status;
    }
    if (cleanup_status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(7);
    }

    LOG(TEST, "finish_notified without outstanding notification returned CRB_DENIED\n");
    return VAL_SUCCESS;
}
