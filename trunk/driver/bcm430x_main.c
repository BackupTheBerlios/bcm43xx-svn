/*

  Broadcom BCM430x wireless driver

  Copyright (c) 2005 Martin Langer <martin-langer@gmx.de>,
                     Stefano Brivio <st3@riseup.net>
                     Michael Buesch <mbuesch@freenet.de>
                     Danny van Dyk <kugelfang@gentoo.org>
                     Andreas Jaggi <andreas.jaggi@waterwave.ch>

  Some parts of the code in this file are derived from the ipw2200
  driver  Copyright(c) 2003 - 2004 Intel Corporation.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
  Boston, MA 02110-1301, USA.

*/

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/if_arp.h>
#include <linux/etherdevice.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/wireless.h>
#include <linux/workqueue.h>
#include <linux/skbuff.h>
#include <net/iw_handler.h>

#include "bcm430x.h"
#include "bcm430x_main.h"
#include "bcm430x_debugfs.h"
#include "bcm430x_radio.h"
#include "bcm430x_phy.h"
#include "bcm430x_dma.h"
#include "bcm430x_pio.h"
#include "bcm430x_power.h"
#include "bcm430x_wx.h"


MODULE_DESCRIPTION("Broadcom BCM430x wireless driver");
MODULE_AUTHOR("Martin Langer");
MODULE_AUTHOR("Stefano Brivio");
MODULE_AUTHOR("Michael Buesch");
MODULE_LICENSE("GPL");

/* module parameters */
static int pio = 0;
static int short_preamble = 0;

/* If you want to debug with just a single device, enable this,
 * where the string is the pci device ID (as given by the kernel's
 * pci_name function) of the device to be used.
 */
//#define DEBUG_SINGLE_DEVICE_ONLY	"0001:11:00.0"

/* If you want to enable printing of each MMIO access, enable this. */
//#define DEBUG_ENABLE_MMIO_PRINT

/* If you want to enable printing of MMIO access within
 * ucode/pcm upload, initvals write, enable this.
 */
//#define DEBUG_ENABLE_UCODE_MMIO_PRINT

/* If you want to enable printing of PCI Config Space access, enable this */
//#define DEBUG_ENABLE_PCILOG


static struct pci_device_id bcm430x_pci_tbl[] = {

	/* Broadcom 4303 802.11b */
	{ PCI_VENDOR_ID_BROADCOM, 0x4301, 0x1028, 0x0407, 0, 0, 0 }, /* Dell TrueMobile 1180 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4301, 0x103c, 0x12f3, 0, 0, 0 }, /* HP Compaq NX9110 802.11b Mini-PCI card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4301, 0x1043, 0x0120, 0, 0, 0 }, /* Asus WL-103b PC Card */

	/* Broadcom 4307 802.11b */
//	{ PCI_VENDOR_ID_BROADCOM, 0x4307, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },

	/* Broadcom 4318 802.11b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x103c, 0x1355, 0, 0, 0 }, /* Compag v2000z Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x106b, 0x4318, 0, 0, 0 }, /* Apple AirPort Extreme 2 Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x1468, 0x0311, 0, 0, 0 }, /* AMBITG T60H906.00 */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x1799, 0x7000, 0, 0, 0 }, /* Belkin F5D7000 PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x17f9, 0x0006, 0, 0, 0 }, /* Amilo A1650 Laptop */

	/* Broadcom 4306 802.11b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x02fa, 0x3010, 0, 0, 0 }, /* Siemens Gigaset PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0001, 0, 0, 0 }, /* Dell TrueMobile 1300 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0002, 0, 0, 0 }, /* Dell TrueMobile 1300 PCMCIA Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0003, 0, 0, 0 }, /* Dell TrueMobile 1350 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x103c, 0x12fa, 0, 0, 0 }, /* Compaq Presario R3xxx PCI on board */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1043, 0x100f, 0, 0, 0 }, /* Asus WL-100G PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1057, 0x7025, 0, 0, 0 }, /* Motorola WN825G PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x106b, 0x004e, 0, 0, 0 }, /* Apple AirPort Extreme Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x14e4, 0x0013, 0, 0, 0 }, /* Linksys WMP54G PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1737, 0x0015, 0, 0, 0 }, /* Linksys WMP54GS PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1799, 0x7001, 0, 0, 0 }, /* Belkin F5D7001 PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1799, 0x7010, 0, 0, 0 }, /* Belkin F5D7010 PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x185f, 0x1220, 0, 0, 0 }, /* Linksys WMP54G PCI Card */

	/* Broadcom 4306 802.11a */
//	{ PCI_VENDOR_ID_BROADCOM, 0x4321, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },

	/* Broadcom 4309 802.11a/b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4324, 0x1028, 0x0001, 0, 0, 0 }, /* Dell TrueMobile 1400 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4324, 0x1028, 0x0003, 0, 0, 0 }, /* Dell TrueMobile 1450 Mini-PCI Card */

	/* Broadcom 43XG 802.11b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4325, 0x1414, 0x0003, 0, 0, 0 }, /* Microsoft MN-720 PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4325, 0x1414, 0x0004, 0, 0, 0 }, /* Microsoft MN-730 PCI Card */

	/* required last entry */
	{ 0, },
};


static void bcm430x_recover_from_fatal(struct bcm430x_private *bcm, int error);
static void bcm430x_free_board(struct bcm430x_private *bcm);
static int bcm430x_init_board(struct bcm430x_private *bcm);


static void bcm430x_ram_write(struct bcm430x_private *bcm, u16 offset, u32 val)
{
	int hwswap;

	hwswap = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	hwswap &= BCM430x_SBF_XFER_REG_BYTESWAP;
	if (!hwswap)
		val = swab32(val);

	bcm430x_write16(bcm, BCM430x_MMIO_RAM_CONTROL, offset);
	bcm430x_write32(bcm, BCM430x_MMIO_RAM_DATA, val);
}

void bcm430x_shm_control_word(struct bcm430x_private *bcm,
			      u16 routing, u16 offset)
{
	u32 control;

	control = routing;
	control <<= 16;
	control |= offset;
	bcm430x_write32(bcm, BCM430x_MMIO_SHM_CONTROL, control);
}

u32 bcm430x_shm_read32(struct bcm430x_private *bcm,
		       u16 routing, u16 offset)
{
	u32 ret;

	if (routing == BCM430x_SHM_SHARED) {
		if (offset & 0x0003) {
			/* Unaligned access */
			bcm430x_shm_control_byte(bcm, routing, offset);
			ret = bcm430x_read16(bcm, BCM430x_MMIO_SHM_DATA_UNALIGNED);
			ret <<= 16;
			bcm430x_shm_control_word(bcm, routing, (offset >> 2) + 4);
			ret |= bcm430x_read16(bcm, BCM430x_MMIO_SHM_DATA);

			return ret;
		}
		offset >>= 2;
	}
	bcm430x_shm_control_word(bcm, routing, offset);
	ret = bcm430x_read32(bcm, BCM430x_MMIO_SHM_DATA);

	return ret;
}

u16 bcm430x_shm_read16(struct bcm430x_private *bcm,
		       u16 routing, u16 offset)
{
	u16 ret;

	if (routing == BCM430x_SHM_SHARED) {
		if (offset & 0x0003) {
			/* Unaligned access */
			bcm430x_shm_control_byte(bcm, routing, offset);
			ret = bcm430x_read16(bcm, BCM430x_MMIO_SHM_DATA_UNALIGNED);

			return ret;
		}
		offset >>= 2;
	}
	bcm430x_shm_control_word(bcm, routing, offset);
	ret = bcm430x_read16(bcm, BCM430x_MMIO_SHM_DATA);

	return ret;
}

void bcm430x_shm_write32(struct bcm430x_private *bcm,
			 u16 routing, u16 offset,
			 u32 value)
{
	if (routing == BCM430x_SHM_SHARED) {
		if (offset & 0x0003) {
			/* Unaligned access */
			bcm430x_shm_control_byte(bcm, routing, offset);
			bcm430x_write16(bcm, BCM430x_MMIO_SHM_DATA_UNALIGNED,
					(value >> 16) & 0xffff);
			bcm430x_shm_control_word(bcm, routing, (offset >> 2) + 4);
			bcm430x_write16(bcm, BCM430x_MMIO_SHM_DATA,
					value & 0xffff);
			return;
		}
		offset >>= 2;
	}
	bcm430x_shm_control_word(bcm, routing, offset);
	bcm430x_write32(bcm, BCM430x_MMIO_SHM_DATA, value);
}

void bcm430x_shm_write16(struct bcm430x_private *bcm,
			 u16 routing, u16 offset,
			 u16 value)
{
	if (routing == BCM430x_SHM_SHARED) {
		if (offset & 0x0003) {
			/* Unaligned access */
			bcm430x_shm_control_byte(bcm, routing, offset);
			bcm430x_write16(bcm, BCM430x_MMIO_SHM_DATA_UNALIGNED,
					value);
			return;
		}
		offset >>= 2;
	}
	bcm430x_shm_control_word(bcm, routing, offset);
	bcm430x_write16(bcm, BCM430x_MMIO_SHM_DATA, value);
}

int bcm430x_pci_read_config_16(struct pci_dev *pdev, u16 offset,
			       u16 *val)
{
	int err;

	err = pci_read_config_word(pdev, offset, val);
#ifdef DEBUG_ENABLE_PCILOG
	dprintk(KERN_INFO PFX "pciread16   offset: 0x%04x, value: 0x%04x\n", offset, *val);
#endif

	return err;
}

int bcm430x_pci_read_config_32(struct pci_dev *pdev, u16 offset,
			       u32 *val)
{
	int err;

	err = pci_read_config_dword(pdev, offset, val);
#ifdef DEBUG_ENABLE_PCILOG
	dprintk(KERN_INFO PFX "pciread32   offset: 0x%04x, value: 0x%08x\n", offset, *val);
#endif

	return err;
}

int bcm430x_pci_write_config_16(struct pci_dev *pdev, int offset,
				u16 val)
{
	int err;

	err = pci_write_config_word(pdev, offset, val);
#ifdef DEBUG_ENABLE_PCILOG
	dprintk(KERN_INFO PFX "pciwrite16  offset: 0x%04x, value: 0x%04x\n", offset, val);
#endif

	return err;
}

int bcm430x_pci_write_config_32(struct pci_dev *pdev, int offset,
				       u32 val)
{
	int err;

	err = pci_write_config_dword(pdev, offset, val);
#ifdef DEBUG_ENABLE_PCILOG
	dprintk(KERN_INFO PFX "pciwrite32  offset: 0x%04x, value: 0x%08x\n", offset, val);
#endif

	return err;
}

static inline
void bcm430x_do_generate_plcp_hdr(u32 *data, unsigned char *raw,
				  const u16 octets, const u8 bitrate,
				  const int ofdm_modulation)
{
	/* "data" and "raw" address the same memory area,
	 * but with different data types.
	 */

	/*TODO: This can be optimized, but first let's get it working. */

	if (ofdm_modulation) {
		switch (bitrate) {
		case IEEE80211_OFDM_RATE_6MB:
			*data = 0xB;	break;
		case IEEE80211_OFDM_RATE_9MB:
			*data = 0xF;	break;
		case IEEE80211_OFDM_RATE_12MB:
			*data = 0xA;	break;
		case IEEE80211_OFDM_RATE_18MB:
			*data = 0xE;	break;
		case IEEE80211_OFDM_RATE_24MB:
			*data = 0x9;	break;
		case IEEE80211_OFDM_RATE_36MB:
			*data = 0xD;	break;
		case IEEE80211_OFDM_RATE_48MB:
			*data = 0x8;	break;
		case IEEE80211_OFDM_RATE_54MB:
			*data = 0xC;	break;
		default:
			assert(0);
		}
		assert(!(octets & 0xF000));
		*data |= (octets << 5);
		*data = cpu_to_le32(*data);
	} else {
		u32 plen;

		plen = octets * 16 / bitrate;
		if ((octets * 16 % bitrate) > 0) {
			plen++;
			if ((bitrate == IEEE80211_CCK_RATE_11MB)
			    && ((octets * 8 % 11) < 4)) {
				raw[1] = 0x84;
			} else
				raw[1] = 0x04;
		} else
			raw[1] = 0x04;
		*data |= cpu_to_le32(plen << 16);

		switch (bitrate) {
		case IEEE80211_CCK_RATE_1MB:
			raw[0] = 0x0A;	break;
		case IEEE80211_CCK_RATE_2MB:
			raw[0] = 0x14;	break;
		case IEEE80211_CCK_RATE_5MB:
			raw[0] = 0x37;	break;
		case IEEE80211_CCK_RATE_11MB:
			raw[0] = 0x6E;	break;
		default:
			assert(0);
		}
	}

//bcm430x_printk_bitdump(raw, 4, 0, "PLCP");
}

#define bcm430x_generate_plcp_hdr(plcp, octets, bitrate, ofdm_modulation) \
	do {									\
		bcm430x_do_generate_plcp_hdr(&((plcp)->data), (plcp)->raw,	\
					     (octets), (bitrate),		\
					     (ofdm_modulation));		\
	} while (0)

