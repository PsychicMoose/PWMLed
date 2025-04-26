#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xe75734e2, "pwm_apply_state" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x8c8569cb, "kstrtoint" },
	{ 0x828ce6bb, "mutex_lock" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xc358aaf8, "snprintf" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0x695bf5e9, "hrtimer_cancel" },
	{ 0x38d612f3, "pwm_free" },
	{ 0x17b8e2f4, "gpio_to_desc" },
	{ 0x7f8b7106, "gpiod_set_raw_value" },
	{ 0xfe990052, "gpio_free" },
	{ 0xbcef3707, "device_remove_file" },
	{ 0xd910fabf, "device_destroy" },
	{ 0xcf9c6ce6, "class_unregister" },
	{ 0x25775ba2, "class_destroy" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x92997ed8, "_printk" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x6761c53d, "gpiod_get_raw_value" },
	{ 0x5cc2a511, "hrtimer_forward" },
	{ 0xde4bf88b, "__mutex_init" },
	{ 0x1ec5db1a, "__register_chrdev" },
	{ 0x7b7fcf8d, "__class_create" },
	{ 0xeed31c02, "device_create" },
	{ 0xdb428edf, "device_create_file" },
	{ 0x3493e403, "pwm_request" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xd8ecb590, "gpiod_direction_output_raw" },
	{ 0x3a42e43b, "gpiod_direction_input" },
	{ 0xa362bf8f, "hrtimer_init" },
	{ 0xec523f88, "hrtimer_start_range_ns" },
	{ 0x78a319e7, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "F4C36D17BEAC154A3E93D13");
