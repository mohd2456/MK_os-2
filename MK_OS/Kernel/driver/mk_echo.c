// mk_echo.c
// Minimal character device (echo) as a tiny Linux kernel module.

#ifndef MK_ECHO_C
#define MK_ECHO_C

#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#define DRIVER_NAME "mk_echo"
#define DEVICE_NAME "mk_echo"
#define BUF_SIZE (16 * 1024)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("you");
MODULE_DESCRIPTION("Tiny echo character device for MK testing");
MODULE_VERSION("0.1");

struct echo_dev {
    char *buf;
    size_t head;
    size_t tail;
    size_t used;
    struct mutex lock;
};

static struct echo_dev *edev;

static ssize_t mk_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    ssize_t to_copy = 0;
    if (!edev) return -ENODEV;
    if (count == 0) return 0;

    mutex_lock(&edev->lock);
    if (edev->used == 0) {
        mutex_unlock(&edev->lock);
        return 0;
    }

    to_copy = min(count, edev->used);

    if (edev->tail + to_copy <= BUF_SIZE) {
        if (copy_to_user(user_buf, edev->buf + edev->tail, to_copy)) {
            mutex_unlock(&edev->lock);
            return -EFAULT;
        }
        edev->tail = (edev->tail + to_copy) % BUF_SIZE;
    } else {
        size_t first = BUF_SIZE - edev->tail;
        size_t second = to_copy - first;
        if (copy_to_user(user_buf, edev->buf + edev->tail, first)) {
            mutex_unlock(&edev->lock);
            return -EFAULT;
        }
        if (copy_to_user(user_buf + first, edev->buf, second)) {
            mutex_unlock(&edev->lock);
            return -EFAULT;
        }
        edev->tail = second;
    }
    edev->used -= to_copy;
    mutex_unlock(&edev->lock);
    return to_copy;
}

static ssize_t mk_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    ssize_t space, to_copy;
    if (!edev) return -ENODEV;
    if (count == 0) return 0;

    mutex_lock(&edev->lock);
    space = BUF_SIZE - edev->used;
    if (space == 0) {
        mutex_unlock(&edev->lock);
        return -ENOSPC;
    }

    to_copy = min((size_t)space, count);

    if (edev->head + to_copy <= BUF_SIZE) {
        if (copy_from_user(edev->buf + edev->head, user_buf, to_copy)) {
            mutex_unlock(&edev->lock);
            return -EFAULT;
        }
        edev->head = (edev->head + to_copy) % BUF_SIZE;
    } else {
        size_t first = BUF_SIZE - edev->head;
        size_t second = to_copy - first;
        if (copy_from_user(edev->buf + edev->head, user_buf, first)) {
            mutex_unlock(&edev->lock);
            return -EFAULT;
        }
        if (copy_from_user(edev->buf, user_buf + first, second)) {
            mutex_unlock(&edev->lock);
            return -EFAULT;
        }
        edev->head = second;
    }
    edev->used += to_copy;
    mutex_unlock(&edev->lock);
    return to_copy;
}

static int mk_open(struct inode *inode, struct file *file) { return 0; }
static int mk_release(struct inode *inode, struct file *file) { return 0; }

static const struct file_operations mk_fops = {
    .owner = THIS_MODULE,
    .read = mk_read,
    .write = mk_write,
    .open = mk_open,
    .release = mk_release,
};

static struct miscdevice mk_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &mk_fops,
};

static int __init mk_init(void)
{
    int ret;
    edev = kzalloc(sizeof(*edev), GFP_KERNEL);
    if (!edev) return -ENOMEM;

    edev->buf = kzalloc(BUF_SIZE, GFP_KERNEL);
    if (!edev->buf) { 
        kfree(edev); 
        return -ENOMEM; 
    }
    edev->head = edev->tail = edev->used = 0;
    mutex_init(&edev->lock);

    ret = misc_register(&mk_misc);
    if (ret) {
        kfree(edev->buf);
        kfree(edev);
        pr_err("%s: misc_register failed: %d\n", DRIVER_NAME, ret);
        return ret;
    }

    pr_info("%s: device registered (minor=%d)\n", DRIVER_NAME, mk_misc.minor);
    return 0;
}

static void __exit mk_exit(void)
{
    misc_deregister(&mk_misc);
    kfree(edev->buf);
    kfree(edev);
    pr_info("%s: driver unloaded\n", DRIVER_NAME);
}

module_init(mk_init);
module_exit(mk_exit);

#endif // MK_ECHO_C
