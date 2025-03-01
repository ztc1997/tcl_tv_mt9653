// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2018 MediaTek Inc.

#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/remoteproc.h>
#include <linux/platform_data/mtk_ccd_controls.h>
#include <linux/platform_data/mtk_ccd.h>
#include <linux/remoteproc/mtk_ccd_mem.h>
#include <linux/rpmsg/mtk_ccd_rpmsg.h>


#include "remoteproc_internal.h"

#define CCD_DEV_NAME	"mtk_ccd"
#define MAX_CODE_SIZE 0x500000

char ccd_firmware[100] = {0};

DECLARE_BUILTIN_FIRMWARE("remoteproc_ccd", ccd_firmware);

struct platform_device *ccd_get_pdev(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *ccd_node;
	struct platform_device *ccd_pdev;

	ccd_node = of_parse_phandle(dev->of_node, "mediatek,ccd", 0);
	if (!ccd_node) {
		dev_err(dev, "can't get ccd node\n");
		return NULL;
	}

	ccd_pdev = of_find_device_by_node(ccd_node);
	if (WARN_ON(!ccd_pdev)) {
		dev_err(dev, "ccd pdev failed\n");
		of_node_put(ccd_node);
		return NULL;
	}

	return ccd_pdev;
}
EXPORT_SYMBOL_GPL(ccd_get_pdev);

void ccd_wdt_handler(struct mtk_ccd *ccd)
{
	rproc_report_crash(ccd->rproc, RPROC_WATCHDOG);
}

void ccd_init_ipi_handler(void *data, unsigned int len, void *priv)
{
	pr_debug("%s default impl\n", __func__);
}

int ccd_ipi_init(struct mtk_ccd *ccd)
{
	return 0;
}

static int ccd_load(struct rproc *rproc, const struct firmware *fw)
{
	const struct mtk_ccd *ccd = rproc->priv;
	struct device *dev = ccd->dev;
	int ret = 0;

	dev_info(dev, "remote_ccd loaded!\n");
	return ret;
}

static int ccd_start(struct rproc *rproc)
{
	struct mtk_ccd *ccd = (struct mtk_ccd *)rproc->priv;
	struct device *dev = ccd->dev;
	int ret = 0;

	dev_info(dev, "ccd started: %p\n", dev);

	return ret;
}

static void *ccd_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct mtk_ccd *ccd = (struct mtk_ccd *)rproc->priv;

	dev_info(ccd->dev, "%s: %p\n", __func__, ccd->dev);

	return NULL;
}

static int ccd_stop(struct rproc *rproc)
{
	struct mtk_ccd *ccd = (struct mtk_ccd *)rproc->priv;
	int ret = 0;

	dev_info(ccd->dev, "%s\n", __func__);

	return ret;
}

static const struct rproc_ops ccd_ops = {
	.start		= ccd_start,
	.stop		= ccd_stop,
	.load		= ccd_load,
	.da_to_va	= ccd_da_to_va,
};

void *ccd_mapping_dm_addr(struct platform_device *pdev, u32 mem_addr)
{
	struct mtk_ccd *ccd = platform_get_drvdata(pdev);
	void *ptr = ccd_da_to_va(ccd->rproc, mem_addr, 0);

	if (!ptr)
		return ERR_PTR(-EINVAL);

	return ptr;
}
EXPORT_SYMBOL_GPL(ccd_mapping_dm_addr);

static struct mtk_ccd_rpmsg_ops ccd_rpmsg_ops = {
	.ccd_send = rpmsg_ccd_ipi_send,
};

static void ccd_add_rpmsg_subdev(struct mtk_ccd *ccd)
{
	ccd->rpmsg_subdev =
		mtk_rpmsg_create_rproc_subdev(to_platform_device(ccd->dev),
					      &ccd_rpmsg_ops);
	if (ccd->rpmsg_subdev)
		rproc_add_subdev(ccd->rproc, ccd->rpmsg_subdev);
}

