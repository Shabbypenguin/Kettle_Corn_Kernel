/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*
 * Modem Restart Notifier -- Provides notification
 *			     of modem restart events.
 */

#include <linux/notifier.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <mach/sec_debug.h>
#include "modem_notifier.h"

#define DEBUG

#ifdef CONFIG_SEC_DEBUG_LOW_LOG
#include <asm/uaccess.h>
#include <linux/io.h>
#endif

static struct srcu_notifier_head modem_notifier_list;
static struct workqueue_struct *modem_notifier_wq;

static void notify_work_smsm_init(struct work_struct *work)
{
	modem_notify(0, MODEM_NOTIFIER_SMSM_INIT);
}
static DECLARE_WORK(modem_notifier_smsm_init_work, &notify_work_smsm_init);

void modem_queue_smsm_init_notify(void)
{
	int ret;

	ret = queue_work(modem_notifier_wq, &modem_notifier_smsm_init_work);

	if (!ret)
		printk(KERN_ERR "%s\n", __func__);
}
EXPORT_SYMBOL(modem_queue_smsm_init_notify);

static void notify_work_start_reset(struct work_struct *work)
{
	modem_notify(0, MODEM_NOTIFIER_START_RESET);
}
static DECLARE_WORK(modem_notifier_start_reset_work, &notify_work_start_reset);

void modem_queue_start_reset_notify(void)
{
	int ret;

	ret = queue_work(modem_notifier_wq, &modem_notifier_start_reset_work);

	if (!ret)
		printk(KERN_ERR "%s\n", __func__);
}
EXPORT_SYMBOL(modem_queue_start_reset_notify);

static void notify_work_end_reset(struct work_struct *work)
{
	modem_notify(0, MODEM_NOTIFIER_END_RESET);
}
static DECLARE_WORK(modem_notifier_end_reset_work, &notify_work_end_reset);

void modem_queue_end_reset_notify(void)
{
	int ret;

	ret = queue_work(modem_notifier_wq, &modem_notifier_end_reset_work);

	if (!ret)
		printk(KERN_ERR "%s\n", __func__);
}
EXPORT_SYMBOL(modem_queue_end_reset_notify);

int modem_register_notifier(struct notifier_block *nb)
{
	int ret;

	ret = srcu_notifier_chain_register(
		&modem_notifier_list, nb);

	return ret;
}
EXPORT_SYMBOL(modem_register_notifier);

int modem_unregister_notifier(struct notifier_block *nb)
{
	int ret;

	ret = srcu_notifier_chain_unregister(
		&modem_notifier_list, nb);

	return ret;
}
EXPORT_SYMBOL(modem_unregister_notifier);

void modem_notify(void *data, unsigned int state)
{
	srcu_notifier_call_chain(&modem_notifier_list, state, data);
}
EXPORT_SYMBOL(modem_notify);

#if defined(CONFIG_DEBUG_FS)
static int debug_reset_start(const char __user *buf, int count)
{
	modem_queue_start_reset_notify();
	return 0;
}

static int debug_reset_end(const char __user *buf, int count)
{
	modem_queue_end_reset_notify();
	return 0;
}

static ssize_t debug_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	int (*fling)(const char __user *buf, int max) = file->private_data;
	fling(buf, count);
	return count;
}

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static const struct file_operations debug_ops = {
	.write = debug_write,
	.open = debug_open,
};

static struct dentry *debug_create(const char *name,
			mode_t mode, struct dentry *dent,
			 int (*fling)(const char __user *buf, int max))
{
	struct dentry *dentry_file;
	dentry_file = debugfs_create_file(name, mode, dent, fling, &debug_ops);
	if (IS_ERR(dentry_file))
		return NULL;
	return dentry_file;
}

static int modem_notifier_debugfs_init(void)
{
	struct dentry *dent;
	struct dentry *reset_start_file, *reset_end_file;

	dent = debugfs_create_dir("modem_notifier", 0);
	if (IS_ERR(dent))
		return PTR_ERR(dent);

	reset_start_file = debug_create("reset_start", 0444, dent,
							debug_reset_start);
	if (reset_start_file == NULL) {
		debugfs_remove(dent);
		return PTR_ERR(reset_start_file);
	}
	reset_end_file = debug_create("reset_end", 0444, dent, debug_reset_end);
	if (reset_end_file == NULL) {
		debugfs_remove(reset_start_file);
		debugfs_remove(dent);
		return PTR_ERR(reset_end_file);
	}
	return 0;
}
#else
static void modem_notifier_debugfs_init(void) {}
#endif

#define RESET_REASON_NORMAL			0x1A2B3C00
#if defined(DEBUG)
static int modem_notifier_test_call(struct notifier_block *this,
				  unsigned long code,
				  void *_cmd)
{
	switch (code) {
	case MODEM_NOTIFIER_START_RESET:
		printk(KERN_ERR "Notify: start reset\n");
		break;
	case MODEM_NOTIFIER_END_RESET:
		printk(KERN_ERR "Notify: end reset\n");
		break;
	case MODEM_NOTIFIER_SMSM_INIT:
	{
		printk(KERN_ERR "Notify: smsm init\n");

#ifdef CONFIG_SEC_DEBUG_LOW_LOG
		if (sec_debug_is_enabled() == 0 &&
		sec_debug_get_reset_reason() != RESET_REASON_NORMAL) {

				loff_t pos = 0;
				struct file *fp;
				mm_segment_t old_fs;
				static char dump_filename[100];
				unsigned char *logicalKlogBase;

				logicalKlogBase = ioremap(
				(sec_log_reserve_base+8), 512*1024);
				/* change to KERNEL_DS address limit */
				old_fs = get_fs();
				set_fs(get_ds());

				/* open file to write */
				sprintf(dump_filename, "/data/resetdump");

				fp = filp_open(dump_filename,
						O_WRONLY|O_CREAT, 0666);
				if (!fp) {
						printk(KERN_EMERG "failed to open the file\n");
						goto exit;
		}
				/* Write buf to file */
				fp->f_op->write(fp,
				logicalKlogBase, 512*1024, &pos);

				/* close file before return */
				if (fp)
					filp_close(fp, NULL);

exit:
		/* restore previous address limit */
		iounmap((void __iomem *)logicalKlogBase);
		set_fs(old_fs);
		}
#endif
	}
		break;
	default:
		printk(KERN_ERR "Notify: general\n");
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block nb = {
	.notifier_call = modem_notifier_test_call,
};

static void register_test_notifier(void)
{
	modem_register_notifier(&nb);
}
#endif

static int __init init_modem_notifier_list(void)
{
	int ret;
	srcu_init_notifier_head(&modem_notifier_list);
	ret = modem_notifier_debugfs_init();
	if (ret < 0)
		return ret;
#if defined(DEBUG)
	register_test_notifier();
#endif

	/* Create the workqueue */
	modem_notifier_wq = create_singlethread_workqueue("modem_notifier");
	if (!modem_notifier_wq) {
		srcu_cleanup_notifier_head(&modem_notifier_list);
		return -ENOMEM;
	}

	return 0;
}
module_init(init_modem_notifier_list);
