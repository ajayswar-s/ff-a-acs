/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "val_endpoint_info.h"
#include "val_ffa_helpers.h"
#include "val_irq.h"
#include "val_tpm_crb_common_abi.h"

#include <stddef.h>

#define VAL_TPM_ENDPOINT_ID_SHIFT 16

#if (PLATFORM_NS_HYPERVISOR_PRESENT == 0) && (TARGET_LINUX == 0)
static volatile uint32_t g_tpm_sri_seen;

/**
 * @brief - Records TPM SRI interrupt delivery.
 * @param - void.
 * @return - Returns 0 after recording the interrupt.
 */
static int val_tpm_sri_irq_handler(void)
{
    g_tpm_sri_seen = 1;
    return 0;
}
#endif

/**
 * @brief - Returns the current client endpoint ID and TPM SP endpoint ID.
 * @param client_id - Pointer to store the current endpoint ID.
 * @param tpm_sp_id - Pointer to store the TPM SP endpoint ID.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_get_endpoint_ids(uint16_t *client_id, uint16_t *tpm_sp_id)
{
    val_endpoint_info_t *ep_info;

    if ((client_id == NULL) || (tpm_sp_id == NULL))
    {
        return VAL_ERROR;
    }

    /* Read endpoint table */
    ep_info = val_get_endpoint_info();
    if (ep_info == NULL)
    {
        LOG(ERROR, "TPM endpoint info not available\n");
        return VAL_ERROR;
    }

    /* Return client and TPM SP IDs */
    *client_id = val_get_curr_endpoint_id();
    *tpm_sp_id = ep_info[TPM_SP].id;

    return VAL_SUCCESS;
}

/**
 * @brief - Sends a TPM service FF-A direct request.
 * @param sender_id - Endpoint ID of the sender.
 * @param tpm_sp_id - Endpoint ID of the TPM service partition.
 * @param function_id - TPM service ABI function ID.
 * @param arg0 - TPM service ABI argument in x5/w5.
 * @param arg1 - TPM service ABI argument in x6/w6.
 * @param arg2 - TPM service ABI argument in x7/w7.
 * @param payload - FF-A argument structure used for request and response.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_send_abi(uint16_t sender_id, uint16_t tpm_sp_id,
                          uint32_t function_id, uint32_t arg0, uint32_t arg1,
                          uint32_t arg2, ffa_args_t *payload)
{
    if (payload == NULL)
    {
        return VAL_ERROR;
    }

    /* Prepare TPM direct request */
    val_memset(payload, 0, sizeof(*payload));
    payload->arg1 = ((uint32_t)sender_id << VAL_TPM_ENDPOINT_ID_SHIFT) | tpm_sp_id;
    payload->arg4 = function_id;
    payload->arg5 = arg0;
    payload->arg6 = arg1;
    payload->arg7 = arg2;

    /* Send TPM direct request */
    val_ffa_msg_send_direct_req_64(payload);

    return VAL_SUCCESS;
}

