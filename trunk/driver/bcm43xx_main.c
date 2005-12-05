/*

  Broadcom BCM43xx wireless driver

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

#include "bcm43xx.h"
#include "bcm43xx_main.h"
#include "bcm43xx_debugfs.h"
#include "bcm43xx_radio.h"
#include "bcm43xx_phy.h"
#include "bcm43xx_dma.h"
#include "bcm43xx_pio.h"
#include "bcm43xx_power.h"
#include "bcm43xx_wx.h"


MODULE_DESCRIPTION("Broadcom BCM43xx wireless driver");
MODULE_AUTHOR("Martin Langer");
MODULE_AUTHOR("Stefano Brivio");
MODULE_AUTHOR("Michael Buesch");
MODULE_LICENSE("GPL");

/* Module parameters */
static int modparam_pio;
module_param_named(pio, modparam_pio, int, 0444);
MODULE_PARM_DESC(pio, "enable(1) / disable(0) PIO mode");

static int modparam_bad_frames_preempt;
module_param_named(bad_frames_preempt, modparam_bad_frames_preempt, int, 0444);
MODULE_PARM_DESC(bad_frames_preempt, "enable(1) / disable(0) Bad Frames Preemption");

static int modparam_locale = -1;
module_param_named(locale, modparam_locale, int, 0444);
MODULE_PARM_DESC(country, "Select LocaleCode 0-11 (For travelers)");

static int modparam_outdoor;
module_param_named(outdoor, modparam_outdoor, int, 0444);
MODULE_PARM_DESC(outdoor, "Set to 1, if you are using the device outdoor, 0 otherwise.");

#ifdef BCM43xx_DEBUG
static char modparam_fwpostfix[64];
module_param_string(fwpostfix, modparam_fwpostfix, 64, 0444);
MODULE_PARM_DESC(fwpostfix, "Postfix for .fw files. Useful for debugging.");
#else
# define modparam_fwpostfix  ""
#endif /* BCM43xx_DEBUG */


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


static struct pci_device_id bcm43xx_pci_tbl[] = {

	/* Broadcom 4303 802.11b */
	{ PCI_VENDOR_ID_BROADCOM, 0x4301, 0x1028, 0x0407, 0, 0, 0 }, /* Dell TrueMobile 1180 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4301, 0x103c, 0x12f3, 0, 0, 0 }, /* HP Compaq NX9110 802.11b Mini-PCI card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4301, 0x1043, 0x0120, 0, 0, 0 }, /* Asus WL-103b PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4301, 0x1737, 0x4301, 0, 0, 0 }, /* Linksys WMP11 rev2.7 */

	/* Broadcom 4307 802.11b */
//	{ PCI_VENDOR_ID_BROADCOM, 0x4307, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },

	/* Broadcom 4318 802.11b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x1028, 0x0005, 0, 0, 0 }, /* Dell TrueMobile 1370 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x103c, 0x1355, 0, 0, 0 }, /* Compag v2000z Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x1043, 0x120f, 0, 0, 0 }, /* Asus Z9200K Laptop */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x106b, 0x4318, 0, 0, 0 }, /* Apple AirPort Extreme 2 Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x1468, 0x0311, 0, 0, 0 }, /* AMBITG T60H906.00 */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x16ec, 0x0119, 0, 0, 0 }, /* U.S.Robotics Wireless MAXg PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x1799, 0x7000, 0, 0, 0 }, /* Belkin F5D7000 PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x17f9, 0x0006, 0, 0, 0 }, /* Amilo A1650 Laptop */

	/* Broadcom 4306 802.11b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x02fa, 0x3010, 0, 0, 0 }, /* Siemens Gigaset PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0001, 0, 0, 0 }, /* Dell TrueMobile 1300 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0002, 0, 0, 0 }, /* Dell TrueMobile 1300 PCMCIA Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0003, 0, 0, 0 }, /* Dell TrueMobile 1350 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x103c, 0x12f4, 0, 0, 0 }, /* HP nx9105 Laptop */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x103c, 0x12f8, 0, 0, 0 }, /* HP zd8000 Laptop */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x103c, 0x12fa, 0, 0, 0 }, /* Compaq Presario R3xxx PCI on board */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1043, 0x100f, 0, 0, 0 }, /* Asus WL-100G PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1057, 0x7025, 0, 0, 0 }, /* Motorola WN825G PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x106b, 0x004e, 0, 0, 0 }, /* Apple AirPort Extreme Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x14e4, 0x0013, 0, 0, 0 }, /* Linksys WMP54G PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x14e4, 0x0417, 0, 0, 0 }, /* TRENDnet TEW-401PC */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1737, 0x0015, 0, 0, 0 }, /* Linksys WMP54GS PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1737, 0x4320, 0, 0, 0 }, /* Linksys WPC54G PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1799, 0x7001, 0, 0, 0 }, /* Belkin F5D7001 PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1799, 0x7010, 0, 0, 0 }, /* Belkin F5D7010 PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1799, 0x7011, 0, 0, 0 }, /* Belkin F5D7011 PC Card */
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


static void bcm43xx_recover_from_fatal(struct bcm43xx_private *bcm, const char *error);
static void bcm43xx_free_board(struct bcm43xx_private *bcm);
static int bcm43xx_init_board(struct bcm43xx_private *bcm);


static void bcm43xx_ram_write(struct bcm43xx_private *bcm, u16 offset, u32 val)
{
	u32 oldsbf;

	oldsbf = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD,
			oldsbf | BCM43xx_SBF_XFER_REG_BYTESWAP);

	bcm43xx_write16(bcm, BCM43xx_MMIO_RAM_CONTROL, offset);
	bcm43xx_write32(bcm, BCM43xx_MMIO_RAM_DATA, val);

	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, oldsbf);
}

static inline
void bcm43xx_shm_control_word(struct bcm43xx_private *bcm,
			      u16 routing, u16 offset)
{
	u32 control;

	/* "offset" is the WORD offset. */

	control = routing;
	control <<= 16;
	control |= offset;
	bcm43xx_write32(bcm, BCM43xx_MMIO_SHM_CONTROL, control);
}

u32 bcm43xx_shm_read32(struct bcm43xx_private *bcm,
		       u16 routing, u16 offset)
{
	u32 ret;

	if (routing == BCM43xx_SHM_SHARED) {
		if (offset & 0x0003) {
			/* Unaligned access */
			bcm43xx_shm_control_word(bcm, routing, offset >> 2);
			ret = bcm43xx_read16(bcm, BCM43xx_MMIO_SHM_DATA_UNALIGNED);
			ret <<= 16;
			bcm43xx_shm_control_word(bcm, routing, (offset >> 2) + 1);
			ret |= bcm43xx_read16(bcm, BCM43xx_MMIO_SHM_DATA);

			return ret;
		}
		offset >>= 2;
	}
	bcm43xx_shm_control_word(bcm, routing, offset);
	ret = bcm43xx_read32(bcm, BCM43xx_MMIO_SHM_DATA);

	return ret;
}

u16 bcm43xx_shm_read16(struct bcm43xx_private *bcm,
		       u16 routing, u16 offset)
{
	u16 ret;

	if (routing == BCM43xx_SHM_SHARED) {
		if (offset & 0x0003) {
			/* Unaligned access */
			bcm43xx_shm_control_word(bcm, routing, offset >> 2);
			ret = bcm43xx_read16(bcm, BCM43xx_MMIO_SHM_DATA_UNALIGNED);

			return ret;
		}
		offset >>= 2;
	}
	bcm43xx_shm_control_word(bcm, routing, offset);
	ret = bcm43xx_read16(bcm, BCM43xx_MMIO_SHM_DATA);

	return ret;
}

void bcm43xx_shm_write32(struct bcm43xx_private *bcm,
			 u16 routing, u16 offset,
			 u32 value)
{
	if (routing == BCM43xx_SHM_SHARED) {
		if (offset & 0x0003) {
			/* Unaligned access */
			bcm43xx_shm_control_word(bcm, routing, offset >> 2);
			bcm43xx_write16(bcm, BCM43xx_MMIO_SHM_DATA_UNALIGNED,
					(value >> 16) & 0xffff);
			bcm43xx_shm_control_word(bcm, routing, (offset >> 2) + 1);
			bcm43xx_write16(bcm, BCM43xx_MMIO_SHM_DATA,
					value & 0xffff);
			return;
		}
		offset >>= 2;
	}
	bcm43xx_shm_control_word(bcm, routing, offset);
	bcm43xx_write32(bcm, BCM43xx_MMIO_SHM_DATA, value);
}

void bcm43xx_shm_write16(struct bcm43xx_private *bcm,
			 u16 routing, u16 offset,
			 u16 value)
{
	if (routing == BCM43xx_SHM_SHARED) {
		if (offset & 0x0003) {
			/* Unaligned access */
			bcm43xx_shm_control_word(bcm, routing, offset >> 2);
			bcm43xx_write16(bcm, BCM43xx_MMIO_SHM_DATA_UNALIGNED,
					value);
			return;
		}
		offset >>= 2;
	}
	bcm43xx_shm_control_word(bcm, routing, offset);
	bcm43xx_write16(bcm, BCM43xx_MMIO_SHM_DATA, value);
}

int bcm43xx_pci_read_config_16(struct pci_dev *pdev, u16 offset,
			       u16 *val)
{
	int err;

	err = pci_read_config_word(pdev, offset, val);
#ifdef DEBUG_ENABLE_PCILOG
	dprintk(KERN_INFO PFX "pciread16   offset: 0x%04x, value: 0x%04x\n", offset, *val);
#endif

	return err;
}

int bcm43xx_pci_read_config_32(struct pci_dev *pdev, u16 offset,
			       u32 *val)
{
	int err;

	err = pci_read_config_dword(pdev, offset, val);
#ifdef DEBUG_ENABLE_PCILOG
	dprintk(KERN_INFO PFX "pciread32   offset: 0x%04x, value: 0x%08x\n", offset, *val);
#endif

	return err;
}

int bcm43xx_pci_write_config_16(struct pci_dev *pdev, int offset,
				u16 val)
{
	int err;

	err = pci_write_config_word(pdev, offset, val);
#ifdef DEBUG_ENABLE_PCILOG
	dprintk(KERN_INFO PFX "pciwrite16  offset: 0x%04x, value: 0x%04x\n", offset, val);
#endif

	return err;
}

int bcm43xx_pci_write_config_32(struct pci_dev *pdev, int offset,
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
u8 bcm43xx_plcp_get_bitrate(struct bcm43xx_plcp_hdr4 *plcp,
			    const int ofdm_modulation)
{
	u8 rate;

	if (ofdm_modulation) {
		switch (plcp->raw[0] & 0xF) {
		case 0xB:
			rate = IEEE80211_OFDM_RATE_6MB;
			break;
		case 0xF:
			rate = IEEE80211_OFDM_RATE_9MB;
			break;
		case 0xA:
			rate = IEEE80211_OFDM_RATE_12MB;
			break;
		case 0xE:
			rate = IEEE80211_OFDM_RATE_18MB;
			break;
		case 0x9:
			rate = IEEE80211_OFDM_RATE_24MB;
			break;
		case 0xD:
			rate = IEEE80211_OFDM_RATE_36MB;
			break;
		case 0x8:
			rate = IEEE80211_OFDM_RATE_48MB;
			break;
		case 0xC:
			rate = IEEE80211_OFDM_RATE_54MB;
			break;
		default:
			rate = 0;
			assert(0);
		}
	} else {
		switch (plcp->raw[0]) {
		case 0x0A:
			rate = IEEE80211_CCK_RATE_1MB;
			break;
		case 0x14:
			rate = IEEE80211_CCK_RATE_2MB;
			break;
		case 0x37:
			rate = IEEE80211_CCK_RATE_5MB;
			break;
		case 0x6E:
			rate = IEEE80211_CCK_RATE_11MB;
			break;
		default:
			rate = 0;
			assert(0);
		}
	}

	return rate;
}

static inline
void bcm43xx_do_generate_plcp_hdr(u32 *data, unsigned char *raw,
				  u16 octets, const u8 bitrate,
				  const int ofdm_modulation)
{
	/* "data" and "raw" address the same memory area,
	 * but with different data types.
	 */

	/*TODO: This can be optimized, but first let's get it working. */

	/* Account for hardware-appended FCS. */
	octets += IEEE80211_FCS_LEN;

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

//bcm43xx_printk_bitdump(raw, 4, 0, "PLCP");
}

#define bcm43xx_generate_plcp_hdr(plcp, octets, bitrate, ofdm_modulation) \
	do {									\
		bcm43xx_do_generate_plcp_hdr(&((plcp)->data), (plcp)->raw,	\
					     (octets), (bitrate),		\
					     (ofdm_modulation));		\
	} while (0)

void fastcall
bcm43xx_generate_txhdr(struct bcm43xx_private *bcm,
		       struct bcm43xx_txhdr *txhdr,
		       const unsigned char *fragment_data,
		       const unsigned int fragment_len,
		       const int is_first_fragment,
		       const u16 cookie)
{
	const struct bcm43xx_phyinfo *phy = bcm->current_core->phy;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 14)
	const struct ieee80211_hdr *wireless_header = (const struct ieee80211_hdr *)fragment_data;
#else
	const struct ieee80211_hdr_1addr *wireless_header = (const struct ieee80211_hdr_1addr *)fragment_data;
#endif
	u8 bitrate;
	int ofdm_modulation;
	u8 fallback_bitrate;
	int fallback_ofdm_modulation;
	u16 tmp;

	/* Now construct the TX header. */
	memset(txhdr, 0, sizeof(*txhdr));

	//TODO: Some RTS/CTS stuff has to be done.
	//TODO: Encryption stuff.
	//TODO: others?

	bitrate = bcm->softmac->txrates.default_rate;
	ofdm_modulation = !(ieee80211_is_cck_rate(bitrate));
	fallback_bitrate = bcm->softmac->txrates.default_fallback;
	fallback_ofdm_modulation = !(ieee80211_is_cck_rate(fallback_bitrate));

	/* Set Frame Control from 80211 header. */
	txhdr->frame_control = wireless_header->frame_ctl;
	/* Copy address1 from 80211 header. */
	memcpy(txhdr->mac1, wireless_header->addr1, 6);
	/* Set the fallback duration ID. */
	//FIXME: We use the original durid for now.
	txhdr->fallback_dur_id = wireless_header->duration_id;
	/* Set the cookie (used as driver internal ID for the frame) */
	txhdr->cookie = cpu_to_le16(cookie);

	/* Generate the PLCP header and the fallback PLCP header. */
	bcm43xx_generate_plcp_hdr(&txhdr->plcp, fragment_len,
				  bitrate, ofdm_modulation);
	bcm43xx_generate_plcp_hdr(&txhdr->fallback_plcp, fragment_len,
				  fallback_bitrate, fallback_ofdm_modulation);

	/* Set the CONTROL field */
	tmp = 0;
	if (ofdm_modulation)
		tmp |= BCM43xx_TXHDRCTL_OFDM;
	if (bcm->short_preamble) //FIXME: could be the other way around, please test
		tmp |= BCM43xx_TXHDRCTL_SHORT_PREAMBLE;
	tmp |= (phy->antenna_diversity << BCM43xx_TXHDRCTL_ANTENNADIV_SHIFT)
		& BCM43xx_TXHDRCTL_ANTENNADIV_MASK;
	txhdr->control = cpu_to_le16(tmp);

	/* Set the FLAGS field */
	tmp = 0;
	if (!is_multicast_ether_addr(wireless_header->addr1) &&
	    !is_broadcast_ether_addr(wireless_header->addr1))
		tmp |= BCM43xx_TXHDRFLAG_EXPECTACK;
	if (1 /* FIXME: PS poll?? */)
		tmp |= 0x10; // FIXME: unknown meaning.
	if (fallback_ofdm_modulation)
		tmp |= BCM43xx_TXHDRFLAG_FALLBACKOFDM;
	if (is_first_fragment)
		tmp |= BCM43xx_TXHDRFLAG_FIRSTFRAGMENT;
	txhdr->flags = cpu_to_le16(tmp);

	/* Set WSEC/RATE field */
	//TODO: rts, wsec
	tmp = (txhdr->plcp.raw[0] << BCM43xx_TXHDR_RATE_SHIFT)
	       & BCM43xx_TXHDR_RATE_MASK;
	txhdr->wsec_rate = cpu_to_le16(tmp);

//bcm43xx_printk_bitdump((const unsigned char *)txhdr, sizeof(*txhdr), 1, "TX header");
}

static
void bcm43xx_macfilter_set(struct bcm43xx_private *bcm,
			   u16 offset,
			   const u8 *mac)
{
	u16 data;

	//FIXME: Only for card rev < 3?

	offset |= 0x0020;
	bcm43xx_write16(bcm, BCM43xx_MMIO_MACFILTER_CONTROL, offset);

	data = mac[0];
	data |= mac[1] << 8;
	bcm43xx_write16(bcm, BCM43xx_MMIO_MACFILTER_DATA, data);
	data = mac[2];
	data |= mac[3] << 8;
	bcm43xx_write16(bcm, BCM43xx_MMIO_MACFILTER_DATA, data);
	data = mac[4];
	data |= mac[5] << 8;
	bcm43xx_write16(bcm, BCM43xx_MMIO_MACFILTER_DATA, data);
}

static inline
void bcm43xx_macfilter_clear(struct bcm43xx_private *bcm,
			     u16 offset)
{
	const u8 zero_addr[ETH_ALEN] = { 0 };

	bcm43xx_macfilter_set(bcm, offset, zero_addr);
}

