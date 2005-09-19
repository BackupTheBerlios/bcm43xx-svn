#ifndef BCM430x_H
#define BCM430x_H

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/stringify.h>
#include <net/ieee80211.h>

#define DRV_NAME			__stringify(KBUILD_MODNAME)
#define DRV_VERSION			__stringify(BCM430x_VERSION)
#define BCM430x_DRIVER_NAME		DRV_NAME " driver " DRV_VERSION
#define PFX				DRV_NAME ": "

#define BCM430x_SWITCH_CORE_MAX_RETRIES	10
#define BCM430x_IRQWAIT_MAX_RETRIES	50 /* FIXME: need more? 50 == 500usec */

#define BCM430x_IO_SIZE			8192
#define BCM430x_REG_ACTIVE_CORE		0x80

/* MMIO offsets */
#define BCM430x_MMIO_DMA1_REASON	0x20
#define BCM430x_MMIO_DMA1_IRQ_MASK	0x24
#define BCM430x_MMIO_DMA2_REASON	0x28
#define BCM430x_MMIO_DMA2_IRQ_MASK	0x2C
#define BCM430x_MMIO_DMA3_REASON	0x30
#define BCM430x_MMIO_DMA3_IRQ_MASK	0x34
#define BCM430x_MMIO_DMA4_REASON	0x38
#define BCM430x_MMIO_DMA4_IRQ_MASK	0x3C
#define BCM430x_MMIO_STATUS_BITFIELD	0x120
#define BCM430x_MMIO_GEN_IRQ_REASON	0x128
#define BCM430x_MMIO_GEN_IRQ_MASK	0x12C
#define BCM430x_MMIO_RAM_CONTROL	0x130
#define BCM430x_MMIO_RAM_DATA		0x134
#define BCM430x_MMIO_SHM_CONTROL	0x160
#define BCM430x_MMIO_SHM_DATA		0x164
#define BCM430x_MMIO_DMA1_BASE		0x200
#define BCM430x_MMIO_DMA2_BASE		0x220
#define BCM430x_MMIO_DMA3_BASE		0x240
#define BCM430x_MMIO_DMA4_BASE		0x260
#define BCM430x_MMIO_PHY_VER		0x3E0
#define BCM430x_MMIO_PHY_RADIO		0x3E2
#define BCM430x_MMIO_ANTENNA		0x3E8
/* #define BCM430x_MMIO_???		0x3Ec*/
#define BCM430x_MMIO_CHANNEL		0x3F0
#define BCM430x_MMIO_RADIO_CONTROL	0x3F6
#define BCM430x_MMIO_RADIO_DATA		0x3F8
#define BCM430x_MMIO_RADIO_DATA_LOW	0x3FA
#define BCM430x_MMIO_PHY_CONTROL	0x3FC
#define BCM430x_MMIO_PHY_DATA		0x3FE
/* #define BCM430x_MMIO_???		0x401*/
#define BCM430x_MMIO_MACFILTER_CONTROL	0x420
#define BCM430x_MMIO_MACFILTER_DATA	0x422
#define BCM430x_MMIO_RADIO_HWENABLED	0x49A
#define BCM430x_MMIO_LED_CONTROL	0x49C
#define BCM430x_MMIO_POWERUP_DELAY	0x6A8

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

/* BCM430x_SPROM_BOARDFLAGS values */
#define BCM430x_BFL_BTCOEXIST		0x0001 /* implements Bluetooth coexistance */
#define BCM430x_BFL_PACTRL		0x0002 /* GPIO 9 controlling the PA */
#define BCM430x_BFL_AIRLINEMODE		0x0004 /* implements GPIO 13 radio disable indication */
#define BCM430x_BFL_RSSI		0x0008 /* FIXME: what's this? */
#define BCM430x_BFL_ENETSPI		0x0010 /* has ephy roboswitch spi */
#define BCM430x_BFL_XTAL		0x0020 /* FIXME: what's this? */
#define BCM430x_BFL_CCKHIPWR		0x0040 /* can do high power CCK transmission */
#define BCM430x_BFL_ENETADM		0x0080 /* has ADMtek switch *//
#define BCM430x_BFL_ENETVLAN		0x0100 /* can do vlan */
#define BCM430x_BFL_AFTERBURNER		0x0200 /* supports Afterburner mode */
#define BCM430x_BFL_NOPCI		0x0400 /* leaves PCI floating */
#define BCM430x_BFL_FEM			0x0800 /* supports the Front End Module */