/**
 * @brief - Sends a TPM service request and checks the response status.
 * @param sender_id - Endpoint ID of the sender.
 * @param tpm_sp_id - Endpoint ID of the TPM service partition.
 * @param function_id - TPM service ABI function ID.
 * @param arg0 - TPM service ABI argument in x5/w5.
 * @param arg1 - TPM service ABI argument in x6/w6.
 * @param arg2 - TPM service ABI argument in x7/w7.
 * @param expected_status - Expected TPM service status in x4/w4.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_send_and_expect(uint16_t sender_id, uint16_t tpm_sp_id,
                                 uint32_t function_id, uint32_t arg0, uint32_t arg1,
                                 uint32_t arg2, uint32_t expected_status)
{
    ffa_args_t payload;

    /* Send TPM ABI request */
    if (val_tpm_send_abi(sender_id, tpm_sp_id, function_id, arg0, arg1, arg2,
                         &payload) != VAL_SUCCESS)
    {
        return VAL_ERROR;
    }

    /* Check TPM direct response */
    if (payload.fid != FFA_MSG_SEND_DIRECT_RESP_64)
    {
        LOG(ERROR, "Expected TPM direct response, fid=0x%lx\n", payload.fid);
        return VAL_ERROR;
    }

    if ((uint32_t)payload.arg4 != expected_status)
    {
        LOG(ERROR, "Expected TPM status=0x%x, got=0x%lx\n", expected_status, payload.arg4);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 * @brief - Prepares a TPM2_GetCapability command buffer.
 * @param cmd - Command buffer to update.
 * @param cmd_size - Command size to encode in the TPM header.
 * @return - void.
 */
void val_tpm_prepare_get_capability(uint8_t *cmd, uint32_t cmd_size)
{
    if (cmd == NULL)
    {
        return;
    }

    val_memset(cmd, 0, cmd_size);
    if (cmd_size < TPM_MALFORMED_GET_CAPABILITY_CMD_SIZE)
    {
        return;
    }

    val_tpm_be16_write(&cmd[TPM_CMD_TAG_OFFSET], TPM2_ST_NO_SESSIONS);
    val_tpm_be32_write(&cmd[TPM_CMD_SIZE_OFFSET], cmd_size);
    val_tpm_be32_write(&cmd[TPM_CMD_CODE_OFFSET], TPM2_CC_GET_CAPABILITY);
    val_tpm_be32_write(&cmd[10], TPM2_CAP_TPM_PROPERTIES);
    val_tpm_be32_write(&cmd[14], TPM2_PT_FIXED);
    if (cmd_size >= TPM_GET_CAPABILITY_CMD_SIZE)
    {
        val_tpm_be32_write(&cmd[18], 1);
    }
}

/**
 * @brief - Relinquishes a granted TPM locality.
 * @param sender_id - FF-A sender endpoint ID.
 * @param tpm_sp_id - TPM service endpoint ID.
 * @param locality - TPM locality to relinquish.
 * @return - Returns VAL_SUCCESS on expected CRB_OK response.
 */
uint32_t val_tpm_relinquish_locality(uint16_t sender_id, uint16_t tpm_sp_id,
                                     uint32_t locality)
{
    uint64_t loc_ctrl;

    if (locality > CRB_LOCALITY_MAX)
    {
        return VAL_ERROR;
    }

    loc_ctrl = val_tpm_crb_loc_ctrl(locality);
    val_tpm_crb_write32(loc_ctrl, TPM_LOC_CTRL_RELINQUISH);

    return val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_START,
                                   CRB_START_TYPE_LOCALITY_REQUEST,
                                   locality, 0, CRB_OK);
}

/**
 * @brief - Checks whether an MMIO byte range is within the TPM CRB window.
 * @param addr - Start address of the MMIO range.
 * @param size - Size of the MMIO range.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
static uint32_t val_tpm_crb_mmio_range_valid(uint64_t addr, uint32_t size)
{
    uint64_t crb_base;
    uint64_t crb_end;
    uint64_t crb_size;

    if (size == 0)
    {
        return VAL_SUCCESS;
    }

    crb_base = VAL_TPM_CRB_BASE;
    crb_size = ((uint64_t)CRB_LOCALITY_MAX + 1) * VAL_TPM_CRB_LOCALITY_SIZE;
    crb_end = crb_base + crb_size;

    if ((crb_end < crb_base) || (addr < crb_base) || (addr >= crb_end))
    {
        return VAL_ERROR;
    }

    if (size > (crb_end - addr))
    {
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 * @brief - Copies bytes into a CRB MMIO buffer.
 * @param dst - Destination CRB MMIO address.
 * @param src - Source buffer.
 * @param size - Number of bytes to copy.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_crb_copy_to_mmio(uint64_t dst, const uint8_t *src,
                                  uint32_t size)
{
    uint32_t i;

    if ((src == NULL) ||
        (val_tpm_crb_mmio_range_valid(dst, size) != VAL_SUCCESS))
    {
        return VAL_ERROR;
    }

    /* Copy CRB bytes */
    for (i = 0; i < size; i++)
    {
        val_tpm_crb_write8(dst + i, src[i]);
    }

    return VAL_SUCCESS;
}

