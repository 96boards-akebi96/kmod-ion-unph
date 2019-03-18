/*
 * Based on work from:
 *   Andrew Andrianov <andrew at ncrmnt.org>
 *   Google
 *   The Linux Foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/io.h>
#include "ion/ion.h"
#include "ion/ion_priv.h"
#include "ion_of.h"

static int ion_of_reserved_mem_device_init(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ion_platform_heap *heap = pdev->dev.platform_data;
	struct device_node *np;
	u32 res[4];
	int cell_base, cell_size;
	u64 val_base, val_size;
	int pos, ret, i;

	np = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!np)
		return -ENODEV;

	cell_base = of_n_addr_cells(np);
	cell_size = of_n_size_cells(np);
	if (cell_base + cell_size > ARRAY_SIZE(res)) {
		return -ENOMEM;
	}

	ret = of_property_read_u32_array(np, "reg", res, cell_base + cell_size);
	if (ret < 0)
		return -ENODEV;

	pos = 0;
	for (val_base = 0, i = 0; i < cell_base; i++) {
		val_base <<= 32;
		val_base |= res[pos];
		pos++;
	}
	for (val_size = 0, i = 0; i < cell_size; i++) {
		val_size <<= 32;
		val_size |= res[pos];
		pos++;
	}

	heap->base = val_base;
	heap->size = val_size;

	pr_info("memory-region: base:%lx, size:%lx\n",
		(long)heap->base, (long)heap->size);

	return 0;
}

int ion_parse_dt_heap_common(struct device_node *heap_node,
			struct ion_platform_heap *heap,
			struct ion_of_heap *compatible)
{
	int i;
	u32 type = 0;
	bool keep = false;
	int ret;

	for (i = 0; compatible[i].name != NULL; i++) {
		if (of_device_is_compatible(heap_node, compatible[i].compat))
			break;
	}

	if (compatible[i].name == NULL)
		return -ENODEV;

	ret = of_property_read_u32(heap_node, "heap_type", &type);
	if (ret < 0)
		type = compatible[i].type;

	keep = of_property_read_bool(heap_node, "socionext,keep-contents");

	heap->id = compatible[i].heap_id;
	heap->type = type;
	heap->name = compatible[i].name;
	heap->align = compatible[i].align;
	if (keep)
		heap->flags = ION_PLAT_FLAG_KEEP;

	/* Some kind of callback function pointer? */

	pr_info("%s: id %d type %d name %s align %lx keep %s\n", __func__,
			heap->id, heap->type, heap->name, heap->align,
			(heap->flags & ION_PLAT_FLAG_KEEP) ? "true" : "false");
	return 0;
}

int ion_setup_heap_common(struct platform_device *parent,
			struct device_node *heap_node,
			struct ion_platform_heap *heap,
			struct platform_device *heap_pdev)
{
//	struct platform_device *heap_pdev;
	int ret = 0;

	switch (heap->type) {
	case ION_HEAP_TYPE_CARVEOUT:
	case ION_HEAP_TYPE_CHUNK:
		if (heap->base && heap->size)
			return 0;

//		heap_pdev = heap->priv;
		ret = ion_of_reserved_mem_device_init(&heap_pdev->dev);
		break;
	case ION_HEAP_TYPE_DMA:
		ret = of_reserved_mem_device_init(&heap_pdev->dev);

		break;
	default:
		break;
	}

	return ret;
}

struct ion_platform_data *ion_parse_dt(struct platform_device *pdev,
					struct ion_of_heap *compatible)
{
	int num_heaps, ret;
	const struct device_node *dt_node = pdev->dev.of_node;
	struct device_node *node;
	struct ion_platform_heap *heaps;
	struct ion_platform_data *data;
	int i = 0;

	num_heaps = of_get_available_child_count(dt_node);

	if (!num_heaps)
		return ERR_PTR(-EINVAL);

	heaps = devm_kzalloc(&pdev->dev,
				sizeof(struct ion_platform_heap)*num_heaps,
				GFP_KERNEL);
	if (!heaps)
		return ERR_PTR(-ENOMEM);

	data = devm_kzalloc(&pdev->dev, sizeof(struct ion_platform_data),
				GFP_KERNEL);
	if (!data)
		return ERR_PTR(-ENOMEM);

	for_each_available_child_of_node(dt_node, node) {
		struct platform_device *heap_pdev;

		ret = ion_parse_dt_heap_common(node, &heaps[i], compatible);
		if (ret)
			return ERR_PTR(ret);

		heap_pdev = of_platform_device_create(node, heaps[i].name,
							&pdev->dev);
		if (!heap_pdev)
			return ERR_PTR(-ENOMEM);
		heap_pdev->dev.platform_data = &heaps[i];

		switch (heaps[i].type) {
			case ION_HEAP_TYPE_DMA:
				heaps[i].priv = &heap_pdev->dev;
				break;
			default:
				heaps[i].priv = heap_pdev;
				break;
		}

		ret = ion_setup_heap_common(pdev, node, &heaps[i], heap_pdev);
		if (ret)
			return ERR_PTR(ret);
		i++;
	}


	data->heaps = heaps;
	data->nr = num_heaps;
	return data;
}

int ion_release_dt(struct platform_device *pdev,
			struct ion_platform_data *pdata)
{
	int num_heaps;
	struct ion_platform_heap *heaps;
	int i = 0;

	if (!pdev || !pdata)
		return -EINVAL;

	heaps = pdata->heaps;
	num_heaps = pdata->nr;

	for (i = 0; i < num_heaps; i++) {
		struct platform_device *heap_pdev = heaps[i].priv;

#ifdef OF_POPULATED
		/*
		 * FIXME: We cannot call of_platform_device_destroy() function,
		 * it is static.
		 */
		of_node_clear_flag(heap_pdev->dev.of_node, OF_POPULATED);
#endif
		platform_device_del(heap_pdev);
		heaps[i].priv = NULL;
	}

	return 0;
}