static void ccd_remove_rpmsg_subdev(struct mtk_ccd *ccd)
{
	if (ccd->rpmsg_subdev) {
		rproc_remove_subdev(ccd->rproc, ccd->rpmsg_subdev);
		mtk_rpmsg_destroy_rproc_subdev(ccd->rpmsg_subdev);
		ccd->rpmsg_subdev = NULL;
	}
}

static ssize_t ccd_debug_read(struct file *filp,
			      char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	char buf[256];
	u32  len = 0;

	len = snprintf(buf, sizeof(buf), "ccu_debug_read\n");

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ccd_debug_write(struct file *filp,
			       const char __user *buffer,
			       size_t count, loff_t *data)
{
	char desc[64];
	s32 len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	return count;
}

static int ccd_open(struct inode *inode,
		    struct file *filp)
{
	int ret = 0;

	struct mtk_ccd *ccd = container_of(inode->i_cdev,
					   struct mtk_ccd,
					   ccd_cdev);
	filp->private_data = ccd;
	dev_info(ccd->dev, "%s: %p\n", __func__, ccd);
	ccd->ccd_masterservice = current;
	ccd->ccd_files = ccd->ccd_masterservice->files;
	return ret;
}

static int ccd_release(struct inode *inode,
		       struct file *filp)
{
	int ret = 0;
	struct ccd_master_status_item master_obj;
	struct mtk_ccd *ccd = (struct mtk_ccd *)filp->private_data;

	master_obj.state = CCD_MASTER_EXIT;
	ccd_master_destroy(ccd, &master_obj);
	dev_info(ccd->dev, "%s: %p\n", __func__, ccd);
	return ret;
}

static long ccd_unlocked_ioctl(struct file *filp, unsigned int cmd,
			       unsigned long arg)
{
	long ret = 0;
	struct mtk_ccd *ccd = (struct mtk_ccd *)filp->private_data;
	unsigned char *user_addr = (unsigned char *)arg;
	struct ccd_master_listen_item listen_obj;
	struct ccd_worker_item read_obj, write_obj;
	struct ccd_master_status_item master_obj;

	switch (cmd) {
	case IOCTL_CCD_MASTER_INIT:
		dev_info(ccd->dev, "enter IOCTL_CCD_MASTER_INIT\n");
		memset(&master_obj, 0, sizeof(master_obj));
		master_obj.state = CCD_MASTER_ACTIVE;
		/*  TBD: Protect by lock? */
		ccd->master_status.state = CCD_MASTER_ACTIVE;

		ret = copy_to_user(user_addr, &master_obj,
				   sizeof(master_obj));
		break;
	case IOCTL_CCD_MASTER_DESTROY:
		dev_info(ccd->dev, "enter IOCTL_CCD_MASTER_DESTROY\n");
		ret = copy_from_user(&master_obj, user_addr,
				     sizeof(master_obj));
		/*  TBD: Protect by lock? */
		ccd->master_status.state = master_obj.state;
		break;
	case IOCTL_CCD_MASTER_LISTEN:
		memset(&listen_obj, 0, sizeof(listen_obj));
		ccd_master_listen(ccd, &listen_obj);

		ret = copy_to_user(user_addr, &listen_obj,
				   sizeof(struct ccd_master_listen_item));
		break;
	case IOCTL_CCD_WORKER_READ:
		ret = copy_from_user(&read_obj, user_addr,
				     sizeof(struct ccd_worker_item));
		ccd_worker_read(ccd, &read_obj);

		ret = copy_to_user(user_addr, &read_obj,
				   sizeof(struct ccd_worker_item));
		break;
	case IOCTL_CCD_WORKER_WRITE:
		ret = copy_from_user(&write_obj, user_addr,
				     sizeof(struct ccd_worker_item));
		ccd_worker_write(ccd, &write_obj);
		break;
	default:
		dev_err(ccd->dev, "Unknown ioctl\n");
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long ccd_ioctl_compat(struct file *filp,
			     unsigned int cmd,
			     unsigned long arg)
{
	long ret = 0;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	ret = filp->f_op->unlocked_ioctl(filp,
					 cmd,
					 (unsigned long)compat_ptr(arg));

	return ret;
}
#endif

static int ccd_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct mtk_ccd *ccd = (struct mtk_ccd *)filp->private_data;
	unsigned long pa_start = vma->vm_pgoff << PAGE_SHIFT;
	long length = 0;
	u32 i;

	dev_info(ccd->dev, "start:0x%llx end:0x%llx offset:0x%llx",
		vma->vm_start, vma->vm_end, vma->vm_pgoff);

	length = vma->vm_end - vma->vm_start;

	for (i = 0; i < CCD_MAP_HW_REG_NUM; i++) {
		if (pa_start == ccd->map_base[i].base &&
		    length <= ccd->map_base[i].len) {
			vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
			dev_info(ccd->dev, "reg base: %x mapped\n", pa_start);
			goto valid_map;
		}
	}

	/* It should be pa here */
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

valid_map:
	dev_info(ccd->dev,
		"remap start: 0x%llx pgoff:0x%llx prot:0x%llx len:0x%llx",
		 vma->vm_start, vma->vm_pgoff, vma->vm_page_prot, length);

	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, length,
			    vma->vm_page_prot) != 0) {
		return -EAGAIN;
	}
	return 0;
}