/**
 * @brief - Copies bytes from a CRB MMIO buffer.
 * @param dst - Destination buffer.
 * @param dst_size - Size of the destination buffer.
 * @param src - Source CRB MMIO address.
 * @param size - Number of bytes to copy.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_crb_copy_from_mmio(uint8_t *dst, uint32_t dst_size,
                                    uint64_t src, uint32_t size)
{
    uint32_t i;

    if ((dst == NULL) || (size > dst_size) ||
        (val_tpm_crb_mmio_range_valid(src, size) != VAL_SUCCESS))
    {
        return VAL_ERROR;
    }

    for (i = 0; i < size; i++)
    {
        dst[i] = val_tpm_crb_read8(src + i);
    }

    return VAL_SUCCESS;
}

/**
 * @brief - Validates the TPM response header and returns response details.
 * @param rsp - TPM response buffer.
 * @param rsp_buf_size - Size of the response buffer.
 * @param rsp_size - Pointer to store the TPM response size.
 * @param rc - Pointer to store the TPM response code.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_validate_response_header(const uint8_t *rsp, uint32_t rsp_buf_size,
                                          uint32_t *rsp_size, uint32_t *rc)
{
    uint32_t size;
    uint32_t response_code;

    if ((rsp == NULL) || (rsp_buf_size < TPM_RSP_HEADER_SIZE))
    {
        return VAL_ERROR;
    }

    if (val_tpm_be16_read(&rsp[TPM_RSP_TAG_OFFSET]) != TPM2_ST_NO_SESSIONS)
    {
        LOG(ERROR, "Invalid TPM response tag=0x%x\n",
            (uint32_t)val_tpm_be16_read(&rsp[TPM_RSP_TAG_OFFSET]));
        return VAL_ERROR;
    }

    /* Read TPM response header */
    size = val_tpm_be32_read(&rsp[TPM_RSP_SIZE_OFFSET]);
    if ((size < TPM_RSP_HEADER_SIZE) || (size > rsp_buf_size))
    {
        LOG(ERROR, "Invalid TPM response size=%u buffer=%u\n", size, rsp_buf_size);
        return VAL_ERROR;
    }

    response_code = val_tpm_be32_read(&rsp[TPM_RSP_RC_OFFSET]);
    if (rsp_size != NULL)
    {
        *rsp_size = size;
    }
    if (rc != NULL)
    {
        *rc = response_code;
    }

    return VAL_SUCCESS;
}

/**
 * @brief - Checks whether the TPM service reports notification support.
 * @param sender_id - Endpoint ID of the sender.
 * @param tpm_sp_id - Endpoint ID of the TPM service partition.
 * @return - Returns VAL_SUCCESS, VAL_SKIP_CHECK, or VAL_ERROR.
 */
uint32_t val_tpm_notification_feature_supported(uint16_t sender_id,
                                                uint16_t tpm_sp_id)
{
    ffa_args_t payload;
    uint32_t status;

    /* Query TPM notification feature */
    status = val_tpm_send_abi(sender_id, tpm_sp_id, CRB_GET_FEATURE_INFO,
                              CRB_FEATURE_NOTIFICATION, 0, 0, &payload);
    if (status != VAL_SUCCESS)
    {
        return status;
    }

    if (payload.fid != FFA_MSG_SEND_DIRECT_RESP_64)
    {
        LOG(ERROR, "Expected direct response for TPM get_feature_info, fid=0x%lx\n",
            payload.fid);
        return VAL_ERROR;
    }

    if ((uint32_t)payload.arg4 == CRB_OK)
    {
        return VAL_SUCCESS;
    }

    if (((uint32_t)payload.arg4 == CRB_NOTSUP) ||
        ((uint32_t)payload.arg4 == CRB_NOFUNC))
    {
        LOG(INFO, "TPM notification feature is not supported, skipping scenario\n");
        return VAL_SKIP_CHECK;
    }

    LOG(ERROR, "Unexpected TPM notification feature status=0x%lx\n", payload.arg4);
    return VAL_ERROR;
}

#if (PLATFORM_NS_HYPERVISOR_PRESENT == 0)
/**
 * @brief - Creates the FF-A notification bitmap used by TPM tests.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS, VAL_SKIP_CHECK, or VAL_ERROR.
 */
uint32_t val_tpm_notification_create_bitmap(val_tpm_notification_context_t *ctx)
{
    ffa_args_t payload;

    if (ctx == NULL)
    {
        return VAL_ERROR;
    }

    val_memset(&payload, 0, sizeof(payload));
    payload.arg1 = ctx->client_id;
    payload.arg2 = VAL_TPM_NOTIFICATION_VCPU_COUNT;

    /* Create FF-A notification bitmap */
    val_ffa_notification_bitmap_create(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        if ((uint32_t)payload.arg2 == FFA_ERROR_NOT_SUPPORTED)
        {
            LOG(INFO, "FF-A notification bitmap create is not supported\n");
            return VAL_SKIP_CHECK;
        }

        LOG(ERROR, "TPM notification bitmap create failed err=0x%lx\n", payload.arg2);
        return VAL_ERROR;
    }

    ctx->bitmap_created = 1;
    return VAL_SUCCESS;
}

