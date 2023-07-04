#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <asm/uaccess.h>
#include <linux/sched/clock.h>
#include <linux/math64.h>

enum {
	LK_S_NONE = 0,
	LK_S_SPINLOCK = 1,
	LK_S_MUTEX = 2,
	LK_S_LAST
};
static int lk_sync_mode = LK_S_SPINLOCK;
module_param_named(sync_mode, lk_sync_mode, int, 0444);
MODULE_PARM_DESC(sync_mode, "Synchronization mode to use (0=none,1=spinlock,2=mutex)");

static unsigned long lk_hold_ns = 100;
module_param_named(hold_ns, lk_hold_ns, ulong, 0444);
MODULE_PARM_DESC(hold_ns, "Lock hold time in nano seconds");

static int lk_major = PHONE_MAJOR;

static DEFINE_SPINLOCK(lk_spinlock);
static DEFINE_MUTEX(lk_mutex);

static ssize_t lk_read(struct file *flip, char *buffer, size_t len, loff_t *offset) {
	size_t orig_size = len;
	u64 wait_start;
	u64 spin_start;
	u64 spin_end;

	wait_start = local_clock();

	switch (lk_sync_mode) {
	case LK_S_SPINLOCK:
		spin_lock(&lk_spinlock);
		break;
	case LK_S_MUTEX:
		mutex_lock(&lk_mutex);
		break;
	}
	spin_start = local_clock();
	spin_end = spin_start + lk_hold_ns;
	while (len) {
		if (put_user('l', buffer++))
			break;
		len--;
	}

	do {
		int_sqrt(jiffies);
	} while (local_clock() < spin_end);


	switch (lk_sync_mode) {
	case LK_S_SPINLOCK:
		spin_unlock(&lk_spinlock);
		break;
	case LK_S_MUTEX:
		mutex_unlock(&lk_mutex);
		break;
	}
	return orig_size - len;
}

static ssize_t lk_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {
	return -EINVAL;
}

static int lk_open(struct inode *inode, struct file *file) {
        if (!try_module_get(THIS_MODULE))
                return -ENODEV;
	return 0;
}
/* Called when a process closes our device */
static int lk_release(struct inode *inode, struct file *file) {
	module_put(THIS_MODULE);
	return 0;
}

static struct file_operations file_ops = {
	.read = lk_read,
	.write = lk_write,
	.open = lk_open,
	.release = lk_release
};

static int __init lk_init(void) {
	int rc;
	if (lk_sync_mode < LK_S_NONE || lk_sync_mode >= LK_S_LAST) {
		printk("lock_dev: sync_mode invalid value\n");
		return -EINVAL;
	}
	rc = register_chrdev(lk_major, "lock_dev", &file_ops);
	if (rc < 0) {
		printk(KERN_ALERT "Could not register device: %d\n", lk_major);
		return rc;
	}

	printk(KERN_INFO "lock_dev: loaded with device major number %d\n", lk_major);
	return 0;
}

static void __exit lk_exit(void) {
	unregister_chrdev(lk_major, "lock_dev");
	printk("lock_dev: exit\n");
}
module_init(lk_init);
module_exit(lk_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dmitry Monakhov");
MODULE_DESCRIPTION("Lock primitives test device");
MODULE_VERSION("0.1");