static void bcm43xx_write_mac_bssid_templates(struct bcm43xx_private *bcm)
{
	const u8 *mac = (const u8 *)(bcm->net_dev->dev_addr);
	const u8 *bssid = (const u8 *)(bcm->ieee->bssid);
	u8 mac_bssid[ETH_ALEN * 2];
	int i;

	memcpy(mac_bssid, mac, ETH_ALEN);
	memcpy(mac_bssid + ETH_ALEN, bssid, ETH_ALEN);

	/* Write our MAC address and BSSID to template ram */
	for (i = 0; i < ARRAY_SIZE(mac_bssid); i += sizeof(u32))
		bcm43xx_ram_write(bcm, 0x20 + i, *((u32 *)(mac_bssid + i)));
	for (i = 0; i < ARRAY_SIZE(mac_bssid); i += sizeof(u32))
		bcm43xx_ram_write(bcm, 0x78 + i, *((u32 *)(mac_bssid + i)));
	for (i = 0; i < ARRAY_SIZE(mac_bssid); i += sizeof(u32))
		bcm43xx_ram_write(bcm, 0x478 + i, *((u32 *)(mac_bssid + i)));
}

static inline
void bcm43xx_set_slot_time(struct bcm43xx_private *bcm, u16 slot_time)
{
	/* slot_time is in usec. */
	if (bcm->current_core->phy->type != BCM43xx_PHYTYPE_G)
		return;
	bcm43xx_write16(bcm, 0x684, 510 + slot_time);
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0010, slot_time);
}

static inline
void bcm43xx_short_slot_timing_enable(struct bcm43xx_private *bcm)
{
	bcm43xx_set_slot_time(bcm, 9);
}

static inline
void bcm43xx_short_slot_timing_disable(struct bcm43xx_private *bcm)
{
	bcm43xx_set_slot_time(bcm, 20);
}

//FIXME: rename this func?
static void bcm43xx_disassociate(struct bcm43xx_private *bcm)
{
	bcm43xx_mac_suspend(bcm);
	bcm43xx_macfilter_clear(bcm, BCM43xx_MACFILTER_ASSOC);

	bcm43xx_ram_write(bcm, 0x0026, 0x0000);
	bcm43xx_ram_write(bcm, 0x0028, 0x0000);
	bcm43xx_ram_write(bcm, 0x007E, 0x0000);
	bcm43xx_ram_write(bcm, 0x0080, 0x0000);
	bcm43xx_ram_write(bcm, 0x047E, 0x0000);
	bcm43xx_ram_write(bcm, 0x0480, 0x0000);

	if (bcm->current_core->rev < 3) {
		bcm43xx_write16(bcm, 0x0610, 0x8000);
		bcm43xx_write16(bcm, 0x060E, 0x0000);
	} else
		bcm43xx_write32(bcm, 0x0188, 0x80000000);

	bcm43xx_shm_write32(bcm, BCM43xx_SHM_WIRELESS, 0x0004, 0x000003ff);

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_G &&
	    ieee80211_is_ofdm_rate(bcm->softmac->txrates.default_rate))
		bcm43xx_short_slot_timing_enable(bcm);

	bcm43xx_mac_enable(bcm);
}

//FIXME: rename this func?
static void bcm43xx_associate(struct bcm43xx_private *bcm,
			      const u8 *mac)
{
	memcpy(bcm->ieee->bssid, mac, ETH_ALEN);

	bcm43xx_mac_suspend(bcm);
	bcm43xx_macfilter_set(bcm, BCM43xx_MACFILTER_ASSOC, mac);
	bcm43xx_write_mac_bssid_templates(bcm);
	bcm43xx_mac_enable(bcm);
}

/* Enable a Generic IRQ. "mask" is the mask of which IRQs to enable.
 * Returns the _previously_ enabled IRQ mask.
 */
static inline u32 bcm43xx_interrupt_enable(struct bcm43xx_private *bcm, u32 mask)
{
	u32 old_mask;

	old_mask = bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_MASK);
	bcm43xx_write32(bcm, BCM43xx_MMIO_GEN_IRQ_MASK, old_mask | mask);

	return old_mask;
}

/* Disable a Generic IRQ. "mask" is the mask of which IRQs to disable.
 * Returns the _previously_ enabled IRQ mask.
 */
static inline u32 bcm43xx_interrupt_disable(struct bcm43xx_private *bcm, u32 mask)
{
	u32 old_mask;

	old_mask = bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_MASK);
	bcm43xx_write32(bcm, BCM43xx_MMIO_GEN_IRQ_MASK, old_mask & ~mask);

	return old_mask;
}

/* Make sure we don't receive more data from the device. */
static int bcm43xx_disable_interrupts_sync(struct bcm43xx_private *bcm, u32 *oldstate)
{
	u32 old;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);
	if (bcm43xx_is_initializing(bcm) || bcm->shutting_down) {
		spin_unlock_irqrestore(&bcm->lock, flags);
		return -EBUSY;
	}
	old = bcm43xx_interrupt_disable(bcm, BCM43xx_IRQ_ALL);
	tasklet_disable(&bcm->isr_tasklet);
	spin_unlock_irqrestore(&bcm->lock, flags);
	if (oldstate)
		*oldstate = old;

	return 0;
}

static int bcm43xx_read_radioinfo(struct bcm43xx_private *bcm)
{
	u32 radio_id;
	u16 manufact;
	u16 version;
	u8 revision;
	s8 i;

	if (bcm->chip_id == 0x4317) {
		if (bcm->chip_rev == 0x00)
			radio_id = 0x3205017F;
		else if (bcm->chip_rev == 0x01)
			radio_id = 0x4205017F;
		else
			radio_id = 0x5205017F;
	} else {
		bcm43xx_write16(bcm, BCM43xx_MMIO_RADIO_CONTROL, BCM43xx_RADIOCTL_ID);
		radio_id = bcm43xx_read16(bcm, BCM43xx_MMIO_RADIO_DATA_HIGH);
		radio_id <<= 16;
		bcm43xx_write16(bcm, BCM43xx_MMIO_RADIO_CONTROL, BCM43xx_RADIOCTL_ID);
		radio_id |= bcm43xx_read16(bcm, BCM43xx_MMIO_RADIO_DATA_LOW);
	}

	manufact = (radio_id & 0x00000FFF);
	version = (radio_id & 0x0FFFF000) >> 12;
	revision = (radio_id & 0xF0000000) >> 28;

	dprintk(KERN_INFO PFX "Detected Radio:  ID: %x (Manuf: %x Ver: %x Rev: %x)\n",
		radio_id, manufact, version, revision);

	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_A:
		if ((version != 0x2060) || (revision != 1) || (manufact != 0x17f))
			goto err_unsupported_radio;
		break;
	case BCM43xx_PHYTYPE_B:
		if ((version & 0xFFF0) != 0x2050)
			goto err_unsupported_radio;
		break;
	case BCM43xx_PHYTYPE_G:
		if (version != 0x2050)
			goto err_unsupported_radio;
		break;
	}

	bcm->current_core->radio->manufact = manufact;
	bcm->current_core->radio->version = version;
	bcm->current_core->radio->revision = revision;
	bcm->current_core->radio->_id = radio_id;

	/* Set default attenuation values. */
	bcm->current_core->radio->txpower[0] = 2;
	bcm->current_core->radio->txpower[1] = 2;
	if (revision == 1)
		bcm->current_core->radio->txpower[2] = 3;
	else
		bcm->current_core->radio->txpower[2] = 0;

	/* Initialize the in-memory nrssi Lookup Table. */
	for (i = 0; i < 64; i++)
		bcm->current_core->radio->nrssi_lt[i] = i;

	return 0;

err_unsupported_radio:
	printk(KERN_ERR PFX "Unsupported Radio connected to the PHY!\n");
	return -ENODEV;
}

static const char * bcm43xx_locale_iso(u8 locale)
{
	/* ISO 3166-1 country codes.
	 * Note that there aren't ISO 3166-1 codes for
	 * all or locales. (Not all locales are countries)
	 */
	switch (locale) {
	case BCM43xx_LOCALE_WORLD:
	case BCM43xx_LOCALE_ALL:
		return "XX";
	case BCM43xx_LOCALE_THAILAND:
		return "TH";
	case BCM43xx_LOCALE_ISRAEL:
		return "IL";
	case BCM43xx_LOCALE_JORDAN:
		return "JO";
	case BCM43xx_LOCALE_CHINA:
		return "CN";
	case BCM43xx_LOCALE_JAPAN:
	case BCM43xx_LOCALE_JAPAN_HIGH:
		return "JP";
	case BCM43xx_LOCALE_USA_CANADA_ANZ:
	case BCM43xx_LOCALE_USA_LOW:
		return "US";
	case BCM43xx_LOCALE_EUROPE:
		return "EU";
	case BCM43xx_LOCALE_NONE:
		return "  ";
	}
	assert(0);
	return "  ";
}

static const char * bcm43xx_locale_string(u8 locale)
{
	switch (locale) {
	case BCM43xx_LOCALE_WORLD:
		return "World";
	case BCM43xx_LOCALE_THAILAND:
		return "Thailand";
	case BCM43xx_LOCALE_ISRAEL:
		return "Israel";
	case BCM43xx_LOCALE_JORDAN:
		return "Jordan";
	case BCM43xx_LOCALE_CHINA:
		return "China";
	case BCM43xx_LOCALE_JAPAN:
		return "Japan";
	case BCM43xx_LOCALE_USA_CANADA_ANZ:
		return "USA/Canada/ANZ";
	case BCM43xx_LOCALE_EUROPE:
		return "Europe";
	case BCM43xx_LOCALE_USA_LOW:
		return "USAlow";
	case BCM43xx_LOCALE_JAPAN_HIGH:
		return "JapanHigh";
	case BCM43xx_LOCALE_ALL:
		return "All";
	case BCM43xx_LOCALE_NONE:
		return "None";
	}
	assert(0);
	return "";
}

static inline
u16 sprom_read(struct bcm43xx_private *bcm,
	       u16 offset)
{
	return bcm43xx_read16(bcm, BCM43xx_SPROM_BASE + (2 * offset));
}

/* Read the SPROM and store its adjusted values in bcm->sprom */
static int bcm43xx_read_sprom(struct bcm43xx_private *bcm)
{
	u16 value;

	/* boardflags2 */
	value = sprom_read(bcm, BCM43xx_SPROM_BOARDFLAGS2);
	bcm->sprom.boardflags2 = value;

	/* il0macaddr */
	value = sprom_read(bcm, BCM43xx_SPROM_IL0MACADDR + 0);
	*(((u16 *)bcm->sprom.il0macaddr) + 0) = cpu_to_be16(value);
	value = sprom_read(bcm, BCM43xx_SPROM_IL0MACADDR + 1);
	*(((u16 *)bcm->sprom.il0macaddr) + 1) = cpu_to_be16(value);
	value = sprom_read(bcm, BCM43xx_SPROM_IL0MACADDR + 2);
	*(((u16 *)bcm->sprom.il0macaddr) + 2) = cpu_to_be16(value);

	/* et0macaddr */
	value = sprom_read(bcm, BCM43xx_SPROM_ET0MACADDR + 0);
	*(((u16 *)bcm->sprom.et0macaddr) + 0) = cpu_to_be16(value);
	value = sprom_read(bcm, BCM43xx_SPROM_ET0MACADDR + 1);
	*(((u16 *)bcm->sprom.et0macaddr) + 1) = cpu_to_be16(value);
	value = sprom_read(bcm, BCM43xx_SPROM_ET0MACADDR + 2);
	*(((u16 *)bcm->sprom.et0macaddr) + 2) = cpu_to_be16(value);

	/* et1macaddr */
	value = sprom_read(bcm, BCM43xx_SPROM_ET1MACADDR + 0);
	*(((u16 *)bcm->sprom.et1macaddr) + 0) = cpu_to_be16(value);
	value = sprom_read(bcm, BCM43xx_SPROM_ET1MACADDR + 1);
	*(((u16 *)bcm->sprom.et1macaddr) + 1) = cpu_to_be16(value);
	value = sprom_read(bcm, BCM43xx_SPROM_ET1MACADDR + 2);
	*(((u16 *)bcm->sprom.et1macaddr) + 2) = cpu_to_be16(value);

	/* ethernat phy settings */
	value = sprom_read(bcm, BCM43xx_SPROM_ETHPHY);
	bcm->sprom.et0phyaddr = (value & 0x001F);
	bcm->sprom.et1phyaddr = (value & 0x03E0) >> 5;
	bcm->sprom.et0mdcport = (value & (1 << 14)) >> 14;
	bcm->sprom.et1mdcport = (value & (1 << 15)) >> 15;

	/* boardrev, antennas, locale */
	value = sprom_read(bcm, BCM43xx_SPROM_BOARDREV);
	bcm->sprom.boardrev = (value & 0x00FF);
	bcm->sprom.locale = (value & 0x0F00) >> 8;
	bcm->sprom.antennas_aphy = (value & 0x3000) >> 12;
	bcm->sprom.antennas_bgphy = (value & 0xC000) >> 14;
	if (modparam_locale != -1) {
		if (modparam_locale >= 0 && modparam_locale <= 11) {
			bcm->sprom.locale = modparam_locale;
			printk(KERN_WARNING PFX "Operating with modified "
						"LocaleCode %u (%s)\n",
			       bcm->sprom.locale,
			       bcm43xx_locale_string(bcm->sprom.locale));
		} else {
			printk(KERN_WARNING PFX "Module parameter \"locale\" "
						"invalid value. (0 - 11)\n");
		}
	}

	/* pa0b* */
	value = sprom_read(bcm, BCM43xx_SPROM_PA0B0);
	bcm->sprom.pa0b0 = value;
	value = sprom_read(bcm, BCM43xx_SPROM_PA0B1);
	bcm->sprom.pa0b1 = value;
	value = sprom_read(bcm, BCM43xx_SPROM_PA0B2);
	bcm->sprom.pa0b2 = value;

	/* wl0gpio* */
	value = sprom_read(bcm, BCM43xx_SPROM_WL0GPIO0);
	if (value == 0xFFFF)
		value = 0x0000;
	bcm->sprom.wl0gpio0 = (value & 0x00FF);
	bcm->sprom.wl0gpio1 = (value & 0xFF00) >> 8;
	value = sprom_read(bcm, BCM43xx_SPROM_WL0GPIO2);
	if (value == 0xFFFF)
		value = 0x0000;
	bcm->sprom.wl0gpio2 = (value & 0x00FF);
	bcm->sprom.wl0gpio3 = (value & 0xFF00) >> 8;

	/* maxpower */
	value = sprom_read(bcm, BCM43xx_SPROM_MAXPWR);
	bcm->sprom.maxpower_aphy = (value & 0x00FF);
	bcm->sprom.maxpower_bgphy = (value & 0xFF00) >> 8;

	/* pa1b* */
	value = sprom_read(bcm, BCM43xx_SPROM_PA1B0);
	bcm->sprom.pa1b0 = value;
	value = sprom_read(bcm, BCM43xx_SPROM_PA1B1);
	bcm->sprom.pa1b1 = value;
	value = sprom_read(bcm, BCM43xx_SPROM_PA1B2);
	bcm->sprom.pa1b2 = value;

	/* idle tssi target */
	value = sprom_read(bcm, BCM43xx_SPROM_IDL_TSSI_TGT);
	bcm->sprom.idle_tssi_tgt_aphy = (value & 0x00FF);
	bcm->sprom.idle_tssi_tgt_bgphy = (value & 0xFF00) >> 8;

	/* boardflags */
	value = sprom_read(bcm, BCM43xx_SPROM_BOARDFLAGS);
	if (value == 0xFFFF)
		value = 0x0000;
	bcm->sprom.boardflags = value;

	/* antenna gain */
	value = sprom_read(bcm, BCM43xx_SPROM_ANTENNA_GAIN);
	if (value == 0x0000 || value == 0xFFFF)
		value = 0x0202;
	bcm->sprom.antennagain_aphy = (value & 0x00FF);
	bcm->sprom.antennagain_bgphy = (value & 0xFF00) >> 8;

	/* SPROM version, crc8 */
	value = sprom_read(bcm, BCM43xx_SPROM_VERSION);
	bcm->sprom.spromversion = value;

	return 0;
}

