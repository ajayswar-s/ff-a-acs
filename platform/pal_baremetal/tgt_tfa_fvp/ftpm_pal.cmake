#-------------------------------------------------------------------------------
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

include_guard()

set(TPM_FTPM_EDK2_ROOT "${TPM_FTPM_EDK2_ROOT}" CACHE PATH "EDK2 source root")
if(NOT TPM_FTPM_EDK2_ROOT)
    set(TPM_FTPM_EDK2_ROOT "${ROOT_DIR}/platform/common/src/edk2")
endif()
set(TPM_FTPM_EDK2_LIB_BASE "${TPM_FTPM_EDK2_ROOT}/Build/ArmVExpress-FVP-AArch64/DEBUG_GCC/AARCH64")
set(TPM_FTPM_TPMLIB "${TPM_FTPM_EDK2_LIB_BASE}/TcgTpmPkg/Library/TpmLib/TpmLib/OUTPUT/TpmLib.lib")

if(ENABLE_EDK2_STMM AND NOT EXISTS "${TPM_FTPM_TPMLIB}")
    message(STATUS "[ACS] : Building EDK2 TPM libraries from ${TPM_FTPM_EDK2_ROOT}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env
                ACS_ROOT_DIR=${ROOT_DIR}
                CROSS_COMPILE=${CROSS_COMPILE}
                EDK2_TOOLCHAIN_TAG=GCC
                bash ${ROOT_DIR}/tools/cmake/edk2/build_stmm_edk2.sh
        RESULT_VARIABLE TPM_FTPM_BUILD_RESULT
        WORKING_DIRECTORY ${ROOT_DIR})
    if(NOT TPM_FTPM_BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "[ACS] : Failed to build EDK2 TPM libraries")
    endif()
endif()

set(TPM_FTPM_LIBS
    MdePkg/Library/BasePcdLibNull/BasePcdLibNull/OUTPUT/BasePcdLibNull.lib
    MdePkg/Library/BaseLib/BaseLib/OUTPUT/BaseLib.lib
    MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull/OUTPUT/FilterLibNull.lib
    MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic/OUTPUT/BaseIoLibIntrinsic.lib
    ArmPkg/Library/ArmLib/ArmBaseLib/OUTPUT/ArmBaseLib.lib
    ArmPkg/Library/ArmGenericTimerPhyCounterLib/ArmGenericTimerPhyCounterLib/OUTPUT/ArmGenericTimerPhyCounterLib.lib
    ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib/OUTPUT/ArmArchTimerLib.lib
    MdePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib/OUTPUT/BaseRngLibTimerLib.lib
    MdePkg/Library/StandaloneMmServicesTableLib/StandaloneMmServicesTableLib/OUTPUT/StandaloneMmServicesTableLib.lib
    MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib/OUTPUT/BaseSynchronizationLib.lib
    StandaloneMmPkg/Library/StandaloneMmMemoryAllocationLib/StandaloneMmMemoryAllocationLib/OUTPUT/MemoryAllocationLib.lib
    EmbeddedPkg/Library/TimeBaseLib/TimeBaseLib/OUTPUT/TimeBaseLib.lib
    MdePkg/Library/StackCheckLibNull/StackCheckLibNull/OUTPUT/StackCheckLibNull.lib
    StandaloneMmPkg/Library/StandaloneMmHobLib/StandaloneMmHobLib/OUTPUT/HobLib.lib
    CryptoPkg/Library/OpensslLib/OpensslLibFull/OUTPUT/OpensslLibFull.lib
    CryptoPkg/Library/BaseCryptLib/SmmCryptLib/OUTPUT/SmmCryptLib.lib
    TcgTpmPkg/Library/TpmLib/TpmLib/OUTPUT/TpmLib.lib
    MdePkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib/OUTPUT/CompilerIntrinsicsLib.lib
)
foreach(TPM_FTPM_LIB IN LISTS TPM_FTPM_LIBS)
    set(TPM_FTPM_LIB_PATH "${TPM_FTPM_EDK2_LIB_BASE}/${TPM_FTPM_LIB}")
    if(NOT EXISTS "${TPM_FTPM_LIB_PATH}")
        message(FATAL_ERROR "Missing EDK2 TPM library: ${TPM_FTPM_LIB_PATH}")
    endif()
    list(APPEND EXTRA_LINK_LIBS "${TPM_FTPM_LIB_PATH}")
endforeach()

set(FTPM_PAL_SRC
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_tpm.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_tpm_tcg_platform.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_tpm_abi_shim.c
)

set(TPM_FTPM_LIB_ROOT "${TPM_FTPM_EDK2_ROOT}/TcgTpmPkg/Library/TpmLib")
set(TPM_FTPM_TCG_ROOT "${TPM_FTPM_LIB_ROOT}/TPM/TPMCmd")
set(FTPM_PAL_INCLUDE_DIRS ${TPM_FTPM_LIB_ROOT}/Include)
foreach(TPM_FTPM_INC IN ITEMS
        TpmConfiguration
        tpm/include
        tpm/include/private
        tpm/include/private/prototypes
        tpm/include/platform_interface
        tpm/include/platform_interface/prototypes
        tpm/include/tpm_public
        tpm/include/tpm_public/prototypes)
    list(APPEND FTPM_PAL_INCLUDE_DIRS "${TPM_FTPM_TCG_ROOT}/${TPM_FTPM_INC}")
endforeach()

set(FTPM_PAL_COMPILE_DEFINITIONS
    HASH_LIB=Ossl SYM_LIB=Ossl MATH_LIB=TpmBigNum BN_MATH_LIB=Ossl
    USE_PLATFORM_EPS=YES SIXTY_FOUR_BIT_LONG L_ENDIAN
)

set(FTPM_PAL_COMPILE_OPTIONS
    -Wno-unused-function -Wno-implicit-fallthrough -Wno-cast-align
    -Wno-cast-function-type -Wno-missing-braces -Wno-missing-field-initializers
    -Wno-sign-compare -Wno-switch-default -Wno-unused-value -Wno-unused-parameter
    -Wno-missing-prototypes -Wno-missing-declarations -Wno-strict-prototypes
    -Wno-conversion -Wno-sign-conversion
    $<$<COMPILE_LANGUAGE:C>:-include>
    $<$<COMPILE_LANGUAGE:C>:${ROOT_DIR}/platform/common/inc/pal_tpm_platform_abi.h>
)

function(configure_ftpm_pal_target PAL_TARGET)
    target_include_directories(${PAL_TARGET} PRIVATE ${FTPM_PAL_INCLUDE_DIRS})
    target_compile_definitions(${PAL_TARGET} PRIVATE ${FTPM_PAL_COMPILE_DEFINITIONS})
    target_compile_options(${PAL_TARGET} PRIVATE ${FTPM_PAL_COMPILE_OPTIONS})
endfunction()
