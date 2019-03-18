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

#define ION_UNIPHIER_DRVNAME     "ion-uniphier"

#define pr_fmt(fmt) ION_UNIPHIER_DRVNAME ": " fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/highmem.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>

#include <ion/ion.h>
#include <ion/ion_priv.h>
#include <ion_of.h>

#include "ion_uniphier_core.h"
#include "uapi/ion_uniphier.h"

struct ion_uniphier_device {
	struct ion_device *ion_dev;
	struct ion_platform_data *ion_pdata;
	struct ion_platform_heap *ion_plat_heaps;
	int ion_num_heaps;
	struct ion_heap **ion_heaps;
	int use_dt;
};

static struct ion_device *ion_dev;
struct ion_device *ion_uniphier_get_ion_device(void)
{
	return ion_dev;
}
EXPORT_SYMBOL(ion_uniphier_get_ion_device);

/*
 * Use Device Tree:
 *   struct ion_of_heap of_heaps[]
 *     --[ion_parse_dt()]-----> struct ion_platform_data *
 *     --[.heaps]-------------> struct ion_platform_heap *
 *     --[ion_heap_create()]--> struct ion_heap *
 *
 * Not use Device Tree:
 *   struct platform_device *
 *     --[.dev]---------------> struct device *
 *     --[.platform_data]-----> struct ion_platform_data *
 *     --[.heaps]-------------> struct ion_platform_heap *
 *     --[ion_heap_create()]--> struct ion_heap *
 */
static struct ion_of_heap of_heaps[] = {
	PLATFORM_HEAP("socionext,media-heap", ION_HEAP_ID_MEDIA, ION_HEAP_TYPE_CARVEOUT, "media"),
	PLATFORM_HEAP("socionext,gpu-heap", ION_HEAP_ID_GPU, ION_HEAP_TYPE_CARVEOUT, "gpu"),
	PLATFORM_HEAP("socionext,fb-heap", ION_HEAP_ID_FB, ION_HEAP_TYPE_CARVEOUT, "fb"),
	PLATFORM_HEAP("socionext,vio-heap", ION_HEAP_ID_VIO, ION_HEAP_TYPE_CARVEOUT, "vio"),
	PLATFORM_HEAP("socionext,ch0-heap", ION_HEAP_ID_CH0, ION_HEAP_TYPE_CARVEOUT, "ch0"),
	PLATFORM_HEAP("socionext,ch1-heap", ION_HEAP_ID_CH1, ION_HEAP_TYPE_CARVEOUT, "ch1"),
	PLATFORM_HEAP("socionext,ch2-heap", ION_HEAP_ID_CH2, ION_HEAP_TYPE_CARVEOUT, "ch2"),
	PLATFORM_HEAP("socionext,hscadbs-heap", ION_HEAP_ID_HSCADBS, ION_HEAP_TYPE_CARVEOUT, "hscadbs"),
	PLATFORM_HEAP("socionext,vmla-heap", ION_HEAP_ID_VMLA, ION_HEAP_TYPE_CARVEOUT, "vmla"),
	PLATFORM_HEAP("socionext,system-heap", ION_HEAP_TYPE_SYSTEM, ION_HEAP_TYPE_SYSTEM, "system"),
	{}
};

/**
 * Get the page table entry of specified virtual address.
 *
 * @param mm memory context that has page tables
 * @param vaddr virtual address
 * @return page table entry
 */
static pte_t *ion_uniphier_pte_offset(struct mm_struct *mm, unsigned long vaddr)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgd = pgd_offset(mm, vaddr);
	if (!pgd || pgd_none(*pgd)) {
		return NULL;
	}

	pud = pud_offset(pgd, vaddr);
	if (!pud || pud_none(*pud)) {
		return NULL;
	}

	pmd = pmd_offset(pud, vaddr);
	if (!pmd || pmd_none(*pmd)) {
		return NULL;
	}

	pte = pte_offset_map(pmd, vaddr);
	if (!pte) {
		return NULL;
	}
	if (pte_none(*pte)) {
		pte_unmap(pte);
		return NULL;
	}

	return pte;
}

/**
 * Convert the specified virtual address to physical address.
 * This function is used only for user space address.
 * (Please use virt_to_phys() for kernel space address.)
 *
 * @param virt virtual address
 * @return page table entry
 */