static int bcm43xx_channel_is_allowed(struct bcm43xx_private *bcm, u8 channel,
				      u8 *max_power, u8 *flags)
{
	/* THIS FUNCTION DOES _NOT_ ENFORCE REGULATORY DOMAIN COMPLIANCE.
	 * It is only a helper function to make life easier to
	 * select legal channels and transmission powers.
	 */

	u8 phytype = bcm->current_core->phy->type;
	int allowed = 0;

	*max_power = 0;
	*flags = 0;

	//FIXME: Set max_power and maybe flags
	/*FIXME: Allowed channels are sometimes different for outdoor
	 *       or indoor use. See modparam_outdoor.
	 */
	/* From b specs Max Power BPHY:
	 *	USA:	1000mW
	 *	Europe:	100mW
	 *	Japan:	10mW/MHz
	 */

	switch (bcm->sprom.locale) {
	case BCM43xx_LOCALE_WORLD:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			if (channel >= 1 && channel <= 13)
				allowed = 1;
		} else {
			if (channel >= 1 && channel <= 13)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_THAILAND:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			if (channel >= 1 && channel <= 14)
				allowed = 1;
		} else {
			if (channel >= 1 && channel <= 14)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_ISRAEL:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			if (channel >= 5 && channel <= 7)
				allowed = 1;
		} else {
			if (channel >= 5 && channel <= 7)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_JORDAN:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			if (channel >= 10 && channel <= 13)
				allowed = 1;
		} else {
			if (channel >= 10 && channel <= 13)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_CHINA:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			if (channel >= 1 && channel <= 13)
				allowed = 1;
		} else {
			if (channel >= 1 && channel <= 13)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_JAPAN:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			//FIXME: This seems to be wrong.
			if (channel >= 1 && channel <= 14)
				allowed = 1;
		} else {
			//FIXME: This seems to be wrong.
			if (channel >= 1 && channel <= 14)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_USA_CANADA_ANZ:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			if (channel >= 1 && channel <= 13)
				allowed = 1;
		} else {
			if (channel >= 1 && channel <= 11)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_EUROPE:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			if (channel >= 1 && channel <= 13)
				allowed = 1;
		} else {
			if (channel >= 1 && channel <= 13)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_USA_LOW:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			if (channel >= 1 && channel <= 13)
				allowed = 1;
		} else {
			if (channel >= 1 && channel <= 11)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_JAPAN_HIGH:
		if (phytype == BCM43xx_PHYTYPE_A) {
			allowed = 1;//FIXME
		} else if (phytype == BCM43xx_PHYTYPE_B) {
			//FIXME?
			if (channel >= 1 && channel <= 14)
				allowed = 1;
		} else {
			if (channel >= 1 && channel <= 14)
				allowed = 1;
		}
		break;
	case BCM43xx_LOCALE_ALL:
		allowed = 1;
		break;
	case BCM43xx_LOCALE_NONE:
		break;
	default:
		assert(0);
	}

	return allowed;
}

static void bcm43xx_geo_init(struct bcm43xx_private *bcm)
{
	struct ieee80211_geo geo;
	struct ieee80211_channel *chan;
	int have_a = 0, have_bg = 0;
	int i, num80211;
	u8 channel, flags, max_power;
	struct bcm43xx_phyinfo *phy;
	const char *iso_country;

	memset(&geo, 0, sizeof(geo));
	num80211 = bcm43xx_num_80211_cores(bcm);
	for (i = 0; i < num80211; i++) {
		phy = bcm->phy + i;
		switch (phy->type) {
		case BCM43xx_PHYTYPE_B:
		case BCM43xx_PHYTYPE_G:
			have_bg = 1;
			break;
		case BCM43xx_PHYTYPE_A:
			have_a = 1;
			break;
		default:
			assert(0);
		}
	}
	iso_country = bcm43xx_locale_iso(bcm->sprom.locale);

 	if (have_a) {
		for (i = 0, channel = 0; channel < 201; channel++) {
			if (!bcm43xx_channel_is_allowed(bcm, channel,
							&max_power, &flags))
				continue;
			chan = &geo.a[i++];
			chan->freq = bcm43xx_channel_to_freq(bcm, channel);
			chan->channel = channel;
			chan->flags = flags;
			chan->max_power = max_power;
		}
		geo.a_channels = i;
	}
	if (have_bg) {
		for (i = 0, channel = 1; channel < 15; channel++) {
			if (!bcm43xx_channel_is_allowed(bcm, channel,
							&max_power, &flags))
				continue;
			chan = &geo.bg[i++];
			chan->freq = bcm43xx_channel_to_freq(bcm, channel);
			chan->channel = channel;
			chan->flags = flags;
			chan->max_power = max_power;
		}
		geo.bg_channels = i;
	}
	memcpy(geo.name, iso_country, 2);
	if (0 /*TODO: Outdoor use only */)
		geo.name[2] = 'O';
	else if (0 /*TODO: Indoor use only */)
		geo.name[2] = 'I';
	else
		geo.name[2] = ' ';
	geo.name[3] = '\0';

	ieee80211_set_geo(bcm->ieee, &geo);
}

/* Read and adjust LED infos */
static int bcm43xx_leds_init(struct bcm43xx_private *bcm)
{
	int i;

	bcm->leds[0] = bcm->sprom.wl0gpio0;
	bcm->leds[1] = bcm->sprom.wl0gpio1;
	bcm->leds[2] = bcm->sprom.wl0gpio2;
	bcm->leds[3] = bcm->sprom.wl0gpio3;

	for (i = 0; i < BCM43xx_LED_COUNT; i++) {
		if ((bcm->leds[i] & ~BCM43xx_LED_ACTIVELOW) == BCM43xx_LED_INACTIVE) {
			bcm->leds[i] = 0xFF;
			continue;
		};
		if (bcm->leds[i] == 0xFF) {
			switch (i) {
			case 0:
				bcm->leds[0] = ((bcm->board_vendor == PCI_VENDOR_ID_COMPAQ)
				                ? BCM43xx_LED_RADIO_ALL
				                : BCM43xx_LED_ACTIVITY);
				break;
			case 1:
				bcm->leds[1] = BCM43xx_LED_RADIO_B;
				break;
			case 2:
				bcm->leds[2] = BCM43xx_LED_RADIO_A;
				break;
			case 3:
				bcm->leds[3] = BCM43xx_LED_OFF;
				break;
			}
		}
	}

	return 0;
}

/* DummyTransmission function, as documented on 
 * http://bcm-specs.sipsolutions.net/DummyTransmission
 */
void bcm43xx_dummy_transmission(struct bcm43xx_private *bcm)
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
	case BCM43xx_PHYTYPE_A:
		max_loop = 0x1E;
		buffer[0] = 0xCC010200;
		break;
	case BCM43xx_PHYTYPE_B:
	case BCM43xx_PHYTYPE_G:
		max_loop = 0xFA;
		buffer[0] = 0x6E840B00; 
		break;
	default:
		assert(0);
		return;
	}

	for (i = 0; i < 5; i++)
		bcm43xx_ram_write(bcm, i * 4, buffer[i]);

	bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD); /* dummy read */

	bcm43xx_write16(bcm, 0x0568, 0x0000);
	bcm43xx_write16(bcm, 0x07C0, 0x0000);
	bcm43xx_write16(bcm, 0x050C, ((bcm->current_core->phy->type == BCM43xx_PHYTYPE_A) ? 1 : 0));
	bcm43xx_write16(bcm, 0x0508, 0x0000);
	bcm43xx_write16(bcm, 0x050A, 0x0000);
	bcm43xx_write16(bcm, 0x054C, 0x0000);
	bcm43xx_write16(bcm, 0x056A, 0x0014);
	bcm43xx_write16(bcm, 0x0568, 0x0826);
	bcm43xx_write16(bcm, 0x0500, 0x0000);
	bcm43xx_write16(bcm, 0x0502, 0x0030);

	for (i = 0x00; i < max_loop; i++) {
		value = bcm43xx_read16(bcm, 0x050E);
		if ((value & 0x0080) != 0)
			break;
		udelay(10);
	}
	for (i = 0x00; i < 0x0A; i++) {
		value = bcm43xx_read16(bcm, 0x050E);
		if ((value & 0x0400) != 0)
			break;
		udelay(10);
	}
	for (i = 0x00; i < 0x0A; i++) {
		value = bcm43xx_read16(bcm, 0x0690);
		if ((value & 0x0100) == 0)
			break;
		udelay(10);
	}
}

/* Puts the index of the current core into user supplied core variable.
 * This function reads the value from the device.
 * Almost always you don't want to call this, but use bcm->current_core
 */
static inline
int _get_current_core(struct bcm43xx_private *bcm, int *core)
{
	int err;

	err = bcm43xx_pci_read_config_32(bcm->pci_dev, BCM43xx_REG_ACTIVE_CORE, core);
	if (unlikely(err)) {
		dprintk(KERN_ERR PFX "BCM43xx_REG_ACTIVE_CORE read failed!\n");
		return -ENODEV;
	}
	*core = (*core - 0x18000000) / 0x1000;

	return 0;
}

/* Lowlevel core-switch function. This is only to be used in
 * bcm43xx_switch_core() and bcm43xx_probe_cores()
 */
static int _switch_core(struct bcm43xx_private *bcm, int core)
{
	int err;
	int attempts = 0;
	int current_core = -1;

	assert(core >= 0);

	err = _get_current_core(bcm, &current_core);
	if (unlikely(err))
		goto out;

	/* Write the computed value to the register. This doesn't always
	   succeed so we retry BCM43xx_SWITCH_CORE_MAX_RETRIES times */
	while (current_core != core) {
		if (unlikely(attempts++ > BCM43xx_SWITCH_CORE_MAX_RETRIES)) {
			err = -ENODEV;
			printk(KERN_ERR PFX
			       "unable to switch to core %u, retried %i times\n",
			       core, attempts);
			goto out;
		}
		err = bcm43xx_pci_write_config_32(bcm->pci_dev,
						  BCM43xx_REG_ACTIVE_CORE,
						  (core * 0x1000) + 0x18000000);
		if (unlikely(err)) {
			dprintk(KERN_ERR PFX "BCM43xx_REG_ACTIVE_CORE write failed!\n");
			continue;
		}
		_get_current_core(bcm, &current_core);
	}

	assert(err == 0);
out:
	return err;
}

int bcm43xx_switch_core(struct bcm43xx_private *bcm, struct bcm43xx_coreinfo *new_core)
{
	int err;

	if (!new_core)
		return 0;

	if (!(new_core->flags & BCM43xx_COREFLAG_AVAILABLE))
		return -ENODEV;
	if (bcm->current_core == new_core)
		return 0;
	err = _switch_core(bcm, new_core->index);
	if (!err)
		bcm->current_core = new_core;

	return err;
}

static inline int bcm43xx_core_enabled(struct bcm43xx_private *bcm)
{
	u32 value;

	value = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATELOW);
	value &= BCM43xx_SBTMSTATELOW_CLOCK | BCM43xx_SBTMSTATELOW_RESET
		 | BCM43xx_SBTMSTATELOW_REJECT;

	return (value == BCM43xx_SBTMSTATELOW_CLOCK);
}

/* disable current core */
static int bcm43xx_core_disable(struct bcm43xx_private *bcm, u32 core_flags)
{
	u32 sbtmstatelow;
	u32 sbtmstatehigh;
	int i;

	/* fetch sbtmstatelow from core information registers */
	sbtmstatelow = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATELOW);

	/* core is already in reset */
	if (sbtmstatelow & BCM43xx_SBTMSTATELOW_RESET)
		goto out;

	if (sbtmstatelow & BCM43xx_SBTMSTATELOW_CLOCK) {
		sbtmstatelow = BCM43xx_SBTMSTATELOW_CLOCK |
			       BCM43xx_SBTMSTATELOW_REJECT;
		bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, sbtmstatelow);

		for (i = 0; i < 1000; i++) {
			sbtmstatelow = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATELOW);
			if (sbtmstatelow & BCM43xx_SBTMSTATELOW_REJECT) {
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
			sbtmstatehigh = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATEHIGH);
			if (!(sbtmstatehigh & BCM43xx_SBTMSTATEHIGH_BUSY)) {
				i = -1;
				break;
			}
			udelay(10);
		}
		if (i != -1) {
			printk(KERN_ERR PFX "Error: core_disable() BUSY timeout!\n");
			return -EBUSY;
		}

		sbtmstatelow = BCM43xx_SBTMSTATELOW_FORCE_GATE_CLOCK |
			       BCM43xx_SBTMSTATELOW_REJECT |
			       BCM43xx_SBTMSTATELOW_RESET |
			       BCM43xx_SBTMSTATELOW_CLOCK |
			       core_flags;
		bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, sbtmstatelow);
		udelay(10);
	}

	sbtmstatelow = BCM43xx_SBTMSTATELOW_RESET |
		       BCM43xx_SBTMSTATELOW_REJECT |
		       core_flags;
	bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, sbtmstatelow);

out:
	bcm->current_core->flags &= ~ BCM43xx_COREFLAG_ENABLED;
	return 0;
}

/* enable (reset) current core */
static int bcm43xx_core_enable(struct bcm43xx_private *bcm, u32 core_flags)
{
	u32 sbtmstatelow;
	u32 sbtmstatehigh;
	u32 sbimstate;
	int err;

	err = bcm43xx_core_disable(bcm, core_flags);
	if (err)
		goto out;

	sbtmstatelow = BCM43xx_SBTMSTATELOW_CLOCK |
		       BCM43xx_SBTMSTATELOW_RESET |
		       BCM43xx_SBTMSTATELOW_FORCE_GATE_CLOCK |
		       core_flags;
	bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);

	sbtmstatehigh = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATEHIGH);
	if (sbtmstatehigh & BCM43xx_SBTMSTATEHIGH_SERROR) {
		sbtmstatehigh = 0x00000000;
		bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATEHIGH, sbtmstatehigh);
	}

	sbimstate = bcm43xx_read32(bcm, BCM43xx_CIR_SBIMSTATE);
	if (sbimstate & (BCM43xx_SBIMSTATE_IB_ERROR | BCM43xx_SBIMSTATE_TIMEOUT)) {
		sbimstate &= ~(BCM43xx_SBIMSTATE_IB_ERROR | BCM43xx_SBIMSTATE_TIMEOUT);
		bcm43xx_write32(bcm, BCM43xx_CIR_SBIMSTATE, sbimstate);
	}

	sbtmstatelow = BCM43xx_SBTMSTATELOW_CLOCK |
		       BCM43xx_SBTMSTATELOW_FORCE_GATE_CLOCK |
		       core_flags;
	bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);

	sbtmstatelow = BCM43xx_SBTMSTATELOW_CLOCK | core_flags;
	bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);

	bcm->current_core->flags |= BCM43xx_COREFLAG_ENABLED;
	assert(err == 0);
out:
	return err;
}

/* http://bcm-specs.sipsolutions.net/80211CoreReset */
void bcm43xx_wireless_core_reset(struct bcm43xx_private *bcm, int connect_phy)
{
	u32 flags = 0x00040000;

	if ((bcm43xx_core_enabled(bcm)) && (!bcm->pio_mode)) {
		/* reset all used DMA controllers. */
		bcm43xx_dmacontroller_tx_reset(bcm, BCM43xx_MMIO_DMA1_BASE);
		bcm43xx_dmacontroller_tx_reset(bcm, BCM43xx_MMIO_DMA2_BASE);
		bcm43xx_dmacontroller_tx_reset(bcm, BCM43xx_MMIO_DMA3_BASE);
		bcm43xx_dmacontroller_tx_reset(bcm, BCM43xx_MMIO_DMA4_BASE);
		bcm43xx_dmacontroller_rx_reset(bcm, BCM43xx_MMIO_DMA1_BASE);
		if (bcm->current_core->rev < 5)
			bcm43xx_dmacontroller_rx_reset(bcm, BCM43xx_MMIO_DMA4_BASE);
	}
	if (bcm->shutting_down) {
		bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD,
		                bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD)
				& ~(BCM43xx_SBF_MAC_ENABLED | 0x00000002));
	} else {
		if (connect_phy)
			flags |= 0x20000000;
		bcm43xx_phy_connect(bcm, connect_phy);
		bcm43xx_core_enable(bcm, flags);
		bcm43xx_write16(bcm, 0x03E6, 0x0000);
		bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD,
				bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD)
				| BCM43xx_SBF_400);
	}
}

static void bcm43xx_wireless_core_disable(struct bcm43xx_private *bcm)
{
	bcm43xx_radio_turn_off(bcm);
	bcm43xx_write16(bcm, 0x03E6, 0x00F4);
	bcm43xx_core_disable(bcm, 0);
}

/* Mark the current 80211 core inactive.
 * "active_80211_core" is the other 80211 core, which is used.
 */
static int bcm43xx_wireless_core_mark_inactive(struct bcm43xx_private *bcm,
					       struct bcm43xx_coreinfo *active_80211_core)
{
	u32 sbtmstatelow;
	struct bcm43xx_coreinfo *old_core;
	int err = 0;

	bcm43xx_interrupt_disable(bcm, BCM43xx_IRQ_ALL);
	bcm43xx_radio_turn_off(bcm);
	sbtmstatelow = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATELOW);
	sbtmstatelow &= ~0x200a0000;
	sbtmstatelow |= 0xa0000;
	bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);
	sbtmstatelow = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATELOW);
	sbtmstatelow &= ~0xa0000;
	sbtmstatelow |= 0x80000;
	bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, sbtmstatelow);
	udelay(1);

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_G) {
		old_core = bcm->current_core;
		err = bcm43xx_switch_core(bcm, active_80211_core);
		if (err)
			goto out;
		sbtmstatelow = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATELOW);
		sbtmstatelow &= ~0x20000000;
		sbtmstatelow |= 0x20000000;
		bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, sbtmstatelow);
		err = bcm43xx_switch_core(bcm, old_core);
	}

out:
	return err;
}

/* Read the Transmit Status from MMIO and build the Transmit Status array. */
static inline int build_transmit_status(struct bcm43xx_private *bcm,
					struct bcm43xx_hwxmitstatus *status)
{
	u32 v170;
	u32 v174;
	u8 tmp[2];

	v170 = bcm43xx_read32(bcm, 0x170);
	if (v170 == 0x00000000)
		return -1;
	v174 = bcm43xx_read32(bcm, 0x174);

	/* Internal Sending ID. */
	status->cookie = cpu_to_le16( (v170 >> 16) & 0x0000FFFF );
	/* 2 counters (both 4 bits) in the upper byte and flags in the lower byte. */
	*((u16 *)tmp) = cpu_to_le16( (u16)((v170 & 0xfff0) | ((v170 & 0xf) >> 1)) );
	status->flags = tmp[0];
	status->cnt1 = (tmp[1] & 0x0f);
	status->cnt2 = (tmp[1] & 0xf0) >> 4;
	/* FIXME: 802.11 sequence number? */
	status->seq = cpu_to_le16( (u16)(v174 & 0xffff) );
	/* FIXME: Unknown. */
	status->unknown = cpu_to_le16( (u16)((v174 >> 16) & 0xff) );

