/*
 * Reference: http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/
 */

#include <linux/init.h>             // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>           // Core header for loading LKMs into the kernel
#include <linux/kernel.h>           // Contains types, macros, functions for the kernel
#include <linux/fs.h> // file_operations
#include <linux/device.h> // class_create and others

#include "uipc.h"

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Zihan Yang");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux driver to test monitor/mwait in kernel space.");  ///< The description -- see modinfo
MODULE_VERSION("0.1");

static char *name = "user_level_ipc";        ///< An example LKM argument -- default value is "world"
module_param(name, charp, S_IRUGO); ///< Param desc. charp = char ptr, S_IRUGO can be read/not changed
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");  ///< parameter description

#define  DEVICE_NAME "uipc-mwait"    ///< The device will appear at /dev/uipc-mwait using this value
#define  CLASS_NAME  "uipc"        ///< The device class -- this is a character device driver

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static struct class*  uipccharClass  = NULL; ///< The device-driver class struct pointer
static struct device* uipccharDevice = NULL; ///< The device-driver device struct pointer

#define MONITOR_COUNTER 13000

static inline void monitor(uint64_t rax, uint64_t rcx, uint64_t rdx)
{
    /* "monitor %rax,%rcx,%rdx;" */
    asm volatile(
            "monitor\n"
            : 
            :"a" (rax), "c" (rcx), "d"(rdx));
}

static inline void mwait(uint64_t rax, uint64_t rcx)
{
    /* "mwait %rax,%rcx;" */
    asm volatile(
            "mwait\n"
            : 
            :"a" (rax), "c" (rcx));
}

/* export such symbol, so that we can modify it in another core */
char trigger[64];
EXPORT_SYMBOL(trigger);

static int monitor_cnt;

static int enter_monitor_mwait(void)
{
    while (1) {
        if (trigger[0] != 'D') {
            monitor((uint64_t)&trigger[0], 0, 0);
        }
        rmb();
        if (trigger[0] != 'D') {
            mwait(0, 0);
        } else {
            // we are triggered by the real value modification
            break;
        }
        if ((++monitor_cnt) > MONITOR_COUNTER)
            break;
    }
    printk(KERN_INFO "[MWAIT]: I am triggered, triggered value = %d, monitor_cnt = %d\n", trigger[0], monitor_cnt);
    monitor_cnt = 0;
    return 0;
}

static int uipc_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "******* UIPC driver is opened! ********\n");
    return 0;
}

static int uipc_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "******* UIPC driver is closed! ********\n");
    return 0;
}

static long uipc_ioctl(struct file *filp,
                      unsigned int ioctl, unsigned long arg)
{
    long r = -EINVAL;

    switch (ioctl) {
    case UIPC_ENTER_MONITOR_MWAIT:
        r = enter_monitor_mwait();
        break;
    case UIPC_TRIGGER_MONITOR:
        printk(KERN_INFO "[KERNEL] Im wriing to %p\n", trigger);
        trigger[0] = 'D';
        r = 0;
        break;
    default:
        printk(KERN_ERR "[UIPC]: Unkown ioctl number %u ...\n", ioctl);
        break;
    }

    return r;
}

static struct file_operations fops = {
   .owner  = THIS_MODULE,
   .open = uipc_open,
   .release = uipc_release,
   /* when 64 app calls 64 bit kernel or 32 bit app calls 32 bit kernel, .unlocked_ioctl is called
    * Only when 32bit app calls 64bit kernel, .compat_ioctl is called */
   .unlocked_ioctl = uipc_ioctl,
};

static int __init uipc_init(void){
   printk(KERN_INFO "UIPC: Hello %s from the UIPC LKM!\n", name);
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "[UIPC] failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "UIPC: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   uipccharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(uipccharClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(uipccharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "[UIPC]: device class registered correctly\n");

   // Register the device driver
   uipccharDevice = device_create(uipccharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(uipccharDevice)){               // Clean up if there is an error
      class_destroy(uipccharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(uipccharDevice);
   }
   return 0;
}
 
static void __exit uipc_exit(void){
   device_destroy(uipccharClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(uipccharClass);                          // unregister the device class
   class_destroy(uipccharClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "UIPC: Goodbye %s from the UIPC LKM!\n", name);
}
 
module_init(uipc_init);
module_exit(uipc_exit);

