/*
 * Ion driver for Socionext UniPhier series.
 *
 * Copyright (c) 2016 Socionext Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UAPI_LINUX_ION_UNIPHIER_H__
#define _UAPI_LINUX_ION_UNIPHIER_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#include "ion.h"

enum ion_heap_id_uniphier {
	ION_HEAP_ID_MEDIA = ION_NUM_HEAPS - 1,
	ION_HEAP_ID_GPU   = ION_NUM_HEAPS - 2,
	ION_HEAP_ID_FB    = ION_NUM_HEAPS - 3,
	ION_HEAP_ID_VIO   = ION_NUM_HEAPS - 4,
	ION_HEAP_ID_CH0   = ION_NUM_HEAPS - 5,
	ION_HEAP_ID_CH1   = ION_NUM_HEAPS - 6,
	ION_HEAP_ID_CH2   = ION_NUM_HEAPS - 7,
	ION_HEAP_ID_HSCADBS = ION_NUM_HEAPS - 8,
	ION_HEAP_ID_VMLA  = ION_NUM_HEAPS - 9,
};

#define ION_HEAP_ID_MEDIA_MASK    (1 << ION_HEAP_ID_MEDIA)
#define ION_HEAP_ID_GPU_MASK      (1 << ION_HEAP_ID_GPU)
#define ION_HEAP_ID_FB_MASK       (1 << ION_HEAP_ID_FB)
#define ION_HEAP_ID_VIO_MASK      (1 << ION_HEAP_ID_VIO)
#define ION_HEAP_ID_CH0_MASK      (1 << ION_HEAP_ID_CH0)
#define ION_HEAP_ID_CH1_MASK      (1 << ION_HEAP_ID_CH1)
#define ION_HEAP_ID_CH2_MASK      (1 << ION_HEAP_ID_CH2)
#define ION_HEAP_ID_HSCADBS_MASK  (1 << ION_HEAP_ID_HSCADBS)
#define ION_HEAP_ID_VMLA_MASK     (1 << ION_HEAP_ID_VMLA)

/**
 * struct ion_handle_data - a handle passed to/from the kernel
 *
 * @param handle  A handle of Ion buffer.
 */
struct ion_uniphier_handle_data {
	ion_user_handle_t handle;
};

/**
 * struct ion_uniphier_virt_to_phys_data - a physical address from virtual address
 *
 * @param handle  A handle of Ion buffer.
 * @param virt    A virtual address of user buffer.
 * @param len     A length of user buffer.
 * @param phys    A physical address of user buffer.
 * @param cont    An user buffer is physical-contineous or not.
 */
struct ion_uniphier_virt_to_phys_data {
	ion_user_handle_t handle;
	uint64_t virt;
	uint64_t len;
	uint64_t phys;
	int cont;
};

/**
 * struct ion_uniphier_phys_data - a physical address of the buffer
 *
 * @param handle  A handle of Ion buffer.
 * @param phys    A physical address of Ion buffer.
 * @param len     A length of Ion buffer.
 */
struct ion_uniphier_phys_data {
	ion_user_handle_t handle;
	uint64_t phys;
	uint64_t len;
};


#define ION_UNIP_IOC_MAGIC           'U'
#define ION_UNIP_IOC_VIRT_TO_PHYS    _IOWR(ION_UNIP_IOC_MAGIC, 0, struct ion_uniphier_virt_to_phys_data)
#define ION_UNIP_IOC_PHYS            _IOWR(ION_UNIP_IOC_MAGIC, 1, struct ion_uniphier_phys_data)


#endif /* _UAPI_LINUX_ION_UNIPHIER_H__ */