	return 0;
}

static inline void interpret_transmit_status(struct bcm43xx_private *bcm,
					     struct bcm43xx_hwxmitstatus *hwstatus)
{
	struct bcm43xx_xmitstatus status;

	status.cookie = le16_to_cpu(hwstatus->cookie);
	status.flags = hwstatus->flags;
	status.cnt1 = hwstatus->cnt1;
	status.cnt2 = hwstatus->cnt2;
	status.seq = le16_to_cpu(hwstatus->seq);
	status.unknown = le16_to_cpu(hwstatus->unknown);

	bcm43xx_debugfs_log_txstat(bcm, &status);

	if (status.flags & BCM43xx_TXSTAT_FLAG_IGNORE)
		return;
	if (!(status.flags & BCM43xx_TXSTAT_FLAG_ACK)) {
		//TODO: packet was not acked (was lost)
	}
	//TODO: There are more (unknown) flags to test. see bcm43xx_main.h

	if (bcm->pio_mode)
		bcm43xx_pio_handle_xmitstatus(bcm, &status);
	else
		bcm43xx_dma_handle_xmitstatus(bcm, &status);
}

static inline void handle_irq_transmit_status(struct bcm43xx_private *bcm)
{
	assert(bcm->current_core->id == BCM43xx_COREID_80211);

	//TODO: In AP mode, this also causes sending of powersave responses.

	if (bcm->current_core->rev < 5) {
		struct bcm43xx_xmitstatus_queue *q, *tmp;

		/* If we received an xmit status, it is already saved
		 * in the xmit status queue.
		 */
		list_for_each_entry_safe(q, tmp, &bcm->xmitstatus_queue, list) {
			interpret_transmit_status(bcm, &q->status);
			list_del(&q->list);
			bcm->nr_xmitstatus_queued--;
			kfree(q);
		}
		assert(bcm->nr_xmitstatus_queued == 0);
		assert(list_empty(&bcm->xmitstatus_queue));
	} else {
		int res;
		struct bcm43xx_hwxmitstatus transmit_status;

		while (1) {
			res = build_transmit_status(bcm, &transmit_status);
			if (res)
				break;
			interpret_transmit_status(bcm, &transmit_status);
		}
	}
}

static inline void bcm43xx_generate_noise_sample(struct bcm43xx_private *bcm)
{
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x408, 0x7F7F);
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x40A, 0x7F7F);
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS2_BITFIELD,
			bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS2_BITFIELD) | (1 << 4));
	assert(bcm->noisecalc.core_at_start == bcm->current_core);
	assert(bcm->noisecalc.channel_at_start == bcm->current_core->radio->channel);
}

static void bcm43xx_calculate_link_quality(struct bcm43xx_private *bcm)
{
	/* Top half of Link Quality calculation. */

	if (bcm->noisecalc.calculation_running)
		return;
	bcm->noisecalc.core_at_start = bcm->current_core;
	bcm->noisecalc.channel_at_start = bcm->current_core->radio->channel;
	bcm->noisecalc.calculation_running = 1;
	bcm->noisecalc.nr_samples = 0;

	bcm43xx_generate_noise_sample(bcm);
}

static inline void handle_irq_noise(struct bcm43xx_private *bcm)
{
	struct bcm43xx_radioinfo *radio = bcm->current_core->radio;
	u16 tmp;
	u8 noise[4];
	u8 i, j;
	s32 average;

	/* Bottom half of Link Quality calculation. */

	assert(bcm->noisecalc.calculation_running);
	if (bcm->noisecalc.core_at_start != bcm->current_core ||
	    bcm->noisecalc.channel_at_start != radio->channel)
		goto drop_calculation;
	tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x408);
	noise[0] = (tmp & 0x00FF);
	noise[1] = (tmp & 0xFF00) >> 8;
	tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x40A);
	noise[2] = (tmp & 0x00FF);
	noise[3] = (tmp & 0xFF00) >> 8;
	if (noise[0] == 0x7F || noise[1] == 0x7F ||
	    noise[2] == 0x7F || noise[3] == 0x7F)
		goto generate_new;

	/* Get the noise samples. */
	assert(bcm->noisecalc.nr_samples <= 8);
	i = bcm->noisecalc.nr_samples;
	assert(noise[0] < sizeof(radio->nrssi_lt));
	assert(noise[1] < sizeof(radio->nrssi_lt));
	assert(noise[2] < sizeof(radio->nrssi_lt));
	assert(noise[3] < sizeof(radio->nrssi_lt));
	bcm->noisecalc.samples[i][0] = radio->nrssi_lt[noise[0]];
	bcm->noisecalc.samples[i][1] = radio->nrssi_lt[noise[1]];
	bcm->noisecalc.samples[i][2] = radio->nrssi_lt[noise[2]];
	bcm->noisecalc.samples[i][3] = radio->nrssi_lt[noise[3]];
	bcm->noisecalc.nr_samples++;
	if (bcm->noisecalc.nr_samples == 8) {
		/* Calculate the Link Quality by the noise samples. */
		average = 0;
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 4; j++)
				average += bcm->noisecalc.samples[i][j];
		}
		average /= (8 * 4);
		average *= 125;
		average += 64;
		average /= 128;
		tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x40C);
		tmp = (tmp / 128) & 0x1F;
		if (tmp >= 8)
			average += 2;
		else
			average -= 25;
		if (tmp == 8)
			average -= 72;
		else
			average -= 48;

		if (average > -65)
			bcm->stats.link_quality = 0;
		else if (average > -75)
			bcm->stats.link_quality = 1;
		else if (average > -85)
			bcm->stats.link_quality = 2;
		else
			bcm->stats.link_quality = 3;
//		dprintk(KERN_INFO PFX "Link Quality: %u (avg was %d)\n", bcm->stats.link_quality, average);
drop_calculation:
		bcm->noisecalc.calculation_running = 0;
		return;
	}
generate_new:
	bcm43xx_generate_noise_sample(bcm);
}

static inline
void handle_irq_ps(struct bcm43xx_private *bcm)
{
	if (bcm->ieee->iw_mode == IW_MODE_MASTER) {
		///TODO: PS TBTT
	} else {
		if (1/*FIXME: the last PSpoll frame was sent successfully */)
			bcm43xx_power_saving_ctl_bits(bcm, -1, -1);
	}
	if (bcm->ieee->iw_mode == IW_MODE_ADHOC)
		bcm->reg124_set_0x4 = 1;
	//FIXME else set to false?
}

static inline
void handle_irq_reg124(struct bcm43xx_private *bcm)
{
	if (!bcm->reg124_set_0x4)
		return;
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS2_BITFIELD,
			bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS2_BITFIELD)
			| 0x4);
	//FIXME: reset reg124_set_0x4 to false?
}

static inline
void handle_irq_pmq(struct bcm43xx_private *bcm)
{
	u32 tmp;

	//TODO: AP mode.

	while (1) {
		tmp = bcm43xx_read32(bcm, BCM43xx_MMIO_PS_STATUS);
		if (!(tmp & 0x00000008))
			break;
	}
	/* 16bit write is odd, but correct. */
	bcm43xx_write16(bcm, BCM43xx_MMIO_PS_STATUS, 0x0002);
}

static inline
void handle_irq_beacon(struct bcm43xx_private *bcm)
{
	if (bcm->ieee->iw_mode != IW_MODE_MASTER)
		return;
	//TODO: UpdateBeaconPacket
}

/* Debug helper for irq bottom-half to print all reason registers. */
#define bcmirq_print_reasons(description) \
	do {											\
		dprintkl(KERN_ERR PFX description "\n"						\
			 KERN_ERR PFX "  Generic Reason: 0x%08x\n"				\
			 KERN_ERR PFX "  DMA reasons:    0x%08x, 0x%08x, 0x%08x, 0x%08x\n"	\
			 KERN_ERR PFX "  DMA TX status:  0x%08x, 0x%08x, 0x%08x, 0x%08x\n",	\
			 reason,								\
			 dma_reason[0], dma_reason[1],						\
			 dma_reason[2], dma_reason[3],						\
			 bcm43xx_read32(bcm, BCM43xx_MMIO_DMA1_BASE + BCM43xx_DMA_TX_STATUS),	\
			 bcm43xx_read32(bcm, BCM43xx_MMIO_DMA2_BASE + BCM43xx_DMA_TX_STATUS),	\
			 bcm43xx_read32(bcm, BCM43xx_MMIO_DMA3_BASE + BCM43xx_DMA_TX_STATUS),	\
			 bcm43xx_read32(bcm, BCM43xx_MMIO_DMA4_BASE + BCM43xx_DMA_TX_STATUS));	\
	} while (0)

/* Interrupt handler bottom-half */
static void bcm43xx_interrupt_tasklet(struct bcm43xx_private *bcm)
{
	u32 reason;
	u32 dma_reason[4];
	unsigned long flags;

#ifdef BCM43xx_DEBUG
	u32 _handled = 0x00000000;
# define bcmirq_handled(irq)	do { _handled |= (irq); } while (0)
#else
# define bcmirq_handled(irq)	do { /* nothing */ } while (0)
#endif /* BCM43xx_DEBUG */

	spin_lock_irqsave(&bcm->lock, flags);
	reason = bcm->irq_reason;
	dma_reason[0] = bcm->dma_reason[0];
	dma_reason[1] = bcm->dma_reason[1];
	dma_reason[2] = bcm->dma_reason[2];
	dma_reason[3] = bcm->dma_reason[3];

	if (unlikely(reason & BCM43xx_IRQ_XMIT_ERROR)) {
		/* TX error. We get this when Template Ram is written in wrong endianess
		 * in dummy_tx(). We also get this if something is wrong with the TX header
		 * on DMA or PIO queues.
		 * Maybe we get this in other error conditions, too.
		 */
		bcmirq_print_reasons("XMIT ERROR");
		bcmirq_handled(BCM43xx_IRQ_XMIT_ERROR);
	}

	if (reason & BCM43xx_IRQ_PS) {
		handle_irq_ps(bcm);
		bcmirq_handled(BCM43xx_IRQ_PS);
	}

	if (reason & BCM43xx_IRQ_REG124) {
		handle_irq_reg124(bcm);
		bcmirq_handled(BCM43xx_IRQ_REG124);
	}

	if (reason & BCM43xx_IRQ_BEACON) {
		handle_irq_beacon(bcm);
		bcmirq_handled(BCM43xx_IRQ_BEACON);
	}

	if (reason & BCM43xx_IRQ_PMQ) {
		handle_irq_pmq(bcm);
		bcmirq_handled(BCM43xx_IRQ_PMQ);
	}

	if (reason & BCM43xx_IRQ_SCAN) {
		/*TODO*/
		//bcmirq_handled(BCM43xx_IRQ_SCAN);
	}

	if (reason & BCM43xx_IRQ_NOISE) {
		handle_irq_noise(bcm);
		bcmirq_handled(BCM43xx_IRQ_NOISE);
	}

	/* Check the DMA reason registers for received data. */
	assert(!(dma_reason[1] & BCM43xx_DMAIRQ_RX_DONE));
	assert(!(dma_reason[2] & BCM43xx_DMAIRQ_RX_DONE));
	if (dma_reason[0] & BCM43xx_DMAIRQ_RX_DONE) {
		if (bcm->pio_mode)
			bcm43xx_pio_rx(bcm->current_core->pio->queue0);
		else
			bcm43xx_dma_rx(bcm->current_core->dma->rx_ring0);
	}
	if (dma_reason[3] & BCM43xx_DMAIRQ_RX_DONE) {
		if (likely(bcm->current_core->rev < 5)) {
			if (bcm->pio_mode)
				bcm43xx_pio_rx(bcm->current_core->pio->queue3);
			else
				bcm43xx_dma_rx(bcm->current_core->dma->rx_ring1);
		} else
			assert(0);
	}
	bcmirq_handled(BCM43xx_IRQ_RX);

	if (reason & BCM43xx_IRQ_XMIT_STATUS) {
		handle_irq_transmit_status(bcm);
		bcmirq_handled(BCM43xx_IRQ_XMIT_STATUS);
	}

	/* We get spurious IRQs, althought they are masked.
	 * Assume they are void and ignore them.
	 */
	bcmirq_handled(~(bcm->irq_savedstate));
	/* IRQ_PIO_WORKAROUND is handled in the top-half. */
	bcmirq_handled(BCM43xx_IRQ_PIO_WORKAROUND);
#ifdef BCM43xx_DEBUG
	if (unlikely(reason & ~_handled)) {
		printkl(KERN_WARNING PFX
			"Unhandled IRQ! Reason: 0x%08x,  Unhandled: 0x%08x,  "
			"DMA: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			reason, (reason & ~_handled),
			dma_reason[0], dma_reason[1],
			dma_reason[2], dma_reason[3]);
	}
#endif
#undef bcmirq_handled

	bcm43xx_interrupt_enable(bcm, bcm->irq_savedstate);
	spin_unlock_irqrestore(&bcm->lock, flags);
}

#undef bcmirq_print_reasons

static inline
void bcm43xx_interrupt_ack(struct bcm43xx_private *bcm,
			   u32 reason, u32 mask)
{
	bcm->dma_reason[0] = bcm43xx_read32(bcm, BCM43xx_MMIO_DMA1_REASON)
			     & 0x0001dc00;
	bcm->dma_reason[1] = bcm43xx_read32(bcm, BCM43xx_MMIO_DMA2_REASON)
			     & 0x0000dc00;
	bcm->dma_reason[2] = bcm43xx_read32(bcm, BCM43xx_MMIO_DMA3_REASON)
			     & 0x0000dc00;
	bcm->dma_reason[3] = bcm43xx_read32(bcm, BCM43xx_MMIO_DMA4_REASON)
			     & 0x0001dc00;

	if ((bcm->pio_mode) &&
	    (bcm->current_core->rev < 3) &&
	    (!(reason & BCM43xx_IRQ_PIO_WORKAROUND))) {
		/* Apply a PIO specific workaround to the dma_reasons */

#define apply_pio_workaround(BASE, QNUM) \
	do {											\
	if (bcm43xx_read16(bcm, BASE + BCM43xx_PIO_RXCTL) & BCM43xx_PIO_RXCTL_DATAAVAILABLE)	\
		bcm->dma_reason[QNUM] |= 0x00010000;						\
	else											\
		bcm->dma_reason[QNUM] &= ~0x00010000;						\
	} while (0)

		apply_pio_workaround(BCM43xx_MMIO_PIO1_BASE, 0);
		apply_pio_workaround(BCM43xx_MMIO_PIO2_BASE, 1);
		apply_pio_workaround(BCM43xx_MMIO_PIO3_BASE, 2);
		apply_pio_workaround(BCM43xx_MMIO_PIO4_BASE, 3);

#undef apply_pio_workaround
	}

	bcm43xx_write32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON,
			reason & mask);

	bcm43xx_write32(bcm, BCM43xx_MMIO_DMA1_REASON,
			bcm->dma_reason[0]);
	bcm43xx_write32(bcm, BCM43xx_MMIO_DMA2_REASON,
			bcm->dma_reason[1]);
	bcm43xx_write32(bcm, BCM43xx_MMIO_DMA3_REASON,
			bcm->dma_reason[2]);
	bcm43xx_write32(bcm, BCM43xx_MMIO_DMA4_REASON,
			bcm->dma_reason[3]);
}

/* Interrupt handler top-half */
static irqreturn_t bcm43xx_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	struct bcm43xx_private *bcm = dev_id;
	u32 reason, mask;

	if (!bcm)
		return IRQ_NONE;

	spin_lock(&bcm->lock);

	reason = bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON);
	if (reason == 0xffffffff) {
		/* irq not for us (shared irq) */
		spin_unlock(&bcm->lock);
		return IRQ_NONE;
	}
	mask = bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_MASK);
	if (!(reason & mask)) {
		spin_unlock(&bcm->lock);
		return IRQ_HANDLED;
	}

	bcm43xx_interrupt_ack(bcm, reason, mask);

	/* disable all IRQs. They are enabled again in the bottom half. */
	bcm->irq_savedstate = bcm43xx_interrupt_disable(bcm, BCM43xx_IRQ_ALL);

	/* save the reason code and call our bottom half. */
	bcm->irq_reason = reason;
	tasklet_schedule(&bcm->isr_tasklet);

	spin_unlock(&bcm->lock);

	return IRQ_HANDLED;
}

static inline void bcm43xx_write_microcode(struct bcm43xx_private *bcm,
				    const u32 *data, const unsigned int len)
{
	unsigned int i;
	bcm43xx_shm_control_word(bcm, BCM43xx_SHM_UCODE, 0x0000);
	for (i = 0; i < len; i++) {
		bcm43xx_write32(bcm, BCM43xx_MMIO_SHM_DATA,
				be32_to_cpu(data[i]));
		udelay(10);
	}
}

static inline void bcm43xx_write_pcm(struct bcm43xx_private *bcm,
			      const u32 *data, const unsigned int len)
{
	unsigned int i;
	bcm43xx_shm_control_word(bcm, BCM43xx_SHM_PCM, 0x01ea);
	bcm43xx_write32(bcm, BCM43xx_MMIO_SHM_DATA, 0x00004000);
	bcm43xx_shm_control_word(bcm, BCM43xx_SHM_PCM, 0x01eb);
	for (i = 0; i < len; i++) {
		bcm43xx_write32(bcm, BCM43xx_MMIO_SHM_DATA,
				be32_to_cpu(data[i]));
		udelay(10);
	}
}

