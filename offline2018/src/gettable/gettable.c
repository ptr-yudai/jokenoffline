#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>

static unsigned long **find_sys_call_table(void)
{
  unsigned long offset;
  unsigned long **sct;
  
  // PAGE_OFFSETから探索
  for(offset = PAGE_OFFSET; offset < ULLONG_MAX; offset += sizeof(void *)) {
    // 仮にoffsetがsys_call_tableの先頭だとする
    sct = (unsigned long**)offset;
    // __NR_close番目のポインタがsys_closeであれば一致
    if(sct[__NR_close] == (unsigned long*) sys_close) {
      printk(KERN_INFO "gettable: Found sys_call_table at 0x%lx", (unsigned long)sct);
      return sct;
    }
  }
  
  printk(KERN_INFO "gettable: Found nothing");
  return NULL;
}

/*
 * モジュール初期化
 */
int init_module(void)
{
  unsigned long x;

  printk(KERN_INFO "gettable: init\n");

  // sys_call_tableを探索
  x = (unsigned long)find_sys_call_table();
  // 物理アドレスも表示
  x = __phys_addr_nodebug(x);
  printk(KERN_INFO "gettable: Physical address is 0x%lx", x);
  
  return 0;
}

/*
 * モジュール解放
 */
void cleanup_module(void)
{
  printk(KERN_INFO "gettable: exit\n");
}