void fastcall
bcm430x_generate_txhdr(struct bcm430x_private *bcm,
		       struct bcm430x_txhdr *txhdr,
		       const struct sk_buff *fragment_skb,
		       const int is_first_fragment,
		       const u16 cookie)
{
	const struct bcm430x_phyinfo *phy = bcm->current_core->phy;
	const int ofdm_modulation = (bcm->ieee->modulation == IEEE80211_OFDM_MODULATION);
	const u8 bitrate = phy->default_bitrate;
	u8 fallback_bitrate;
	u16 tmp;

	/* Values from the 80211 header */
	const u16 *frame_control;
	const unsigned char *macaddr1;
	const u16 *duration_id;

	/* First do some black magic to retrieve some
	 * values from the 80211 header of the packet.
	 */
	frame_control = (const u16 *)fragment_skb->data;
	macaddr1 = fragment_skb->data + 4;
	duration_id = (const u16 *)(fragment_skb->data + 2);

	/* Now contruct the TX header. */
	memset(txhdr, 0, sizeof(*txhdr));

	//TODO: Some RTS/CTS stuff has to be done.
	//TODO: Encryption stuff.
	//TODO: others?

	switch (bitrate) {
	case IEEE80211_OFDM_RATE_6MB:
	case IEEE80211_OFDM_RATE_9MB:
	case IEEE80211_OFDM_RATE_12MB:
		fallback_bitrate = IEEE80211_OFDM_RATE_6MB;
		break;
	case IEEE80211_OFDM_RATE_18MB:
		fallback_bitrate = IEEE80211_OFDM_RATE_9MB;
		break;
	case IEEE80211_OFDM_RATE_24MB:
		fallback_bitrate = IEEE80211_OFDM_RATE_12MB;
		break;
	case IEEE80211_OFDM_RATE_36MB:
		fallback_bitrate = IEEE80211_OFDM_RATE_18MB;
		break;
	case IEEE80211_OFDM_RATE_48MB:
	case IEEE80211_OFDM_RATE_54MB:
		fallback_bitrate = IEEE80211_OFDM_RATE_24MB;
		break;
	case IEEE80211_CCK_RATE_1MB:
	case IEEE80211_CCK_RATE_2MB:
		fallback_bitrate = IEEE80211_CCK_RATE_1MB;
		break;
	case IEEE80211_CCK_RATE_5MB:
		fallback_bitrate = IEEE80211_CCK_RATE_2MB;
		break;
	case IEEE80211_CCK_RATE_11MB:
		fallback_bitrate = IEEE80211_CCK_RATE_5MB;
		break;
	default:
		assert(0);
		fallback_bitrate = 0;
	}

	/* Set Frame Control from 80211 header. */
	txhdr->frame_control = cpu_to_le16(*frame_control);
	/* Copy address1 from 80211 header. */
	memcpy(txhdr, macaddr1, 6);
	/* Set the cookie (used as driver internal ID for the frame) */
	txhdr->cookie = cpu_to_le16(cookie);
	/* Set the fallback duration ID. */
	//FIXME: We use the original durid for now.
	txhdr->fallback_dur_id = cpu_to_le16(*duration_id);

	/* Generate the PLCP header and the fallback PLCP header. */
	bcm430x_generate_plcp_hdr(&txhdr->plcp, fragment_skb->len,
				  bitrate, ofdm_modulation);
	bcm430x_generate_plcp_hdr(&txhdr->fallback_plcp, fragment_skb->len,
				  fallback_bitrate, ofdm_modulation);

	/* Set the CONTROL field */
	tmp = 0;
	if (ofdm_modulation)
		tmp |= BCM430x_TXHDRCTL_OFDM;
	tmp |= (phy->antenna_diversity << BCM430x_TXHDRCTL_ANTENNADIV_SHIFT)
		& BCM430x_TXHDRCTL_ANTENNADIV_MASK;
	txhdr->control = cpu_to_le16(tmp);

	/* Set the FLAGS field */
	tmp = 0;
	if (!is_multicast_ether_addr((const u8 *)macaddr1))
		tmp |= BCM430x_TXHDRFLAG_NOMCAST;
	tmp |= 0x10; // FIXME: unknown meaning.
	if (ofdm_modulation)
		tmp |= BCM430x_TXHDRFLAG_FALLBACKOFDM;
	if (is_first_fragment)
		tmp |= BCM430x_TXHDRFLAG_FIRSTFRAGMENT;
	txhdr->flags = cpu_to_le16(tmp);

	/* Set WSEC/RATE field */
	//TODO: rts, wsec
	tmp = (txhdr->plcp.raw[0] << BCM430x_TXHDR_RATE_SHIFT)
	       & BCM430x_TXHDR_RATE_MASK;
	txhdr->wsec_rate = cpu_to_le16(tmp);


bcm430x_printk_bitdump((const unsigned char *)txhdr, sizeof(*txhdr), 1, "TX header");
}

/* Enable a Generic IRQ. "mask" is the mask of which IRQs to enable.
 * Returns the _previously_ enabled IRQ mask.
 */
static inline u32 bcm430x_interrupt_enable(struct bcm430x_private *bcm, u32 mask)
{
	u32 old_mask;

	old_mask = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_MASK);
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_MASK, old_mask | mask);

	return old_mask;
}

/* Disable a Generic IRQ. "mask" is the mask of which IRQs to disable.
 * Returns the _previously_ enabled IRQ mask.
 */
static inline u32 bcm430x_interrupt_disable(struct bcm430x_private *bcm, u32 mask)
{
	u32 old_mask;

	old_mask = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_MASK);
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_MASK, old_mask & ~mask);

	return old_mask;
}

/* Make sure we don't receive more data from the device. */
static void bcm430x_disable_interrupts_sync(struct bcm430x_private *bcm)
{
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);
	bcm430x_interrupt_disable(bcm, BCM430x_IRQ_ALL);
	spin_unlock_irqrestore(&bcm->lock, flags);
	tasklet_disable(&bcm->isr_tasklet);
}

static int bcm430x_read_radioinfo(struct bcm430x_private *bcm)
{
	u32 radio_id;

	if (bcm->chip_id == 0x4317) {
		if (bcm->chip_rev == 0x00)
			radio_id = 0x3205017F;
		else if (bcm->chip_rev == 0x01)
			radio_id = 0x4205017F;
		else
			radio_id = 0x5205017F;
	} else {
		bcm430x_write16(bcm, BCM430x_MMIO_RADIO_CONTROL, BCM430x_RADIO_ID);
		radio_id = bcm430x_read16(bcm, BCM430x_MMIO_RADIO_DATA_HIGH);
		radio_id <<= 16;
		bcm430x_write16(bcm, BCM430x_MMIO_RADIO_CONTROL, BCM430x_RADIO_ID);
		radio_id |= bcm430x_read16(bcm, BCM430x_MMIO_RADIO_DATA_LOW);
	}

	dprintk(KERN_INFO PFX "Detected Radio:  ID: %x (Manuf: %x Ver: %x Rev: %x)\n",
		radio_id,
		(radio_id & 0x00000FFF),
		(radio_id & 0x0FFFF000) >> 12,
		(radio_id & 0xF0000000) >> 28);

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		//TODO: fail, if the radio is not 2060 with rev 1 and vendor 0x17f
		break;
	case BCM430x_PHYTYPE_B:
		//TODO: fail, if the radio is not 205x
		break;
	case BCM430x_PHYTYPE_G:
		//TODO: fail, if the radio is not 2050
		break;
	}

	bcm->current_core->radio->id = radio_id;

	return 0;
}

/* Read SPROM and fill the useful values in the net_device struct */
static void bcm430x_read_sprom(struct bcm430x_private *bcm)
{
	int i;
	u16 value;
	struct net_device *net_dev = bcm->net_dev;

	/* read MAC address into dev->dev_addr */
	value = bcm430x_sprom_read(bcm, BCM430x_SPROM_IL0MACADDR + 0);
	*((u16 *)net_dev->dev_addr + 0) = cpu_to_be16(value);
	value = bcm430x_sprom_read(bcm, BCM430x_SPROM_IL0MACADDR + 1);
	*((u16 *)net_dev->dev_addr + 1) = cpu_to_be16(value);
	value = bcm430x_sprom_read(bcm, BCM430x_SPROM_IL0MACADDR + 2);
	*((u16 *)net_dev->dev_addr + 2) = cpu_to_be16(value);

	value = bcm430x_sprom_read(bcm, BCM430x_SPROM_BOARDFLAGS);
	if (value == 0xffff)
		value = 0x0000;
	bcm->sprom.boardflags = value;

	/* read LED infos */
	value = bcm430x_sprom_read(bcm, BCM430x_SPROM_WL0GPIO0);
	bcm->leds[0] = value & 0x00FF;
	bcm->leds[1] = (value & 0xFF00) >> 8;
	value = bcm430x_sprom_read(bcm, BCM430x_SPROM_WL0GPIO2);
	bcm->leds[2] = value & 0x00FF;
	bcm->leds[3] = (value & 0xFF00) >> 8;

	for (i = 0; i < BCM430x_LED_COUNT; i++) {
		if ((bcm->leds[i] & ~BCM430x_LED_ACTIVELOW) == BCM430x_LED_INACTIVE) {
			bcm->leds[i] = 0xFF;
			continue;
		};
		if (bcm->leds[i] == 0xFF) {
			switch (i) {
			case 0:
				bcm->leds[0] = ((bcm->board_vendor == PCI_VENDOR_ID_COMPAQ)
				                ? BCM430x_LED_RADIO_ALL
				                : BCM430x_LED_ACTIVITY);
				break;
			case 1:
				bcm->leds[1] = BCM430x_LED_RADIO_B;
				break;
			case 2:
				bcm->leds[2] = BCM430x_LED_RADIO_A;
				break;
			case 3:
				bcm->leds[3] = BCM430x_LED_OFF;
				break;
			}
		}
	}
}

/* DummyTransmission function, as documented on 
 * http://bcm-specs.sipsolutions.net/DummyTransmission
 */
void bcm430x_dummy_transmission(struct bcm430x_private *bcm)
{
	unsigned int i, max_loop;
	u16 value = 0;
	u32 buffer[5] = {
		0x00000000,
		0x0000D400,
		0x00000000,
		0x00000001,
		0x00000000,
	};

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		max_loop = 0x1E;
		buffer[0] = 0xCC010200;
		break;
	case BCM430x_PHYTYPE_B:
	case BCM430x_PHYTYPE_G:
		max_loop = 0xFA;
		buffer[0] = 0x6E840B00; 
		break;
	default:
		assert(0);
		return;
	}

	for (i = 0; i < 5; i++)
		bcm430x_ram_write(bcm, i * 4, buffer[i]);

	bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD); /* dummy read */

	bcm430x_write16(bcm, 0x0568, 0x0000);
	bcm430x_write16(bcm, 0x07C0, 0x0000);
	bcm430x_write16(bcm, 0x050C, ((bcm->current_core->phy->type == BCM430x_PHYTYPE_A) ? 1 : 0));
	bcm430x_write16(bcm, 0x0508, 0x0000);
	bcm430x_write16(bcm, 0x050A, 0x0000);
	bcm430x_write16(bcm, 0x054C, 0x0000);
	bcm430x_write16(bcm, 0x056A, 0x0014);
	bcm430x_write16(bcm, 0x0568, 0x0826);
	bcm430x_write16(bcm, 0x0500, 0x0000);
	bcm430x_write16(bcm, 0x0502, 0x0030);

	for (i = 0x00; i < max_loop; i++) {
		value = bcm430x_read16(bcm, 0x050E);
//		dprintk(KERN_INFO PFX "dummy_tx(): loop1, iteration %d, value = %04x\n", i, value);
		if ((value & 0x0080) != 0)
			break;
		udelay(10);
	}
	if (unlikely(i >= max_loop))
		printk(KERN_WARNING PFX "dummy_tx() Warning: first loop timed out!\n");

	for (i = 0x00; i < 0x0A; i++) {
		value = bcm430x_read16(bcm, 0x050E);
//		dprintk(KERN_INFO PFX "dummy_tx(): loop2, iteration %d, value = %04x\n", i, value);
		if ((value & 0x0400) != 0)
			break;
		udelay(10);
	}
	if (unlikely(i >= 0x0A))
		printk(KERN_WARNING PFX "dummy_tx() Warning: second loop timed out!\n");

	for (i = 0x00; i < 0x0A; i++) {
		value = bcm430x_read16(bcm, 0x0690);
//		dprintk(KERN_INFO PFX "dummy_tx(): loop3, iteration %d, value = %04x\n", i, value);
		if ((value & 0x0100) == 0)
			break;
		udelay(10);
	}
	if (unlikely(i >= 0x0A))
		printk(KERN_WARNING PFX "dummy_tx() Warning: third loop timed out!\n");
}

/* Puts the index of the current core into user supplied core variable.
 * This function reads the value from the device.
 * Almost always you don't want to call this, but use bcm->current_core
 */
static inline
int _get_current_core(struct bcm430x_private *bcm, int *core)
{
	int err;

	err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_REG_ACTIVE_CORE, core);
	if (unlikely(err)) {
		dprintk(KERN_ERR PFX "BCM430x_REG_ACTIVE_CORE read failed!\n");
		return -ENODEV;
	}
	*core = (*core - 0x18000000) / 0x1000;

	return 0;
}

/* Lowlevel core-switch function. This is only to be used in
 * bcm430x_switch_core() and bcm430x_probe_cores()
 */
static int _switch_core(struct bcm430x_private *bcm, int core)
{
	int err;
	int attempts = 0;
	int current_core = -1;

	assert(core >= 0);

	err = _get_current_core(bcm, &current_core);
	if (unlikely(err))
		goto out;

	/* Write the computed value to the register. This doesn't always
	   succeed so we retry BCM430x_SWITCH_CORE_MAX_RETRIES times */
	while (current_core != core) {
		if (unlikely(attempts++ > BCM430x_SWITCH_CORE_MAX_RETRIES)) {
			err = -ENODEV;
			printk(KERN_ERR PFX
			       "unable to switch to core %u, retried %i times\n",
			       core, attempts);
			goto out;
		}
		err = bcm430x_pci_write_config_32(bcm->pci_dev,
						  BCM430x_REG_ACTIVE_CORE,
						  (core * 0x1000) + 0x18000000);
		if (unlikely(err)) {
			dprintk(KERN_ERR PFX "BCM430x_REG_ACTIVE_CORE write failed!\n");
			continue;
		}
		_get_current_core(bcm, &current_core);
	}

	assert(err == 0);
out:
	return err;
}

