/* 
 * kbleds.c - Blink keyboard leds until module is unloaded
 *
 * created by Anvesh G. Jhuboo
 * on Feb/23/2021
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>

MODULE_DESCRIPTION("Blinking Keyboard LEDS - Cool stuff!");
MODULE_AUTHOR("Anvesh G. Jhuboo");
MODULE_LICENSE("GPL");

struct time_list timer;
struct tty_driver *driver;
char kbledstatus = 0;

#define BLINK_DELAY     HZ/5
#define ALL_LEDS_ON     0x07
#define RESTORE_LEDS    0xFF


static void timer_func(unsigned long ptr)
{
    int *p = (int *)ptr;

    if (*p == ALL_LEDS_ON)
        *p = RESTORE_LEDS;
    else
        *p = ALL_LEDS_ON;

    (driver->ioctl) (vs_cons[fg_console].d->vc_tty, NULL, KDSETLED, *p);

    timer.expires = jiffies + BLINK_DELAY;
    add_timer(&my_timer);
}


static int __init kbleds_init(void)
{
    int i;

    printk(KERN_INFO "kbleds: loading\n");
    printk(KERN_INFO "kbleds: fgconsole is %x\n", fg_console);

    for (i = 0; i < MAX_NR_CONSOLES; i++) {
        if (!vc_cons[i].d)
            break;
        printk(KERN_INFO "poet_atkm: console[%i/%i] #%i, tty %lx\n", 
                i, MAX_NR_CONSOLES, vc_cons[i].d->vc_num,
                (unsigned long)vc_cons[i].d->vc_tty)
    }

    printk(KERN_INFO "kbleds: finished scanning consoles\n");

    driver = vc_cons[fg_console].d->vc_tty->driver;
    printk(KERN_INFO "kbleds: tty driver magic %x\n", driver->magic);

    /* Set up first time usage for LEDS */
    init_timer(&timer);
    timer.function = timer_func;
    timer.data = (unsigned long) &kbledstatus;
    timer.expires = jiffies + BLINK_DELAY;
    add_timer(&timer);

    return 0;
}


static void __exit kbleds_cleanup(void)
{
    printk(KERN_INFO "kbleds: unloading...\n");
    del_timer(&timer);
    (driver->ioctl) (vc_cons[fg_console].d->vc_tty, NULL, KDSETLED, 
                    RESTORE_LEDS) 
}


module_init(kbleds_init);
module_exit(kbleds_cleanup);
