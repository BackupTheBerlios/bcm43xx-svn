#ifndef BCM430x_H
#define BCM430x_H

#define DRV_NAME			"bcm430x"
#define DRV_VERSION			"0.0.1"
#define BCM430x_DRIVER_NAME		DRV_NAME " driver " DRV_VERSION
#define PFX				DRV_NAME ": "

#define BCM430x_IO_SIZE			8192
#define BCM430x_REG_ACTIVE_CORE		0x80
#define BCM430x_PHY_CONTROL		0x3FC
#define BCM430x_PHY_DATA		0x3FE
#define BCM430x_SHM_CONTROL		0x160
#define BCM430x_SHM_DATA		0x164
#define BCM430x_SPROM_IL0MACADDR	0x24
#define BCM430x_SWITCH_CORE_MAX_RETRIES	10

#ifdef BCM430x_NDEBUG
#define assert(expr) do {} while (0)
#else
#define assert(expr) \
	do {								\
		if (unlikely(!(expr))) {				\
		printk(KERN_ERR "Assertion failed! %s,%s,%s,line=%d\n",	\
		#expr,__FILE__,__FUNCTION__,__LINE__);			\
		}							\
	} while (0)
#endif

struct bcm430x_private {
	void *mmio_addr;
	struct pci_dev *pci_dev;
	unsigned int regs_len;

	u16 chip_id;
	u8 chip_rev;

	u16 core_id;
	u8 core_rev;

	u32 capabilities;
	u32 corecontrol;
	u32 irq_status;
	u32 irq_mask;
	u32 irq_control;
	u32 pll_on_delay;
	u32 fref_sel_delay;
	u32 slow_clk_delay;

	u32 sbimstate;
	u32 sbtmstatelow;
	u32 sbtmstatehigh;
	u32 sbidhigh;

	u16 radio_manufacturer;
	u16 radio_version;
	u8 radio_rev;
	u16 radio_ctrl;
	u16 radio_hi;
	u16 radio_lo;
	u8 phy_version;
	u8 phy_type;
	u8 phy_rev;
	u16 phy_ctrl;
	u16 phy_data;
	u32 status;
	u32 shm_ctrl;
	u32 shm_data;
	u16 led_ctrl;

	u16 r40;
	u16 r42;
	u16 r44_2;
	u32 r44;
	u32 r88;

	u32 m054;
	u32 m06c;
	u32 m128;
	u16 m3e2;
	u16 m3e4;
	u16 m3e6;
	u16 m49e;
	u16 m11fe;
};

#endif /* BCM430x_H */
