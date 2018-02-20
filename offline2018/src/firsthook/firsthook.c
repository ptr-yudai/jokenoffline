#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/semaphore.h>
#include <asm/cacheflush.h>

void **sys_call_table;

// 古いgetdentsのアドレス
asmlinkage long (*original_getdents) (unsigned int, struct linux_dirent __user*, unsigned int);
// 新しいgetdentsの宣言
asmlinkage long hacked_getdents(
				unsigned int fd,
				struct linux_dirent __user *dirent,
				unsigned int count
				)
{
  printk(KERN_INFO "firsthook: Hacked getdents is called!\n");
  return original_getdents(fd, dirent, count);
}

/*
 * sys_call_tableのアドレスを探す
 */
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
      printk(KERN_INFO "firsthook: Found sys_call_table at 0x%lx", (unsigned long)sct);
      return sct;
    }
  }
  
  printk(KERN_INFO "firsthook: Found nothing");
  return NULL;
}

/*
 * システムコールエントリを変更
 */
void modify_syscall(void)
{
  // sys_call_tableのアドレスを取得
  sys_call_table = (void**)find_sys_call_table();
  // 元のgetdentsを保持
  original_getdents = sys_call_table[__NR_getdents];
  // 新しいgetdentsに変更
  sys_call_table[__NR_getdents] = hacked_getdents;
}

/*
 * モジュール初期化
 */
int init_module(void)
{
  printk(KERN_INFO "firsthook: init\n");

  modify_syscall();
  
  return 0;
}

/*
 * モジュール解放
 */
void cleanup_module(void)
{
  printk(KERN_INFO "firsthook: exit\n");

  // 変更を元に戻す
  sys_call_table[__NR_getdents] = original_getdents;
}