static int bcm43xx_upload_microcode(struct bcm43xx_private *bcm)
{
	int err = -ENOENT;
	const struct firmware *fw;
	char buf[22 + sizeof(modparam_fwpostfix) - 1] = { 0 };

#ifdef DEBUG_ENABLE_UCODE_MMIO_PRINT
	bcm43xx_mmioprint_enable(bcm);
#else
	bcm43xx_mmioprint_disable(bcm);
#endif

	snprintf(buf, ARRAY_SIZE(buf), "bcm43xx_microcode%d%s.fw",
		 (bcm->current_core->rev >= 5 ? 5 : bcm->current_core->rev),
		 modparam_fwpostfix);
	if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
		printk(KERN_ERR PFX 
		       "Error: Microcode \"%s\" not available or load failed.\n",
		        buf);
		goto out;
	}
	bcm43xx_write_microcode(bcm, (u32 *)fw->data, fw->size / sizeof(u32));
	release_firmware(fw);

	snprintf(buf, ARRAY_SIZE(buf),
		 "bcm43xx_pcm%d%s.fw",
		 (bcm->current_core->rev < 5 ? 4 : 5),
		 modparam_fwpostfix);
	if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
		printk(KERN_ERR PFX
		       "Error: PCM \"%s\" not available or load failed.\n",
		       buf);
		goto out;
	}
	bcm43xx_write_pcm(bcm, (u32 *)fw->data, fw->size / sizeof(u32));
	release_firmware(fw);

	err = 0;
out:
#ifdef DEBUG_ENABLE_UCODE_MMIO_PRINT
	bcm43xx_mmioprint_disable(bcm);
#else
	bcm43xx_mmioprint_enable(bcm);
#endif
	return err;
}

static void bcm43xx_write_initvals(struct bcm43xx_private *bcm,
				   const struct bcm43xx_initval *data,
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
			bcm43xx_write16(bcm, offset, value);
		else if (size == 4)
			bcm43xx_write32(bcm, offset, value);
		else
			printk(KERN_ERR PFX "InitVals fileformat error.\n");
	}
}

static int bcm43xx_upload_initvals(struct bcm43xx_private *bcm)
{
	int err = -ENOENT;
	u32 sbtmstatehigh;
	const struct firmware *fw;
	char buf[21 + sizeof(modparam_fwpostfix) - 1] = { 0 };

#ifdef DEBUG_ENABLE_UCODE_MMIO_PRINT
	bcm43xx_mmioprint_enable(bcm);
#else
	bcm43xx_mmioprint_disable(bcm);
#endif

	if ((bcm->current_core->rev == 2) || (bcm->current_core->rev == 4)) {
		switch (bcm->current_core->phy->type) {
			case BCM43xx_PHYTYPE_A:
				snprintf(buf, ARRAY_SIZE(buf), "bcm43xx_initval%02d%s.fw",
					 3, modparam_fwpostfix);
				break;
			case BCM43xx_PHYTYPE_B:
			case BCM43xx_PHYTYPE_G:
				snprintf(buf, ARRAY_SIZE(buf), "bcm43xx_initval%02d%s.fw",
					 1, modparam_fwpostfix);
				break;
			default:
				goto out_noinitval;
		}
	
	} else if (bcm->current_core->rev >= 5) {
		switch (bcm->current_core->phy->type) {
			case BCM43xx_PHYTYPE_A:
				snprintf(buf, ARRAY_SIZE(buf), "bcm43xx_initval%02d%s.fw",
					 7, modparam_fwpostfix);
				break;
			case BCM43xx_PHYTYPE_B:
			case BCM43xx_PHYTYPE_G:
				snprintf(buf, ARRAY_SIZE(buf), "bcm43xx_initval%02d%s.fw",
					 5, modparam_fwpostfix);
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
	if (fw->size % sizeof(struct bcm43xx_initval)) {
		printk(KERN_ERR PFX "InitVals fileformat error.\n");
		release_firmware(fw);
		goto out;
	}

	bcm43xx_write_initvals(bcm, (struct bcm43xx_initval *)fw->data,
			       fw->size / sizeof(struct bcm43xx_initval));

	release_firmware(fw);

	if (bcm->current_core->rev >= 5) {
		switch (bcm->current_core->phy->type) {
			case BCM43xx_PHYTYPE_A:
				sbtmstatehigh = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATEHIGH);
				if (sbtmstatehigh & 0x00010000)
					snprintf(buf, ARRAY_SIZE(buf), "bcm43xx_initval%02d%s.fw",
						 9, modparam_fwpostfix);
				else
					snprintf(buf, ARRAY_SIZE(buf), "bcm43xx_initval%02d%s.fw",
						 10, modparam_fwpostfix);
				break;
			case BCM43xx_PHYTYPE_B:
			case BCM43xx_PHYTYPE_G:
					snprintf(buf, ARRAY_SIZE(buf), "bcm43xx_initval%02d%s.fw",
						 6, modparam_fwpostfix);
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
		if (fw->size % sizeof(struct bcm43xx_initval)) {
			printk(KERN_ERR PFX "InitVals fileformat error.\n");
			release_firmware(fw);
			goto out;
		}

		bcm43xx_write_initvals(bcm, (struct bcm43xx_initval *)fw->data,
				       fw->size / sizeof(struct bcm43xx_initval));
	
		release_firmware(fw);
		
	}

	dprintk(KERN_INFO PFX "InitVals written\n");
	err = 0;
out:
#ifdef DEBUG_ENABLE_UCODE_MMIO_PRINT
	bcm43xx_mmioprint_disable(bcm);
#else
	bcm43xx_mmioprint_enable(bcm);
#endif

	return err;

out_noinitval:
	printk(KERN_ERR PFX "Error: No InitVals available!\n");
	goto out;
}

static int bcm43xx_initialize_irq(struct bcm43xx_private *bcm)
{
	int res;
	unsigned int i;
	u32 data;

	res = request_irq(bcm->pci_dev->irq, bcm43xx_interrupt_handler,
			  SA_SHIRQ, DRV_NAME, bcm);
	if (res) {
		printk(KERN_ERR PFX "Cannot register IRQ%d\n", bcm->pci_dev->irq);
		return -EFAULT;
	}
	bcm43xx_write32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON, 0xffffffff);
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, 0x00020402);
	i = 0;
	while (1) {
		data = bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON);
		if (data == BCM43xx_IRQ_READY)
			break;
		i++;
		if (i >= BCM43xx_IRQWAIT_MAX_RETRIES) {
			printk(KERN_ERR PFX "Card IRQ register not responding. "
					    "Giving up.\n");
			free_irq(bcm->pci_dev->irq, bcm);
			return -ENODEV;
		}
		udelay(10);
	}
	// dummy read
	bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON);

	return 0;
}

/* Keep this slim, as we're going to call it from within the interrupt tasklet! */
static void bcm43xx_update_leds(struct bcm43xx_private *bcm)
{
	int id;
	u16 value = bcm43xx_read16(bcm, BCM43xx_MMIO_GPIO_CONTROL);
	u16 state;

	for (id = 0; id < BCM43xx_LED_COUNT; id++) {
		if (bcm->leds[id] == 0xFF)
			continue;

		state = 0;

		switch (bcm->leds[id] & ~BCM43xx_LED_ACTIVELOW) {
		case BCM43xx_LED_OFF:
			break;
		case BCM43xx_LED_ON:
			state = 1;
			break;
		case BCM43xx_LED_RADIO_ALL:
			state = ((bcm->radio[0].enabled) || (bcm->radio[1].enabled)) ? 1 : 0;
			break;
		case BCM43xx_LED_RADIO_A:
			if ((bcm->phy[0].type == BCM43xx_PHYTYPE_A) && (bcm->radio[0].enabled))
				state = 1;
			if ((bcm->phy[1].type == BCM43xx_PHYTYPE_A) && (bcm->radio[1].enabled))
				state = 1;
			break;
		case BCM43xx_LED_RADIO_B:
			if ((bcm->phy[0].type == BCM43xx_PHYTYPE_B) && (bcm->radio[0].enabled))
				state = 1;
			if ((bcm->phy[1].type == BCM43xx_PHYTYPE_B) && (bcm->radio[1].enabled))
				state = 1;
			break;
		/*
		 * TODO: LED_ACTIVITY, MODE_BG, ASSOC
		 */
		default:
			break;
		};

		if (bcm->leds[id] & BCM43xx_LED_ACTIVELOW)
			state = 0x0001 & (state ^ 0x0001);
		value &= ~(1 << id);
		value |= (state << id);
	}

	bcm43xx_write16(bcm, BCM43xx_MMIO_GPIO_CONTROL, value);
}

/* Switch to the core used to write the GPIO register.
 * This is either the ChipCommon, or the PCI core.
 */
static inline int switch_to_gpio_core(struct bcm43xx_private *bcm)
{
	int err;

	/* Where to find the GPIO register depends on the chipset.
	 * If it has a ChipCommon, its register at offset 0x6c is the GPIO
	 * control register. Otherwise the register at offset 0x6c in the
	 * PCI core is the GPIO control register.
	 */
	err = bcm43xx_switch_core(bcm, &bcm->core_chipcommon);
	if (err == -ENODEV) {
		err = bcm43xx_switch_core(bcm, &bcm->core_pci);
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
static int bcm43xx_gpio_init(struct bcm43xx_private *bcm)
{
	struct bcm43xx_coreinfo *old_core;
	int err;
	u32 mask, value;

	value = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
	value &= ~0xc000;
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, value);

	mask = 0x0000001F;
	value = 0x0000000F;
	bcm43xx_write16(bcm, BCM43xx_MMIO_GPIO_CONTROL,
			bcm43xx_read16(bcm, BCM43xx_MMIO_GPIO_CONTROL) & 0xFFF0);
	bcm43xx_write16(bcm, BCM43xx_MMIO_GPIO_MASK,
			bcm43xx_read16(bcm, BCM43xx_MMIO_GPIO_MASK) | 0x000F);

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
	if (bcm->sprom.boardflags & BCM43xx_BFL_PACTRL) {
		mask  |= 0x200;
		value |= 0x200;
	}

	bcm43xx_write32(bcm, BCM43xx_GPIO_CONTROL,
	                (bcm43xx_read32(bcm, BCM43xx_GPIO_CONTROL) & mask) | value);

	err = bcm43xx_switch_core(bcm, old_core);
	assert(err == 0);

	return 0;
}

/* Turn off all GPIO stuff. Call this on module unload, for example. */
static int bcm43xx_gpio_cleanup(struct bcm43xx_private *bcm)
{
	struct bcm43xx_coreinfo *old_core;
	int err;

	old_core = bcm->current_core;
	err = switch_to_gpio_core(bcm);
	if (err)
		return err;
	bcm43xx_write32(bcm, BCM43xx_GPIO_CONTROL, 0x00000000);
	err = bcm43xx_switch_core(bcm, old_core);
	assert(err == 0);

	return 0;
}

/* http://bcm-specs.sipsolutions.net/EnableMac */
void bcm43xx_mac_enable(struct bcm43xx_private *bcm)
{
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD,
	                bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD)
			| BCM43xx_SBF_MAC_ENABLED);
	bcm43xx_write32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON, BCM43xx_IRQ_READY);
	bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD); /* dummy read */
	bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON); /* dummy read */
	bcm43xx_power_saving_ctl_bits(bcm, -1, -1);
}

/* http://bcm-specs.sipsolutions.net/SuspendMAC */
void bcm43xx_mac_suspend(struct bcm43xx_private *bcm)
{
	int i;
	u32 tmp;

	bcm43xx_power_saving_ctl_bits(bcm, -1, 1);
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD,
	                bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD)
			& ~BCM43xx_SBF_MAC_ENABLED);
	bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON); /* dummy read */
	for (i = 1000; i > 0; i--) {
		tmp = bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON);
		if (tmp & BCM43xx_IRQ_READY) {
			i = -1;
			break;
		}
		udelay(10);
	}
	if (!i)
		printkl(KERN_ERR PFX "Failed to suspend mac!\n");
}

void bcm43xx_set_iwmode(struct bcm43xx_private *bcm,
			int iw_mode)
{
	unsigned long flags;

	spin_lock_irqsave(&bcm->ieee->lock, flags);
	bcm->ieee->iw_mode = iw_mode;
	spin_unlock_irqrestore(&bcm->ieee->lock, flags);
	if (iw_mode == IW_MODE_MONITOR)
		bcm->net_dev->type = ARPHRD_IEEE80211;
	else
		bcm->net_dev->type = ARPHRD_ETHER;

	if (!bcm->initialized)
		return;

	TODO();//TODO
}

/* This is the opposite of bcm43xx_chip_init() */
static void bcm43xx_chip_cleanup(struct bcm43xx_private *bcm)
{
	bcm43xx_radio_turn_off(bcm);
	bcm43xx_gpio_cleanup(bcm);
	free_irq(bcm->pci_dev->irq, bcm);
}

/* Initialize the chip
 * http://bcm-specs.sipsolutions.net/ChipInit
 */
static int bcm43xx_chip_init(struct bcm43xx_private *bcm)
{
	int err;
	int iw_mode = bcm->ieee->iw_mode;
	int tmp;
	u32 value32;
	u16 value16;

	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD,
			BCM43xx_SBF_CORE_READY
			| BCM43xx_SBF_400);

	err = bcm43xx_upload_microcode(bcm);
	if (err)
		goto out;

	err = bcm43xx_initialize_irq(bcm);
	if (err)
		goto out;

	err = bcm43xx_gpio_init(bcm);
	if (err)
		goto err_free_irq;

	err = bcm43xx_upload_initvals(bcm);
	if (err)
		goto err_gpio_cleanup;

	err = bcm43xx_radio_turn_on(bcm);
	if (err)
		goto err_gpio_cleanup;

	bcm43xx_update_leds(bcm);

	bcm43xx_write16(bcm, 0x03E6, 0x0000);
	err = bcm43xx_phy_init(bcm);
	if (err)
		goto err_radio_off;

	/* Select initial Interference Mitigation. */
	tmp = bcm->current_core->radio->interfmode;
	bcm->current_core->radio->interfmode = BCM43xx_RADIO_INTERFMODE_NONE;
	bcm43xx_radio_set_interference_mitigation(bcm, tmp);

	bcm43xx_phy_set_antenna_diversity(bcm);
	bcm43xx_radio_set_txantenna(bcm, BCM43xx_RADIO_DEFAULT_ANTENNA);
	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_B) {
		value16 = bcm43xx_read16(bcm, 0x005E);
		value16 |= 0x0004;
		bcm43xx_write16(bcm, 0x005E, value16);
	}
	bcm43xx_write32(bcm, 0x0100, 0x01000000);
	bcm43xx_write32(bcm, 0x010C, 0x01000000);

	value32 = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
	value32 &= ~ BCM43xx_SBF_MODE_NOTADHOC;
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, value32);
	value32 = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
	value32 |= BCM43xx_SBF_MODE_NOTADHOC;
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, value32);

	if ((iw_mode == IW_MODE_MASTER) && (bcm->net_dev->flags & IFF_PROMISC)) {
		value32 = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
		value32 |= BCM43xx_SBF_MODE_PROMISC;
		bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, value32);
	} else if (iw_mode == IW_MODE_MONITOR) {
		value32 = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
		value32 |= BCM43xx_SBF_MODE_PROMISC;
		value32 |= BCM43xx_SBF_MODE_MONITOR;
		bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, value32);
	}
	value32 = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
	value32 |= 0x100000; //FIXME: What's this? Is this correct?
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, value32);

	if (bcm->pio_mode) {
		bcm43xx_write32(bcm, 0x0210, 0x00000100);
		bcm43xx_write32(bcm, 0x0230, 0x00000100);
		bcm43xx_write32(bcm, 0x0250, 0x00000100);
		bcm43xx_write32(bcm, 0x0270, 0x00000100);
		bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0034, 0x0000);
	}

	/* Probe Response Timeout value */
	/* FIXME: Default to 0, has to be set by ioctl probably... :-/ */
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0074, 0x0000);

	if (iw_mode != IW_MODE_ADHOC && iw_mode != IW_MODE_MASTER) {
		if ((bcm->chip_id == 0x4306) && (bcm->chip_rev == 3))
			bcm43xx_write16(bcm, 0x0612, 0x0064);
		else
			bcm43xx_write16(bcm, 0x0612, 0x0032);
	} else
		bcm43xx_write16(bcm, 0x0612, 0x0002);

	if (bcm->current_core->rev < 3) {
		bcm43xx_write16(bcm, 0x060E, 0x0000);
		bcm43xx_write16(bcm, 0x0610, 0x8000);
		bcm43xx_write16(bcm, 0x0604, 0x0000);
		bcm43xx_write16(bcm, 0x0606, 0x0200);
	} else {
		bcm43xx_write32(bcm, 0x0188, 0x80000000);
		bcm43xx_write32(bcm, 0x018C, 0x02000000);
	}
	bcm43xx_write32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON, 0x00004000);
	bcm43xx_write32(bcm, BCM43xx_MMIO_DMA1_IRQ_MASK, 0x0001DC00);
	bcm43xx_write32(bcm, BCM43xx_MMIO_DMA2_IRQ_MASK, 0x0000DC00);
	bcm43xx_write32(bcm, BCM43xx_MMIO_DMA3_IRQ_MASK, 0x0000DC00);
	bcm43xx_write32(bcm, BCM43xx_MMIO_DMA4_IRQ_MASK, 0x0001DC00);

	value32 = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATELOW);
	value32 |= 0x00100000;
	bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, value32);

	bcm43xx_write16(bcm, BCM43xx_MMIO_POWERUP_DELAY, bcm43xx_pctl_powerup_delay(bcm));

	assert(err == 0);
	dprintk(KERN_INFO PFX "Chip initialized\n");
