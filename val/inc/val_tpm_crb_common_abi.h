/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _VAL_TPM_CRB_COMMON_ABI_H_
#define _VAL_TPM_CRB_COMMON_ABI_H_

#include "val_ffa_helpers.h"
#include "val_tpm_crb_ffa.h"
#include "val_tpm_crb.h"

#include <stdint.h>

/* Common notification test values. */
#define VAL_TPM_NOTIFICATION_ID          1
#define VAL_TPM_NOTIFICATION_BITMAP      FFA_NOTIFICATION(VAL_TPM_NOTIFICATION_ID)
#define VAL_TPM_NOTIFICATION_VCPU_COUNT  1
#define VAL_TPM_NOTIFICATION_PRESENT     1
#define VAL_TPM_NOTIFICATION_ABSENT      0

/* Common TPM command and locality test values. */
#define TPM_TEST_LOCALITY                         0
#define TPM_GET_CAPABILITY_CMD_SIZE               22
#define TPM_MALFORMED_GET_CAPABILITY_CMD_SIZE     18
#define TPM_INVALID_LOCALITY                      (CRB_LOCALITY_MAX + 1)
#define TPM_INVALID_START_QUALIFIER               2
#define TPM_START_ARG_HIGH_BIT                    0x100

/* TPM notification test context. */
typedef struct {
    uint16_t client_id;
    uint16_t tpm_sp_id;
    ffa_notification_bitmap_t bitmap;
    uint32_t bitmap_created;
    uint32_t bound;
    uint32_t registered;
#if (PLATFORM_NS_HYPERVISOR_PRESENT == 0) && (TARGET_LINUX == 0)
    uint32_t sri_id;
    uint32_t sri_registered;
    uint32_t sri_enabled;
#endif
} val_tpm_notification_context_t;

/* Common TPM ABI helpers. */
uint32_t val_tpm_get_endpoint_ids(uint16_t *client_id, uint16_t *tpm_sp_id);
uint32_t val_tpm_send_abi(uint16_t sender_id, uint16_t tpm_sp_id,
                          uint32_t function_id, uint32_t arg0, uint32_t arg1,
                          uint32_t arg2, ffa_args_t *payload);
uint32_t val_tpm_send_and_expect(uint16_t sender_id, uint16_t tpm_sp_id,
                                 uint32_t function_id, uint32_t arg0, uint32_t arg1,
                                 uint32_t arg2, uint32_t expected_status);
uint32_t val_tpm_crb_copy_to_mmio(uint64_t dst, const uint8_t *src,
                                  uint32_t size);
void val_tpm_prepare_get_capability(uint8_t *cmd, uint32_t cmd_size);
uint32_t val_tpm_relinquish_locality(uint16_t sender_id, uint16_t tpm_sp_id,
                                     uint32_t locality);
uint32_t val_tpm_crb_copy_from_mmio(uint8_t *dst, uint32_t dst_size,
                                    uint64_t src, uint32_t size);
uint32_t val_tpm_validate_response_header(const uint8_t *rsp, uint32_t rsp_buf_size,
                                          uint32_t *rsp_size, uint32_t *rc);
uint32_t val_tpm_notification_feature_supported(uint16_t sender_id,
                                                uint16_t tpm_sp_id);
uint32_t val_tpm_notification_init(val_tpm_notification_context_t *ctx);
uint32_t val_tpm_notification_setup_interrupt(val_tpm_notification_context_t *ctx);
uint32_t val_tpm_notification_create_bitmap(val_tpm_notification_context_t *ctx);
uint32_t val_tpm_notification_bind(val_tpm_notification_context_t *ctx);
uint32_t val_tpm_notification_cleanup(val_tpm_notification_context_t *ctx);
uint32_t val_tpm_notification_prepare_interrupt(val_tpm_notification_context_t *ctx);
uint32_t val_tpm_notification_complete_interrupt(val_tpm_notification_context_t *ctx);
uint32_t val_tpm_notification_register(val_tpm_notification_context_t *ctx,
                                       uint32_t notification_type,
                                       uint32_t vcpu_id,
                                       uint32_t notification_id,
                                       uint32_t expected_status);
uint32_t val_tpm_notification_unregister(val_tpm_notification_context_t *ctx,
                                         uint32_t expected_status);
uint32_t val_tpm_finish_notified(uint16_t sender_id, uint16_t tpm_sp_id,
                                 uint32_t expected_status);
uint32_t val_tpm_check_pending_notification(const val_tpm_notification_context_t *ctx,
                                            uint32_t expected_present);

#endif /* _VAL_TPM_CRB_COMMON_ABI_H_ */
