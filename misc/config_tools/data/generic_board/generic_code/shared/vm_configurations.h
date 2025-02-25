/*
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef VM_CONFIGURATIONS_H
#define VM_CONFIGURATIONS_H

#include <misc_cfg.h>
#include <pci_devices.h>
/* SERVICE_VM_NUM can only be 0 or 1; When SERVICE_VM_NUM is 1, MAX_POST_VM_NUM must be 0 too. */
#define PRE_VM_NUM 0U
#define SERVICE_VM_NUM 1U
#define MAX_POST_VM_NUM 15U
#define CONFIG_MAX_VM_NUM 16U
/* SERVICE_VM == VM0 */
#define SERVICE_VM_OS_BOOTARGS SERVICE_VM_ROOTFS SERVICE_VM_OS_CONSOLE SERVICE_VM_IDLE SERVICE_VM_BOOTARGS_DIFF

#endif /* VM_CONFIGURATIONS_H */
