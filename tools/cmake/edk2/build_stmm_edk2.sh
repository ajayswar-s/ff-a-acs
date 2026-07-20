#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set -euo pipefail

: "${ACS_ROOT_DIR:?ACS_ROOT_DIR is not set}"
: "${CROSS_COMPILE:?CROSS_COMPILE is not set}"

EDK2_DIR="$ACS_ROOT_DIR/platform/common/src/edk2"
EDK2_PLATFORMS_DIR="$ACS_ROOT_DIR/platform/common/src/edk2-platforms"
EDK2_PLATFORMS_LINK="$EDK2_DIR/edk2-platforms"
EDK2_PLATFORM_COMPAT_LINK="$EDK2_DIR/Platform"
EDK2_ARM_LIB_COMPAT_LINK="$EDK2_DIR/ArmPkg/Library/ArmLib"
EDK2_TOOLCHAIN_TAG="${EDK2_TOOLCHAIN_TAG:-GCC}"

for tool in git make python3 gcc g++ "${CROSS_COMPILE}gcc"; do
    command -v "$tool" >/dev/null 2>&1 || { echo "Error: $tool is not installed or not in PATH." >&2; exit 1; }
done

git -C "$ACS_ROOT_DIR" submodule sync platform/common/src/edk2 platform/common/src/edk2-platforms
git -C "$ACS_ROOT_DIR" submodule update --init platform/common/src/edk2 platform/common/src/edk2-platforms
git -C "$EDK2_DIR" submodule sync
git -C "$EDK2_DIR" submodule update --init

if [ ! -d "$EDK2_PLATFORMS_DIR" ]; then
    echo "Error: $EDK2_PLATFORMS_DIR does not exist." >&2
    exit 1
fi

if [ ! -e "$EDK2_PLATFORMS_LINK" ]; then
    ln -s ../edk2-platforms "$EDK2_PLATFORMS_LINK"
fi

if [ ! -e "$EDK2_PLATFORM_COMPAT_LINK" ]; then
    ln -s ../edk2-platforms/Platform "$EDK2_PLATFORM_COMPAT_LINK"
fi

if [ ! -e "$EDK2_ARM_LIB_COMPAT_LINK" ] && [ -d "$EDK2_DIR/MdePkg/Library/ArmLib" ]; then
    ln -s ../../MdePkg/Library/ArmLib "$EDK2_ARM_LIB_COMPAT_LINK"
fi

pushd "$EDK2_DIR" >/dev/null
unset WORKSPACE
export PACKAGES_PATH="$EDK2_DIR:$EDK2_PLATFORMS_DIR"
export GCC_AARCH64_PREFIX="$CROSS_COMPILE"
export GCC5_AARCH64_PREFIX="$CROSS_COMPILE"
export PYTHON_COMMAND=python3

make -C BaseTools/Source/C clean CC=gcc CXX=g++
make -C BaseTools/Source/C CC=gcc CXX=g++
set +u
source edksetup.sh
set -u

build -a AARCH64 -t "$EDK2_TOOLCHAIN_TAG" -b DEBUG \
    -p Platform/ARM/VExpressPkg/PlatformStandaloneMm.dsc \
    -D ENABLE_UEFI_SECURE_VARIABLE=TRUE \
    -D ENABLE_TPM=TRUE \
    --pcd PcdTpmUniqueValue=0x12345678
popd >/dev/null

echo "EDK2 StandaloneMM TPM libraries built under $EDK2_DIR/Build/ArmVExpress-FVP-AArch64/DEBUG_${EDK2_TOOLCHAIN_TAG}/AARCH64"