out:
	return err;

err_radio_off:
	bcm43xx_radio_turn_off(bcm);
err_gpio_cleanup:
	bcm43xx_gpio_cleanup(bcm);
err_free_irq:
	free_irq(bcm->pci_dev->irq, bcm);
	goto out;
}
	
/* Validate chip access
 * http://bcm-specs.sipsolutions.net/ValidateChipAccess */
static int bcm43xx_validate_chip(struct bcm43xx_private *bcm)
{
	int err = -ENODEV;
	u32 value;
	u32 shm_backup;

	shm_backup = bcm43xx_shm_read32(bcm, BCM43xx_SHM_SHARED, 0x0000);
	bcm43xx_shm_write32(bcm, BCM43xx_SHM_SHARED, 0x0000, 0xAA5555AA);
	if (bcm43xx_shm_read32(bcm, BCM43xx_SHM_SHARED, 0x0000) != 0xAA5555AA) {
		printk(KERN_ERR PFX "Error: SHM mismatch (1) validating chip\n");
		goto out;
	}

	bcm43xx_shm_write32(bcm, BCM43xx_SHM_SHARED, 0x0000, 0x55AAAA55);
	if (bcm43xx_shm_read32(bcm, BCM43xx_SHM_SHARED, 0x0000) != 0x55AAAA55) {
		printk(KERN_ERR PFX "Error: SHM mismatch (2) validating chip\n");
		goto out;
	}

	bcm43xx_shm_write32(bcm, BCM43xx_SHM_SHARED, 0x0000, shm_backup);

	value = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
	if ((value | 0x80000000) != 0x80000400) {
		printk(KERN_ERR PFX "Error: Bad Status Bitfield while validating chip\n");
		goto out;
	}

	value = bcm43xx_read32(bcm, BCM43xx_MMIO_GEN_IRQ_REASON);
	if (value != 0x00000000) {
		printk(KERN_ERR PFX "Error: Bad interrupt reason code while validating chip\n");
		goto out;
	}

	err = 0;
out:
	return err;
}

static int bcm43xx_probe_cores(struct bcm43xx_private *bcm)
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
	sb_id_hi = bcm43xx_read32(bcm, BCM43xx_CIR_SB_ID_HI);

	core_id = (sb_id_hi & 0xFFF0) >> 4;
	core_rev = (sb_id_hi & 0xF);
	core_vendor = (sb_id_hi & 0xFFFF0000) >> 16;

	/* if present, chipcommon is always core 0; read the chipid from it */
	if (core_id == BCM43xx_COREID_CHIPCOMMON) {
		chip_id_32 = bcm43xx_read32(bcm, 0);
		chip_id_16 = chip_id_32 & 0xFFFF;
		bcm->core_chipcommon.flags |= BCM43xx_COREFLAG_AVAILABLE;
		bcm->core_chipcommon.id = core_id;
		bcm->core_chipcommon.rev = core_rev;
		bcm->core_chipcommon.index = 0;
		/* While we are at it, also read the capabilities. */
		bcm->chipcommon_capabilities = bcm43xx_read32(bcm, BCM43xx_CHIPCOMMON_CAPABILITIES);
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
	if ((core_id == BCM43xx_COREID_CHIPCOMMON) && (core_rev >= 4)) {
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
	if (bcm->core_chipcommon.flags & BCM43xx_COREFLAG_AVAILABLE) {
		dprintk(KERN_INFO PFX "Core 0: ID 0x%x, rev 0x%x, vendor 0x%x, %s\n",
			core_id, core_rev, core_vendor,
			bcm43xx_core_enabled(bcm) ? "enabled" : "disabled");
	}

	if (bcm->core_chipcommon.flags & BCM43xx_COREFLAG_AVAILABLE)
		current_core = 1;
	else
		current_core = 0;
	for ( ; current_core < core_count; current_core++) {
		struct bcm43xx_coreinfo *core;

		err = _switch_core(bcm, current_core);
		if (err)
			goto out;
		/* Gather information */
		/* fetch sb_id_hi from core information registers */
		sb_id_hi = bcm43xx_read32(bcm, BCM43xx_CIR_SB_ID_HI);

		/* extract core_id, core_rev, core_vendor */
		core_id = (sb_id_hi & 0xFFF0) >> 4;
		core_rev = (sb_id_hi & 0xF);
		core_vendor = (sb_id_hi & 0xFFFF0000) >> 16;

		dprintk(KERN_INFO PFX "Core %d: ID 0x%x, rev 0x%x, vendor 0x%x, %s\n",
			current_core, core_id, core_rev, core_vendor,
			bcm43xx_core_enabled(bcm) ? "enabled" : "disabled" );

		core = NULL;
		switch (core_id) {
		case BCM43xx_COREID_PCI:
			core = &bcm->core_pci;
			if (core->flags & BCM43xx_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple PCI cores found.\n");
				continue;
			}
			memset(core, 0, sizeof(*core));
			break;
		case BCM43xx_COREID_V90:
			core = &bcm->core_v90;
			if (core->flags & BCM43xx_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple V90 cores found.\n");
				continue;
			}
			memset(core, 0, sizeof(*core));
			break;
		case BCM43xx_COREID_PCMCIA:
			core = &bcm->core_pcmcia;
			if (core->flags & BCM43xx_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple PCMCIA cores found.\n");
				continue;
			}
			memset(core, 0, sizeof(*core));
			break;
		case BCM43xx_COREID_ETHERNET:
			core = &bcm->core_ethernet;
			if (core->flags & BCM43xx_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple Ethernet cores found.\n");
				continue;
			}
			memset(core, 0, sizeof(*core));
			break;
		case BCM43xx_COREID_80211:
			for (i = 0; i < BCM43xx_MAX_80211_CORES; i++) {
				core = &(bcm->core_80211[i]);
				if (!(core->flags & BCM43xx_COREFLAG_AVAILABLE))
					break;
				core = NULL;
			}
			if (!core) {
				printk(KERN_WARNING PFX "More than %d cores of type 802.11 found.\n",
				       BCM43xx_MAX_80211_CORES);
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
			core->radio->interfmode = BCM43xx_RADIO_INTERFMODE_NONE; //FIXME: Set to AUTO?
			core->radio->channel = 0xFF;
			core->radio->initial_channel = 0xFF;
			core->radio->lofcal = 0xFFFF;
			core->radio->initval = 0xFFFF;
			core->radio->nrssi[0] = -1000;
			core->radio->nrssi[1] = -1000;
			core->dma = &bcm->dma[i];
			core->pio = &bcm->pio[i];
			break;
		case BCM43xx_COREID_CHIPCOMMON:
			printk(KERN_WARNING PFX "Multiple CHIPCOMMON cores found.\n");
			break;
		default:
			printk(KERN_WARNING PFX "Unknown core found (ID 0x%x)\n", core_id);
		}
		if (core) {
			core->flags |= BCM43xx_COREFLAG_AVAILABLE;
			core->id = core_id;
			core->rev = core_rev;
			core->index = current_core;
		}
	}

	if (!(bcm->core_80211[0].flags & BCM43xx_COREFLAG_AVAILABLE)) {
		printk(KERN_ERR PFX "Error: No 80211 core found!\n");
		err = -ENODEV;
		goto out;
	}

	err = bcm43xx_switch_core(bcm, &bcm->core_80211[0]);

	assert(err == 0);
out:
	return err;
}

static void bcm43xx_gen_bssid(struct bcm43xx_private *bcm)
{
	const u8 *mac = (const u8*)(bcm->net_dev->dev_addr);
	u8 *bssid = bcm->ieee->bssid;

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
		memcpy(bssid, mac, ETH_ALEN);
		break;
	default:
		assert(0);
	}
}

static void bcm43xx_rate_memory_write(struct bcm43xx_private *bcm,
				      u16 double_rate,
				      int is_ofdm)
{
	u16 offset;

	if (is_ofdm)
		offset = 0x480;
	else
		offset = 0x4C0;
	offset += (double_rate & 0x000F);
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, offset + 0x20,
			    bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, offset));
}

static void bcm43xx_rate_memory_init(struct bcm43xx_private *bcm)
{
	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_A:
	case BCM43xx_PHYTYPE_G:
		bcm43xx_rate_memory_write(bcm, IEEE80211_OFDM_RATE_6MB, 1);
		bcm43xx_rate_memory_write(bcm, IEEE80211_OFDM_RATE_12MB, 1);
		bcm43xx_rate_memory_write(bcm, IEEE80211_OFDM_RATE_18MB, 1);
		bcm43xx_rate_memory_write(bcm, IEEE80211_OFDM_RATE_24MB, 1);
		bcm43xx_rate_memory_write(bcm, IEEE80211_OFDM_RATE_36MB, 1);
		bcm43xx_rate_memory_write(bcm, IEEE80211_OFDM_RATE_48MB, 1);
		bcm43xx_rate_memory_write(bcm, IEEE80211_OFDM_RATE_54MB, 1);
	case BCM43xx_PHYTYPE_B:
		bcm43xx_rate_memory_write(bcm, IEEE80211_CCK_RATE_1MB, 0);
		bcm43xx_rate_memory_write(bcm, IEEE80211_CCK_RATE_2MB, 0);
		bcm43xx_rate_memory_write(bcm, IEEE80211_CCK_RATE_5MB, 0);
		bcm43xx_rate_memory_write(bcm, IEEE80211_CCK_RATE_11MB, 0);
		break;
	default:
		assert(0);
	}
}

static void bcm43xx_wireless_core_cleanup(struct bcm43xx_private *bcm)
{
	bcm43xx_chip_cleanup(bcm);
	bcm43xx_pio_free(bcm);
	bcm43xx_dma_free(bcm);

	bcm->current_core->flags &= ~ BCM43xx_COREFLAG_INITIALIZED;
}

/* http://bcm-specs.sipsolutions.net/80211Init */
static int bcm43xx_wireless_core_init(struct bcm43xx_private *bcm)
{
	u32 ucodeflags;
	int err;
	u32 sbimconfiglow;

	if (bcm->chip_rev < 5) {
		sbimconfiglow = bcm43xx_read32(bcm, BCM43xx_CIR_SBIMCONFIGLOW);
		sbimconfiglow &= ~ BCM43xx_SBIMCONFIGLOW_REQUEST_TOUT_MASK;
		sbimconfiglow &= ~ BCM43xx_SBIMCONFIGLOW_SERVICE_TOUT_MASK;
		if (bcm->bustype == BCM43xx_BUSTYPE_PCI)
			sbimconfiglow |= 0x32;
		else if (bcm->bustype == BCM43xx_BUSTYPE_SB)
			sbimconfiglow |= 0x53;
		else
			assert(0);
		bcm43xx_write32(bcm, BCM43xx_CIR_SBIMCONFIGLOW, sbimconfiglow);
	}

	bcm43xx_phy_calibrate(bcm);
	err = bcm43xx_chip_init(bcm);
	if (err)
		goto out;

	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0016, bcm->current_core->rev);
	ucodeflags = bcm43xx_shm_read32(bcm, BCM43xx_SHM_SHARED, BCM43xx_UCODEFLAGS_OFFSET);

	if (0 /*FIXME: which condition has to be used here? */)
		ucodeflags |= 0x00000010;

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_G) {
		ucodeflags |= BCM43xx_UCODEFLAG_UNKBGPHY;
		if (bcm->current_core->phy->rev == 1)
			ucodeflags |= BCM43xx_UCODEFLAG_UNKGPHY;
		if (bcm->sprom.boardflags & BCM43xx_BFL_PACTRL)
			ucodeflags |= BCM43xx_UCODEFLAG_UNKPACTRL;
	} else if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_B) {
		ucodeflags |= BCM43xx_UCODEFLAG_UNKBGPHY;
		if ((bcm->current_core->phy->rev >= 2) &&
		    (bcm->current_core->radio->version == 0x2050))
			ucodeflags &= ~BCM43xx_UCODEFLAG_UNKGPHY;
	}

	if (ucodeflags != bcm43xx_shm_read32(bcm, BCM43xx_SHM_SHARED,
					     BCM43xx_UCODEFLAGS_OFFSET)) {
		bcm43xx_shm_write32(bcm, BCM43xx_SHM_SHARED,
				    BCM43xx_UCODEFLAGS_OFFSET, ucodeflags);
	}

	/* FIXME: Short/Long Retry Limit: Using defaults as of http://bcm-specs.sipsolutions.net/SHM: 0x0002 */
	bcm43xx_shm_write32(bcm, BCM43xx_SHM_WIRELESS, 0x0006, 7);
	bcm43xx_shm_write32(bcm, BCM43xx_SHM_WIRELESS, 0x0007, 4);

	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0044, 3);
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0046, 2);

	bcm43xx_rate_memory_init(bcm);

	/* Minimum Contention Window */
	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_B)
		bcm43xx_shm_write32(bcm, BCM43xx_SHM_WIRELESS, 0x0003, 0x0000001f);
	else
		bcm43xx_shm_write32(bcm, BCM43xx_SHM_WIRELESS, 0x0003, 0x0000000f);
	/* Maximum Contention Window */
	bcm43xx_shm_write32(bcm, BCM43xx_SHM_WIRELESS, 0x0004, 0x000003ff);

	bcm43xx_gen_bssid(bcm);
	bcm43xx_write_mac_bssid_templates(bcm);

	if (bcm->current_core->rev >= 5)
		bcm43xx_write16(bcm, 0x043C, 0x000C);

	if (!bcm->pio_mode) {
		err = bcm43xx_dma_init(bcm);
		if (err)
			goto err_chip_cleanup;
	} else {
		err = bcm43xx_pio_init(bcm);
		if (err)
			goto err_chip_cleanup;
	}
	bcm43xx_write16(bcm, 0x0612, 0x0050);
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0416, 0x0050);
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0414, 0x01F4);

	bcm43xx_mac_enable(bcm);
	bcm43xx_interrupt_enable(bcm, bcm->irq_savedstate);

	bcm->current_core->flags |= BCM43xx_COREFLAG_INITIALIZED;
out:
	return err;

err_chip_cleanup:
	bcm43xx_chip_cleanup(bcm);
	goto out;
}

/* Hard-reset the chip. Do not call this directly.
 * Use bcm43xx_recover_from_fatal()
 */
