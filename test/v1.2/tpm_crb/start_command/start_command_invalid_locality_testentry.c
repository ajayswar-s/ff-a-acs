/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"

void start_command_invalid_locality_testentry(uint32_t test_num)
{
    if (IS_TEST_FAIL(val_execute_test(test_num, VM1, TPM_SP)))
    {
        return;
    }
}
