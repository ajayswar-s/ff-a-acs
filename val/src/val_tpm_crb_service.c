/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "val_endpoint_info.h"
#include "val_ffa_helpers.h"
#include "val_misc.h"
#include "val_tpm_crb_ffa.h"
#include "val_tpm_crb.h"

static uint32_t g_tpm_active_locality = CRB_LOCALITY_MAX + 1;

/**
 * @brief - Checks whether TPM SP advertises notification support.
 * @param - void.
 * @return - Returns 1 when supported, otherwise 0.
 */
static uint32_t val_tpm_notif_supported(void)
{
    val_endpoint_info_t *ep_info;

    /* Read TPM endpoint properties */
    ep_info = val_get_endpoint_info();
    if (ep_info == NULL)
    {
        return 0;
    }

    return ((ep_info[TPM_SP].ep_properties & FFA_PARTITION_NOTIFICATION) != 0) ? 1 : 0;
}

/**
 * @brief - Handles START(LOCALITY_REQUEST) for a CRB locality.
 * @param locality - TPM locality number.
 * @return - Returns TPM service status code.
 */
static uint32_t val_tpm_crb_start_locality_request(uint32_t locality)
{
    uint32_t ctrl;
    uint32_t ctrl_bits;
    uint64_t loc_ctrl;
    uint64_t loc_sts;

    /* Read CRB locality state */
    loc_ctrl = val_tpm_crb_loc_ctrl(locality);
    loc_sts = val_tpm_crb_loc_sts(locality);
    ctrl = val_tpm_crb_read32(loc_ctrl);
    ctrl_bits = ctrl & TPM_LOC_CTRL_VALID_MASK;

    val_tpm_crb_write32(loc_ctrl, 0);

    /* Handle no-op locality request */
    if (ctrl_bits == 0)
    {
        val_tpm_crb_write32(loc_sts,
                            (g_tpm_active_locality == locality) ?
                            TPM_LOC_STS_GRANTED : 0);
        return CRB_OK;
    }

    /* Handle locality requestAccess */
    if ((ctrl_bits & TPM_LOC_CTRL_REQUEST_ACCESS) != 0)
    {
        if ((g_tpm_active_locality > CRB_LOCALITY_MAX) ||
            (g_tpm_active_locality == locality))
        {
            g_tpm_active_locality = locality;
            val_tpm_crb_write32(loc_sts, TPM_LOC_STS_GRANTED);
        }
        else
        {
            val_tpm_crb_write32(loc_sts, 0);
        }

        return CRB_OK;
    }

    /* Handle locality relinquish */
    if (g_tpm_active_locality == locality)
    {
        val_tpm_crb_write32(loc_sts, 0);
        g_tpm_active_locality = CRB_LOCALITY_MAX + 1;
    }
    else
    {
        val_tpm_crb_write32(loc_sts, 0);
    }

    return CRB_OK;
}

/**
 * @brief - Checks whether a direct request targets the TPM service ABI.
 * @param payload - FF-A direct request arguments.
 * @return - Returns 1 for TPM service requests, otherwise 0.
 */
uint32_t val_tpm_crb_service_is_request(const ffa_args_t *payload)
{
    uint32_t service_func;

    if (payload == NULL)
    {
        return 0;
    }

    /* Check TPM service function ID */
    service_func = (uint32_t)payload->arg4;
    if (((payload->fid == FFA_MSG_SEND_DIRECT_REQ_64) ||
         (payload->fid == FFA_MSG_SEND_DIRECT_REQ_32)) &&
        ((service_func & CRB_FUNCTION_ID_MASK) == CRB_FUNCTION_ID_BASE))
    {
        return 1;
    }

    return 0;
}

/**
 * @brief - Dispatches and responds to TPM service ABI requests.
 * @param payload - FF-A direct request and response arguments.
 * @return - void.
 */
void val_tpm_crb_service_handle_request(ffa_args_t *payload)
{
    uint16_t sender;
    uint16_t receiver;
    uint32_t service_func;
    uint32_t status;
    uint32_t value;
    uint32_t is_64bit_direct_request;

    if (payload == NULL)
    {
        return;
    }

    /* Decode direct request */
    sender = (uint16_t)((payload->arg1 >> 16) & 0xffff);
    receiver = (uint16_t)(payload->arg1 & 0xffff);
    service_func = (uint32_t)payload->arg4;
    is_64bit_direct_request = (payload->fid == FFA_MSG_SEND_DIRECT_REQ_64) ? 1 : 0;
    status = CRB_NOFUNC;
    value = 0;

    /* Handle get_interface_version */
    if (service_func == CRB_GET_INTERFACE_VERSION)
    {
        status = CRB_OK_RESULTS_RETURNED;
        value = CRB_SERVICE_VERSION;
    }
    /* Handle get_feature_info */
    else if (service_func == CRB_GET_FEATURE_INFO)
    {
        if ((uint32_t)payload->arg5 == CRB_FEATURE_NOTIFICATION)
        {
            if (val_tpm_notif_supported() != 0)
            {
                status = CRB_OK;
            }
            else
            {
                status = CRB_NOTSUP;
            }
        }
        else
        {
            status = CRB_INVARG;
        }
    }
    /* Handle start locality request */
    else if (service_func == CRB_START)
    {
        if ((payload->arg5 > 0xff) ||
            (payload->arg6 > CRB_LOCALITY_MAX))
        {
            status = CRB_INVARG;
        }
        else if ((uint32_t)payload->arg5 == CRB_START_TYPE_LOCALITY_REQUEST)
        {
            status = val_tpm_crb_start_locality_request((uint32_t)payload->arg6);
        }
        else
        {
            status = CRB_INVARG;
        }
    }

    /* Send TPM direct response */
    val_memset(payload, 0, sizeof(*payload));
    payload->arg1 = ((uint32_t)receiver << 16) | sender;
    payload->arg4 = status;
    payload->arg5 = value;

    if (is_64bit_direct_request != 0)
    {
        val_ffa_msg_send_direct_resp_64(payload);
    }
    else
    {
        val_ffa_msg_send_direct_resp_32(payload);
    }
}
