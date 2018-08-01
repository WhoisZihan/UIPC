#include <linux/init.h>             // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>           // Core header for loading LKMs into the kernel
#include <linux/kernel.h>           // Contains types, macros, functions for the kernel

MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Zihan Yang");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux driver to test monitor/mwait in kernel space.");  ///< The description -- see modinfo
MODULE_VERSION("0.1");

static char *name = "user_level_ipc";        ///< An example LKM argument -- default value is "world"
module_param(name, charp, S_IRUGO); ///< Param desc. charp = char ptr, S_IRUGO can be read/not changed
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");  ///< parameter description

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

char c;

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init uipc_init(void){
   printk(KERN_INFO "UIPC: Hello %s from the UIPC LKM!\n", name);
   monitor((uint64_t)&c, 0, 0);
   mwait(0, 0);
   return 0;
}
 
/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit uipc_exit(void){
   printk(KERN_INFO "UIPC: Goodbye %s from the UIPC LKM!\n", name);
}
 
/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(uipc_init);
module_exit(uipc_exit);

