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

#define ION_UNIPHIER_DEVNAME     "ion-uniphier"

#define pr_fmt(fmt) ION_UNIPHIER_DEVNAME "-pxs2: " fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <ion/ion.h>
#include <ion/ion_priv.h>

#include "ion_uniphier_core.h"

static struct platform_device *plat_dev;

static struct ion_platform_heap ion_uniphier_pxs2_heaps[] = {
	{
		.type  = ION_HEAP_TYPE_CARVEOUT,
		.id    = 1,
		.name  = "media-heap",
		.base  = 0xc0000000,
		.size  = 0x10000000,
		.align = 0x100000,
		.priv  = NULL,
	},
};

static int __init ion_uniphier_pxs2_init_module(void)
{
	struct ion_platform_data *ion_pdata;
	int result = -ENOMEM;

	pr_debug("%s\n", __func__);

	pr_devel("THIS_MODULE:%p.\n", THIS_MODULE);

	plat_dev = platform_device_alloc(ION_UNIPHIER_DEVNAME, 0);
	if (plat_dev == NULL) {
		pr_warning("platform_device_alloc() failed.\n");
		result = -ENOMEM;
		goto err_out;
	}

	//device info
	//platform_device_add_resources(plat_dev, 
	//	dev->desc->res_regs, dev->desc->n_res_regs);
	//platform_device_add_data(plat_dev, 
	//	&dev, sizeof(struct mn2ws_pcm_dev **));

	ion_pdata = devm_kzalloc(&plat_dev->dev,
		sizeof(*ion_pdata), GFP_KERNEL);
	if (!ion_pdata) {
		pr_warning("devm_kzalloc(ion_pdata) failed.\n");
		result = -ENOMEM;
		goto err_out;
	}

	ion_pdata->heaps = ion_uniphier_pxs2_heaps;
	ion_pdata->nr = ARRAY_SIZE(ion_uniphier_pxs2_heaps);

	plat_dev->dev.platform_data = ion_pdata;

	result = platform_device_add(plat_dev);
	if (result < 0) {
		pr_warning("platform_device_add() failed.\n");
		goto err_out;
	}

	return 0;

err_out:
	platform_device_put(plat_dev);
	plat_dev = NULL;

	return result;
}

static void __exit ion_uniphier_pxs2_exit_module(void)
{
	pr_debug("%s\n", __func__);

	platform_device_del(plat_dev);
}

module_init(ion_uniphier_pxs2_init_module);
module_exit(ion_uniphier_pxs2_exit_module);

MODULE_AUTHOR("Katsuhiro Suzuki <suzuki.katsuhiro@socionext.com>");
MODULE_DESCRIPTION("Socionext UniPhier Ion Driver for PXs2.");
MODULE_LICENSE("GPL");