/**
 * @brief - Destroys the FF-A notification bitmap used by TPM tests.
 * @param client_id - Endpoint ID of the notification receiver.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
static uint32_t val_tpm_notification_bitmap_destroy(uint16_t client_id)
{
    ffa_args_t payload;

    val_memset(&payload, 0, sizeof(payload));
    payload.arg1 = client_id;

    /* Destroy FF-A notification bitmap */
    val_ffa_notification_bitmap_destroy(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "TPM notification bitmap destroy failed err=0x%lx\n", payload.arg2);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
#else
/**
 * @brief - Skips bitmap creation when the platform does not require it.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS.
 */
uint32_t val_tpm_notification_create_bitmap(val_tpm_notification_context_t *ctx)
{
    (void)ctx;
    return VAL_SUCCESS;
}
#endif

/**
 * @brief - Binds the TPM service notification bitmap.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS, VAL_SKIP_CHECK, or VAL_ERROR.
 */
uint32_t val_tpm_notification_bind(val_tpm_notification_context_t *ctx)
{
    ffa_args_t payload;

    if (ctx == NULL)
    {
        return VAL_ERROR;
    }

    val_memset(&payload, 0, sizeof(payload));
    payload.arg1 = ((uint32_t)ctx->tpm_sp_id << VAL_TPM_ENDPOINT_ID_SHIFT) |
                   ctx->client_id;
    payload.arg2 = 0;
    payload.arg3 = (uint32_t)(ctx->bitmap & 0xffffffff);
    payload.arg4 = (uint32_t)(ctx->bitmap >> 32);

    /* Bind FF-A notification bitmap */
    val_ffa_notification_bind(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        if ((uint32_t)payload.arg2 == FFA_ERROR_NOT_SUPPORTED)
        {
            LOG(INFO, "FF-A notification bind is not supported\n");
            return VAL_SKIP_CHECK;
        }

        LOG(ERROR, "TPM notification bind failed err=0x%lx\n", payload.arg2);
        return VAL_ERROR;
    }

    ctx->bound = 1;
    return VAL_SUCCESS;
}

/**
 * @brief - Unbinds the TPM service notification bitmap.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
static uint32_t val_tpm_notification_unbind(val_tpm_notification_context_t *ctx)
{
    ffa_args_t payload;

    val_memset(&payload, 0, sizeof(payload));
    payload.arg1 = ((uint32_t)ctx->tpm_sp_id << VAL_TPM_ENDPOINT_ID_SHIFT) |
                   ctx->client_id;
    payload.arg3 = (uint32_t)(ctx->bitmap & 0xffffffff);
    payload.arg4 = (uint32_t)(ctx->bitmap >> 32);

    /* Unbind FF-A notification bitmap */
    val_ffa_notification_unbind(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(ERROR, "TPM notification unbind failed err=0x%lx\n", payload.arg2);
        return VAL_ERROR;
    }

    ctx->bound = 0;
    return VAL_SUCCESS;
}

#if (PLATFORM_NS_HYPERVISOR_PRESENT == 0) && (TARGET_LINUX == 0)
/**
 * @brief - Sets up interrupt handling for TPM notification tests.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS, VAL_SKIP_CHECK, or VAL_ERROR.
 */
uint32_t val_tpm_notification_setup_interrupt(val_tpm_notification_context_t *ctx)
{
    ffa_args_t payload;

    if (ctx == NULL)
    {
        return VAL_ERROR;
    }

    val_memset(&payload, 0, sizeof(payload));
    payload.arg1 = FFA_FEATURE_SRI;

    /* Query SRI support */
    val_ffa_features(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        LOG(INFO, "FF-A SRI is not supported, skipping TPM notification flow err=0x%lx\n",
            payload.arg2);
        return VAL_SKIP_CHECK;
    }

    /* Register SRI handler */
    ctx->sri_id = ffa_feature_intid(payload);
    if (val_irq_register_handler(ctx->sri_id, val_tpm_sri_irq_handler) != 0)
    {
        LOG(ERROR, "TPM notification SRI register failed intid=0x%x\n", ctx->sri_id);
        return VAL_ERROR;
    }

    ctx->sri_registered = 1;
    g_tpm_sri_seen = 0;
    return VAL_SUCCESS;
}
#else
/**
 * @brief - Skips interrupt setup when no NS interrupt setup is required.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS.
 */