int bcm430x_switch_core(struct bcm430x_private *bcm, struct bcm430x_coreinfo *new_core)
{
	int err;

	if (!new_core)
		return 0;

	if (!(new_core->flags & BCM430x_COREFLAG_AVAILABLE))
		return -ENODEV;
	if (bcm->current_core == new_core)
		return 0;
	err = _switch_core(bcm, new_core->index);
	if (!err)
		bcm->current_core = new_core;

	return err;
}

static inline int bcm430x_core_enabled(struct bcm430x_private *bcm)
{
	u32 value;

	value = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);
	value &= BCM430x_SBTMSTATELOW_CLOCK | BCM430x_SBTMSTATELOW_RESET
		 | BCM430x_SBTMSTATELOW_REJECT;

	return (value == BCM430x_SBTMSTATELOW_CLOCK);
}

/* disable current core */
static int bcm430x_core_disable(struct bcm430x_private *bcm, u32 core_flags)
{
	u32 sbtmstatelow;
	u32 sbtmstatehigh;
	int i;

	/* fetch sbtmstatelow from core information registers */
	sbtmstatelow = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);

	/* core is already in reset */
	if (sbtmstatelow & BCM430x_SBTMSTATELOW_RESET)
		goto out;

	if (sbtmstatelow & BCM430x_SBTMSTATELOW_CLOCK) {
		sbtmstatelow = BCM430x_SBTMSTATELOW_CLOCK |
			       BCM430x_SBTMSTATELOW_REJECT;
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, sbtmstatelow);

		for (i = 0; i < 1000; i++) {
			sbtmstatelow = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);
			if (sbtmstatelow & BCM430x_SBTMSTATELOW_REJECT) {
				i = -1;
				break;
			}
			udelay(10);
		}
		if (i != -1) {
			printk(KERN_ERR PFX "Error: core_disable() REJECT timeout!\n");
			return -EBUSY;
		}

		for (i = 0; i < 1000; i++) {
			sbtmstatehigh = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATEHIGH);
			if (!(sbtmstatehigh & BCM430x_SBTMSTATEHIGH_BUSY)) {
				i = -1;
				break;
			}
			udelay(10);
		}
		if (i != -1) {
			printk(KERN_ERR PFX "Error: core_disable() BUSY timeout!\n");
			return -EBUSY;
		}

		sbtmstatelow = BCM430x_SBTMSTATELOW_FORCE_GATE_CLOCK |
			       BCM430x_SBTMSTATELOW_REJECT |
			       BCM430x_SBTMSTATELOW_RESET |
			       BCM430x_SBTMSTATELOW_CLOCK |
			       core_flags;
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, sbtmstatelow);
		udelay(10);
	}

	sbtmstatelow = BCM430x_SBTMSTATELOW_RESET |
		       BCM430x_SBTMSTATELOW_REJECT |
		       core_flags;
	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, sbtmstatelow);

out:
	bcm->current_core->flags &= ~ BCM430x_COREFLAG_ENABLED;
	return 0;
}

/* enable (reset) current core */
static int bcm430x_core_enable(struct bcm430x_private *bcm, u32 core_flags)
{
	u32 sbtmstatelow;
	u32 sbtmstatehigh;
	u32 sbimstate;
	int err;

	err = bcm430x_core_disable(bcm, core_flags);
	if (err)
		goto out;

	sbtmstatelow = BCM430x_SBTMSTATELOW_CLOCK |
		       BCM430x_SBTMSTATELOW_RESET |
		       BCM430x_SBTMSTATELOW_FORCE_GATE_CLOCK |
		       core_flags;
	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);

	sbtmstatehigh = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATEHIGH);
	if (sbtmstatehigh & BCM430x_SBTMSTATEHIGH_SERROR) {
		sbtmstatehigh = 0x00000000;
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATEHIGH, sbtmstatehigh);
	}

	sbimstate = bcm430x_read32(bcm, BCM430x_CIR_SBIMSTATE);
	if (sbimstate & (BCM430x_SBIMSTATE_IB_ERROR | BCM430x_SBIMSTATE_TIMEOUT)) {
		sbimstate &= ~(BCM430x_SBIMSTATE_IB_ERROR | BCM430x_SBIMSTATE_TIMEOUT);
		bcm430x_write32(bcm, BCM430x_CIR_SBIMSTATE, sbimstate);
	}

	sbtmstatelow = BCM430x_SBTMSTATELOW_CLOCK |
		       BCM430x_SBTMSTATELOW_FORCE_GATE_CLOCK |
		       core_flags;
	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);

	sbtmstatelow = BCM430x_SBTMSTATELOW_CLOCK | core_flags;
	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);

	bcm->current_core->flags |= BCM430x_COREFLAG_ENABLED;
	assert(err == 0);
out:
	return err;
}

/* http://bcm-specs.sipsolutions.net/80211CoreReset */
void bcm430x_wireless_core_reset(struct bcm430x_private *bcm, int connect_phy)
{
	u32 flags = 0x00040000;

	if ((bcm430x_core_enabled(bcm)) && (bcm->data_xfer_mode == BCM430x_DATAXFER_DMA)) {
		/* reset all used DMA controllers. */
		bcm430x_dmacontroller_tx_reset(bcm, BCM430x_MMIO_DMA1_BASE);
		bcm430x_dmacontroller_tx_reset(bcm, BCM430x_MMIO_DMA2_BASE);
		bcm430x_dmacontroller_tx_reset(bcm, BCM430x_MMIO_DMA3_BASE);
		bcm430x_dmacontroller_tx_reset(bcm, BCM430x_MMIO_DMA4_BASE);
		bcm430x_dmacontroller_rx_reset(bcm, BCM430x_MMIO_DMA1_BASE);
		if (bcm->current_core->rev < 5)
			bcm430x_dmacontroller_rx_reset(bcm, BCM430x_MMIO_DMA4_BASE);
	}
	if (bcm->status & BCM430x_STAT_DEVSHUTDOWN)
		bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
		                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
				& ~(BCM430x_SBF_MAC_ENABLED | 0x00000002));
	else {
		if (connect_phy)
			flags |= 0x20000000;
		bcm430x_phy_connect(bcm, connect_phy);
		bcm430x_core_enable(bcm, flags);
		bcm430x_write16(bcm, 0x03E6, 0x0000);
		bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, 0x00000400);
		//XXX: bcm->dma_savestatus[0--3] = { 0, 0, 0, 0 };
	}
}

static void bcm430x_wireless_core_disable(struct bcm430x_private *bcm)
{
	if (bcm->status & BCM430x_STAT_DEVSHUTDOWN) {
		bcm430x_radio_turn_off(bcm);
		bcm430x_write16(bcm, 0x03E6, 0x00F4);
		bcm430x_core_disable(bcm, 0);
	} else {
		if (bcm->current_core->radio->enabled) {
			bcm430x_radio_turn_off(bcm);
		} else {
			if ((bcm->current_core->rev >= 3) && (bcm430x_read32(bcm, 0x0158) & (1 << 16)))
				bcm430x_radio_turn_off(bcm);
			if ((bcm->current_core->rev < 3) && !(bcm430x_read16(bcm, 0x049A) & (1 << 4)))
				bcm430x_radio_turn_off(bcm);
		}
	}
}

/* Mark the current 80211 core inactive.
 * "active_80211_core" is the other 80211 core, which is used.
 */
static int bcm430x_wireless_core_mark_inactive(struct bcm430x_private *bcm,
					       struct bcm430x_coreinfo *active_80211_core)
{
	u32 sbtmstatelow;
	struct bcm430x_coreinfo *old_core;
	int err = 0;

	bcm430x_interrupt_disable(bcm, BCM430x_IRQ_ALL);
	bcm430x_radio_turn_off(bcm);
	sbtmstatelow = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);
	sbtmstatelow &= ~0x200a0000;
	sbtmstatelow |= 0xa0000;
	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);
	sbtmstatelow = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);
	sbtmstatelow &= ~0xa0000;
	sbtmstatelow |= 0x80000;
	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);

	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_G) {
		old_core = bcm->current_core;
		err = bcm430x_switch_core(bcm, active_80211_core);
		if (err)
			goto out;
		sbtmstatelow = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);
		sbtmstatelow &= ~0x20000000;
		sbtmstatelow |= 0x20000000;
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, sbtmstatelow);
		err = bcm430x_switch_core(bcm, old_core);
	}

out:
	return err;
}

/* Read the Transmit Status from MMIO and build the Transmit Status array. */
static inline int build_transmit_status_array(struct bcm430x_private *bcm,
					      unsigned char *status)
{
	u32 v170;
	u32 v174;

	v170 = bcm430x_read32(bcm, 0x170);
	if (v170 == 0x00000000)
		return -1;
	v174 = bcm430x_read32(bcm, 0x174);

	memset(status, 0, BCM430x_XMIT_STAT_ARRAY_SIZE);

	/* Internal Sending ID. */
	*((u16 *)(status + 4)) = cpu_to_le16( (u16)(v170 >> 16) );
	/* 2 counters (both 4 bits) in the upper byte and flags in the lower byte. */
	*((u16 *)(status + 6)) = cpu_to_le16( (u16)((v170 & 0xfff0) | ((v170 & 0xf) >> 1)) );
	/* XXX: 802.11 sequence number? */
	*((u16 *)(status + 10)) = cpu_to_le16( (u16)(v174 & 0xffff) );
	/* XXX: Unknown. */
	*((u16 *)(status + 12)) = cpu_to_le16( (u16)((v174 >> 16) & 0xff) );

	return 0;
}

static inline void interpret_transmit_status_array(struct bcm430x_private *bcm,
						   unsigned char *status)
{
	u16 internal_id;
	u8 flags;
	u8 counter1, counter2;

	flags = *((u8 *)(status + 6));
	if (flags & 0x20) {
printkl(KERN_INFO PFX "Transmit Status ignored\n");
		return;
	}
	/* TODO: What is the meaning of the flags? Tested bits are: 0x01, 0x02, 0x10, 0x40, 0x04, 0x08 */
	counter1 = ( *((u8 *)(status + 7)) & 0xf0) >> 4;
	counter2 = ( *((u8 *)(status + 7)) & 0x0f);
	internal_id = le16_to_cpu( *((u16 *)(status + 4)) );

printkl(KERN_INFO PFX "Transmit Status received:  flags: 0x%02x,  "
		      "cnt1: 0x%02x,  cnt2: 0x%02x,  ID: 0x%04x\n",
	flags, counter1, counter2, internal_id);
	/*TODO*/
}

static inline void handle_irq_transmit_status(struct bcm430x_private *bcm)
{
	unsigned char transmit_status[BCM430x_XMIT_STAT_ARRAY_SIZE];
	int res;

	assert(bcm->current_core->id == BCM430x_COREID_80211);

	//TODO: In AP mode, this also causes sending of powersave responses.

	while (1) {
		if (bcm->current_core->rev < 5) {
			printkl(KERN_ERR PFX "TODO in %s at line %d\n", __FILE__, __LINE__);
			/* TODO: The status is received via the last DMA controller or PIO queue 3. */
			if (bcm->data_xfer_mode == BCM430x_DATAXFER_DMA) {
				/* XXX */
			} else {
				assert(bcm->data_xfer_mode == BCM430x_DATAXFER_PIO);
				/* XXX */
			}
			return;
		} else {
			res = build_transmit_status_array(bcm, transmit_status);
			if (res) {
printkl(KERN_INFO PFX "handle_irq_transmit_status: No transmit status available\n");
				return;
			}
		}
		interpret_transmit_status_array(bcm, transmit_status);
	}
}

/* Interrupt handler bottom-half */
static void bcm430x_interrupt_tasklet(struct bcm430x_private *bcm)
{
	u32 reason;
	unsigned long flags;

#ifdef BCM430x_DEBUG
	u32 _handled = 0x00000000;
# define bcmirq_handled(irq)	do { _handled |= (irq); } while (0)
#else
# define bcmirq_handled(irq)	do { /* nothing */ } while (0)
#endif /* BCM430x_DEBUG */

	spin_lock_irqsave(&bcm->lock, flags);
	reason = bcm->irq_reason;

	if (reason & BCM430x_IRQ_BEACON) {
		/*TODO*/
		//bcmirq_handled(BCM430x_IRQ_BEACON);
	}

	if (reason & BCM430x_IRQ_TBTT) {
		/*TODO*/
		//bcmirq_handled(BCM430x_IRQ_TBTT);
	}

	if (reason & BCM430x_IRQ_REG124) {
		/*TODO*/
		//bcmirq_handled(BCM430x_IRQ_REG124);
	}

	if (reason & BCM430x_IRQ_PMQ) {
		/*TODO*/
		//bcmirq_handled(BCM430x_IRQ_PMQ);
	}

	if (unlikely(reason & BCM430x_IRQ_TXFIFO_ERROR)) {
		dprintkl(KERN_ERR PFX "TX FIFO error. DMA: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			 bcm->dma_reason[0], bcm->dma_reason[1],
			 bcm->dma_reason[2], bcm->dma_reason[3]);
		bcm430x_recover_from_fatal(bcm, BCM430x_FATAL_TXFIFO);
		spin_unlock_irqrestore(&bcm->lock, flags);
		return;
	}

	if (reason & BCM430x_IRQ_SCAN) {
		/*TODO*/
		//bcmirq_handled(BCM430x_IRQ_SCAN);
	}

	if (reason & BCM430x_IRQ_BGNOISE) {
		/*TODO*/
		//bcmirq_handled(BCM430x_IRQ_BGNOISE);
	}

	if (reason & BCM430x_IRQ_XMIT_STATUS) {
		handle_irq_transmit_status(bcm);
		bcmirq_handled(BCM430x_IRQ_XMIT_STATUS);
	}

#ifdef BCM430x_DEBUG
	if (reason & ~_handled) {
		printkl(KERN_WARNING PFX
			"Unhandled IRQ! Reason: 0x%08x,  Unhandled: 0x%08x,  "
			"DMA: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			reason, (reason & ~_handled),
			bcm->dma_reason[0], bcm->dma_reason[1],
			bcm->dma_reason[2], bcm->dma_reason[3]);
	}
#endif
#undef bcmirq_handled

	bcm430x_interrupt_enable(bcm, bcm->irq_savedstate);
	spin_unlock_irqrestore(&bcm->lock, flags);
}

