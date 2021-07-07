// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *   Copyright (C) 2021, LG Electronics.
 *   Author(s): Hyunchul Lee <hyc.lee@gmail.com>
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "stats.h"

static struct proc_dir_entry *ksmbd_proc_fs;

struct proc_dir_entry *ksmbd_proc_create(const char *name,
			     int (*show)(struct seq_file *m, void *v),
			     void *v)
{
	return proc_create_single_data(name, 0400, ksmbd_proc_fs,
			   show, v);
}

static int proc_show_ksmbd_stats(struct seq_file *m, void *v)
{
	int i;

	seq_puts(m, "Server\n");
	seq_printf(m, "sessions: %lld\n",
		   ksmbd_counter_sum(KSMBD_COUNTER_SESSIONS));
	seq_printf(m, "tree connects: %lld\n",
		   ksmbd_counter_sum(KSMBD_COUNTER_TREE_CONNS));
	seq_printf(m, "read bytes: %lld\n",
		   ksmbd_counter_sum(KSMBD_COUNTER_READ_BYTES));
	seq_printf(m, "written bytes: %lld\n",
		   ksmbd_counter_sum(KSMBD_COUNTER_WRITE_BYTES));
	seq_puts(m, "\nSmb2\n");
	for (i = 0; i < KSMBD_COUNTER_MAX_REQS; i++)
		seq_printf(m, "%d:\t%lld\n", i,
			   ksmbd_counter_sum(KSMBD_COUNTER_FIRST_REQ + i));
	return 0;
}

void ksmbd_proc_cleanup(void)
{
	int i;

	if (ksmbd_proc_fs == NULL)
		return;

	proc_remove(ksmbd_proc_fs);

	for (i = 0; i < ARRAY_SIZE(ksmbd_counters.counters); i++)
		percpu_counter_destroy(&ksmbd_counters.counters[i]);

	ksmbd_proc_fs = NULL;
}

void ksmbd_proc_reset(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ksmbd_counters.counters); i++)
		percpu_counter_set(&ksmbd_counters.counters[i], 0);
}

void ksmbd_proc_init(void)
{
	int i;
	int retval;

	ksmbd_proc_fs = proc_mkdir("fs/ksmbd", NULL);
	if (ksmbd_proc_fs == NULL)
		return;

	if (proc_mkdir_mode("sessions", 0400, ksmbd_proc_fs) == NULL)
		goto err_out;

	for (i = 0; i < ARRAY_SIZE(ksmbd_counters.counters); i++) {
		retval = percpu_counter_init(
				&ksmbd_counters.counters[i], 0, GFP_KERNEL);
		if (retval)
			goto err_out;
	}

	if (ksmbd_proc_create("stats", proc_show_ksmbd_stats, NULL) == NULL)
		goto err_out;

	ksmbd_proc_reset();
	return;
err_out:
	ksmbd_proc_cleanup();
}