static const struct file_operations ccd_fops = {
	.owner = THIS_MODULE,
	.open = ccd_open,
	.release = ccd_release,
	.read = ccd_debug_read,
	.write = ccd_debug_write,
	.mmap = ccd_mmap,
	.unlocked_ioctl = ccd_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ccd_ioctl_compat,
#endif
};

static int ccd_regcdev(struct mtk_ccd *ccd)
{
	int ret = 0;
	struct device *dev;

	ret = alloc_chrdev_region(&ccd->ccd_devno, 0, 1, CCD_DEV_NAME);
	if (ret < 0) {
		pr_err("alloc_chrdev_region failed, %d\n", ret);
		return ret;
	}

	/* Attatch file operation. */
	cdev_init(&ccd->ccd_cdev, &ccd_fops);
	ccd->ccd_cdev.owner = THIS_MODULE;
	/* Add to system */
	ret = cdev_add(&ccd->ccd_cdev, ccd->ccd_devno, 1);
	if (ret < 0) {
		pr_err("Attatch file operation failed, %d\n", ret);
		goto err_cdev_add;
	}

	/* Create class register */
	ccd->ccd_class = class_create(THIS_MODULE, "mtk_ccd");
	if (IS_ERR(ccd->ccd_class)) {
		ret = PTR_ERR(ccd->ccd_class);
		pr_err("Unable to create class, err = %d\n", ret);
		goto err_class_create;
	}

	dev = device_create(ccd->ccd_class, NULL,
			    ccd->ccd_devno, NULL,
			    CCD_DEV_NAME);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("Failed to create device: /dev/%s, err = %d\n",
		       CCD_DEV_NAME,
		       ret);
		goto err_device_create;
	}

	return ret;

err_device_create:
	device_destroy(ccd->ccd_class, ccd->ccd_devno);

err_class_create:
	class_destroy(ccd->ccd_class);
	ccd->ccd_class = NULL;

err_cdev_add:
	cdev_del(&ccd->ccd_cdev);
	unregister_chrdev_region(ccd->ccd_devno, 1);
	return ret;
}

static void ccd_unregcdev(struct mtk_ccd *ccd)
{
	/* Release char driver */
	if (ccd->ccd_class) {
		device_destroy(ccd->ccd_class, ccd->ccd_devno);
		class_destroy(ccd->ccd_class);
		ccd->ccd_class = NULL;
	}

	cdev_del(&ccd->ccd_cdev);
	unregister_chrdev_region(ccd->ccd_devno, 1);
}