/* Interrupt handler top-half */
static irqreturn_t bcm430x_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	struct bcm430x_private *bcm = dev_id;
	u32 reason, mask;

	if (!bcm)
		return IRQ_NONE;

	spin_lock(&bcm->lock);

	reason = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);
	if (reason == 0xffffffff) {
		/* irq not for us (shared irq) */
		spin_unlock(&bcm->lock);
		return IRQ_NONE;
	}
	mask = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_MASK);
	if (!(reason & mask)) {
		spin_unlock(&bcm->lock);
		return IRQ_HANDLED;
	}

	bcm->dma_reason[0] = bcm430x_read32(bcm, BCM430x_MMIO_DMA1_REASON)
			     & 0x0001dc00;
	bcm->dma_reason[1] = bcm430x_read32(bcm, BCM430x_MMIO_DMA2_REASON)
			     & 0x0000dc00;
	bcm->dma_reason[2] = bcm430x_read32(bcm, BCM430x_MMIO_DMA3_REASON)
			     & 0x0000dc00;
	bcm->dma_reason[3] = bcm430x_read32(bcm, BCM430x_MMIO_DMA4_REASON)
			     & 0x0001dc00;

	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_REASON,
			reason & mask);

	bcm430x_write32(bcm, BCM430x_MMIO_DMA1_REASON,
			bcm->dma_reason[0]);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA2_REASON,
			bcm->dma_reason[1]);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA3_REASON,
			bcm->dma_reason[2]);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA4_REASON,
			bcm->dma_reason[3]);

	if (bcm->data_xfer_mode == BCM430x_DATAXFER_PIO &&
	    bcm->current_core->rev < 3) {
		/* TODO */
	}

	/* disable all IRQs. They are enabled again in the bottom half. */
	bcm->irq_savedstate = bcm430x_interrupt_disable(bcm, BCM430x_IRQ_ALL);

	/* save the reason code and call our bottom half. */
	bcm->irq_reason = reason;
	tasklet_schedule(&bcm->isr_tasklet);

	spin_unlock(&bcm->lock);

	return IRQ_HANDLED;
}

static inline void bcm430x_write_microcode(struct bcm430x_private *bcm,
				    const u32 *data, const unsigned int len)
{
	unsigned int i;
	bcm430x_shm_control_word(bcm, BCM430x_SHM_UCODE, 0x0000);
	for (i = 0; i < len; i++) {
		bcm430x_write32(bcm, BCM430x_MMIO_SHM_DATA,
				be32_to_cpu(data[i]));
		udelay(10);
	}
}

static inline void bcm430x_write_pcm(struct bcm430x_private *bcm,
			      const u32 *data, const unsigned int len)
{
	unsigned int i;
	bcm430x_shm_control_word(bcm, BCM430x_SHM_PCM, 0x01ea);
	bcm430x_write32(bcm, BCM430x_MMIO_SHM_DATA, 0x00004000);
	bcm430x_shm_control_word(bcm, BCM430x_SHM_PCM, 0x01eb);
	for (i = 0; i < len; i++) {
		bcm430x_write32(bcm, BCM430x_MMIO_SHM_DATA,
				be32_to_cpu(data[i]));
		udelay(10);
	}
}

static int bcm430x_upload_microcode(struct bcm430x_private *bcm)
{
	int err = -ENODEV;
	const struct firmware *fw;
	char buf[22] = { 0 };

#ifdef DEBUG_ENABLE_UCODE_MMIO_PRINT
	bcm430x_mmioprint_enable(bcm);
#else
	bcm430x_mmioprint_disable(bcm);
#endif

	snprintf(buf, ARRAY_SIZE(buf), "bcm430x_microcode%d.fw",
		 (bcm->current_core->rev >= 5 ? 5 : bcm->current_core->rev));
	if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
		printk(KERN_ERR PFX 
		       "Error: Microcode \"%s\" not available or load failed.\n",
		        buf);
		goto out;
	}
	bcm430x_write_microcode(bcm, (u32 *)fw->data, fw->size / sizeof(u32));
#ifdef BCM430x_DEBUG
	bcm->ucode_size = fw->size;
#endif
	release_firmware(fw);

	snprintf(buf, ARRAY_SIZE(buf),
		 "bcm430x_pcm%d.fw", (bcm->current_core->rev < 5 ? 4 : 5));
	if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
		printk(KERN_ERR PFX
		       "Error: PCM \"%s\" not available or load failed.\n",
		       buf);
		goto out;
	}
	bcm430x_write_pcm(bcm, (u32 *)fw->data, fw->size / sizeof(u32));
#ifdef BCM430x_DEBUG
	bcm->pcm_size = fw->size;
#endif
	release_firmware(fw);

	err = 0;
out:
#ifdef DEBUG_ENABLE_UCODE_MMIO_PRINT
	bcm430x_mmioprint_disable(bcm);
#else
	bcm430x_mmioprint_enable(bcm);
#endif
	return err;
}

static void bcm430x_write_initvals(struct bcm430x_private *bcm,
				   const struct bcm430x_initval *data,
				   const unsigned int len)
{
	u16 offset, size;
	u32 value;
	unsigned int i;

	for (i = 0; i < len; i++) {
		offset = be16_to_cpu(data[i].offset);
		size = be16_to_cpu(data[i].size);
		value = be32_to_cpu(data[i].value);

		if (size == 2)
			bcm430x_write16(bcm, offset, value);
		else if (size == 4)
			bcm430x_write32(bcm, offset, value);
		else
			printk(KERN_ERR PFX "InitVals fileformat error.\n");
	}
}

static int bcm430x_upload_initvals(struct bcm430x_private *bcm)
{
	int err = -ENODEV;
	u32 sbtmstatehigh;
	const struct firmware *fw;
	char buf[21] = { 0 };

#ifdef DEBUG_ENABLE_UCODE_MMIO_PRINT
	bcm430x_mmioprint_enable(bcm);
#else
	bcm430x_mmioprint_disable(bcm);
#endif

	if ((bcm->current_core->rev == 2) || (bcm->current_core->rev == 4)) {
		switch (bcm->current_core->phy->type) {
			case BCM430x_PHYTYPE_A:
				snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 3);
				break;
			case BCM430x_PHYTYPE_B:
			case BCM430x_PHYTYPE_G:
				snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 1);
				break;
			default:
				goto out_noinitval;
		}
	
	} else if (bcm->current_core->rev >= 5) {
		switch (bcm->current_core->phy->type) {
			case BCM430x_PHYTYPE_A:
				snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 7);
				break;
			case BCM430x_PHYTYPE_B:
			case BCM430x_PHYTYPE_G:
				snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 5);
				break;
			default:
				goto out_noinitval;
		}
	} else
		goto out_noinitval;

	if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
		printk(KERN_ERR PFX 
		       "Error: InitVals \"%s\" not available or load failed.\n",
		        buf);
		goto out;
	}
	if (fw->size % sizeof(struct bcm430x_initval)) {
		printk(KERN_ERR PFX "InitVals fileformat error.\n");
		release_firmware(fw);
		goto out;
	}

	bcm430x_write_initvals(bcm, (struct bcm430x_initval *)fw->data,
			       fw->size / sizeof(struct bcm430x_initval));

	release_firmware(fw);

	if (bcm->current_core->rev >= 5) {
		switch (bcm->current_core->phy->type) {
			case BCM430x_PHYTYPE_A:
				sbtmstatehigh = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATEHIGH);
				if (sbtmstatehigh & 0x00010000)
					snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 9);
				else
					snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 10);
				break;
			case BCM430x_PHYTYPE_B:
			case BCM430x_PHYTYPE_G:
					snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 6);
				break;
			default:
				goto out_noinitval;
		}
	
		if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
			printk(KERN_ERR PFX 
			       "Error: InitVals \"%s\" not available or load failed.\n",
		        	buf);
			goto out;
		}
		if (fw->size % sizeof(struct bcm430x_initval)) {
			printk(KERN_ERR PFX "InitVals fileformat error.\n");
			release_firmware(fw);
			goto out;
		}

		bcm430x_write_initvals(bcm, (struct bcm430x_initval *)fw->data,
				       fw->size / sizeof(struct bcm430x_initval));
	
		release_firmware(fw);
		
	}

	dprintk(KERN_INFO PFX "InitVals written\n");
	err = 0;
out:
#ifdef DEBUG_ENABLE_UCODE_MMIO_PRINT
	bcm430x_mmioprint_disable(bcm);
#else
	bcm430x_mmioprint_enable(bcm);
#endif

	return err;

out_noinitval:
	printk(KERN_ERR PFX "Error: No InitVals available!\n");
	goto out;
}

static int bcm430x_initialize_irq(struct bcm430x_private *bcm)
{
	int res;
	unsigned int i;
	u32 data;

	res = request_irq(bcm->pci_dev->irq, bcm430x_interrupt_handler,
			  SA_SHIRQ, DRV_NAME, bcm);
	if (res) {
		printk(KERN_ERR PFX "Cannot register IRQ%d\n", bcm->pci_dev->irq);
		return -EFAULT;
	}
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_REASON, 0xffffffff);
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, 0x00020402);
	i = 0;
	while (1) {
		data = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);
		if (data == 0x00000001)
			break;
		i++;
		if (i >= BCM430x_IRQWAIT_MAX_RETRIES) {
			printk(KERN_ERR PFX "Card IRQ register not responding. "
					    "Giving up.\n");
			free_irq(bcm->pci_dev->irq, bcm);
			return -ENODEV;
		}
		udelay(10);
	}
	// dummy read
	bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);

	return 0;
}

/* Keep this slim, as we're going to call it from within the interrupt tasklet! */
static void bcm430x_update_leds(struct bcm430x_private *bcm)
{
	int id;
	u16 value = bcm430x_read16(bcm, BCM430x_MMIO_GPIO_CONTROL);
	u16 state;

	for (id = 0; id < BCM430x_LED_COUNT; id++) {
		if (bcm->leds[id] == 0xFF)
			continue;

		state = 0;

		switch (bcm->leds[id] & ~BCM430x_LED_ACTIVELOW) {
		case BCM430x_LED_OFF:
			break;
		case BCM430x_LED_ON:
			state = 1;
			break;
		case BCM430x_LED_RADIO_ALL:
			state = ((bcm->radio[0].enabled) || (bcm->radio[1].enabled)) ? 1 : 0;
			break;
		case BCM430x_LED_RADIO_A:
			if ((bcm->phy[0].type == BCM430x_PHYTYPE_A) && (bcm->radio[0].enabled))
				state = 1;
			if ((bcm->phy[1].type == BCM430x_PHYTYPE_A) && (bcm->radio[1].enabled))
				state = 1;
			break;
		case BCM430x_LED_RADIO_B:
			if ((bcm->phy[0].type == BCM430x_PHYTYPE_B) && (bcm->radio[0].enabled))
				state = 1;
			if ((bcm->phy[1].type == BCM430x_PHYTYPE_B) && (bcm->radio[1].enabled))
				state = 1;
			break;
		/*
		 * TODO: LED_ACTIVITY, MODE_BG, ASSOC
		 */
		default:
			break;
		};

		if (bcm->leds[id] & BCM430x_LED_ACTIVELOW)
			state = 0x0001 & (state ^ 0x0001);
		value &= ~(1 << id);
		value |= (state << id);
	}

	bcm430x_write16(bcm, BCM430x_MMIO_GPIO_CONTROL, value);
}

/* Switch to the core used to write the GPIO register.
 * This is either the ChipCommon, or the PCI core.
 */
static inline int switch_to_gpio_core(struct bcm430x_private *bcm)
{
	int err;

	/* Where to find the GPIO register depends on the chipset.
	 * If it has a ChipCommon, its register at offset 0x6c is the GPIO
	 * control register. Otherwise the register at offset 0x6c in the
	 * PCI core is the GPIO control register.
	 */
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err == -ENODEV) {
		err = bcm430x_switch_core(bcm, &bcm->core_pci);
		if (err == -ENODEV) {
			printk(KERN_ERR PFX "gpio error: "
			       "Neither ChipCommon nor PCI core available!\n");
			return -ENODEV;
		} else if (err != 0)
			return -ENODEV;
	} else if (err != 0)
		return -ENODEV;

	return 0;
}

/* Initialize the GPIOs
 * http://bcm-specs.sipsolutions.net/GPIO
 */
static int bcm430x_gpio_init(struct bcm430x_private *bcm)
{
	struct bcm430x_coreinfo *old_core;
	int err;
	u32 mask, value;

	value = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	value &= ~0xc000;
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, value);

	mask = 0x0000001F;
	value = 0x0000000F;
	bcm430x_write16(bcm, BCM430x_MMIO_GPIO_CONTROL,
			bcm430x_read16(bcm, BCM430x_MMIO_GPIO_CONTROL) & 0xFFF0);
	bcm430x_write16(bcm, BCM430x_MMIO_GPIO_MASK,
			bcm430x_read16(bcm, BCM430x_MMIO_GPIO_MASK) | 0x000F);

	old_core = bcm->current_core;
	
	err = switch_to_gpio_core(bcm);
	if (err)
		return err;

	if (bcm->current_core->rev >= 2){
		mask  |= 0x10;
		value |= 0x10;
	}
	if (bcm->chip_id == 0x4301) {
		mask  |= 0x60;
		value |= 0x60;
	}
	if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL) {
		mask  |= 0x200;
		value |= 0x200;
	}

	bcm430x_write32(bcm, BCM430x_GPIO_CONTROL,
	                (bcm430x_read32(bcm, BCM430x_GPIO_CONTROL) & mask) | value);

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

	return 0;
}

