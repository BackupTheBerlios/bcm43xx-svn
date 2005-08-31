#ifndef BCM430x_H
#define BCM430x_H

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <net/ieee80211.h>

#define DRV_NAME			"bcm430x"
#define DRV_VERSION			"0.0.1"
#define BCM430x_DRIVER_NAME		DRV_NAME " driver " DRV_VERSION
#define PFX				DRV_NAME ": "

#define BCM430x_SWITCH_CORE_MAX_RETRIES	10
#define BCM430x_IRQWAIT_MAX_RETRIES	50 /* FIXME: need more? 50 == 500usec */

#define BCM430x_IO_SIZE			8192
#define BCM430x_REG_ACTIVE_CORE		0x80

/* MMIO offsets */
#define BCM430x_MMIO_DMA1_REASON	0x20
/* #define BCM430x_MMIO_???		0x24*/
#define BCM430x_MMIO_DMA2_REASON	0x28
#define BCM430x_MMIO_DMA3_REASON	0x30
#define BCM430x_MMIO_DMA4_REASOM	0x38
#define BCM430x_MMIO_STATUS_BITFIELD	0x120
#define BCM430x_MMIO_GEN_IRQ_REASON	0x128
#define BCM430x_MMIO_GEN_IRQ_MASK	0x12c
#define BCM430x_MMIO_SHM_CONTROL	0x160
#define BCM430x_MMIO_SHM_DATA		0x164
#define BCM430x_MMIO_DMA1_BASE		0x200
#define BCM430x_MMIO_DMA2_BASE		0x220
#define BCM430x_MMIO_DMA3_BASE		0x240
#define BCM430x_MMIO_DMA4_BASE		0x260
#define BCM430x_MMIO_PHY_VER		0x3e0
#define BCM430x_MMIO_PHY_RADIO		0x3e2
#define BCM430x_MMIO_ANTENNA		0x3e8
/* #define BCM430x_MMIO_???		0x3ec*/
#define BCM430x_MMIO_CHANNEL		0x3f0
#define BCM430x_MMIO_RADIO_CONTROL	0x3f6
#define BCM430x_MMIO_RADIO_DATA		0x3f8
#define BCM430x_MMIO_RADIO_DATA_LOW	0x3fa
#define BCM430x_MMIO_PHY_CONTROL	0x3FC
#define BCM430x_MMIO_PHY_DATA		0x3FE
/* #define BCM430x_MMIO_???		0x401*/
#define BCM430x_MMIO_MACFILTER_CONTROL	0x420
#define BCM430x_MMIO_MACFILTER_DATA	0x422
#define BCM430x_MMIO_RADIO_HWENABLED	0x49a
#define BCM430x_MMIO_LED_CONTROL	0x49c

/* SPROM offsets. These are actually MMIO offsets. */
#define BCM430x_SPROM_BASE		0x1000
#define BCM430x_SPROM_BOARDFLAGS2	(BCM430x_SPROM_BASE + (2 * 0x1c))
#define BCM430x_SPROM_IL0MACADDR	(BCM430x_SPROM_BASE + (2 * 0x24))
#define BCM430x_SPROM_ET0MACADDR	(BCM430x_SPROM_BASE + (2 * 0x27))
#define BCM430x_SPROM_ET1MACADDR	(BCM430x_SPROM_BASE + (2 * 0x2a))
#define BCM430x_SPROM_ETHPHY		(BCM430x_SPROM_BASE + (2 * 0x2d))
#define BCM430x_SPROM_BOARDREV		(BCM430x_SPROM_BASE + (2 * 0x2e))
#define BCM430x_SPROM_PA0B0		(BCM430x_SPROM_BASE + (2 * 0x2f))
#define BCM430x_SPROM_PA0B1		(BCM430x_SPROM_BASE + (2 * 0x30))
#define BCM430x_SPROM_PA0B2		(BCM430x_SPROM_BASE + (2 * 0x31))
#define BCM430x_SPROM_WL0GPIO0		(BCM430x_SPROM_BASE + (2 * 0x32))
#define BCM430x_SPROM_WL0GPIO1		(BCM430x_SPROM_BASE + (2 * 0x33))
#define BCM430x_SPROM_MAXPWR		(BCM430x_SPROM_BASE + (2 * 0x34))
#define BCM430x_SPROM_PA1B0		(BCM430x_SPROM_BASE + (2 * 0x35))
#define BCM430x_SPROM_PA1B1		(BCM430x_SPROM_BASE + (2 * 0x36))
#define BCM430x_SPROM_PA1B2		(BCM430x_SPROM_BASE + (2 * 0x37))
#define BCM430x_SPROM_IDL_TSSI_TGT	(BCM430x_SPROM_BASE + (2 * 0x38))
#define BCM430x_SPROM_BOARDFLAGS	(BCM430x_SPROM_BASE + (2 * 0x39))
#define BCM430x_SPROM_ANTENNA_GAIN	(BCM430x_SPROM_BASE + (2 * 0x3a))
#define BCM430x_SPROM_VERSION		(BCM430x_SPROM_BASE + (2 * 0x3f))

/* SHM Routing */
#define BCM430x_SHM_SHARED		0x00010000
/* #define BCM430x_SHM_????		0x00020000*/
#define BCM430x_SHM_PCM			0x00030000
#define BCM430x_SHM_HWMAC		0x00040000
#define BCM430x_SHM_UCODE		0x03000000
/* #define BCM_430x_SHM_????		0x03010000*/

/* PowerControl */
#define BCM430x_PCTL_IN			0xB0
#define BCM430x_PCTL_OUT		0xB4
#define BCM430x_PCTL_OUTENABLE		0xB8
#define BCM430x_PCTL_XTAL_POWERUP	0x40
#define BCM430x_PCTL_PLL_POWERDOWN	0x80

/* COREIDs */
#define BCM430x_COREID_CHIPCOMMON	0x800
#define BCM430x_COREID_PCI		0x804
#define BCM430x_COREID_V90		0x807
#define BCM430x_COREID_PCMCIA		0x80d
#define BCM430x_COREID_80211		0x812

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

	u32 sbimstate;
	u32 sbtmstatelow;
	u32 sbtmstatehigh;

	u8 phy_version;
	u8 phy_type;
	u8 phy_rev;
};

static inline
struct bcm430x_private * bcm430x_priv(struct net_device *dev)
{
	return ieee80211_priv(dev);
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
