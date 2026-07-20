/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"

void get_feature_info_invalid_testentry(uint32_t test_num)
{
    if (IS_TEST_FAIL(val_execute_test(test_num, VM1, TPM_SP)))
    {
        return;
    }
}