static unsigned long ion_uniphier_virt_lookup(u64 virt)
{
	u64 pte_v;
	pte_t *pte;

	pte = ion_uniphier_pte_offset(current->mm, virt);
	if (!pte) {
		pr_warning("Cannot get physical address of %llx.\n",
			(unsigned long long)virt);
		return 0;
	}
	pte_v = pte_val(*pte);
	pte_unmap(pte);

	/* FIXME: UniPhier has 34bit physical bus, masked out higher bits */
	pte_v &= 0x3ffffffff;

	return ((pte_v & PAGE_MASK) | (virt & ~PAGE_MASK));
}

static int ion_uniphier_custom_ioctl_dir(unsigned int cmd)
{
	switch (cmd) {
	default:
		return _IOC_DIR(cmd);
	}
}

static long ion_uniphier_custom_ioctl(struct ion_client *client, unsigned int cmd, unsigned long arg)
{
	int dir;
	union {
		struct ion_uniphier_virt_to_phys_data v2p;
		struct ion_uniphier_phys_data phys;
	} buf;

	if (_IOC_SIZE(cmd) > sizeof(buf)) {
		return -EINVAL;
	}

	dir = ion_uniphier_custom_ioctl_dir(cmd);
	if (dir & _IOC_WRITE) {
		if (copy_from_user(&buf, (void *)arg, _IOC_SIZE(cmd))) {
			return -EFAULT;
		}
	}

	switch (cmd) {
	case ION_UNIP_IOC_VIRT_TO_PHYS:
	{
		unsigned long phys, now_phys, next_phys;
		int off = 0, cont = 1;

		if ((unsigned long)buf.v2p.virt > TASK_SIZE) {
			pr_warning("virt:0x%lx is kernel space address.\n",
				(long)buf.v2p.virt);
			return -EINVAL;
		}
		if (buf.v2p.len <= 0) {
			buf.v2p.len = 1;
		}

		phys = ion_uniphier_virt_lookup(buf.v2p.virt);
		next_phys = phys;
		while (off < buf.v2p.len) {
			now_phys = ion_uniphier_virt_lookup(buf.v2p.virt + off);
			if (now_phys == 0) {
				cont = 0;
				pr_warning("virt_lookup() failed. virt:0x%lx.\n",
					(long)(buf.v2p.virt + off));
				break;
			}
			if (now_phys != next_phys) {
				/* not continuous */
				cont = 0;
				pr_warning("not cont. virt:0x%lx, now_phys:%lx, next_phys:%lx.\n",
					(long)(buf.v2p.virt + off), now_phys, next_phys);
				break;
			}
			next_phys = now_phys + PAGE_SIZE;
			off += PAGE_SIZE;
		}

		buf.v2p.phys = phys;
		buf.v2p.cont = cont;

		break;
	}
	case ION_UNIP_IOC_PHYS:
	{
		break;
	}
	/*{
		struct ion_handle *h;
		ion_phys_addr_t addr;
		size_t len;

		h = ion_handle_get_by_id(client, buf.phys.handle);
		if (IS_ERR(h)) {
			pr_warning("ion_handle_get_by_id(phys) failed.\n");
			return PTR_ERR(h);
		}

		ion_phys(client, h, &addr, &len);
		
		buf.phys.phys = addr;
		buf.phys.len = len;
		break;
	}*/
	default:
		pr_warning("Unknown ioctl() cmd:0x%x.\n", cmd);
		return -ENOTTY;
	}

	if (dir & _IOC_READ) {
		if (copy_to_user((void *)arg, &buf, _IOC_SIZE(cmd))) {
			return -EFAULT;
		}
	}

	return 0;
}