uint32_t val_tpm_notification_setup_interrupt(val_tpm_notification_context_t *ctx)
{
    (void)ctx;
    return VAL_SUCCESS;
}
#endif

/**
 * @brief - Initializes TPM notification context.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_notification_init(val_tpm_notification_context_t *ctx)
{
    if (ctx == NULL)
    {
        return VAL_ERROR;
    }

    /* Initialize TPM notification context */
    val_memset(ctx, 0, sizeof(*ctx));
    ctx->bitmap = VAL_TPM_NOTIFICATION_BITMAP;

    return val_tpm_get_endpoint_ids(&ctx->client_id, &ctx->tpm_sp_id);
}

/**
 * @brief - Cleans up TPM notification registration, binding, and bitmap state.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_notification_cleanup(val_tpm_notification_context_t *ctx)
{
    uint32_t status;
    uint32_t cleanup_status;

    if (ctx == NULL)
    {
        return VAL_SUCCESS;
    }

    /* Clean up registered notification resources */
    cleanup_status = VAL_SUCCESS;

#if (PLATFORM_NS_HYPERVISOR_PRESENT == 0) && (TARGET_LINUX == 0)
    if (ctx->sri_enabled != 0)
    {
        val_irq_disable(ctx->sri_id);
        ctx->sri_enabled = 0;
    }
#endif

    if (ctx->registered != 0)
    {
        status = val_tpm_notification_unregister(ctx, CRB_OK);
        if (status != VAL_SUCCESS)
        {
            cleanup_status = status;
        }
    }

    if (ctx->bound != 0)
    {
        status = val_tpm_notification_unbind(ctx);
        if (status != VAL_SUCCESS)
        {
            cleanup_status = status;
        }
    }

#if (PLATFORM_NS_HYPERVISOR_PRESENT == 0)
    if (ctx->bitmap_created != 0)
    {
        status = val_tpm_notification_bitmap_destroy(ctx->client_id);
        if (status != VAL_SUCCESS)
        {
            cleanup_status = status;
        }
        ctx->bitmap_created = 0;
    }
#endif

#if (PLATFORM_NS_HYPERVISOR_PRESENT == 0) && (TARGET_LINUX == 0)
    if (ctx->sri_registered != 0)
    {
        if (val_irq_unregister_handler(ctx->sri_id) != 0)
        {
            LOG(ERROR, "TPM notification SRI unregister failed intid=0x%x\n",
                ctx->sri_id);
            cleanup_status = VAL_ERROR;
        }
        ctx->sri_registered = 0;
    }
#endif

    return cleanup_status;
}

/**
 * @brief - Enables interrupt handling before triggering a TPM notification.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_notification_prepare_interrupt(val_tpm_notification_context_t *ctx)
{
#if (PLATFORM_NS_HYPERVISOR_PRESENT == 0) && (TARGET_LINUX == 0)
    if ((ctx == NULL) || (ctx->sri_registered == 0))
    {
        return VAL_ERROR;
    }

    /* Enable SRI interrupt */
    g_tpm_sri_seen = 0;
    val_irq_enable(ctx->sri_id, 0xA);
    ctx->sri_enabled = 1;
#else
    (void)ctx;
#endif

    return VAL_SUCCESS;
}

/**
 * @brief - Checks and disables interrupt handling after TPM notification delivery.
 * @param ctx - TPM notification context.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_notification_complete_interrupt(val_tpm_notification_context_t *ctx)
{
#if (PLATFORM_NS_HYPERVISOR_PRESENT == 0) && (TARGET_LINUX == 0)
    if ((ctx == NULL) || (ctx->sri_registered == 0))
    {
        return VAL_ERROR;
    }

    /* Check SRI interrupt */
    if (g_tpm_sri_seen == 0)
    {
        LOG(ERROR, "TPM notification SRI was not received\n");
        if (ctx->sri_enabled != 0)
        {
            val_irq_disable(ctx->sri_id);
            ctx->sri_enabled = 0;
        }
        return VAL_ERROR;
    }

    val_irq_disable(ctx->sri_id);
    ctx->sri_enabled = 0;
#else
    (void)ctx;
#endif

    return VAL_SUCCESS;
}