/* GPIO register offset, in both ChipCommon and PCI core. */
#define BCM430x_GPIO_CONTROL		0x6c

/* SHM Routing */
#define BCM430x_SHM_SHARED		0x00010000
/* #define BCM430x_SHM_????		0x00020000*/
#define BCM430x_SHM_PCM			0x00030000
#define BCM430x_SHM_HWMAC		0x00040000
#define BCM430x_SHM_UCODE		0x03000000
/* #define BCM_430x_SHM_????		0x03010000*/

#define BCM430x_CHIPCOMMON_CAPABILITIES 0x04
#define BCM430x_CAPABILITIES_PCTLMASK	0x0040000

/* PowerControl */
#define BCM430x_PCTL_IN			0xB0
#define BCM430x_PCTL_OUT		0xB4
#define BCM430x_PCTL_OUTENABLE		0xB8
#define BCM430x_PCTL_XTAL_POWERUP	0x40
#define BCM430x_PCTL_PLL_POWERDOWN	0x80

/* PowerControl Clock Modes */
#define BCM430x_PCTL_CLK_FAST		0x00
#define BCM430x_PCTL_CLK_SLOW		0x01
#define BCM430x_PCTL_CLK_DYNAMIC	0x02

#define BCM430x_PCTL_FORCE_SLOW		0x0800
#define BCM430x_PCTL_FORCE_PLL		0x1000
#define BCM430x_PCTL_DYN_XTAL		0x2000

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

/* sbtmstatehigh state flags */
#define BCM430x_SBTMSTATEHIGH_SERROR		0x1
#define BCM430x_SBTMSTATEHIGH_BUSY		0x4

/* sbimstate flags */
#define BCM430x_SBIMSTATE_IB_ERROR		0x20000
#define BCM430x_SBIMSTATE_TIMEOUT		0x40000

/* PHYVersioning */
#define BCM430x_PHYTYPE_A		0x00
#define BCM430x_PHYTYPE_B		0x01
#define BCM430x_PHYTYPE_G		0x02

/* RadioRegisters */
#define BCM430x_RADIO_ID		0x01
#define BCM430x_RADIO_ID_NORF		0x4E4F5246
#define BCM430x_RADIO_ID_REVISIONMASK	0xF0000000
#define BCM430x_RADIO_ID_VERSIONMASK	0x0FFFF000
#define BCM430x_RADIO_ID_MANUFACTMASK	0x00000FFF

/* StatusBitField */
#define BCM430x_SBF_MAC_ENABLED		0x00000001
#define BCM430x_SBF_CORE_READY		0x00000004
#define BCM430x_SBF_TEMPLATE_BIGENDIAN	0x00010000
#define BCM430x_SBF_MODE_ADHOC		0x00020000
#define BCM430x_SBF_MODE_AP		0x00040000
#define BCM430x_SBF_RADIOREG_LOCK	0x00080000
#define BCM430x_SBF_MODE_PROMISC	0x00100000
#define BCM430x_SBF_MODE_MONITOR	0x00400000

/* MicrocodeFlagsBitfield (addr + lo-word values?)*/
#define BCM430x_UCODEFLAGS_OFFSET	0x005E

#define BCM430x_UCODEFLAG_AUTODIV	0x0001
#define BCM430x_UCODEFLAG_UNKBGPHY	0x0002
#define BCM430x_UCODEFLAG_UNKBPHY	0x0004
#define BCM430x_UCODEFLAG_UNKGPHY	0x0020
#define BCM430x_UCODEFLAG_UNKPACTL	0x0040
#define BCM430x_UCODEFLAG_JAPAN		0x0080

/* Generic-Interrupt reasons. */
/*TODO: add the missing ones. */
#define BCM430x_IRQ_BEACON		(1 << 1)
#define BCM430x_IRQ_TBTT		(1 << 2) /*FIXME: purpose? */
#define BCM430x_IRQ_REG124		(1 << 5) /*FIXME: purpose? */
#define BCM430x_IRQ_PMQ			(1 << 6) /*FIXME: purpose? */
#define BCM430x_IRQ_TXFIFO_ERROR	(1 << 15)
#define BCM430x_IRQ_SCAN		(1 << 16) /*FIXME: purpose? */
#define BCM430x_IRQ_BGNOISE		(1 << 18)
#define BCM430x_IRQ_XMIT_STATUS		(1 << 29)