static int ion_uniphier_platform_probe(struct platform_device *pdev)
{
	struct ion_uniphier_device *d = NULL;
	struct device_node *node = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	int result = -ENOMEM;
	int i;

	pr_devel("%s\n", __func__);

	d = devm_kzalloc(dev, sizeof(struct ion_uniphier_device), GFP_KERNEL);
	if (!d) {
		pr_warning("devm_kzalloc(uniphier_dev) failed.\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, d);

	d->ion_dev = ion_device_create(ion_uniphier_custom_ioctl);
	ion_dev = d->ion_dev; // for export
	if (IS_ERR_OR_NULL(d->ion_dev)) {
		pr_warning("ion_device_create() failed.\n");
		return -ENOMEM;
	}

	if (node) {
		pr_devel("Probe: Use the Device Tree.\n");
		d->ion_pdata = ion_parse_dt(pdev, of_heaps);
		if (IS_ERR_OR_NULL(d->ion_pdata)) {
			pr_warning("ion_parse_dt() failed.\n");
			result = PTR_ERR(d->ion_pdata);
			d->ion_pdata = NULL;
			goto err_out;
		}
		d->use_dt = 1;
	} else {
		pr_devel("Probe: Use the platform_data of device.\n");
		d->ion_pdata = pdev->dev.platform_data;
		if (!d->ion_pdata) {
			pr_warning("Invalid ion_platform_data or not found.\n");
			result = -ENODEV;
			goto err_out;
		}
		d->use_dt = 0;
	}
	d->ion_plat_heaps = d->ion_pdata->heaps;
	d->ion_num_heaps = d->ion_pdata->nr;

	d->ion_heaps = devm_kzalloc(dev,
		sizeof(struct ion_heap) * d->ion_num_heaps, GFP_KERNEL);
	if (!d->ion_heaps) {
		pr_warning("devm_kzalloc(heaps) failed.\n");
		result = -ENOMEM;
		goto err_out;
	}

	for (i = 0; i < d->ion_num_heaps; i++) {
		d->ion_heaps[i] = ion_heap_create(&d->ion_pdata->heaps[i]);
		if (IS_ERR_OR_NULL(d->ion_heaps[i])) {
			pr_warning("ion_heap_create(i:%d) failed.\n", i);
			result = PTR_ERR(d->ion_heaps[i]);
			result = -ENODEV;
			goto err_out;
		}

		ion_device_add_heap(d->ion_dev, d->ion_heaps[i]);
	}

	pr_info("probe v.0.2.\n");

	return 0;

err_out:
	for (i = 0; i < d->ion_num_heaps; i++) {
		ion_heap_destroy(d->ion_heaps[i]);
		d->ion_heaps[i] = NULL;
	}

	ion_release_dt(pdev, d->ion_pdata);

	if (d->ion_dev) {
		ion_device_destroy(d->ion_dev);
		d->ion_dev = NULL;
	}

	return result;
}

static int ion_uniphier_platform_remove(struct platform_device *pdev)
{
	struct ion_uniphier_device *d = platform_get_drvdata(pdev);
	//struct device *dev = &pdev->dev;
	int i;

	pr_devel("%s\n", __func__);

	for (i = 0; i < d->ion_num_heaps; i++) {
		ion_heap_destroy(d->ion_heaps[i]);
		d->ion_heaps[i] = NULL;
	}

	if (d->use_dt) {
		ion_release_dt(pdev, d->ion_pdata);
	}

	if (d->ion_dev) {
		ion_device_destroy(d->ion_dev);
		d->ion_dev = NULL;
	}

	pr_info("remove.\n");

	return 0;
}

static void ion_uniphier_platform_shutdown(struct platform_device *device)
{
	pr_devel("%s\n", __func__);
}

static int ion_uniphier_platform_suspend(struct platform_device *device, pm_message_t state)
{
	pr_devel("%s\n", __func__);

	return 0;
}

static int ion_uniphier_platform_resume(struct platform_device *device)
{
	pr_devel("%s\n", __func__);

	return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id ion_uniphier_of_match[] = {
	{
		.compatible = "socionext," ION_UNIPHIER_DRVNAME,
	},
	{},
};
MODULE_DEVICE_TABLE(of, ion_uniphier_of_match);
#endif /* CONFIG_OF */

static struct platform_driver ion_uniphier_driver = {
	.driver = {
		.name = ION_UNIPHIER_DRVNAME,
		.of_match_table = of_match_ptr(ion_uniphier_of_match),
	},
	.probe    = ion_uniphier_platform_probe,
	.remove   = ion_uniphier_platform_remove,
	.shutdown = ion_uniphier_platform_shutdown,
	.suspend  = ion_uniphier_platform_suspend,
	.resume   = ion_uniphier_platform_resume,
};
module_platform_driver(ion_uniphier_driver);

MODULE_AUTHOR("Katsuhiro Suzuki <suzuki.katsuhiro@socionext.com>");
MODULE_DESCRIPTION("Socionext UniPhier Ion Driver.");
MODULE_LICENSE("GPL");