/* Turn off all GPIO stuff. Call this on module unload, for example. */
static int bcm430x_gpio_cleanup(struct bcm430x_private *bcm)
{
	struct bcm430x_coreinfo *old_core;
	int err;

	old_core = bcm->current_core;
	err = switch_to_gpio_core(bcm);
	if (err)
		return err;
	bcm430x_write32(bcm, BCM430x_GPIO_CONTROL, 0x00000000);
	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

	return 0;
}

/* http://bcm-specs.sipsolutions.net/EnableMac */
static void bcm430x_mac_enable(struct bcm430x_private *bcm)
{
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
	                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
			| BCM430x_SBF_MAC_ENABLED);
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_REASON, BCM430x_IRQ_READY);
	// dummy reads
	bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);
	//FIXME: FuncPlaceholder (Set PS CTRL)
}

/* http://bcm-specs.sipsolutions.net/SuspendMAC */
static void bcm430x_mac_suspend(struct bcm430x_private *bcm)
{
	int i = 1000;
	//FIXME: FuncPlaceholder (Set PS CTRL)
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
	                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
			& ~BCM430x_SBF_MAC_ENABLED);
	bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON); /* dummy read */
	for ( ; i > 0; i--) {
		if (bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON) & BCM430x_IRQ_READY)
			i = -1;
		udelay(10);
	}
	if (!i)
		printk(KERN_ERR PFX "Failed to suspend mac!\n");
}

static void bcm430x_short_preamble_enable(struct bcm430x_private *bcm)
{
	if (bcm->current_core->phy->type != BCM430x_PHYTYPE_G)
		return;
	bcm430x_write16(bcm, 0x684, 0x0207);
	bcm430x_shm_write16(bcm, BCM430x_SHM_SHARED, 0x0010, 9);
}

static void bcm430x_short_preamble_disable(struct bcm430x_private *bcm)
{
	if (bcm->current_core->phy->type != BCM430x_PHYTYPE_G)
		return;
	bcm430x_write16(bcm, 0x684, 0x0212);
	bcm430x_shm_write16(bcm, BCM430x_SHM_SHARED, 0x0010, 20);
}

/* This is the opposite of bcm430x_chip_init() */
static void bcm430x_chip_cleanup(struct bcm430x_private *bcm)
{
	bcm430x_radio_turn_off(bcm);
	bcm430x_gpio_cleanup(bcm);
	free_irq(bcm->pci_dev->irq, bcm);
}

/* Initialize the chip
 * http://bcm-specs.sipsolutions.net/ChipInit
 */
static int bcm430x_chip_init(struct bcm430x_private *bcm)
{
	int err;
	int iw_mode = bcm->ieee->iw_mode;
	u32 value32;
	u16 value16;

	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
			BCM430x_SBF_CORE_READY
			| BCM430x_SBF_400);

	err = bcm430x_upload_microcode(bcm);
	if (err)
		goto out;

	err = bcm430x_initialize_irq(bcm);
	if (err)
		goto out;

	err = bcm430x_gpio_init(bcm);
	if (err)
		goto err_free_irq;

	err = bcm430x_upload_initvals(bcm);
	if (err)
		goto err_gpio_cleanup;

	err = bcm430x_radio_turn_on(bcm);
	if (err)
		goto err_gpio_cleanup;

	bcm430x_update_leds(bcm);

	bcm430x_write16(bcm, 0x03E6, 0x0000);
	err = bcm430x_phy_init(bcm);
	if (err)
		goto err_radio_off;

	//FIXME: Calling with NONE for the time being. Let the user set it (later?)
	bcm430x_radio_set_interference_mitigation(bcm, BCM430x_RADIO_INTERFMODE_NONE);
	bcm430x_phy_set_antenna_diversity(bcm);
	bcm430x_radio_set_txantenna(bcm, BCM430x_RADIO_DEFAULT_ANTENNA);
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B) {
		value16 = bcm430x_read16(bcm, 0x005E);
		value16 |= 0x0004;
		bcm430x_write16(bcm, 0x005E, value16);
	}
	bcm430x_write32(bcm, 0x0100, 0x01000000);
	bcm430x_write32(bcm, 0x010C, 0x01000000);

	value32 = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	value32 &= ~ BCM430x_SBF_MODE_NOTADHOC;
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, value32);
	value32 = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	value32 |= BCM430x_SBF_MODE_NOTADHOC;
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, value32);

	if ((iw_mode == IW_MODE_MASTER) && (bcm->net_dev->flags & IFF_PROMISC)) {
		value32 = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
		value32 |= BCM430x_SBF_MODE_PROMISC;
		bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, value32);
	} else if (iw_mode == IW_MODE_MONITOR) {
		value32 = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
		value32 |= BCM430x_SBF_MODE_PROMISC;
		value32 |= BCM430x_SBF_MODE_MONITOR;
		bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, value32);
	}
	value32 = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	value32 |= 0x100000; //FIXME: What's this? Is this correct?
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, value32);

	if (bcm->data_xfer_mode == BCM430x_DATAXFER_PIO) {
		//FIXME: write16 correct?
		bcm430x_write16(bcm, 0x0210, 0x0100);
		bcm430x_write16(bcm, 0x0230, 0x0100);
		bcm430x_write16(bcm, 0x0250, 0x0100);
		bcm430x_write16(bcm, 0x0270, 0x0100);
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0034, 0x00000000);
	}

	/* Probe Response Timeout value */
	/* FIXME: Default to 0, has to be set by ioctl probably... :-/ */
	bcm430x_shm_write16(bcm, BCM430x_SHM_SHARED, 0x0074, 0x0000);

	if (iw_mode != IW_MODE_ADHOC && iw_mode != IW_MODE_MASTER) {
		if ((bcm->chip_id == 0x4306) && (bcm->chip_rev == 3))
			bcm430x_write16(bcm, 0x0608, 0x0064);
		else
			bcm430x_write16(bcm, 0x0608, 0x0032);
	} else
		bcm430x_write16(bcm, 0x0608, 0x0002);

	if (short_preamble)
		bcm430x_short_preamble_enable(bcm);
	else
		bcm430x_short_preamble_disable(bcm);

	if (bcm->current_core->rev < 3) {
		bcm430x_write16(bcm, 0x060E, 0x0000);
		bcm430x_write16(bcm, 0x0610, 0x8000);
		bcm430x_write16(bcm, 0x0604, 0x0000);
		bcm430x_write16(bcm, 0x0606, 0x0200);
	} else {
		bcm430x_write32(bcm, 0x0188, 0x80000000);
		bcm430x_write32(bcm, 0x018C, 0x02000000);
	}
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_REASON, 0x00004000);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA1_IRQ_MASK, 0x0001DC00);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA2_IRQ_MASK, 0x0000DC00);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA3_IRQ_MASK, 0x0000DC00);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA4_IRQ_MASK, 0x0001DC00);

	value32 = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);
	value32 |= 0x00100000;
	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, value32);

	bcm430x_write16(bcm, BCM430x_MMIO_POWERUP_DELAY, bcm430x_pctl_powerup_delay(bcm));

	assert(err == 0);
	dprintk(KERN_INFO PFX "Chip initialized\n");
out:
	return err;

err_radio_off:
	bcm430x_radio_turn_off(bcm);
err_gpio_cleanup:
	bcm430x_gpio_cleanup(bcm);
err_free_irq:
	free_irq(bcm->pci_dev->irq, bcm);
	goto out;
}
	
/* Validate chip access
 * http://bcm-specs.sipsolutions.net/ValidateChipAccess */
static int bcm430x_validate_chip(struct bcm430x_private *bcm)
{
	int err = -ENODEV;
	u32 value;
	u32 shm_backup;

	shm_backup = bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED, 0x0000);
	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0000, 0xAA5555AA);
	if (bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED, 0x0000) != 0xAA5555AA) {
		printk(KERN_ERR PFX "Error: SHM mismatch (1) validating chip\n");
		goto out;
	}

	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0000, 0x55AAAA55);
	if (bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED, 0x0000) != 0x55AAAA55) {
		printk(KERN_ERR PFX "Error: SHM mismatch (2) validating chip\n");
		goto out;
	}

	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0000, shm_backup);

	value = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	if ((value | 0x80000000) != 0x80000400) {
		printk(KERN_ERR PFX "Error: Bad Status Bitfield while validating chip\n");
		goto out;
	}

	value = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);
	if (value != 0x00000000) {
		printk(KERN_ERR PFX "Error: Bad interrupt reason code while validating chip\n");
		goto out;
	}

	err = 0;
out:
	return err;
}

static int bcm430x_probe_cores(struct bcm430x_private *bcm)
{
	int err, i;
	int current_core;
	u32 core_vendor, core_id, core_rev;
	u32 sb_id_hi, chip_id_32 = 0;
	u16 pci_device, chip_id_16;
	u8 core_count;

	/* map core 0 */
	err = _switch_core(bcm, 0);
	if (err)
		goto out;

	/* fetch sb_id_hi from core information registers */
	sb_id_hi = bcm430x_read32(bcm, BCM430x_CIR_SB_ID_HI);

	core_id = (sb_id_hi & 0xFFF0) >> 4;
	core_rev = (sb_id_hi & 0xF);
	core_vendor = (sb_id_hi & 0xFFFF0000) >> 16;


	/* if present, chipcommon is always core 0; read the chipid from it */
	if (core_id == BCM430x_COREID_CHIPCOMMON) {
		chip_id_32 = bcm430x_read32(bcm, 0);
		chip_id_16 = chip_id_32 & 0xFFFF;
		bcm->core_chipcommon.flags |= BCM430x_COREFLAG_AVAILABLE;
		bcm->core_chipcommon.id = core_id;
		bcm->core_chipcommon.rev = core_rev;
		bcm->core_chipcommon.index = 0;
	} else {
		/* without a chipCommon, use a hard coded table. */
		pci_device = bcm->pci_dev->device;
		if (pci_device == 0x4301)
			chip_id_16 = 0x4301;
		else if ((pci_device >= 0x4305) && (pci_device <= 0x4307))
			chip_id_16 = 0x4307;
		else if ((pci_device >= 0x4402) && (pci_device <= 0x4403))
			chip_id_16 = 0x4402;
		else if ((pci_device >= 0x4610) && (pci_device <= 0x4615))
			chip_id_16 = 0x4610;
		else if ((pci_device >= 0x4710) && (pci_device <= 0x4715))
			chip_id_16 = 0x4710;
		else {
			/* Presumably devices not listed above are not
			 * put into the pci device table for this driver,
			 * so we should never get here */
			assert(0);
			chip_id_16 = 0x2BAD;
		}
	}

	/* ChipCommon with Core Rev >=4 encodes number of cores,
	 * otherwise consult hardcoded table */
	if ((core_id == BCM430x_COREID_CHIPCOMMON) && (core_rev >= 4)) {
		core_count = (chip_id_32 & 0x0F000000) >> 24;
	} else {
		switch (chip_id_16) {
			case 0x4610:
			case 0x4704:
			case 0x4710:
				core_count = 9;
				break;
			case 0x4310:
				core_count = 8;
				break;
			case 0x5365:
				core_count = 7;
				break;
			case 0x4306:
				core_count = 6;
				break;
			case 0x4301:
			case 0x4307:
				core_count = 5;
				break;
			case 0x4402:
				core_count = 3;
				break;
			default:
				/* SOL if we get here */
				assert(0);
				core_count = 1;
		}
	}

	bcm->chip_id = chip_id_16;
	bcm->chip_rev = (chip_id_32 & 0x000f0000) >> 16;

	dprintk(KERN_INFO PFX "Chip ID 0x%x, rev 0x%x\n",
		bcm->chip_id, bcm->chip_rev);
	dprintk(KERN_INFO PFX "Number of cores: %d\n", core_count);
	if (bcm->core_chipcommon.flags & BCM430x_COREFLAG_AVAILABLE) {
		dprintk(KERN_INFO PFX "Core 0: ID 0x%x, rev 0x%x, vendor 0x%x, %s\n",
			core_id, core_rev, core_vendor,
			bcm430x_core_enabled(bcm) ? "enabled" : "disabled");
	}

	if (bcm->core_chipcommon.flags & BCM430x_COREFLAG_AVAILABLE)
		current_core = 1;
	else
		current_core = 0;
	for ( ; current_core < core_count; current_core++) {
		struct bcm430x_coreinfo *core;

		err = _switch_core(bcm, current_core);
		if (err)
			goto out;
		/* Gather information */
		/* fetch sb_id_hi from core information registers */
		sb_id_hi = bcm430x_read32(bcm, BCM430x_CIR_SB_ID_HI);

		/* extract core_id, core_rev, core_vendor */
		core_id = (sb_id_hi & 0xFFF0) >> 4;
		core_rev = (sb_id_hi & 0xF);
		core_vendor = (sb_id_hi & 0xFFFF0000) >> 16;

		dprintk(KERN_INFO PFX "Core %d: ID 0x%x, rev 0x%x, vendor 0x%x, %s\n",
			current_core, core_id, core_rev, core_vendor,
			bcm430x_core_enabled(bcm) ? "enabled" : "disabled" );

		core = 0;
		switch (core_id) {
		case BCM430x_COREID_PCI:
			core = &bcm->core_pci;
			if (core->flags & BCM430x_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple PCI cores found.\n");
				continue;
			}
			memset(core, 0, sizeof(*core));
			break;
		case BCM430x_COREID_V90:
			core = &bcm->core_v90;
			if (core->flags & BCM430x_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple V90 cores found.\n");
				continue;
			}
			memset(core, 0, sizeof(*core));
			break;
		case BCM430x_COREID_PCMCIA:
			core = &bcm->core_pcmcia;
			if (core->flags & BCM430x_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple PCMCIA cores found.\n");
				continue;
			}
			memset(core, 0, sizeof(*core));
			break;
		case BCM430x_COREID_ETHERNET:
			core = &bcm->core_ethernet;
			if (core->flags & BCM430x_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple Ethernet cores found.\n");
				continue;
			}
			memset(core, 0, sizeof(*core));
			break;
		case BCM430x_COREID_80211:
			for (i = 0; i < BCM430x_MAX_80211_CORES; i++) {
				core = &(bcm->core_80211[i]);
				if (!(core->flags & BCM430x_COREFLAG_AVAILABLE))
					break;
				core = 0;
			}
			if (!core) {
				printk(KERN_WARNING PFX "More than %d cores of type 802.11 found.\n",
				       BCM430x_MAX_80211_CORES);
				continue;
			}
			if (i != 0) {
				/* More than one 80211 core is only supported
				 * by special chips.
				 * There are chips with two 80211 cores, but with
				 * dangling pins on the second core. Be careful
				 * and ignore these cores here.
				 */
				if (bcm->pci_dev->device != 0x4324) {
					dprintk(KERN_INFO PFX "Ignoring additional 802.11 core.\n");
					continue;
				}
			}
			switch (core_rev) {
			case 2:
			case 4:
			case 5:
			case 6:
			case 9:
				break;
			default:
				printk(KERN_ERR PFX "Error: Unsupported 80211 core revision %u\n",
				       core_rev);
				err = -ENODEV;
				goto out;
			}
			memset(core, 0, sizeof(*core));
			core->phy = &bcm->phy[i];
			core->phy->antenna_diversity = 0xffff;
			core->phy->savedpctlreg = 0xFFFF;
			core->phy->minlowsig[0] = 0xFFFF;
			core->phy->minlowsig[1] = 0xFFFF;
			core->phy->minlowsigpos[0] = 0;
			core->phy->minlowsigpos[1] = 0;
			core->radio = &bcm->radio[i];
			core->radio->channel = 0xffff;
			core->radio->lofcal = 0xFFFF;
			core->radio->initval = 0xFFFF;
			core->radio->nrssi[0] = -1000;
			core->radio->nrssi[1] = -1000;
			core->dma = &bcm->dma[i];
			core->pio = &bcm->pio[i];
			break;
		case BCM430x_COREID_CHIPCOMMON:
			printk(KERN_WARNING PFX "Multiple CHIPCOMMON cores found.\n");
			break;
		default:
			printk(KERN_WARNING PFX "Unknown core found (ID 0x%x)\n", core_id);
		}
		if (core) {
			core->flags |= BCM430x_COREFLAG_AVAILABLE;
			core->id = core_id;
			core->rev = core_rev;
			core->index = current_core;
		}
	}

	if (!(bcm->core_80211[0].flags & BCM430x_COREFLAG_AVAILABLE)) {
		printk(KERN_ERR PFX "Error: No 80211 core found!\n");
		err = -ENODEV;
		goto out;
	}

	err = bcm430x_switch_core(bcm, &bcm->core_80211[0]);

	assert(err == 0);
out:
	return err;
}

