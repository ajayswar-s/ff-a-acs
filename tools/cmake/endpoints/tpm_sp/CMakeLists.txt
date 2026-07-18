#-------------------------------------------------------------------------------
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
project(tpm_sp LANGUAGES C ASM)
set(EXE_NAME "${PROJECT_NAME}")
add_definitions(-DTPM_SP_COMPILE)
set(VAL_LIB ${EXE_NAME}_val_lib)
set(PAL_LIB ${EXE_NAME}_pal_lib)
set(COMMON_VAL_LIB ${EXE_NAME}_common_val_lib)
set(TEST_LIB ${EXE_NAME}_test_lib)
if(DEFINED FFA_V_1_2)
    set(PLATFORM_FFA_V ${FFA_V_1_2})
    add_definitions(-DPLATFORM_FFA_V=${FFA_V_1_2})
    message(STATUS "[ACS] : TPM_SP PLATFORM_FFA_V is set to ${FFA_V_1_2}")
endif()
include(${ROOT_DIR}/platform/pal_baremetal/${TARGET}/pal.cmake)
include(${COMMON_VAL_PATH}/common_val.cmake)
include(${ROOT_DIR}/val/val.cmake)
include(${ROOT_DIR}/test/test.cmake)
create_executable(${EXE_NAME})
