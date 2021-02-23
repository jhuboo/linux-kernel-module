/*
 * chardev.c : creates a readonly char devices that says how
 *             many times you've read from the dev file
 * by Anvesh G. Jhuboo
 * on Feb/23/2021
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux.fs.h>
#include <asm/uaccess.h>

/* Prototypes */
#define SUCCESS     0
#define DEVICE_NAME "chardev"   /* Dev name as in /proc/devices */
#define BUF_LEN     80          /* Max length of msg from device */

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

/* Global Vars */
static int Major;               /* Major no assigned to device driver */
static int Device_Open = 0;     /* Used to prevent multiple access to device */
static char msg[BUF_LEN];       /* Msg device will printed when prompted */
static char *msg_Ptr;

static struct file_operations fops = {
    read: device_read,
    write: device_write,
    open: device_open,
    release: device_release,
};


/* init_module: called when module is loaded, return Success or not */
int init_module(void)
{
    Major = register_chrdev(0, DEVICE_NAME, &fops);

    if (Major < 0) {
        printk(KERN_ALERT "Registering char device failed with %d\n", Major);
        return Major;
    }

    printk(KERN_INFO "Major No: %d", Major);
    printk(KERN_INFO "Device Name: %s", DEVICE_NAME);
    return SUCCESS;
}


/* cleanup_module: called when modules is unloaded, unregistering device */
void  cleanup_module(void)
{
    int n = unregister_chrdev(Major, DEVICE_NAME);
    if (n < 0)
        printk(KERN_ALERT "Error in unregister_chrdev: %d", n);
}

/*
 * Methods
 */

/* device_open: increments counter when process tries to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
    static int counter = 0;

    if (Device_Open)
        reutrn -EBUSY;      /* Device busy, and can't be shared */

    Device_Open++;
    sprintf(msg, "Counter: %d", ++counter);
    msg_Ptr = msg;
    try_module_get(THIS_MODULE);

    return SUCCESS;
}


/* device_release: Decrements usage count when process closes device file */
static int device_release(struct inode *node, struct file *file)
{
    Device_Open--;      /* Get ready for next caller */
    module_put(THIS_MODULE);

    return 0;
}


/* device_read: Attempts to read device file, returning bytes read */
static ssize_t device_read(struct file *fp, char *buffer,
                        size_t length, loff_t *offset)
{
    int bytes_read = 0;

    if (*msg_Ptr == 0)
        return 0;

    while (length && *msg_Ptr) {
        /*
         * Buffer is in user data segment, not Kernel segment
         * so '*' won't work. We need to use put_user which copies
         * date from kernel data segment to user data_segment
         */
        put_user(*(msg_Ptr++), buffer++);
        length--;
        bytes_read++;
    }

    return bytes_read;
}


/* device_write: called when process writes to device file */
static ssize_t device_write(struct file *fp, const char *buffer,
                        size_t len, loff_t *off)
{
    printk(KERN_ALERT "Operation not supported\n");
    return -EINVAL;     /* Invalid Argument */
}
