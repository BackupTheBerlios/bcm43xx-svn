#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

#undef unix
struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = __stringify(KBUILD_MODNAME),
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x2b8ea879, "struct_module" },
	{ 0xba86b60b, "pci_unregister_driver" },
	{ 0x3ecd7db7, "pci_register_driver" },
	{ 0x12b2de09, "unregister_netdev" },
	{ 0x61c9dba1, "pci_request_regions" },
	{ 0x110a7f90, "pci_disable_device" },
	{ 0x8475b352, "pci_enable_device" },
	{ 0xaf460982, "alloc_etherdev" },
	{ 0xedc03953, "iounmap" },
	{ 0xf5b10ea1, "free_netdev" },
	{ 0xc94f308, "pci_release_regions" },
	{ 0x4222ca2d, "pci_bus_write_config_dword" },
	{ 0xddf54e3f, "pci_bus_write_config_word" },
	{ 0xd7df38ba, "pci_bus_write_config_byte" },
	{ 0xde2b173e, "pci_bus_read_config_dword" },
	{ 0x9248d3e2, "pci_bus_read_config_word" },
	{ 0x20668ce6, "pci_bus_read_config_byte" },
	{ 0x908aa9b2, "iowrite32" },
	{ 0x2241a928, "ioread32" },
	{ 0x4775eacf, "iowrite16" },
	{ 0x1b7d4074, "printk" },
	{ 0x3960713, "ioread16" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";

