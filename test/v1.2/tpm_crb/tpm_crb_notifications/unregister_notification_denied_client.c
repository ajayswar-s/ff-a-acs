/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_tpm_crb_common_abi.h"
#include "val_tpm_crb.h"

uint32_t unregister_notification_denied_client(uint32_t test_run_data)
{
    val_tpm_notification_context_t notification;
    uint32_t status;

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

    LOG(DBG, "Calling unregister_from_notification before registration\n");
    /* Unregister TPM notification */
    status = val_tpm_notification_unregister(&notification, CRB_DENIED);
    if (status != VAL_SUCCESS)
    {
        return VAL_ERROR_POINT(3);
    }

    LOG(TEST, "Unregistered TPM notification caller returned CRB_DENIED\n");
    return VAL_SUCCESS;
}
