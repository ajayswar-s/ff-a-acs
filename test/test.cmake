#-------------------------------------------------------------------------------
# Copyright (c) 2021-2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Compile TPM CRB tests only for the TPM CRB suite. Normal SUITE=all keeps
# using the regular ACS database and excludes TPM CRB test sources.
if((${ENABLE_TPM_CRB} EQUAL 1) AND (${SUITE} STREQUAL "tpm_crb"))
file(GLOB TEST_SRC
    "${ROOT_DIR}/test/v1.2/tpm_crb/*/*.h"
    "${ROOT_DIR}/test/v1.2/tpm_crb/*/*.c"
)
list(APPEND TEST_SRC ${ROOT_DIR}/test/common/test_database_tpm.c)
else()
if(${SUITE} STREQUAL "all")
file(GLOB TEST_SRC
    "${ROOT_DIR}/test/*/*/*.h"
    "${ROOT_DIR}/test/*/*/*/*.c"
    "${ROOT_DIR}/test/*/*/*.c"
    "${ROOT_DIR}/test/*/*/*/*.S"
)
list(FILTER TEST_SRC EXCLUDE REGEX "${ROOT_DIR}/test/v1\.2/tpm_crb/.*")
else()
file(GLOB TEST_SRC
    "${ROOT_DIR}/test/v1.0/${SUITE}/*/*.h"
    "${ROOT_DIR}/test/v1.0/${SUITE}/*/*.c"
    "${ROOT_DIR}/test/v1.0/${SUITE}/*/*/*.c"
    "${ROOT_DIR}/test/v1.0/${SUITE}/*/*/*.S"
    "${ROOT_DIR}/test/v1.1/${SUITE}/*/*.h"
    "${ROOT_DIR}/test/v1.1/${SUITE}/*/*.c"
    "${ROOT_DIR}/test/v1.1/${SUITE}/*/*/*.c"
    "${ROOT_DIR}/test/v1.1/${SUITE}/*/*/*.S"
    "${ROOT_DIR}/test/v1.2/${SUITE}/*/*.h"
    "${ROOT_DIR}/test/v1.2/${SUITE}/*/*.c"
    "${ROOT_DIR}/test/v1.2/${SUITE}/*/*/*.c"
    "${ROOT_DIR}/test/v1.2/${SUITE}/*/*/*.S"
)

endif()

if(${TARGET_LINUX} STREQUAL 1)
    list(APPEND TEST_SRC ${ROOT_DIR}/test/common/test_database_v_1_2_linux.c)
elseif(${XEN_SUPPORT} STREQUAL 1)
    list(APPEND TEST_SRC ${ROOT_DIR}/test/common/test_database_v_1_2_xen.c)
else()
    if((${PLATFORM_FFA_V_ALL} EQUAL 1) OR (${PLATFORM_FFA_V_1_2} EQUAL 1))
        list(APPEND TEST_SRC ${ROOT_DIR}/test/common/test_database_v_1_2.c)
    elseif(${PLATFORM_FFA_V_MULTI} EQUAL 1)
        list(APPEND TEST_SRC ${ROOT_DIR}/test/common/test_database_v_multi.c)
    elseif(${PLATFORM_FFA_V_1_1} EQUAL 1)
        list(APPEND TEST_SRC ${ROOT_DIR}/test/common/test_database_v_1_1.c)
    elseif(${PLATFORM_FFA_V_1_0} EQUAL 1)
        list(APPEND TEST_SRC ${ROOT_DIR}/test/common/test_database_v_1_0.c)
    endif()
endif()
endif()

# Create TEST library
add_library(${TEST_LIB} STATIC ${TEST_SRC})

set(MY_INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ROOT_DIR}/val/inc/
    ${ROOT_DIR}/platform/common/inc/
    ${ROOT_DIR}/test/common/
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/inc/
    ${COMMON_VAL_PATH}/inc/
)

if(DEFINED VM_TARGET_LINUX_APP)
    list(APPEND MY_INCLUDE_DIRS ${ROOT_DIR}/linux-acs/ffa-acs-drv/inc/)
else()
    list(APPEND MY_INCLUDE_DIRS
        ${ROOT_DIR}/platform/common/inc/aarch64/
        ${ROOT_DIR}/platform/driver/inc/)
endif()
target_include_directories(${TEST_LIB} PRIVATE ${MY_INCLUDE_DIRS})

unset(MY_INCLUDE_DIRS)
unset(TEST_SRC)
