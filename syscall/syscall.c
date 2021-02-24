/* 
 * syscall.c: steals system calls 
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

/* Global Vars */
extern void *sys_call_table[];
static int uid;
module_param(uid, int, 0644);

/* Declarations */
asmlinkage int (*original_call) (const char *, int, int);


/* our_sys_open: spy on user, returning original sys_open  */
asmlinkage int our_sys_open(const char *filename, int flags, int mode)
{
    int i = 0;
    char c;

    if (uid == current->uid) {
        printk("Opened file by UID: %d", uid);  /* Report file if relevant */

        do {
            get_user(c, filename + 1);
            i++;
            printk("%c", c);
        } while (c != 0);
        printk("\n");
    }

    /* Call original sys_open, otherwise we lose ability to open file s*/
    return original_call(filename, flags, mode);
}


/* init_module: initalizes the module */
int init_module()
{
    printk(KERN_ALERT "Good luck, this module is dangerous *-*");

    original_call = sys_call_table[__NR_open];  /* keep ptr to original func */
    sys_call_table[__NR_open] = our_sys_open;   /* replace syscall in table */

    printk(KERN_INFO "Spying on UID: %d\n", uid);
    return 0;
}


/* cleanup_module: unregisters file from /proc */
void cleanup_module()
{
    if (sys_call_table[__NR_open] != our_sys_open) {
        printk(KERN_ALERT "Open System Call was interfered by another one\n");
        printk(KERN_ALERT "System may be left in Unstable state\n");
    }

    sys_call_table[__NR_open] = original_call;
}