static void bcm430x_pio_free(struct bcm430x_private *bcm)
{
	bcm430x_destroy_pioqueue(bcm->current_core->pio->queue3);
	bcm->current_core->pio->queue3 = 0;
	bcm430x_destroy_pioqueue(bcm->current_core->pio->queue2);
	bcm->current_core->pio->queue2 = 0;
	bcm430x_destroy_pioqueue(bcm->current_core->pio->queue1);
	bcm->current_core->pio->queue1 = 0;
	bcm430x_destroy_pioqueue(bcm->current_core->pio->queue0);
	bcm->current_core->pio->queue0 = 0;
}

static int bcm430x_pio_init(struct bcm430x_private *bcm)
{
	struct bcm430x_pioqueue *queue;
	int err = -ENOMEM;

	queue = bcm430x_setup_pioqueue(bcm, BCM430x_MMIO_PIO1_BASE);
	if (!queue)
		goto out;
	bcm->current_core->pio->queue0 = queue;

	queue = bcm430x_setup_pioqueue(bcm, BCM430x_MMIO_PIO2_BASE);
	if (!queue)
		goto err_destroy0;
	bcm->current_core->pio->queue1 = queue;

	queue = bcm430x_setup_pioqueue(bcm, BCM430x_MMIO_PIO3_BASE);
	if (!queue)
		goto err_destroy1;
	bcm->current_core->pio->queue2 = queue;

	queue = bcm430x_setup_pioqueue(bcm, BCM430x_MMIO_PIO4_BASE);
	if (!queue)
		goto err_destroy2;
	bcm->current_core->pio->queue3 = queue;

	if (bcm->current_core->rev < 3)
		bcm->irq_savedstate |= BCM430x_IRQ_PIO_INIT;

	dprintk(KERN_INFO PFX "PIO initialized\n");
	err = 0;
out:
	return err;

err_destroy2:
	bcm430x_destroy_pioqueue(bcm->current_core->pio->queue2);
	bcm->current_core->pio->queue2 = 0;
err_destroy1:
	bcm430x_destroy_pioqueue(bcm->current_core->pio->queue1);
	bcm->current_core->pio->queue1 = 0;
err_destroy0:
	bcm430x_destroy_pioqueue(bcm->current_core->pio->queue0);
	bcm->current_core->pio->queue0 = 0;
	goto out;
}

static void bcm430x_dma_free(struct bcm430x_private *bcm)
{
	bcm430x_destroy_dmaring(bcm->current_core->dma->rx_ring1);
	bcm->current_core->dma->rx_ring1 = 0;
	bcm430x_destroy_dmaring(bcm->current_core->dma->rx_ring0);
	bcm->current_core->dma->rx_ring0 = 0;
	bcm430x_destroy_dmaring(bcm->current_core->dma->tx_ring3);
	bcm->current_core->dma->tx_ring3 = 0;
	bcm430x_destroy_dmaring(bcm->current_core->dma->tx_ring2);
	bcm->current_core->dma->tx_ring2 = 0;
	bcm430x_destroy_dmaring(bcm->current_core->dma->tx_ring1);
	bcm->current_core->dma->tx_ring1 = 0;
	bcm430x_destroy_dmaring(bcm->current_core->dma->tx_ring0);
	bcm->current_core->dma->tx_ring0 = 0;
}

static int bcm430x_dma_init(struct bcm430x_private *bcm)
{
	struct bcm430x_dmaring *ring;
	int err = -ENOMEM;

	/* setup TX DMA channels. */
	ring = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA1_BASE,
				     BCM430x_TXRING_SLOTS, 1);
	if (!ring)
		goto out;
	bcm->current_core->dma->tx_ring0 = ring;

	ring = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA2_BASE,
				     BCM430x_TXRING_SLOTS, 1);
	if (!ring)
		goto err_destroy_tx0;
	bcm->current_core->dma->tx_ring1 = ring;

	ring = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA3_BASE,
				     BCM430x_TXRING_SLOTS, 1);
	if (!ring)
		goto err_destroy_tx1;
	bcm->current_core->dma->tx_ring2 = ring;

	ring = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA4_BASE,
				     BCM430x_TXRING_SLOTS, 1);
	if (!ring)
		goto err_destroy_tx2;
	bcm->current_core->dma->tx_ring3 = ring;

	/* setup RX DMA channels. */
	ring = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA1_BASE,
				     BCM430x_RXRING_SLOTS, 0);
	if (!ring)
		goto err_destroy_tx3;
	bcm->current_core->dma->rx_ring0 = ring;

	if (bcm->current_core->rev < 5) {
		ring = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA4_BASE,
					     BCM430x_RXRING_SLOTS, 0);
		if (!ring)
			goto err_destroy_rx0;
		bcm->current_core->dma->rx_ring1 = ring;
	}

	dprintk(KERN_INFO PFX "DMA initialized\n");
	err = 0;
out:
	return err;

err_destroy_rx0:
	bcm430x_destroy_dmaring(bcm->current_core->dma->rx_ring0);
	bcm->current_core->dma->rx_ring0 = 0;
err_destroy_tx3:
	bcm430x_destroy_dmaring(bcm->current_core->dma->tx_ring3);
	bcm->current_core->dma->tx_ring3 = 0;
err_destroy_tx2:
	bcm430x_destroy_dmaring(bcm->current_core->dma->tx_ring2);
	bcm->current_core->dma->tx_ring2 = 0;
err_destroy_tx1:
	bcm430x_destroy_dmaring(bcm->current_core->dma->tx_ring1);
	bcm->current_core->dma->tx_ring1 = 0;
err_destroy_tx0:
	bcm430x_destroy_dmaring(bcm->current_core->dma->tx_ring0);
	bcm->current_core->dma->tx_ring0 = 0;
	goto out;
}

static void bcm430x_gen_bssid(struct bcm430x_private *bcm, 
			      unsigned char *bssid,
			      unsigned char *mac)
{
	switch (bcm->ieee->iw_mode) {
	case IW_MODE_MASTER:
	case IW_MODE_ADHOC:
	case IW_MODE_INFRA:
	case IW_MODE_REPEAT:
	case IW_MODE_SECOND:
	case IW_MODE_MONITOR:
		/*FIXME: For now we always return the mac address.
		 *       I hope this is ok. Wikipedia states something about
		 *       randomizing the bssid in non-MASTER mode... .
		 *       I did not find something about this in the IEEE specs, yet.
		 */
		memcpy(bssid, mac, 6);
		break;
	default:
		assert(0);
	}
}

static void bcm430x_write_mac_bssid_templates(struct bcm430x_private *bcm)
{
	struct net_device *net_dev = bcm->net_dev;
	unsigned char *mac = net_dev->dev_addr;
	unsigned char bssid[6];
	int mac_len = net_dev->addr_len;
	int i;

	assert(mac_len == 6);

	/* Write our MAC address to template ram */
	for (i = 0; i < mac_len; i += sizeof(u32))
		bcm430x_ram_write(bcm, 0x20 + i * sizeof(u32), *((u32 *)(mac + i)));

	/* Write the BSSID to template ram */
	bcm430x_gen_bssid(bcm, bssid, mac);
	for (i = 0; i < ARRAY_SIZE(bssid); i += sizeof(u32))
		bcm430x_ram_write(bcm, 0x26 + i * sizeof(u32), *((u32 *)(bssid + i)));
}

static void bcm430x_wireless_core_cleanup(struct bcm430x_private *bcm)
{
	bcm430x_chip_cleanup(bcm);
	bcm430x_pio_free(bcm);
	bcm430x_dma_free(bcm);

	bcm->current_core->flags &= ~ BCM430x_COREFLAG_INITIALIZED;
}

/* http://bcm-specs.sipsolutions.net/80211Init */
static int bcm430x_wireless_core_init(struct bcm430x_private *bcm)
{
	u32 ucodeflags;
	int err;
	u32 sbimconfiglow;

	if (bcm->chip_rev < 5) {
		sbimconfiglow = bcm430x_read32(bcm, BCM430x_CIR_SBIMCONFIGLOW);
		sbimconfiglow &= ~ BCM430x_SBIMCONFIGLOW_REQUEST_TOUT_MASK;
		sbimconfiglow &= ~ BCM430x_SBIMCONFIGLOW_SERVICE_TOUT_MASK;
		if (1/*FIXME: this is a PCI bus*/)
			sbimconfiglow |= 0x32;
		else if (0/*FIXME: this is a silicone backplane bus*/)
			sbimconfiglow |= 0x53;
		bcm430x_write32(bcm, BCM430x_CIR_SBIMCONFIGLOW, sbimconfiglow);
	}

	bcm430x_phy_calibrate(bcm);
	err = bcm430x_chip_init(bcm);
	if (err)
		goto out;

	bcm430x_shm_write16(bcm, BCM430x_SHM_SHARED, 0x0016, bcm->current_core->rev);
	ucodeflags = bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED, BCM430x_UCODEFLAGS_OFFSET);

	if (0 /*FIXME: which condition has to be used here? */)
		ucodeflags |= 0x00000010;

	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_G) {
		ucodeflags |= BCM430x_UCODEFLAG_UNKBGPHY;
		if (bcm->current_core->phy->rev == 1)
			ucodeflags |= BCM430x_UCODEFLAG_UNKGPHY;
		if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL)
			ucodeflags |= BCM430x_UCODEFLAG_UNKPACTRL;
	} else if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B) {
		ucodeflags |= BCM430x_UCODEFLAG_UNKBGPHY;
		if ((bcm->current_core->phy->rev >= 2)
		    && ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000))
			ucodeflags &= ~BCM430x_UCODEFLAG_UNKGPHY;
	}

	if (ucodeflags != bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED,
					     BCM430x_UCODEFLAGS_OFFSET)) {
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED,
				    BCM430x_UCODEFLAGS_OFFSET, ucodeflags);
	}

	/* FIXME: Short/Long Retry Limit: Using defaults as of http://bcm-specs.sipsolutions.net/SHM: 0x0002 */
	bcm430x_shm_write32(bcm, BCM430x_SHM_WIRELESS, 0x0006, 7);
	bcm430x_shm_write32(bcm, BCM430x_SHM_WIRELESS, 0x0007, 4);

	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0044, 3);
	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0046, 2);

	/* Minimum Contention Window */
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B)
		bcm430x_shm_write32(bcm, BCM430x_SHM_WIRELESS, 0x0003, 0x0000001f);
	else
		bcm430x_shm_write32(bcm, BCM430x_SHM_WIRELESS, 0x0003, 0x0000000f);
	/* Maximum Contention Window */
	bcm430x_shm_write32(bcm, BCM430x_SHM_WIRELESS, 0x0004, 0x000003ff);

	bcm430x_write_mac_bssid_templates(bcm);

	if (bcm->current_core->rev >= 5)
		bcm430x_write16(bcm, 0x043C, 0x000C);

	if (bcm->data_xfer_mode == BCM430x_DATAXFER_DMA) {
		err = bcm430x_dma_init(bcm);
		if (err)
			goto err_chip_cleanup;
	} else {
		assert(bcm->data_xfer_mode == BCM430x_DATAXFER_PIO);
		err = bcm430x_pio_init(bcm);
		if (err)
			goto err_chip_cleanup;
	}

	bcm430x_mac_enable(bcm);
	bcm430x_interrupt_enable(bcm, bcm->irq_savedstate);

	bcm->current_core->flags |= BCM430x_COREFLAG_INITIALIZED;