static void bcm43xx_chip_reset(void *_bcm)
{
	struct bcm43xx_private *bcm = _bcm;
	int err;

	netif_tx_disable(bcm->net_dev);
	tasklet_disable(&bcm->isr_tasklet);
	bcm43xx_free_board(bcm);
	bcm->irq_savedstate = BCM43xx_IRQ_INITIAL;
	err = bcm43xx_init_board(bcm);
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
static void bcm43xx_recover_from_fatal(struct bcm43xx_private *bcm, const char *error)
{
	bcm43xx_interrupt_disable(bcm, BCM43xx_IRQ_ALL);
	printk(KERN_ERR PFX "FATAL ERROR (%s): Resetting the chip...\n", error);
	INIT_WORK(&bcm->fatal_work, bcm43xx_chip_reset, bcm);
	queue_work(bcm->workqueue, &bcm->fatal_work);
}

static int bcm43xx_chipset_attach(struct bcm43xx_private *bcm)
{
	int err;
	u16 pci_status;

	err = bcm43xx_pctl_set_crystal(bcm, 1);
	if (err)
		goto out;
	bcm43xx_pci_read_config_16(bcm->pci_dev, PCI_STATUS, &pci_status);
	bcm43xx_pci_write_config_16(bcm->pci_dev, PCI_STATUS, pci_status & ~PCI_STATUS_SIG_TARGET_ABORT);

out:
	return err;
}

static void bcm43xx_chipset_detach(struct bcm43xx_private *bcm)
{
	bcm43xx_pctl_set_clock(bcm, BCM43xx_PCTL_CLK_SLOW);
	bcm43xx_pctl_set_crystal(bcm, 0);
}

static inline void bcm43xx_pcicore_broadcast_value(struct bcm43xx_private *bcm,
						   u32 address,
						   u32 data)
{
	bcm43xx_write32(bcm, BCM43xx_PCICORE_BCAST_ADDR, address);
	bcm43xx_write32(bcm, BCM43xx_PCICORE_BCAST_DATA, data);
}

static int bcm43xx_pcicore_commit_settings(struct bcm43xx_private *bcm)
{
	int err;
	struct bcm43xx_coreinfo *old_core;

	old_core = bcm->current_core;
	err = bcm43xx_switch_core(bcm, &bcm->core_pci);
	if (err)
		goto out;

	bcm43xx_pcicore_broadcast_value(bcm, 0xfd8, 0x00000000);

	bcm43xx_switch_core(bcm, old_core);
	assert(err == 0);
out:
	return err;
}

/* Make an I/O Core usable. "core_mask" is the bitmask of the cores to enable.
 * To enable core 0, pass a core_mask of 1<<0
 */
static int bcm43xx_setup_backplane_pci_connection(struct bcm43xx_private *bcm,
						  u32 core_mask)
{
	u32 backplane_flag_nr;
	u32 value;
	struct bcm43xx_coreinfo *old_core;
	int err;

	value = bcm43xx_read32(bcm, BCM43xx_CIR_SBTPSFLAG);
	backplane_flag_nr = value & BCM43xx_BACKPLANE_FLAG_NR_MASK;

	old_core = bcm->current_core;
	err = bcm43xx_switch_core(bcm, &bcm->core_pci);
	if (err)
		goto out;

	if (bcm->core_pci.rev < 6) {
		value = bcm43xx_read32(bcm, BCM43xx_CIR_SBINTVEC);
		value |= (1 << backplane_flag_nr);
		bcm43xx_write32(bcm, BCM43xx_CIR_SBINTVEC, value);
	} else {
		err = bcm43xx_pci_read_config_32(bcm->pci_dev, BCM43xx_PCICFG_ICR, &value);
		if (err) {
			printk(KERN_ERR PFX "Error: ICR setup failure!\n");
			goto out_switch_back;
		}
		value |= core_mask << 8;
		err = bcm43xx_pci_write_config_32(bcm->pci_dev, BCM43xx_PCICFG_ICR, value);
		if (err) {
			printk(KERN_ERR PFX "Error: ICR setup failure!\n");
			goto out_switch_back;
		}
	}

	value = bcm43xx_read32(bcm, BCM43xx_PCICORE_SBTOPCI2);
	value |= BCM43xx_SBTOPCI2_PREFETCH | BCM43xx_SBTOPCI2_BURST;
	bcm43xx_write32(bcm, BCM43xx_PCICORE_SBTOPCI2, value);

	if (bcm->core_pci.rev < 5) {
		value = bcm43xx_read32(bcm, BCM43xx_CIR_SBIMCONFIGLOW);
		value |= (2 << BCM43xx_SBIMCONFIGLOW_SERVICE_TOUT_SHIFT)
			 & BCM43xx_SBIMCONFIGLOW_SERVICE_TOUT_MASK;
		value |= (3 << BCM43xx_SBIMCONFIGLOW_REQUEST_TOUT_SHIFT)
			 & BCM43xx_SBIMCONFIGLOW_REQUEST_TOUT_MASK;
		bcm43xx_write32(bcm, BCM43xx_CIR_SBIMCONFIGLOW, value);
		err = bcm43xx_pcicore_commit_settings(bcm);
		assert(err == 0);
	}

out_switch_back:
	err = bcm43xx_switch_core(bcm, old_core);
out:
	return err;
}

static void bcm43xx_softmac_init(struct bcm43xx_private *bcm)
{
	ieee80211softmac_start(bcm->net_dev);
}

static void bcm43xx_periodic_work0_handler(void *d)
{
	struct bcm43xx_private *bcm = d;
	unsigned long flags;
	//TODO: unsigned int aci_average;

	spin_lock_irqsave(&bcm->lock, flags);

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_G) {
		//FIXME: aci_average = bcm43xx_update_aci_average(bcm);
		if (bcm->current_core->radio->aci_enable && bcm->current_core->radio->aci_wlan_automatic) {
			bcm43xx_mac_suspend(bcm);
			if (!bcm->current_core->radio->aci_enable &&
			    1 /*FIXME: We are not scanning? */) {
				/*FIXME: First add bcm43xx_update_aci_average() before
				 * uncommenting this: */
				//if (bcm43xx_radio_aci_scan)
				//	bcm43xx_radio_set_interference_mitigation(bcm,
				//	                                          BCM43xx_RADIO_INTERFMODE_MANUALWLAN);
			} else if (1/*FIXME*/) {
				//if ((aci_average > 1000) && !(bcm43xx_radio_aci_scan(bcm)))
				//	bcm43xx_radio_set_interference_mitigation(bcm,
				//	                                          BCM43xx_RADIO_INTERFMODE_MANUALWLAN);
			}
			bcm43xx_mac_enable(bcm);
		} else if  (bcm->current_core->radio->interfmode == BCM43xx_RADIO_INTERFMODE_NONWLAN) {
			if (bcm->current_core->phy->rev == 1) {
				//FIXME: implement rev1 workaround
			}
		}
	}
	bcm43xx_phy_xmitpower(bcm); //FIXME: unless scanning?
	//TODO for APHY (temperature?)

	if (likely(!bcm->shutting_down)) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work0,
				   BCM43xx_PERIODIC_0_DELAY);
	}
	spin_unlock_irqrestore(&bcm->lock, flags);
}

static void bcm43xx_periodic_work1_handler(void *d)
{
	struct bcm43xx_private *bcm = d;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);

	bcm43xx_phy_lo_mark_all_unused(bcm);
	if (bcm->sprom.boardflags & BCM43xx_BFL_RSSI) {
		bcm43xx_mac_suspend(bcm);
		bcm43xx_calc_nrssi_slope(bcm);
		bcm43xx_mac_enable(bcm);
	}

	if (likely(!bcm->shutting_down)) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work1,
				   BCM43xx_PERIODIC_1_DELAY);
	}
	spin_unlock_irqrestore(&bcm->lock, flags);
}

static void bcm43xx_periodic_work2_handler(void *d)
{
	struct bcm43xx_private *bcm = d;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);

	assert(bcm->current_core->phy->type == BCM43xx_PHYTYPE_G);
	assert(bcm->current_core->phy->rev >= 2);

	bcm43xx_mac_suspend(bcm);
	bcm43xx_phy_lo_g_measure(bcm);
	bcm43xx_mac_enable(bcm);

	if (likely(!bcm->shutting_down)) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work2,
				   BCM43xx_PERIODIC_2_DELAY);
	}
	spin_unlock_irqrestore(&bcm->lock, flags);
}

static void bcm43xx_periodic_work3_handler(void *d)
{
	struct bcm43xx_private *bcm = d;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);

	/* Update device statistics. */
	bcm43xx_calculate_link_quality(bcm);

	if (likely(!bcm->shutting_down)) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work3,
				   BCM43xx_PERIODIC_3_DELAY);
	}
	spin_unlock_irqrestore(&bcm->lock, flags);
}

/* Delete all periodic tasks and make
 * sure they are not running any longer
 */
static void bcm43xx_periodic_tasks_delete(struct bcm43xx_private *bcm)
{
	cancel_delayed_work(&bcm->periodic_work0);
	cancel_delayed_work(&bcm->periodic_work1);
	cancel_delayed_work(&bcm->periodic_work2);
	cancel_delayed_work(&bcm->periodic_work3);
	flush_workqueue(bcm->workqueue);
}

/* Setup all periodic tasks. */
static void bcm43xx_periodic_tasks_setup(struct bcm43xx_private *bcm)
{
	INIT_WORK(&bcm->periodic_work0, bcm43xx_periodic_work0_handler, bcm);
	INIT_WORK(&bcm->periodic_work1, bcm43xx_periodic_work1_handler, bcm);
	INIT_WORK(&bcm->periodic_work2, bcm43xx_periodic_work2_handler, bcm);
	INIT_WORK(&bcm->periodic_work3, bcm43xx_periodic_work3_handler, bcm);

	/* Periodic task 0: Delay ~15sec */
	queue_delayed_work(bcm->workqueue, &bcm->periodic_work0,
			   BCM43xx_PERIODIC_0_DELAY);

	/* Periodic task 1: Delay ~60sec */
	queue_delayed_work(bcm->workqueue, &bcm->periodic_work1,
			   BCM43xx_PERIODIC_1_DELAY);

	/* Periodic task 2: Delay ~120sec */
	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_G &&
	    bcm->current_core->phy->rev >= 2) {
		queue_delayed_work(bcm->workqueue, &bcm->periodic_work2,
				   BCM43xx_PERIODIC_2_DELAY);
	}

	/* Periodic task 3: Delay ~30sec */
	queue_delayed_work(bcm->workqueue, &bcm->periodic_work3,
			   BCM43xx_PERIODIC_3_DELAY);
}

/* This is the opposite of bcm43xx_init_board() */
static void bcm43xx_free_board(struct bcm43xx_private *bcm)
{
	int i, err;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);
	bcm->initialized = 0;
	bcm->shutting_down = 1;
	spin_unlock_irqrestore(&bcm->lock, flags);

	bcm43xx_periodic_tasks_delete(bcm);

	for (i = 0; i < BCM43xx_MAX_80211_CORES; i++) {
		if (!(bcm->core_80211[i].flags & BCM43xx_COREFLAG_AVAILABLE))
			continue;
		if (!(bcm->core_80211[i].flags & BCM43xx_COREFLAG_INITIALIZED))
			continue;

		err = bcm43xx_switch_core(bcm, &bcm->core_80211[i]);
		assert(err == 0);
		bcm43xx_wireless_core_cleanup(bcm);
	}

	bcm43xx_pctl_set_crystal(bcm, 0);

	spin_lock_irqsave(&bcm->lock, flags);
	bcm->shutting_down = 0;
	spin_unlock_irqrestore(&bcm->lock, flags);
}

static int bcm43xx_init_board(struct bcm43xx_private *bcm)
{
	int i, err;
	int num_80211_cores;
	int connect_phy;
	unsigned long flags;

	might_sleep();

	spin_lock_irqsave(&bcm->lock, flags);
	bcm->initialized = 0;
	bcm->shutting_down = 0;
	spin_unlock_irqrestore(&bcm->lock, flags);

	err = bcm43xx_pctl_set_crystal(bcm, 1);
	if (err)
		goto out;
	err = bcm43xx_pctl_init(bcm);
	if (err)
		goto err_crystal_off;
	err = bcm43xx_pctl_set_clock(bcm, BCM43xx_PCTL_CLK_FAST);
	if (err)
		goto err_crystal_off;

	tasklet_enable(&bcm->isr_tasklet);
	num_80211_cores = bcm43xx_num_80211_cores(bcm);
	for (i = 0; i < num_80211_cores; i++) {
		err = bcm43xx_switch_core(bcm, &bcm->core_80211[i]);
		assert(err != -ENODEV);
		if (err)
			goto err_80211_unwind;

		/* Enable the selected wireless core.
		 * Connect PHY only on the first core.
		 */
		if (!bcm43xx_core_enabled(bcm)) {
			if (num_80211_cores == 1) {
				connect_phy = bcm->current_core->phy->connected;
			} else {
				if (i == 0)
					connect_phy = 1;
				else
					connect_phy = 0;
			}
			bcm43xx_wireless_core_reset(bcm, connect_phy);
		}

		if (i != 0)
			bcm43xx_wireless_core_mark_inactive(bcm, &bcm->core_80211[0]);

		err = bcm43xx_wireless_core_init(bcm);
		if (err)
			goto err_80211_unwind;

		if (i != 0) {
			bcm43xx_mac_suspend(bcm);
			bcm43xx_interrupt_disable(bcm, BCM43xx_IRQ_ALL);
			bcm43xx_radio_turn_off(bcm);
		}
	}
	bcm->active_80211_core = &bcm->core_80211[0];
	if (num_80211_cores >= 2) {
		bcm43xx_switch_core(bcm, &bcm->core_80211[0]);
		bcm43xx_mac_enable(bcm);
	}
	bcm43xx_macfilter_clear(bcm, BCM43xx_MACFILTER_ASSOC);
	bcm43xx_macfilter_set(bcm, BCM43xx_MACFILTER_SELF, (u8 *)(bcm->net_dev->dev_addr));
	dprintk(KERN_INFO PFX "80211 cores initialized\n");
	bcm43xx_softmac_init(bcm);

	bcm43xx_pctl_set_clock(bcm, BCM43xx_PCTL_CLK_DYNAMIC);

	spin_lock_irqsave(&bcm->lock, flags);
	bcm->initialized = 1;
	spin_unlock_irqrestore(&bcm->lock, flags);

	if (bcm->current_core->radio->initial_channel != 0xFF) {
		bcm43xx_mac_suspend(bcm);
		bcm43xx_radio_selectchannel(bcm, bcm->current_core->radio->initial_channel, 0);
		bcm43xx_mac_enable(bcm);
	}
	bcm43xx_periodic_tasks_setup(bcm);

	assert(err == 0);
out:
	return err;

err_80211_unwind:
	tasklet_disable(&bcm->isr_tasklet);
	/* unwind all 80211 initialization */
	for (i = 0; i < num_80211_cores; i++) {
		if (!(bcm->core_80211[i].flags & BCM43xx_COREFLAG_INITIALIZED))
			continue;
		bcm43xx_interrupt_disable(bcm, BCM43xx_IRQ_ALL);
		bcm43xx_wireless_core_cleanup(bcm);
	}
err_crystal_off:
	bcm43xx_pctl_set_crystal(bcm, 0);
	goto out;
}

static void bcm43xx_detach_board(struct bcm43xx_private *bcm)
{
	struct pci_dev *pci_dev = bcm->pci_dev;
	int i;

	bcm43xx_chipset_detach(bcm);
	/* Do _not_ access the chip, after it is detached. */
	iounmap(bcm->mmio_addr);

	pci_release_regions(pci_dev);
	pci_disable_device(pci_dev);

	/* Free allocated structures/fields */
	for (i = 0; i < BCM43xx_MAX_80211_CORES; i++) {
		kfree(bcm->phy[i]._lo_pairs);
	}
}	

static int bcm43xx_read_phyinfo(struct bcm43xx_private *bcm)
{
	u16 value;
	u8 phy_version;
	u8 phy_type;
	u8 phy_rev;
	int phy_rev_ok = 1;
	void *p;

	value = bcm43xx_read16(bcm, BCM43xx_MMIO_PHY_VER);

	phy_version = (value & 0xF000) >> 12;
	phy_type = (value & 0x0F00) >> 8;
	phy_rev = (value & 0x000F);

	dprintk(KERN_INFO PFX "Detected PHY: Version: %x, Type %x, Revision %x\n",
		phy_version, phy_type, phy_rev);

	switch (phy_type) {
	case BCM43xx_PHYTYPE_A:
		if (phy_rev >= 4)
			phy_rev_ok = 0;
		/*FIXME: We need to switch the ieee->modulation, etc.. flags,
		 *       if we switch 80211 cores after init is done.
		 *       As we do not implement on the fly switching between
		 *       wireless cores, I will leave this as a future task.
		 */
		bcm->ieee->modulation = IEEE80211_OFDM_MODULATION;
		bcm->ieee->mode = IEEE_A;
		bcm->ieee->freq_band = IEEE80211_52GHZ_BAND |
				       IEEE80211_24GHZ_BAND;
		break;
	case BCM43xx_PHYTYPE_B:
		if (phy_rev != 2 && phy_rev != 4 && phy_rev != 6 && phy_rev != 7)
			phy_rev_ok = 0;
		bcm->ieee->modulation = IEEE80211_CCK_MODULATION;
		bcm->ieee->mode = IEEE_B;
		bcm->ieee->freq_band = IEEE80211_24GHZ_BAND;
		break;
	case BCM43xx_PHYTYPE_G:
		if (phy_rev >= 3)
			phy_rev_ok = 0;
		bcm->ieee->modulation = IEEE80211_OFDM_MODULATION |
					IEEE80211_CCK_MODULATION;
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
	if ((phy_type == BCM43xx_PHYTYPE_B) || (phy_type == BCM43xx_PHYTYPE_G)) {
		p = kzalloc(sizeof(struct bcm43xx_lopair) * BCM43xx_LO_COUNT,
			    GFP_KERNEL);
		if (!p)
			return -ENOMEM;
		bcm->current_core->phy->_lo_pairs = p;
	}

	return 0;
}

static int bcm43xx_attach_board(struct bcm43xx_private *bcm)
{
	struct pci_dev *pci_dev = bcm->pci_dev;
	struct net_device *net_dev = bcm->net_dev;
	int err;
	int i;
	void __iomem *ioaddr;
	unsigned long mmio_start, mmio_end, mmio_flags, mmio_len;
	int num_80211_cores;
	u32 coremask;

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
	if (mmio_len != BCM43xx_IO_SIZE) {
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

	net_dev->base_addr = (unsigned long)ioaddr;
	bcm->mmio_addr = ioaddr;
	bcm->mmio_len = mmio_len;

	bcm43xx_pci_read_config_16(bcm->pci_dev, PCI_SUBSYSTEM_VENDOR_ID,
	                           &bcm->board_vendor);
	bcm43xx_pci_read_config_16(bcm->pci_dev, PCI_SUBSYSTEM_ID,
	                           &bcm->board_type);
	bcm43xx_pci_read_config_16(bcm->pci_dev, PCI_REVISION_ID,
	                           &bcm->board_revision);

	err = bcm43xx_chipset_attach(bcm);
	if (err)
		goto err_iounmap;
	err = bcm43xx_pctl_init(bcm);
	if (err)
		goto err_chipset_detach;
	err = bcm43xx_probe_cores(bcm);
	if (err)
		goto err_chipset_detach;
	
	num_80211_cores = bcm43xx_num_80211_cores(bcm);

	/* Attach all IO cores to the backplane. */
	coremask = 0;
	for (i = 0; i < num_80211_cores; i++)
		coremask |= (1 << bcm->core_80211[i].index);
	//FIXME: Also attach some non80211 cores?
	err = bcm43xx_setup_backplane_pci_connection(bcm, coremask);
	if (err) {
		printk(KERN_ERR PFX "Backplane->PCI connection failed!\n");
		goto err_chipset_detach;
	}

	err = bcm43xx_read_sprom(bcm);
	if (err)
		goto err_chipset_detach;
	err = bcm43xx_leds_init(bcm);
	if (err)
		goto err_chipset_detach;

	for (i = 0; i < num_80211_cores; i++) {
		err = bcm43xx_switch_core(bcm, &bcm->core_80211[i]);
		assert(err != -ENODEV);
		if (err)
			goto err_80211_unwind;

		/* Enable the selected wireless core.
		 * Connect PHY only on the first core.
		 */
		bcm43xx_wireless_core_reset(bcm, (i == 0));

		err = bcm43xx_read_phyinfo(bcm);
		if (err && (i == 0))
			goto err_80211_unwind;

		err = bcm43xx_read_radioinfo(bcm);
		if (err && (i == 0))
			goto err_80211_unwind;

		err = bcm43xx_validate_chip(bcm);
		if (err && (i == 0))
			goto err_80211_unwind;

		bcm43xx_radio_turn_off(bcm);
		err = bcm43xx_phy_init_tssi2dbm_table(bcm);
		if (err)
			goto err_80211_unwind;
		bcm43xx_wireless_core_disable(bcm);
	}
	bcm43xx_pctl_set_crystal(bcm, 0);

	/* Set the MAC address in the networking subsystem */
	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_A)
		memcpy(bcm->net_dev->dev_addr, bcm->sprom.et1macaddr, 6);
	else
		memcpy(bcm->net_dev->dev_addr, bcm->sprom.il0macaddr, 6);

	bcm43xx_geo_init(bcm);

	snprintf(bcm->nick, IW_ESSID_MAX_SIZE,
		 "Broadcom %04X", bcm->chip_id);

	assert(err == 0);
out:
	return err;

err_80211_unwind:
	for (i = 0; i < BCM43xx_MAX_80211_CORES; i++) {
		kfree(bcm->phy[i]._lo_pairs);
	}
err_chipset_detach:
	bcm43xx_chipset_detach(bcm);
err_iounmap:
	iounmap(bcm->mmio_addr);
err_pci_release:
	pci_release_regions(pci_dev);
err_pci_disable:
	pci_disable_device(pci_dev);
	goto out;
}

int fastcall bcm43xx_rx_transmitstatus(struct bcm43xx_private *bcm,
				       const struct bcm43xx_hwxmitstatus *status)
{
	struct bcm43xx_xmitstatus_queue *q;

	/*XXX: This code is untested, as we currently do not have a rev < 5 card. */
dprintkl("processing received xmitstatus...\n");

	if (unlikely(bcm->nr_xmitstatus_queued >= 50)) {
		dprintkl(KERN_ERR PFX "Transmit Status Queue full!\n");
		return -ENOSPC;
	}
	q = kmalloc(sizeof(*q), GFP_ATOMIC);
	if (unlikely(!q))
		return -ENOMEM;
	INIT_LIST_HEAD(&q->list);
	memcpy(&q->status, status, sizeof(*status));
	list_add_tail(&q->list, &bcm->xmitstatus_queue);
	bcm->nr_xmitstatus_queued++;

	return 0;
}

/* Postprocess the RSSI value in the RX header. */
static inline
s8 bcm43xx_rssi_postprocess(struct bcm43xx_private *bcm, u8 in_rssi)
{
	s8 ret = 0;
	s32 tmp;

	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_A:
		TODO();//TODO
		break;
	case BCM43xx_PHYTYPE_B:
	case BCM43xx_PHYTYPE_G:
		if (bcm->current_core->radio->version == 0x2050) {
			if (bcm->sprom.boardflags & BCM43xx_BFL_RSSI) {
				if (in_rssi > 63)
					in_rssi = 63;
				tmp = bcm->current_core->radio->nrssi_lt[in_rssi];
				tmp -= 31;
				tmp *= 65;
				tmp /= 128;
				tmp -= 57;
				if (tmp < -34)
					tmp += 15;
				ret = (s8)tmp;
			} else {
				tmp = in_rssi;
				tmp -= 31;
				tmp *= -149;
				tmp /= 128;
				tmp -= 68;
				if (tmp < -35)
					tmp += 15;
				ret = (s8)tmp;
			}
		} else {
			tmp = in_rssi;
			tmp *= 103;
			tmp += 1133;
			tmp /= 64;
			tmp -= 1133;
			ret = (s8)tmp;
		}
		break;
	default:
		assert(0);
	}

	return ret;
}

