/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"

const test_suite_info_t test_suite_list[] = {
    {0, ""},
    {TESTSUITE_SETUP_DISCOVERY, ""},
    {TESTSUITE_DIRECT_MESSAGING, ""},
    {TESTSUITE_INDIRECT_MESSAGING, ""},
    {TESTSUITE_MEMORY_MANAGE, ""},
    {TESTSUITE_NOTIFICATIONS, ""},
    {TESTSUITE_INTERRUPTS, ""},
    {TESTSUITE_TPM, "tpm_crb"},
};

const test_db_t test_list[] = {
    /* {suite_num, "testname", client_fn_list, server_fn_list} */
    {0, "", NULL, NULL, NULL, NULL, NULL, 0},

#if (SUITE == tpm_crb)
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, get_interface_version,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, tpm_crb_unknown_function_id,
        FFA_MSG_SEND_DIRECT_REQ_64),

    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, get_feature_info,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, get_feature_info_invalid,
        FFA_MSG_SEND_DIRECT_REQ_64),

    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, start_locality_flow,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, notification_start_locality,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, start_locality_ctrl_data,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, start_locality_relinquish_inactive,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, start_locality_invalid_args,
        FFA_MSG_SEND_DIRECT_REQ_64),

    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, start_command,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, notification_start_command,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, start_command_invalid_locality,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, start_command_no_execute,
        FFA_MSG_SEND_DIRECT_REQ_64),
    CLIENT_TEST_FN_ONLY(
        TESTSUITE_TPM, start_command_invalid_buffer,
        FFA_MSG_SEND_DIRECT_REQ_64),
#endif

    {0, "", NULL, NULL, NULL, NULL, NULL, 0},
};

const uint32_t total_tests = sizeof(test_list)/sizeof(test_list[0]);
