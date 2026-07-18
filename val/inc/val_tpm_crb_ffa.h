/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _VAL_TPM_CRB_FFA_H_
#define _VAL_TPM_CRB_FFA_H_

#include "val_ffa.h"

#include <stdint.h>

/* TPM service status codes returned in x4/w4 of the direct response. */
#define CRB_OK                  0x05000001
#define CRB_OK_RESULTS_RETURNED 0x05000002
#define CRB_NOFUNC              0x8e000001
#define CRB_NOTSUP              0x8e000002
#define CRB_INVARG              0x8e000005
#define CRB_INV_CRB_CTRL_DATA   0x8e000006
#define CRB_ALREADY             0x8e000009
#define CRB_DENIED              0x8e00000a

/* TPM service ABI version fields. */
#define CRB_VERSION_MAJOR_SHIFT         16
#define CRB_VERSION_MINOR_MASK          0xffff
#define CRB_SERVICE_VERSION_MAJOR       1
#define CRB_SERVICE_VERSION_MINOR       0
#define CRB_SERVICE_VERSION             \
    ((CRB_SERVICE_VERSION_MAJOR << CRB_VERSION_MAJOR_SHIFT) | \
     CRB_SERVICE_VERSION_MINOR)

/*
 * Request register usage:
 *   x4/w4 = CRB_* function ID
 *   x5/w5 = function argument 0
 *   x6/w6 = function argument 1
 *   x7/w7 = function argument 2
 */
/* TPM service ABI function IDs. */
#define CRB_FUNCTION_ID_MASK      0xffff0000
#define CRB_FUNCTION_ID_BASE      0x0f000000
#define CRB_GET_INTERFACE_VERSION 0x0f000001
#define CRB_GET_FEATURE_INFO      0x0f000101
#define CRB_START                 0x0f000201
#define CRB_REGISTER_NOTIFICATION 0x0f000301
#define CRB_UNREGISTER_NOTIFICATION 0x0f000401
#define CRB_FINISH_NOTIFIED       0x0f000501
#define CRB_UNKNOWN_FUNCTION_ID   0x0f00ffff

/* TPM service feature selectors. */
#define CRB_FEATURE_NOTIFICATION  0xfea70000
#define CRB_INVALID_FEATURE_ID    0xffffffff

/* TPM service start qualifiers and locality range. */
#define CRB_START_TYPE_COMMAND          0
#define CRB_START_TYPE_LOCALITY_REQUEST 1
#define CRB_LOCALITY_MAX                 4

/* TPM service notification argument fields. */
#define CRB_NOTIFICATION_TYPE_GLOBAL   0
#define CRB_NOTIFICATION_TYPE_SHIFT    16
#define CRB_NOTIFICATION_ID_MASK       0xff

uint32_t val_tpm_crb_service_is_request(const ffa_args_t *payload);
void val_tpm_crb_service_handle_request(ffa_args_t *payload);

#endif /* _VAL_TPM_CRB_FFA_H_ */