#define BCM430x_IRQ_ALL			0xffffffff
#define BCM430x_IRQ_INITIAL		0x20058864

/* DMA-Interrupt reasons. */
/*TODO: add the missing ones. */
#define BCM430x_DMAIRQ_ERR0		(1 << 10)
#define BCM430x_DMAIRQ_ERR1		(1 << 11)
#define BCM430x_DMAIRQ_ERR2		(1 << 12)
#define BCM430x_DMAIRQ_ERR3		(1 << 13)
#define BCM430x_DMAIRQ_ERR4		(1 << 14)
#define BCM430x_DMAIRQ_ERR5		(1 << 15)
#define BCM430x_DMAIRQ_RX_DONE		(1 << 16)
/* helpers */
#define BCM430x_DMAIRQ_ANYERR		(BCM430x_DMAIRQ_ERR0 | \
					 BCM430x_DMAIRQ_ERR1 | \
					 BCM430x_DMAIRQ_ERR2 | \
					 BCM430x_DMAIRQ_ERR3 | \
					 BCM430x_DMAIRQ_ERR4 | \
					 BCM430x_DMAIRQ_ERR5)
#define BCM430x_DMAIRQ_FATALERR		(BCM430x_DMAIRQ_ERR0 | \
					 BCM430x_DMAIRQ_ERR1 | \
					 BCM430x_DMAIRQ_ERR2 | \
					 BCM430x_DMAIRQ_ERR4 | \
					 BCM430x_DMAIRQ_ERR5)
#define BCM430x_DMAIRQ_NONFATALERR	BCM430x_DMAIRQ_ERR3

/* DMA controller register offsets. (relative to BCM430x_DMA#_BASE) */
#define BCM430x_DMA_TX_CONTROL		0x00
#define BCM430x_DMA_TX_DESC_RING	0x04
#define BCM430x_DMA_TX_DESC_INDEX	0x08
#define BCM430x_DMA_TX_STATUS		0x0c
#define BCM430x_DMA_RX_CONTROL		0x10
#define BCM430x_DMA_RX_DESC_RING	0x14
#define BCM430x_DMA_RX_DESC_INDEX	0x18
#define BCM430x_DMA_RX_STATUS		0x1c

/* DMA controller channel control word values. */
#define BCM430x_DMA_TXCTRL_ENABLE		(1 << 0)
#define BCM430x_DMA_TXCTRL_SUSPEND		(1 << 1)
#define BCM430x_DMA_TXCTRL_LOOPBACK		(1 << 2)
#define BCM430x_DMA_TXCTRL_FLUSH		(1 << 4)
#define BCM430x_DMA_RXCTRL_ENABLE		(1 << 0)
#define BCM430x_DMA_RXCTRL_FRAMEOFF_MASK	0x000000fe
#define BCM430x_DMA_RXCTRL_PIO			(1 << 8)
/* DMA controller channel status word values. */
#define BCM430x_DMA_TXSTAT_DPTR_MASK		0x00000fff
#define BCM430x_DMA_TXSTAT_STAT_MASK		0x0000f000
#define BCM430x_DMA_TXSTAT_ERROR_MASK		0x000f0000
#define BCM430x_DMA_TXSTAT_FLUSHED		(1 << 20)
#define BCM430x_DMA_RXSTAT_DPTR_MASK		0x00000fff
#define BCM430x_DMA_RXSTAT_STAT_MASK		0x0000f000
#define BCM430x_DMA_RXSTAT_ERROR_MASK		0x000f0000

/* DMA descriptor control field values. */
#define BCM430x_DMADTOR_BYTECNT_MASK		0x00001fff
#define BCM430x_DMADTOR_DTABLEEND		(1 << 28) /* end of descriptor table */
#define BCM430x_DMADTOR_COMPIRQ			(1 << 29) /* irq on comppletion request */
#define BCM430x_DMADTOR_FRAMEEND		(1 << 30)
#define BCM430x_DMADTOR_FRAMESTART		(1 << 31)

#define BCM430x_TXRING_SLOTS			256
#define BCM430x_RXRING_SLOTS			256


