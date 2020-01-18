#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
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
__used
__attribute__((section("__versions"))) = {
	{ 0xd1b09e08, "module_layout" },
	{ 0x33137551, "class_destroy" },
	{ 0x93801213, "device_destroy" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x9ee9d2da, "cdev_del" },
	{ 0x7c32d0f0, "printk" },
	{ 0x28ee49e5, "device_create" },
	{ 0x492e9752, "__class_create" },
	{ 0x9c1cb9ae, "cdev_add" },
	{ 0xd3725fc4, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xe1537255, "__list_del_entry_valid" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0x37a0cba, "kfree" },
	{ 0x68f31cbd, "__list_add_valid" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xc3c01ffa, "kmem_cache_alloc_trace" },
	{ 0xe1ea0878, "kmalloc_caches" },
	{ 0xbdfb6dbb, "__fentry__" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