out:
	return err;

err_chip_cleanup:
	bcm430x_chip_cleanup(bcm);
	goto out;
}

/* Hard-reset the chip. Do not call this directly.
 * Use bcm430x_recover_from_fatal()
 */
static void bcm430x_chip_reset(void *_bcm)
{
	struct bcm430x_private *bcm = _bcm;
	int err;

	netif_tx_disable(bcm->net_dev);
	bcm430x_free_board(bcm);
	err = bcm430x_init_board(bcm);
	if (err) {
		printk(KERN_ERR PFX "Chip reset failed!\n");
		return;
	}
	netif_wake_queue(bcm->net_dev);
}

/* Call this function on _really_ fatal error conditions.
 * It will hard-reset the chip.
 * This can be called from interrupt or process context.
 * Make sure to _not_ re-enable device interrupts after this has been called.
 */
static void bcm430x_recover_from_fatal(struct bcm430x_private *bcm, int error)
{
	bcm430x_interrupt_disable(bcm, BCM430x_IRQ_ALL);
	printk(KERN_ERR PFX "FATAL ERROR (%d): Resetting the chip...\n", error);
	INIT_WORK(&bcm->fatal_work, bcm430x_chip_reset, bcm);
	queue_work(bcm->workqueue, &bcm->fatal_work);
}

static int bcm430x_chipset_attach(struct bcm430x_private *bcm)
{
	int err;
	u16 pci_status;

	err = bcm430x_pctl_set_crystal(bcm, 1);
	if (err)
		goto out;
	bcm430x_pci_read_config_16(bcm->pci_dev, PCI_STATUS, &pci_status);
	bcm430x_pci_write_config_16(bcm->pci_dev, PCI_STATUS, pci_status & ~PCI_STATUS_SIG_TARGET_ABORT);

out:
	return err;
}

static void bcm430x_chipset_detach(struct bcm430x_private *bcm)
{
	bcm430x_pctl_set_clock(bcm, BCM430x_PCTL_CLK_SLOW);
	bcm430x_pctl_set_crystal(bcm, 0);
}

static inline void bcm430x_pcicore_broadcast_value(struct bcm430x_private *bcm,
						   u32 address,
						   u32 data)
{
	bcm430x_write32(bcm, BCM430x_CIR_PCI_BCAST_ADDR, address);
	bcm430x_write32(bcm, BCM430x_CIR_PCI_BCAST_DATA, data);
}

static int bcm430x_pcicore_commit_settings(struct bcm430x_private *bcm)
{
	int err;
	struct bcm430x_coreinfo *old_core;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_pci);
	if (err)
		goto out;

	bcm430x_pcicore_broadcast_value(bcm, 0xfd8, 0x00000000);

	bcm430x_switch_core(bcm, old_core);
	assert(err == 0);
out:
	return err;
}

/* Make an I/O Core usable. "core_mask" is the bitmask of the cores to enable.
 * To enable core 0, pass a core_mask of 1<<0
 */
static int bcm430x_setup_backplane_pci_connection(struct bcm430x_private *bcm,
						  u32 core_mask)
{
	u32 backplane_flag_nr;
	u32 value;
	struct bcm430x_coreinfo *old_core;
	int err;

	value = bcm430x_read32(bcm, BCM430x_CIR_SBTPSFLAG);
	backplane_flag_nr = value & BCM430x_BACKPLANE_FLAG_NR_MASK;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_pci);
	if (err)
		goto out;

	if (bcm->core_pci.rev < 6) {
		value = bcm430x_read32(bcm, BCM430x_CIR_SBINTVEC);
		value |= (1 << backplane_flag_nr);
		bcm430x_write32(bcm, BCM430x_CIR_SBINTVEC, value);
	} else {
		err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCICFG_ICR, &value);
		if (err) {
			printk(KERN_ERR PFX "Error: ICR setup failure!\n");
			goto out_switch_back;
		}
		value |= core_mask << 8;
		err = bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCICFG_ICR, value);
		if (err) {
			printk(KERN_ERR PFX "Error: ICR setup failure!\n");
			goto out_switch_back;
		}
	}

	value = bcm430x_read32(bcm, BCM430x_CIR_PCI_SBTOPCI2);
	value |= BCM430x_SBTOPCI2_PREFETCH | BCM430x_SBTOPCI2_BURST;
	bcm430x_write32(bcm, BCM430x_CIR_PCI_SBTOPCI2, value);

	if (bcm->core_pci.rev < 5) {
		value = bcm430x_read32(bcm, BCM430x_CIR_SBIMCONFIGLOW);
		value |= (2 << BCM430x_SBIMCONFIGLOW_SERVICE_TOUT_SHIFT)
			 & BCM430x_SBIMCONFIGLOW_SERVICE_TOUT_MASK;
		value |= (3 << BCM430x_SBIMCONFIGLOW_REQUEST_TOUT_SHIFT)
			 & BCM430x_SBIMCONFIGLOW_REQUEST_TOUT_MASK;
		bcm430x_write32(bcm, BCM430x_CIR_SBIMCONFIGLOW, value);
		err = bcm430x_pcicore_commit_settings(bcm);
		assert(err == 0);
	}

out_switch_back:
	err = bcm430x_switch_core(bcm, old_core);
out:
	return err;
}

static void bcm430x_periodic_work0_handler(void *d)
{
	struct bcm430x_private *bcm = d;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);

	bcm430x_phy_xmitpower(bcm);
	//TODO for APHY (temperature?)

	if (likely(!(bcm->status & BCM430x_STAT_DEVSHUTDOWN))) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work0,
				   BCM430x_PERIODIC_0_DELAY);
	}
	spin_unlock_irqrestore(&bcm->lock, flags);
}

static void bcm430x_periodic_work1_handler(void *d)
{
	struct bcm430x_private *bcm = d;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);

	bcm430x_mac_suspend(bcm);
	bcm430x_calc_nrssi_slope(bcm);
	bcm430x_mac_enable(bcm);

	if (likely(!(bcm->status & BCM430x_STAT_DEVSHUTDOWN))) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work1,
				   BCM430x_PERIODIC_1_DELAY);
	}
	spin_unlock_irqrestore(&bcm->lock, flags);
}

static void bcm430x_periodic_work2_handler(void *d)
{
	struct bcm430x_private *bcm = d;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);

	bcm430x_mac_suspend(bcm);
	bcm430x_phy_measurelowsig(bcm);
	bcm430x_mac_enable(bcm);

	if (likely(!(bcm->status & BCM430x_STAT_DEVSHUTDOWN))) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work2,
				   BCM430x_PERIODIC_2_DELAY);
	}
	spin_unlock_irqrestore(&bcm->lock, flags);
}

/* Delete all periodic tasks and make
 * sure they are not running any longer
 */
static void bcm430x_periodic_tasks_delete(struct bcm430x_private *bcm)
{
	cancel_delayed_work(&bcm->periodic_work0);
	cancel_delayed_work(&bcm->periodic_work1);
	cancel_delayed_work(&bcm->periodic_work2);
	flush_workqueue(bcm->workqueue);
}

/* Setup all periodic tasks. */
static void bcm430x_periodic_tasks_setup(struct bcm430x_private *bcm)
{
	INIT_WORK(&bcm->periodic_work0, bcm430x_periodic_work0_handler, bcm);
	INIT_WORK(&bcm->periodic_work1, bcm430x_periodic_work1_handler, bcm);
	INIT_WORK(&bcm->periodic_work2, bcm430x_periodic_work2_handler, bcm);

	/* Periodic task 0: Delay ~15sec */
	queue_delayed_work(bcm->workqueue, &bcm->periodic_work0,
			   BCM430x_PERIODIC_0_DELAY);

	/* Periodic task 1: Delay ~60sec */
	if (bcm->sprom.boardflags & BCM430x_BFL_RSSI) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work1,
				   BCM430x_PERIODIC_1_DELAY);
	}

	/* Periodic task 2: Delay ~120sec */
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_G &&
	    bcm->current_core->phy->rev >= 2) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work2,
				   BCM430x_PERIODIC_2_DELAY);
	}
}

/* This is the opposite of bcm430x_init_board() */
static void bcm430x_free_board(struct bcm430x_private *bcm)
{
	int i, err;

	bcm->status &= ~BCM430x_STAT_BOARDINITDONE;
	bcm->status |= BCM430x_STAT_DEVSHUTDOWN;
	mb();

	bcm430x_periodic_tasks_delete(bcm);

	for (i = 0; i < BCM430x_MAX_80211_CORES; i++) {
		if (!(bcm->core_80211[i].flags & BCM430x_COREFLAG_AVAILABLE))
			continue;
		assert(bcm->core_80211[i].flags & BCM430x_COREFLAG_INITIALIZED);

		err = bcm430x_switch_core(bcm, &bcm->core_80211[i]);
		assert(err == 0);
		bcm430x_wireless_core_cleanup(bcm);
	}

	bcm430x_pctl_set_crystal(bcm, 0);
}

static int bcm430x_init_board(struct bcm430x_private *bcm)
{
	int i, err;
	int num_80211_cores;
	int connect_phy;

	bcm->status &= ~ BCM430x_STAT_DEVSHUTDOWN;
	mb();

	err = bcm430x_pctl_set_crystal(bcm, 1);
	if (err)
		goto out;
	err = bcm430x_pctl_init(bcm);
	if (err)
		goto err_crystal_off;
	err = bcm430x_pctl_set_clock(bcm, BCM430x_PCTL_CLK_FAST);
	if (err)
		goto err_crystal_off;

	num_80211_cores = bcm430x_num_80211_cores(bcm);
	for (i = 0; i < num_80211_cores; i++) {
		err = bcm430x_switch_core(bcm, &bcm->core_80211[i]);
		assert(err != -ENODEV);
		if (err)
			goto err_80211_unwind;

		/* Enable the selected wireless core.
		 * Connect PHY only on the first core.
		 */
		if (!bcm430x_core_enabled(bcm)) {
			if (num_80211_cores == 1) {
				connect_phy = bcm->current_core->phy->connected;
			} else {
				if (i == 0)
					connect_phy = 1;
				else
					connect_phy = 0;
			}
			bcm430x_wireless_core_reset(bcm, connect_phy);
		}

		if (i != 0)
			bcm430x_wireless_core_mark_inactive(bcm, &bcm->core_80211[0]);

		err = bcm430x_wireless_core_init(bcm);
		if (err)
			goto err_80211_unwind;

		if (i != 0) {
			bcm430x_mac_suspend(bcm);
			bcm430x_interrupt_disable(bcm, BCM430x_IRQ_ALL);
			bcm430x_mac_suspend(bcm);
		}
	}
	if (num_80211_cores >= 2) {
		bcm430x_switch_core(bcm, &bcm->core_80211[0]);
		bcm430x_mac_enable(bcm);
	}
	dprintk(KERN_INFO PFX "80211 cores initialized\n");
	//TODO: Set up LEDs
	//TODO: Initialize PIO

	bcm430x_pctl_set_clock(bcm, BCM430x_PCTL_CLK_DYNAMIC);

	bcm430x_periodic_tasks_setup(bcm);

	mb();
	bcm->status |= BCM430x_STAT_BOARDINITDONE;
	assert(err == 0);
out:
	return err;

err_80211_unwind:
	/* unwind all 80211 initialization */
	for (i = 0; i < num_80211_cores; i++) {
		if (!(bcm->core_80211[i].flags & BCM430x_COREFLAG_INITIALIZED))
			continue;
		bcm430x_interrupt_disable(bcm, BCM430x_IRQ_ALL);
		bcm430x_wireless_core_cleanup(bcm);
	}
err_crystal_off:
	bcm430x_pctl_set_crystal(bcm, 0);
	goto out;
}

static void bcm430x_detach_board(struct bcm430x_private *bcm)
{
	struct pci_dev *pci_dev = bcm->pci_dev;
	int i;

	bcm430x_chipset_detach(bcm);
	/* Do _not_ access the chip, after it is detached. */
	iounmap(bcm->mmio_addr);

	pci_release_regions(pci_dev);
	pci_disable_device(pci_dev);

	/* Free allocated structures/fields */
	for (i = 0; i < BCM430x_MAX_80211_CORES; i++) {
		kfree(bcm->phy[i].desired_power);
		kfree(bcm->phy[i].lo_pairs);
	}
}	