#ifdef assert
# undef assert
#endif
#ifdef BCM430x_NDEBUG
#define assert(expr) do {} while (0)
#else
#define assert(expr) \
	do {									\
		if (unlikely(!(expr))) {					\
		printkl(KERN_ERR PFX "Assertion failed! %s,%s,%s,line=%d\n",	\
		#expr,__FILE__,__FUNCTION__,__LINE__);				\
		}								\
	} while (0)
#endif

/* rate limited printk(). */
#ifdef printkl
# undef printkl
#endif
#define printkl(f, x...)  do { if (printk_ratelimit()) printk(f ,##x); } while (0)


struct net_device;
struct pci_dev;
struct bcm430x_dmaring;

struct bcm430x_sprominfo {
	u16 boardflags;
};

#define BCM430x_COREFLAG_AVAILABLE	(1 << 0)
#define BCM430x_COREFLAG_ENABLED	(1 << 1)

struct bcm430x_coreinfo {
	/** Driver internal flags. See BCM430x_COREFLAG_* */
	u32 flags;
	/** core_id ID number */
	u16 id;
	/** core_rev revision number */
	u8 rev;
	/** Index number for _switch_core() */
	u8 index;
};

/* Driver STATUS values */
#define BCM430x_STAT_BOARDINITDONE		(1 << 0)
#define BCM430x_STAT_PHYCALIBRATED		(1 << 1)
#define BCM430x_STAT_PHYCONNECTED		(1 << 2)
#define BCM430x_STAT_DEVSHUTDOWN		(1 << 3)	// Are we shutting down?

struct bcm430x_private {
	struct ieee80211_device *ieee;

	struct net_device *net_dev;
	struct pci_dev *pci_dev;

	void *mmio_addr;
	unsigned int mmio_len;

	u32 shm_addr;

	spinlock_t lock;

	/* Driver status flags BCM430x_STAT_XXX */
	u32 status;

	u16 board_vendor;
	u16 board_type;

	u16 chip_id;
	u8 chip_rev;
	
	u32 radio_id;
	// 0: baseband attenuation,
	// 1: generic attenuation, 
	// 2: tx_CTL1 attenuation
	u16 radio_txpower[3];
	u16 radio_interfmode;
	u16 radio_interfsize;
	u16 radio_interfstack[20];

	u32 sbimstate;
	u32 sbtmstatelow;
	u32 sbtmstatehigh;

	u8 phy_version;
	u8 phy_type;
	u8 phy_rev;

	u16 curr_channel;

	u16 antenna_diversity;

#ifdef BCM430x_DEBUG
	u16 ucode_size;
	u16 pcm_size;
#endif
	struct bcm430x_sprominfo sprom;

	u8 core_count;
	/* The currently active core. NULL if not initialized, yet. */
	struct bcm430x_coreinfo *current_core;
	/* coreinfo structs for all possible cores follow.
	 * Note that a core might not exist.
	 * So check the coreinfo flags before using it.
	 */
	struct bcm430x_coreinfo core_chipcommon;
	struct bcm430x_coreinfo core_pci;
	struct bcm430x_coreinfo core_v90;
	struct bcm430x_coreinfo core_pcmcia;
	struct bcm430x_coreinfo core_80211;
	/*TODO: add the remaining coreinfo structs. */

	u32 chipcommon_capabilities;

	/* Reason code of the last interrupt. */
	u32 irq_reason;
	u32 dma_reason[4];
	/* saved irq enable/disable state bitfield. */
	u32 irq_savedstate;

	/* Interrupt Service Routine tasklet (bottom-half) */
	struct tasklet_struct isr_tasklet;

	/* DMA */
	struct bcm430x_dmaring *tx_ring;
	struct bcm430x_dmaring *rx_ring;

	struct ieee80211_txb *txb;
};

static inline
struct bcm430x_private * bcm430x_priv(struct net_device *dev)
{
	return ieee80211_priv(dev);
}

struct bcm430x_initval {
	u16 offset;
	u16 size;
	u32 value;
} __attribute__((__packed__));

/* 
 * Wrapper for older kernels 
 */

/* pm_message_t type */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
#include <linux/pm.h>
typedef u32 /*__bitwise*/ pm_message_t;
#endif

#endif /* BCM430x_H */