static void ccd_service_init(struct mtk_ccd *ccd)
{
	ccd->ccd_masterservice = NULL;
	ccd->ccd_files = NULL;
}

static void ccd_service_release(struct mtk_ccd *ccd)
{
	ccd->ccd_masterservice = NULL;
	ccd->ccd_files = NULL;
}

static int ccd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct mtk_ccd *ccd;
	struct resource *res;
	struct rproc *rproc;
	char *fw_name = "remoteproc_ccd";
	int ret;
	u32 i;

	rproc = rproc_alloc(dev,
			    np->name,
			    &ccd_ops,
			    fw_name,
			    sizeof(*ccd));
	if (!rproc) {
		dev_err(dev, "unable to allocate remoteproc\n");
		return -ENOMEM;
	}

	if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(34)))
		dev_dbg(dev, "No suitable DMA available\n");

#ifdef CONFIG_MTK_IOMMU_PGTABLE_EXT
#if (CONFIG_MTK_IOMMU_PGTABLE_EXT > 32)
	*dev->dma_mask = DMA_BIT_MASK(34);
	dev->coherent_dma_mask = DMA_BIT_MASK(34);
#endif
#endif

	ccd = (struct mtk_ccd *)rproc->priv;
	ccd->rproc = rproc;
	ccd->dev = dev;
	platform_set_drvdata(pdev, ccd);
	ccd_regcdev(ccd);
	dev_info(ccd->dev, "ccd is created: %p\n", ccd);

	for (i = 0; i < CCD_MAP_HW_REG_NUM; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (res == NULL) {
			dev_info(dev, "No memory resource got\n");
			continue;
		}
		ccd->map_base[i].base = res->start;
		ccd->map_base[i].len = resource_size(res);
		dev_info(dev, "Reg baseaddr [%d]: 0x%lx 0x%lx", i,
			 ccd->map_base[i].base,
			 ccd->map_base[i].len);
	}

	/* register SCP initialization IPI */
	ret = ccd_ipi_register(pdev,
			       CCD_IPI_INIT,
			       ccd_init_ipi_handler,
			       ccd);
	if (ret) {
		dev_err(dev, "Failed to register IPI_SCP_INIT\n");
		goto free_rproc;
	}

	ccd_add_rpmsg_subdev(ccd);

	ccd_service_init(ccd);
	ccd->ccd_memory = mtk_ccd_mem_init(ccd->dev);

	ret = rproc_add(rproc);
	if (ret)
		goto remove_subdev;

	return 0;

remove_subdev:
	ccd_remove_rpmsg_subdev(ccd);
free_rproc:
	rproc_free(rproc);

	return ret;
}

static int ccd_remove(struct platform_device *pdev)
{
	struct mtk_ccd *ccd = platform_get_drvdata(pdev);

	ccd_service_release(ccd);
	mtk_ccd_mem_release(ccd);
	ccd_unregcdev(ccd);
	ccd_remove_rpmsg_subdev(ccd);
	rproc_del(ccd->rproc);
	rproc_free(ccd->rproc);

	return 0;
}

static const struct of_device_id mtk_ccd_of_match[] = {
	{ .compatible = "mediatek,ccd"},
	{},
};
MODULE_DEVICE_TABLE(of, mtk_ccd_of_match);

static struct platform_driver mtk_ccd_driver = {
	.probe = ccd_probe,
	.remove = ccd_remove,
	.driver = {
		.name = CCD_DEV_NAME,
		.of_match_table = of_match_ptr(mtk_ccd_of_match),
	},
};

static int __init ccd_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_ccd_driver);
	return ret;
}

static void __exit ccd_exit(void)
{
	platform_driver_unregister(&mtk_ccd_driver);
}

late_initcall(ccd_init);
module_exit(ccd_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek CCD driver");
