#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

/*
 * モジュール初期化
 */
int init_module(void)
{
  printk(KERN_INFO "modhello: init\n");
  return 0;
}

/*
 * モジュール解放
 */
void cleanup_module(void)
{
  printk(KERN_INFO "modhello: exit\n");
}