/**
 * @brief - Registers the client for TPM service notifications.
 * @param ctx - TPM notification context.
 * @param notification_type - TPM notification type.
 * @param vcpu_id - vCPU ID used for per-vCPU notifications.
 * @param notification_id - FF-A notification ID.
 * @param expected_status - Expected TPM service status in x4/w4.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_notification_register(val_tpm_notification_context_t *ctx,
                                       uint32_t notification_type,
                                       uint32_t vcpu_id,
                                       uint32_t notification_id,
                                       uint32_t expected_status)
{
    uint32_t arg0;
    uint32_t status;

    if (ctx == NULL)
    {
        return VAL_ERROR;
    }

    if ((vcpu_id & 0xffff0000) != 0)
    {
        return VAL_ERROR;
    }

    /* Encode notification registration arguments */
    arg0 = (notification_type << CRB_NOTIFICATION_TYPE_SHIFT) | (vcpu_id & 0xffff);
    status = val_tpm_send_and_expect(ctx->client_id, ctx->tpm_sp_id,
                                     CRB_REGISTER_NOTIFICATION,
                                     arg0, notification_id, 0, expected_status);
    if ((status == VAL_SUCCESS) && (expected_status == CRB_OK))
    {
        ctx->registered = 1;
    }

    return status;
}

/**
 * @brief - Unregisters the client from TPM service notifications.
 * @param ctx - TPM notification context.
 * @param expected_status - Expected TPM service status in x4/w4.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_notification_unregister(val_tpm_notification_context_t *ctx,
                                         uint32_t expected_status)
{
    uint32_t status;

    if (ctx == NULL)
    {
        return VAL_ERROR;
    }

    /* Send unregister notification request */
    status = val_tpm_send_and_expect(ctx->client_id, ctx->tpm_sp_id,
                                     CRB_UNREGISTER_NOTIFICATION,
                                     0, 0, 0, expected_status);
    if ((status == VAL_SUCCESS) && (expected_status == CRB_OK))
    {
        ctx->registered = 0;
    }

    return status;
}

/**
 * @brief - Sends finish_notified to complete outstanding TPM notification work.
 * @param sender_id - Endpoint ID of the sender.
 * @param tpm_sp_id - Endpoint ID of the TPM service partition.
 * @param expected_status - Expected TPM service status in x4/w4.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_finish_notified(uint16_t sender_id, uint16_t tpm_sp_id,
                                 uint32_t expected_status)
{
    /* Send finish_notified request */
    return val_tpm_send_and_expect(sender_id, tpm_sp_id, CRB_FINISH_NOTIFIED,
                                   0, 0, 0, expected_status);
}

/**
 * @brief - Checks whether the expected TPM notification bitmap is pending.
 * @param ctx - TPM notification context.
 * @param expected_present - Expected notification bitmap presence.
 * @return - Returns VAL_SUCCESS or VAL_ERROR.
 */
uint32_t val_tpm_check_pending_notification(const val_tpm_notification_context_t *ctx,
                                            uint32_t expected_present)
{
    ffa_args_t payload;
    ffa_notification_bitmap_t pending_bitmap;

    if (ctx == NULL)
    {
        return VAL_ERROR;
    }

    val_memset(&payload, 0, sizeof(payload));
    payload.arg1 = ctx->client_id;
    payload.arg2 = FFA_NOTIFICATIONS_FLAG_BITMAP_SP;

    /* Read pending notification bitmap */
    val_ffa_notification_get(&payload);
    if (payload.fid == FFA_ERROR_32)
    {
        if ((expected_present == VAL_TPM_NOTIFICATION_ABSENT) &&
            ((uint32_t)payload.arg2 == FFA_ERROR_NODATA))
        {
            return VAL_SUCCESS;
        }

        LOG(ERROR, "TPM notification get failed err=0x%lx\n", payload.arg2);
        return VAL_ERROR;
    }

    pending_bitmap = FFA_NOTIFICATIONS_BITMAP((uint32_t)payload.arg2,
                                              (uint32_t)payload.arg3);

    if (expected_present == VAL_TPM_NOTIFICATION_PRESENT)
    {
        if ((pending_bitmap & ctx->bitmap) == 0)
        {
            LOG(ERROR, "TPM notification bitmap missing, pending=0x%lx\n",
                pending_bitmap);
            return VAL_ERROR;
        }
    }
    else if ((pending_bitmap & ctx->bitmap) != 0)
    {
        LOG(ERROR, "TPM notification bitmap unexpectedly pending=0x%lx\n",
            pending_bitmap);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
