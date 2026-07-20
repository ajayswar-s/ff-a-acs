/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb.h"

uint32_t register_notification_duplicate_client(uint32_t test_run_data)
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

    LOG(DBG, "Registering TPM notification id=%u\n", VAL_TPM_NOTIFICATION_ID);
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

    LOG(DBG, "Registering TPM notification id=%u again\n", VAL_TPM_NOTIFICATION_ID);
    /* Register TPM notification */
    status = val_tpm_notification_register(&notification,
                                           CRB_NOTIFICATION_TYPE_GLOBAL,
                                           0, VAL_TPM_NOTIFICATION_ID,
                                           CRB_ALREADY);
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

    LOG(TEST, "Duplicate TPM notification registration returned CRB_ALREADY\n");
    return VAL_SUCCESS;
}
