#ifndef BCM430x_H
#define BCM430x_H

#include <linux/spinlock.h>
#include <net/ieee80211.h>

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
#define BCM430x_SPROM_BASE		0x1000
#define BCM430x_SPROM_IL0MACADDR	BCM430x_SPROM_BASE + (2 * 0x24)
#define BCM430x_SWITCH_CORE_MAX_RETRIES	10

/* PowerControl */
#define BCM430x_PCTL_IN			0xB0
#define BCM430x_PCTL_OUT		0xB4
#define BCM430x_PCTL_OUTENABLE		0xB8
#define BCM430x_PCTL_XTAL_POWERUP	0x40
#define BCM430x_PCTL_PLL_POWERDOWN	0x80

/* COREIDs */
#define BCM430x_COREID_CHIPCOMMON	0x800
#define BCM430x_COREID_80211		0x812
#define BCM430x_COREID_PCI		0x804
#define BCM430x_COREID_PCMCIA		0x80d

/* Core Information Registers */
#define BCM430x_CIR_BASE		0xf00
#define BCM430x_CIR_SBIMSTATE		(BCM430x_CIR_BASE + 0x90)
#define BCM430x_CIR_SBTMSTATELOW	(BCM430x_CIR_BASE + 0x98)
#define BCM430x_CIR_SBTMSTATEHIGH	(BCM430x_CIR_BASE + 0x9c)
#define BCM430x_CIR_SB_ID_HI		(BCM430x_CIR_BASE + 0xfc)

/* sbtmstatelow state flags */
#define BCM430x_SBTMSTATELOW_RESET		0x01
#define BCM430x_SBTMSTATELOW_REJECT		0x02
#define BCM430x_SBTMSTATELOW_CLOCK		0x10000
#define BCM430x_SBTMSTATELOW_FORCE_GATE_CLOCK	0x20000

/* sbtmstatelow state flags */
#define BCM430x_SBTMSTATEHIGH_SERROR		0x1
#define BCM430x_SBTMSTATEHIGH_BUSY		0x4

/* sbimstate flags */
#define BCM430x_SBIMSTATE_IB_ERROR		0x20000
#define BCM430x_SBIMSTATE_TIMEOUT		0x40000

#ifdef assert
# undef assert
#endif
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

struct net_device;
struct pci_dev;

struct bcm430x_private {
	struct ieee80211_device *ieee;
	struct ieee80211_security sec;

	struct net_device *net_dev;
	struct pci_dev *pci_dev;

	void *mmio_addr;
	unsigned int regs_len;

	spinlock_t lock;

	u16 chip_id;
	u8 chip_rev;

	u16 core_id;
	u8 core_rev;
	u8 core_index;

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

static inline
struct bcm430x_private * bcm430x_priv(struct net_device *dev)
{
	return ieee80211_priv(dev);
}

static inline 
struct net_device_stats * bcm430x_get_stats(struct net_device *dev)
{
	return &(bcm430x_priv(dev)->ieee->stats);
}

/* 
 * Wrapper for older kernels 
 */

/* pm_message_t type */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
#include <linux/pm.h>
typedef u32 /*__bitwise*/ pm_message_t;
#endif

#endif /* BCM430x_H */