static int bcm430x_read_phyinfo(struct bcm430x_private *bcm)
{
	u16 value;
	u8 phy_version;
	u8 phy_type;
	u8 phy_rev;
	int phy_rev_ok = 1;

	value = bcm430x_read16(bcm, BCM430x_MMIO_PHY_VER);

	phy_version = (value & 0xF000) >> 12;
	phy_type = (value & 0x0F00) >> 8;
	phy_rev = (value & 0x000F);

	dprintk(KERN_INFO PFX "Detected PHY: Version: %x, Type %x, Revision %x\n",
		phy_version, phy_type, phy_rev);

	switch (phy_type) {
	case BCM430x_PHYTYPE_A:
		if (phy_rev >= 4)
			phy_rev_ok = 0;
		bcm->current_core->phy->default_bitrate = IEEE80211_OFDM_RATE_6MB;
		/*FIXME: We need to switch the ieee->modulation, etc.. flags,
		 *       if we switch 80211 cores after init is done.
		 *       As we do not implement on the fly switching between
		 *       wireless cores, I will leave this as a future task.
		 */
		bcm->ieee->modulation = IEEE80211_OFDM_MODULATION;
		bcm->ieee->mode = IEEE_A;
		bcm->ieee->freq_band = IEEE80211_52GHZ_BAND;
		break;
	case BCM430x_PHYTYPE_B:
		if (phy_rev != 2 && phy_rev != 4 && phy_rev != 6 && phy_rev != 7)
			phy_rev_ok = 0;
		bcm->current_core->phy->default_bitrate = IEEE80211_CCK_RATE_1MB;
		bcm->ieee->modulation = IEEE80211_CCK_MODULATION;
		bcm->ieee->mode = IEEE_B;
		bcm->ieee->freq_band = IEEE80211_24GHZ_BAND;
		break;
	case BCM430x_PHYTYPE_G:
		if (phy_rev >= 3)
			phy_rev_ok = 0;
		bcm->current_core->phy->default_bitrate = IEEE80211_OFDM_RATE_6MB;
		bcm->ieee->modulation = IEEE80211_OFDM_MODULATION;
		bcm->ieee->mode = IEEE_G;
		bcm->ieee->freq_band = IEEE80211_24GHZ_BAND;
		break;
	default:
		printk(KERN_ERR PFX "Error: Unknown PHY Type %x\n",
		       phy_type);
		return -ENODEV;
	};
	if (!phy_rev_ok) {
		printk(KERN_WARNING PFX "Invalid PHY Revision %x\n",
		       phy_rev);
	}

	bcm->current_core->phy->version = phy_version;
	bcm->current_core->phy->type = phy_type;
	bcm->current_core->phy->rev = phy_rev;
	if ((phy_type == BCM430x_PHYTYPE_B) || (phy_type == BCM430x_PHYTYPE_G)) {
		bcm->current_core->phy->desired_power = kmalloc(sizeof(s8) * BCM430x_LO_COUNT, GFP_KERNEL);
		if (!bcm->current_core->phy->desired_power)
			return -ENOMEM;
		memset(bcm->current_core->phy->desired_power, -1, sizeof(s8) * BCM430x_LO_COUNT);
		bcm->current_core->phy->lo_pairs = kmalloc(sizeof(union bcm430x_lopair) * BCM430x_LO_COUNT,
		                                          GFP_KERNEL);
		if (!bcm->current_core->phy->lo_pairs) {
			kfree(bcm->current_core->phy->desired_power);
			return -ENOMEM;
		}
		memset(bcm->current_core->phy->lo_pairs, 0, sizeof(union bcm430x_lopair) * BCM430x_LO_COUNT);
	}
	bcm->current_core->phy->info_unk16 = 0xFFFF;

	return 0;
}

static int bcm430x_attach_board(struct bcm430x_private *bcm)
{
	struct pci_dev *pci_dev = bcm->pci_dev;
	struct net_device *net_dev = bcm->net_dev;
	int err;
	int i;
	void *ioaddr;
	unsigned long mmio_start, mmio_end, mmio_flags, mmio_len;
	int num_80211_cores;

	err = pci_enable_device(pci_dev);
	if (err) {
		printk(KERN_ERR PFX "unable to wake up pci device (%i)\n", err);
		err = -ENODEV;
		goto out;
	}

	mmio_start = pci_resource_start(pci_dev, 0);
	mmio_end = pci_resource_end(pci_dev, 0);
	mmio_flags = pci_resource_flags(pci_dev, 0);
	mmio_len = pci_resource_len(pci_dev, 0);

	/* make sure PCI base addr is MMIO */
	if (!(mmio_flags & IORESOURCE_MEM)) {
		printk(KERN_ERR PFX
		       "%s, region #0 not an MMIO resource, aborting\n",
		       pci_name(pci_dev));
		err = -ENODEV;
		goto err_pci_disable;
	}
	if (mmio_len != BCM430x_IO_SIZE) {
		printk(KERN_ERR PFX
		       "%s: invalid PCI mem region size(s), aborting\n",
		       pci_name(pci_dev));
		err = -ENODEV;
		goto err_pci_disable;
	}

	err = pci_request_regions(pci_dev, DRV_NAME);
	if (err) {
		printk(KERN_ERR PFX
		       "could not access PCI resources (%i)\n", err);
		goto err_pci_disable;
	}

	/* enable PCI bus-mastering */
	pci_set_master(pci_dev);

	/* ioremap MMIO region */
	ioaddr = ioremap(mmio_start, mmio_len);
	if (!ioaddr) {
		printk(KERN_ERR PFX "%s: cannot remap MMIO, aborting\n",
		       pci_name(pci_dev));
		err = -EIO;
		goto err_pci_release;
	}

	net_dev->base_addr = (long)ioaddr;
	bcm->mmio_addr = ioaddr;
	bcm->mmio_len = mmio_len;

	bcm430x_pci_read_config_16(bcm->pci_dev, PCI_SUBSYSTEM_VENDOR_ID,
	                           &bcm->board_vendor);
	bcm430x_pci_read_config_16(bcm->pci_dev, PCI_SUBSYSTEM_ID,
	                           &bcm->board_type);
	bcm430x_pci_read_config_16(bcm->pci_dev, PCI_REVISION_ID,
	                           &bcm->board_revision);

	bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_CHIPCOMMON_CAPABILITIES,
	                           &bcm->chipcommon_capabilities);

	err = bcm430x_chipset_attach(bcm);
	if (err)
		goto err_iounmap;
	err = bcm430x_pctl_init(bcm);
	if (err)
		goto err_chipset_detach;
	err = bcm430x_pctl_set_clock(bcm, BCM430x_PCTL_CLK_FAST);
	if (err)
		goto err_chipset_detach;
	err = bcm430x_probe_cores(bcm);
	if (err)
		goto err_chipset_detach;
	bcm430x_read_sprom(bcm);

	num_80211_cores = bcm430x_num_80211_cores(bcm);
	for (i = 0; i < num_80211_cores; i++) {
		err = bcm430x_switch_core(bcm, &bcm->core_80211[i]);
		assert(err != -ENODEV);
		if (err)
			goto err_chipset_detach;

		/* make the core usable */
		bcm430x_setup_backplane_pci_connection(bcm, (1 << bcm->current_core->index));

		/* Enable the selected wireless core.
		 * Connect PHY only on the first core.
		 */
		bcm430x_wireless_core_reset(bcm, (i == 0));

		err = bcm430x_read_phyinfo(bcm);
		if (err && (i == 0))
			goto err_chipset_detach;

		err = bcm430x_read_radioinfo(bcm);
		if (err && (i == 0))
			goto err_chipset_detach;

		err = bcm430x_validate_chip(bcm);
		if (err && (i == 0))
			goto err_chipset_detach;

		bcm430x_radio_turn_off(bcm);
		bcm430x_wireless_core_disable(bcm);
	}

	/* Use the first 80211 core as default. */
	bcm->def_80211_core = &bcm->core_80211[0];
	bcm430x_pctl_set_crystal(bcm, 0);

out:
	return err;

err_chipset_detach:
	bcm430x_chipset_detach(bcm);
err_iounmap:
	iounmap(bcm->mmio_addr);
err_pci_release:
	pci_release_regions(pci_dev);
err_pci_disable:
	pci_disable_device(pci_dev);
	goto out;
}

/* Do the Hardware IO operations to send the txb */
static inline int bcm430x_tx(struct bcm430x_private *bcm,
			     struct ieee80211_txb *txb)
{
	int err = -ENODEV;

	if (bcm->data_xfer_mode == BCM430x_DATAXFER_DMA)
		err = bcm430x_dma_transfer_txb(bcm, txb);
	else
		err = bcm430x_pio_transfer_txb(bcm, txb);

	return err;
}

/* set_security() callback in struct ieee80211_device */
static void bcm430x_ieee80211_set_security(struct net_device *net_dev,
					   struct ieee80211_security *sec)
{/*TODO*/
}

/* hard_start_xmit() callback in struct ieee80211_device */
static int bcm430x_ieee80211_hard_start_xmit(struct ieee80211_txb *txb,
					     struct net_device *net_dev)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	int err;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);
	err = bcm430x_tx(bcm, txb);
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
}

/* reset_port() callback in struct ieee80211_device */
static int bcm430x_ieee80211_reset_port(struct net_device *net_dev)
{/*TODO*/
	return 0;
}

static struct net_device_stats * bcm430x_net_get_stats(struct net_device *net_dev)
{
	return &(bcm430x_priv(net_dev)->ieee->stats);
}

static void bcm430x_net_tx_timeout(struct net_device *dev)
{/*TODO*/
}

static int bcm430x_net_open(struct net_device *net_dev)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	int err = 0;

	down(&bcm->sem);
	if (!(bcm->status & BCM430x_STAT_BOARDINITDONE)) {
		err = bcm430x_init_board(bcm);
		if (err)
			goto out_up;
	}
out_up:
	up(&bcm->sem);

	return err;
}

static int bcm430x_net_stop(struct net_device *net_dev)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);

	down(&bcm->sem);

	bcm430x_disable_interrupts_sync(bcm);
	bcm430x_free_board(bcm);

	up(&bcm->sem);

	return 0;
}

static int __devinit bcm430x_init_one(struct pci_dev *pdev,
				      const struct pci_device_id *ent)
{
	struct net_device *net_dev;
	struct bcm430x_private *bcm;
	int err;

#ifdef DEBUG_SINGLE_DEVICE_ONLY
	if (strcmp(pci_name(pdev), DEBUG_SINGLE_DEVICE_ONLY))
		return -ENODEV;
#endif

	net_dev = alloc_ieee80211(sizeof(*bcm));
	if (!net_dev) {
		printk(KERN_ERR PFX
		       "could not allocate ieee80211 device %s\n",
		       pci_name(pdev));
		err = -ENOMEM;
		goto out;
	}
	/* initialize the net_device struct */
	SET_MODULE_OWNER(net_dev);
	SET_NETDEV_DEV(net_dev, &pdev->dev);

	net_dev->open = bcm430x_net_open;
	net_dev->stop = bcm430x_net_stop;
	net_dev->get_stats = bcm430x_net_get_stats;
	net_dev->tx_timeout = bcm430x_net_tx_timeout;
	net_dev->wireless_handlers = &bcm430x_wx_handlers_def;
	net_dev->irq = pdev->irq;

/*FIXME: We disable scatter/gather IO until we figure out
 *       how to turn hardware checksumming on.
 */
#if 0
	net_dev->features |= NETIF_F_HW_CSUM;	/* hardware packet checksumming */
	net_dev->features |= NETIF_F_SG;	/* Scatter/gather IO. */
#endif

	/* initialize the bcm430x_private struct */
	bcm = bcm430x_priv(net_dev);

#ifdef DEBUG_ENABLE_MMIO_PRINT
	bcm430x_mmioprint_initial(bcm, 1);
#else
	bcm430x_mmioprint_initial(bcm, 0);
#endif

	bcm->irq_savedstate = BCM430x_IRQ_INITIAL;
	bcm->ieee = netdev_priv(net_dev);
	bcm->pci_dev = pdev;
	bcm->net_dev = net_dev;
	spin_lock_init(&bcm->lock);
	init_MUTEX(&bcm->sem);
	tasklet_init(&bcm->isr_tasklet,
		     (void (*)(unsigned long))bcm430x_interrupt_tasklet,
		     (unsigned long)bcm);
	bcm->workqueue = create_workqueue(DRV_NAME "_wq");
	if (!bcm->workqueue) {
		err = -ENOMEM;
		goto err_free_netdev;
	}
	if (pio)
		bcm->data_xfer_mode = BCM430x_DATAXFER_PIO;
	else
		bcm->data_xfer_mode = BCM430x_DATAXFER_DMA;

	bcm->ieee->iw_mode = BCM430x_INITIAL_IWMODE;
	bcm->ieee->set_security = bcm430x_ieee80211_set_security;
	bcm->ieee->hard_start_xmit = bcm430x_ieee80211_hard_start_xmit;
	bcm->ieee->reset_port = bcm430x_ieee80211_reset_port;

	pci_set_drvdata(pdev, net_dev);

	err = bcm430x_attach_board(bcm);
	if (err)
		goto err_destroy_wq;

	err = register_netdev(net_dev);
	if (err) {
		printk(KERN_ERR PFX "Cannot register net device, "
		       "aborting.\n");
		err = -ENOMEM;
		goto err_detach_board;
	}

	bcm430x_debugfs_add_device(bcm);

	assert(err == 0);
out:
	return err;

err_detach_board:
	bcm430x_detach_board(bcm);
err_destroy_wq:
	destroy_workqueue(bcm->workqueue);
err_free_netdev:
	free_netdev(net_dev);
	goto out;
}

static void __devexit bcm430x_remove_one(struct pci_dev *pdev)
{
	struct net_device *net_dev = pci_get_drvdata(pdev);
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);

	bcm430x_debugfs_remove_device(bcm);
	unregister_netdev(net_dev);
	bcm430x_detach_board(bcm);
	destroy_workqueue(bcm->workqueue);
	free_netdev(net_dev);
}

#ifdef CONFIG_PM

static int bcm430x_suspend(struct pci_dev *pdev, pm_message_t state)
{/*TODO*/
	return 0;
}

static int bcm430x_resume(struct pci_dev *pdev)
{/*TODO*/
	return 0;
}

#endif				/* CONFIG_PM */

static struct pci_driver bcm430x_pci_driver = {
	.name = BCM430x_DRIVER_NAME,
	.id_table = bcm430x_pci_tbl,
	.probe = bcm430x_init_one,
	.remove = __devexit_p(bcm430x_remove_one),
#ifdef CONFIG_PM
	.suspend = bcm430x_suspend,
	.resume = bcm430x_resume,
#endif				/* CONFIG_PM */
};

static int __init bcm430x_init(void)
{
	printk(KERN_INFO BCM430x_DRIVER_NAME "\n");
	bcm430x_debugfs_init();
	return pci_module_init(&bcm430x_pci_driver);
}

static void __exit bcm430x_exit(void)
{
	pci_unregister_driver(&bcm430x_pci_driver);
	bcm430x_debugfs_exit();
}

module_param(pio, int, 0444);
MODULE_PARM_DESC(pio, "enable(1) / disable(0) PIO mode");

module_param(short_preamble, int, 0444);
MODULE_PARM_DESC(short_preamble, "enable(1) / disable(0) the Short Preamble mode");

module_init(bcm430x_init)
module_exit(bcm430x_exit)

/* vim: set ts=8 sw=8 sts=8: */