static inline
int bcm43xx_rx_packet(struct bcm43xx_private *bcm,
		      struct sk_buff *skb,
		      struct ieee80211_rx_stats *stats)
{
	int err;

	err = ieee80211_rx(bcm->ieee, skb, stats);
	if (unlikely(err == 0)) {
		dprintkl(KERN_ERR PFX "ieee80211_rx() failed\n");
		return -EINVAL;
	}

	return 0;
}

int fastcall bcm43xx_rx(struct bcm43xx_private *bcm,
			struct sk_buff *skb,
			struct bcm43xx_rxhdr *rxhdr)
{
	struct bcm43xx_plcp_hdr4 *plcp;
	const int is_ofdm = !!(rxhdr->flags1 & BCM43xx_RXHDR_FLAGS1_OFDM);
	struct ieee80211_rx_stats stats;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 14)
	struct ieee80211_hdr *wlhdr;
#else
	struct ieee80211_hdr_4addr *wlhdr;
#endif
	u16 frame_ctl;
	int is_packet_for_us = 0;
	int err = -EINVAL;

	if (rxhdr->flags2 & BCM43xx_RXHDR_FLAGS2_TYPE2FRAME) {
		plcp = (struct bcm43xx_plcp_hdr4 *)(skb->data + 2);
		/* Skip two unknown bytes and the PLCP header. */
		skb_pull(skb, 2 + sizeof(struct bcm43xx_plcp_hdr6));
	} else {
		plcp = (struct bcm43xx_plcp_hdr4 *)(skb->data);
		/* Skip the PLCP header. */
		skb_pull(skb, sizeof(struct bcm43xx_plcp_hdr6));
	}
	/* The SKB contains the PAYLOAD (wireless header + data)
	 * at this point. The FCS at the end is stripped.
	 */

	memset(&stats, 0, sizeof(stats));
	stats.mac_time = rxhdr->mactime;
	stats.rssi = bcm43xx_rssi_postprocess(bcm, rxhdr->rssi);
	stats.signal = rxhdr->signal_quality;	//FIXME
//TODO	stats.noise = 
//FIXME: is_ofdm seems to be wrong	stats.rate = bcm43xx_plcp_get_bitrate(plcp, is_ofdm);
//printk("RX ofdm %d, rate == %u\n", is_ofdm, stats.rate);
	stats.received_channel = bcm->current_core->radio->channel;
//TODO	stats.control = 
	stats.mask = IEEE80211_STATMASK_SIGNAL |
//TODO		     IEEE80211_STATMASK_NOISE |
//FIXME		     IEEE80211_STATMASK_RATE |
		     IEEE80211_STATMASK_RSSI;
	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_A)
		stats.freq = IEEE80211_52GHZ_BAND;
	else
		stats.freq = IEEE80211_24GHZ_BAND;
	stats.len = skb->len;


	if (bcm->ieee->iw_mode == IW_MODE_MONITOR)
		return bcm43xx_rx_packet(bcm, skb, &stats);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 14)
	wlhdr = (struct ieee80211_hdr *)(skb->data);
#else
	wlhdr = (struct ieee80211_hdr_4addr *)(skb->data);
#endif

	switch (bcm->ieee->iw_mode) {
	case IW_MODE_ADHOC:
		if (memcmp(wlhdr->addr1, bcm->net_dev->dev_addr, ETH_ALEN) == 0 ||
		    memcmp(wlhdr->addr3, bcm->ieee->bssid, ETH_ALEN) == 0 ||
		    is_broadcast_ether_addr(wlhdr->addr1) ||
		    is_multicast_ether_addr(wlhdr->addr1))
			is_packet_for_us = 1;
		break;
	case IW_MODE_INFRA:
	default:
		if (memcmp(wlhdr->addr3, bcm->ieee->bssid, ETH_ALEN) == 0 ||
		    memcmp(wlhdr->addr1, bcm->net_dev->dev_addr, ETH_ALEN) == 0 ||
		    is_broadcast_ether_addr(wlhdr->addr1) ||
		    is_multicast_ether_addr(wlhdr->addr1))
			is_packet_for_us = 1;
		break;
	}

	frame_ctl = le16_to_cpu(wlhdr->frame_ctl);
	switch (WLAN_FC_GET_TYPE(frame_ctl)) {
	case IEEE80211_FTYPE_MGMT:
		ieee80211_rx_mgt(bcm->ieee, wlhdr, &stats);
		break;
	case IEEE80211_FTYPE_DATA:
		if (is_packet_for_us)
			err = bcm43xx_rx_packet(bcm, skb, &stats);
		else
			dprintkl(KERN_ERR PFX "RX packet dropped (not for us)\n");
		break;
	case IEEE80211_FTYPE_CTL:
		break;
	default:
		assert(0);
		return -EINVAL;
	}

	return err;
}

/* Do the Hardware IO operations to send the txb */
static inline int bcm43xx_tx(struct bcm43xx_private *bcm,
			     struct ieee80211_txb *txb)
{
	int err = -ENODEV;

	if (bcm->pio_mode)
		err = bcm43xx_pio_transfer_txb(bcm, txb);
	else
		err = bcm43xx_dma_transfer_txb(bcm, txb);

	return err;
}

static void bcm43xx_ieee80211_set_chan(struct net_device *net_dev,
				       u8 channel)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);
	bcm43xx_mac_suspend(bcm);
	bcm43xx_radio_selectchannel(bcm, channel, 0);
	bcm43xx_mac_enable(bcm);
	spin_unlock_irqrestore(&bcm->lock, flags);
}

/* set_security() callback in struct ieee80211_device */
static void bcm43xx_ieee80211_set_security(struct net_device *net_dev,
					   struct ieee80211_security *sec)
{/*TODO*/
}

/* hard_start_xmit() callback in struct ieee80211_device */
static int bcm43xx_ieee80211_hard_start_xmit(struct ieee80211_txb *txb,
					     struct net_device *net_dev
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 14)
					     , int pri
#endif
					     )
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	int err = -ENODEV;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);
	if (likely(bcm->initialized))
		err = bcm43xx_tx(bcm, txb);
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
}

static struct net_device_stats * bcm43xx_net_get_stats(struct net_device *net_dev)
{
	return &(bcm43xx_priv(net_dev)->ieee->stats);
}

static void bcm43xx_net_tx_timeout(struct net_device *net_dev)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);

	bcm43xx_recover_from_fatal(bcm, "TX timeout");
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void bcm43xx_net_poll_controller(struct net_device *net_dev)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	unsigned long flags;

	local_irq_save(flags);
	bcm43xx_interrupt_handler(bcm->pci_dev->irq, bcm, NULL);
	local_irq_restore(flags);
}
#endif /* CONFIG_NET_POLL_CONTROLLER */

static int bcm43xx_net_open(struct net_device *net_dev)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);

	return bcm43xx_init_board(bcm);
}

static int bcm43xx_net_stop(struct net_device *net_dev)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);

	ieee80211softmac_stop(net_dev);
	bcm43xx_disable_interrupts_sync(bcm, NULL);
	bcm43xx_free_board(bcm);

	return 0;
}

static int __devinit bcm43xx_init_one(struct pci_dev *pdev,
				      const struct pci_device_id *ent)
{
	struct net_device *net_dev;
	struct bcm43xx_private *bcm;
	int err;

#ifdef DEBUG_SINGLE_DEVICE_ONLY
	if (strcmp(pci_name(pdev), DEBUG_SINGLE_DEVICE_ONLY))
		return -ENODEV;
#endif

	net_dev = alloc_ieee80211softmac(sizeof(*bcm));
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

	net_dev->open = bcm43xx_net_open;
	net_dev->stop = bcm43xx_net_stop;
	net_dev->get_stats = bcm43xx_net_get_stats;
	net_dev->tx_timeout = bcm43xx_net_tx_timeout;
#ifdef CONFIG_NET_POLL_CONTROLLER
	net_dev->poll_controller = bcm43xx_net_poll_controller;
#endif
	net_dev->wireless_handlers = &bcm43xx_wx_handlers_def;
	net_dev->irq = pdev->irq;
	net_dev->watchdog_timeo = BCM43xx_TX_TIMEOUT;

	/* initialize the bcm43xx_private struct */
	bcm = bcm43xx_priv(net_dev);
	bcm->ieee = netdev_priv(net_dev);
	bcm->softmac = ieee80211_priv(net_dev);
	bcm->softmac->set_channel = bcm43xx_ieee80211_set_chan;

#ifdef DEBUG_ENABLE_MMIO_PRINT
	bcm43xx_mmioprint_initial(bcm, 1);
#else
	bcm43xx_mmioprint_initial(bcm, 0);
#endif

	bcm->irq_savedstate = BCM43xx_IRQ_INITIAL;
	bcm->pci_dev = pdev;
	bcm->net_dev = net_dev;
	if (modparam_bad_frames_preempt)
		bcm->bad_frames_preempt = 1;
	spin_lock_init(&bcm->lock);
	INIT_LIST_HEAD(&bcm->xmitstatus_queue);
	tasklet_init(&bcm->isr_tasklet,
		     (void (*)(unsigned long))bcm43xx_interrupt_tasklet,
		     (unsigned long)bcm);
	tasklet_disable_nosync(&bcm->isr_tasklet);
	bcm->workqueue = create_workqueue(DRV_NAME "_wq");
	if (!bcm->workqueue) {
		err = -ENOMEM;
		goto err_free_netdev;
	}
	if (modparam_pio) {
		bcm->pio_mode = 1;
	} else {
		if (pci_set_dma_mask(pdev, DMA_30BIT_MASK) == 0) {
			bcm->pio_mode = 0;
		} else {
			printk(KERN_WARNING PFX "DMA not supported. Falling back to PIO.\n");
			bcm->pio_mode = 1;
		}
	}
	bcm->rts_threshold = BCM43xx_DEFAULT_RTS_THRESHOLD;

	bcm->ieee->iw_mode = BCM43xx_INITIAL_IWMODE;
	bcm->ieee->tx_headroom = sizeof(struct bcm43xx_txhdr);
	bcm->ieee->set_security = bcm43xx_ieee80211_set_security;
	bcm->ieee->hard_start_xmit = bcm43xx_ieee80211_hard_start_xmit;

	pci_set_drvdata(pdev, net_dev);

	err = bcm43xx_attach_board(bcm);
	if (err)
		goto err_destroy_wq;

	err = register_netdev(net_dev);
	if (err) {
		printk(KERN_ERR PFX "Cannot register net device, "
		       "aborting.\n");
		err = -ENOMEM;
		goto err_detach_board;
	}

	bcm43xx_debugfs_add_device(bcm);

	assert(err == 0);
out:
	return err;

err_detach_board:
	bcm43xx_detach_board(bcm);
err_destroy_wq:
	destroy_workqueue(bcm->workqueue);
err_free_netdev:
	free_ieee80211softmac(net_dev);
	goto out;
}

static void __devexit bcm43xx_remove_one(struct pci_dev *pdev)
{
	struct net_device *net_dev = pci_get_drvdata(pdev);
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);

	bcm43xx_debugfs_remove_device(bcm);
	unregister_netdev(net_dev);
	bcm43xx_detach_board(bcm);
	destroy_workqueue(bcm->workqueue);
	free_ieee80211softmac(net_dev);
}

#ifdef CONFIG_PM

static int bcm43xx_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct net_device *net_dev = pci_get_drvdata(pdev);
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	unsigned long flags;
	int try_to_shutdown = 0, err;

	dprintk(KERN_INFO PFX "Suspending...\n");

	spin_lock_irqsave(&bcm->lock, flags);
	bcm->was_initialized = bcm->initialized;
	if (bcm->initialized)
		try_to_shutdown = 1;
	spin_unlock_irqrestore(&bcm->lock, flags);

	netif_device_detach(net_dev);
	if (try_to_shutdown) {
		ieee80211softmac_stop(net_dev);
		err = bcm43xx_disable_interrupts_sync(bcm, &bcm->irq_savedstate);
		if (unlikely(err)) {
			dprintk(KERN_ERR PFX "Suspend failed.\n");
			return -EAGAIN;
		}
		bcm43xx_free_board(bcm);
	}
	bcm43xx_chipset_detach(bcm);

	pci_save_state(pdev);
	pci_disable_device(pdev);
	pci_set_power_state(pdev, pci_choose_state(pdev, state));

	dprintk(KERN_INFO PFX "Device suspended.\n");

	return 0;
}

static int bcm43xx_resume(struct pci_dev *pdev)
{
	struct net_device *net_dev = pci_get_drvdata(pdev);
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	int err = 0;

	dprintk(KERN_INFO PFX "Resuming...\n");

	pci_set_power_state(pdev, 0);
	pci_enable_device(pdev);
	pci_restore_state(pdev);

	bcm43xx_chipset_attach(bcm);
	if (bcm->was_initialized) {
		bcm->irq_savedstate = BCM43xx_IRQ_INITIAL;
		err = bcm43xx_init_board(bcm);
	}
	if (err) {
		printk(KERN_ERR PFX "Resume failed!\n");
		return err;
	}

	netif_device_attach(net_dev);

	dprintk(KERN_INFO PFX "Device resumed.\n");

	return 0;
}

#endif				/* CONFIG_PM */

static struct pci_driver bcm43xx_pci_driver = {
	.name = BCM43xx_DRIVER_NAME,
	.id_table = bcm43xx_pci_tbl,
	.probe = bcm43xx_init_one,
	.remove = __devexit_p(bcm43xx_remove_one),
#ifdef CONFIG_PM
	.suspend = bcm43xx_suspend,
	.resume = bcm43xx_resume,
#endif				/* CONFIG_PM */
};

static int __init bcm43xx_init(void)
{
	printk(KERN_INFO BCM43xx_DRIVER_NAME "\n");
	bcm43xx_debugfs_init();
	return pci_register_driver(&bcm43xx_pci_driver);
}

static void __exit bcm43xx_exit(void)
{
	pci_unregister_driver(&bcm43xx_pci_driver);
	bcm43xx_debugfs_exit();
}

module_init(bcm43xx_init)
module_exit(bcm43xx_exit)

/* vim: set ts=8 sw=8 sts=8: */