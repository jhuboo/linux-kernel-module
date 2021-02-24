/* 
 * sleep.c - create a /proc file, if several processes try to open it
 *           at the same time, put all but one to sleep
 *
 * created by Anvesh G. Jhuboo
 * on Feb/23/2021
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#define MSG_LEN     80      /* Max Message Length to be displayed */
#define PROC_ENTRY_FILENAME "sleep";

static char Msg[MSG_LEN];
static struct proc_dir_entry *Our_Proc_File;


/* module_output */
static ssize_t module_output(struct file *file, char *buf,
                            sizet_t len, loff_t *offset)
{
    static int finished = 0;
    int i;
    char message[MSG_LEN + 30];

    if (finished) {
        finished = 0;
        return 0;
    }

    sprintf(message, "Last input:%s\n", Msg);
    for (i = 0; i < len && message[i]; i++)
        put_user(message[i], buf + i);

    finished = 1;
    return i;       /* Return no of bytes read */
}


/* module_input: */
static ssize_t module_input(struct file *file, char *buf,
                            size_t length, loff_t *offset)
{
    int i;

    for (i = 0; i < MSG_LEN - 1 && i < length; i++)
        get_user(Msg[i], buf + i);
    Msg[i] = '\0';

    return i;       /* Return no of input characters used */
}


int Already_Open = 0;               /* 1 if file is already opened by someone */
DECLARE_WAIT_QUEUE_HEAD(WaitQ);     /* Queue of processes who want our file */


/* module_open: */
static int module_open(struct inode *inode, struct file *file)
{
    if ((file->f_flags & O_NONBLOCK) && Already_Open)
        return -EAGAIN;     /* Resource Temporarily Unavailable, try again */

    try_module_get(THIS_MODULE);

    while (Already_Open) {
        int i, is_sig = 0;

        wait_event_interruptible(WaitQ, !Already_Open);

        for (i = 0; i < _NSIG_WORDS && !is_sig; i++)
            is_sig = (current->pending.signal.sig[i]) & 
                    (~current->blocked.sig[i]);

        if (is_sig) {
            module_put(THIS_MODULE);
            return -EINTR;
        }
    }

    Already_Open = 1;   /* flip value to 1 to show opened file */
    return 0;           /* Allow the access */
}


/* module_close */
int module_close(struct inode *inode, struct file *file)
{
    Already_Open = 0;
    wake_up(&WaitQ);

    module_put(THIS_MODULE);

    return 0;       /* success */
}


/* module_permission: checks file permissions */
static int module_permission(struct inode *inode, int op, struct nameidata *nd)
{
    /*
     * Modes of operation (op) 0 - Execute, 2 - Write, 4 - Read
     * Allow everyone to read from module, but only root to write to it
     */
    if (op == 4 || op == 2 && current->euid == 0)
        return 0;

    return -EACCES;     /* deny access if not above */
}


/* File Operations For Our Proc File, with Ptrs to Functions */
static struct file_operations File_Op {
    read: module_output,
    write: module_input,
    open: module_open,
    release: module_close,
};

/* Inode Operations For Our Proc File */
static struct inode_operations Inode_Op {
    permission: module_permission
};


/* Initialize module - register the proc file */
int init_module()
{
    Our_Proc_File = create_proc_entry(PROC_ENTRY_FILENAME, 0644, NULL);

    if (Our_Proc_File == NULL) {
        remove_proc_entry(PROC_ENTRY_FILENAME, &proc_root);
        printk(KERN_ALERT "Error: Could not Initialize /proc/test\n");
        return -ENOMEM;     /* Memory allocation problem */
    }

    Our_Proc_File->owner = THIS_MODULE;
    Our_Proc_File->proc_iops = &Inode_Op;
    Our_Proc_File->proc_fops = &File_Op;
    Our_Proc_File->mode = S_IFREG | S_IRUGO | S_IWUSR;
    Our_Proc_File->uid  = 0;
    Our_Proc_File->gid  = 0;
    Our_Proc_File->size = 80;

    printk(KERN_INFO "/proc/test created successfully\n");
    return 0;
}


/* Module Cleanup - unregister file from /proc */
void cleanup_module()
{
    remove_proc_entry(PROC_ENTRY_FILENAME, &proc_root);

    printk(KERN_INFO "/proc/test removed\n");
}
