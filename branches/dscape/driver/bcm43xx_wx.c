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

#include <linux/wireless.h>
#include <net/iw_handler.h>
#include <linux/capability.h>
#include <linux/sched.h> /* for capable() */
#include <linux/delay.h>

#include "bcm43xx.h"
#include "bcm43xx_wx.h"
#include "bcm43xx_main.h"
#include "bcm43xx_radio.h"

/* Define to enable a printk on each wx handler function invocation */
//#define BCM43xx_WX_DEBUG


#ifdef BCM43xx_WX_DEBUG
# define printk_wx		printk
#else
# define printk_wx(x...)	do { /* nothing */ } while (0)
#endif
#define wx_enter()		printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

#define MAX_WX_STRING		80


static int bcm43xx_wx_set_nick(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	unsigned long flags;
	size_t len;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	len =  min((size_t)data->data.length, (size_t)IW_ESSID_MAX_SIZE);
	memcpy(bcm->nick, extra, len);
	bcm->nick[len] = '\0';
	spin_unlock_irqrestore(&bcm->lock, flags);

	return 0;
}

static int bcm43xx_wx_get_nick(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	unsigned long flags;
	size_t len;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	len = strlen(bcm->nick) + 1;
	memcpy(extra, bcm->nick, len);
	data->data.length = (__u16)len;
	data->data.flags = 1;
	spin_unlock_irqrestore(&bcm->lock, flags);

	return 0;
}

static int bcm43xx_wx_set_interfmode(struct net_device *net_dev,
				     struct iw_request_info *info,
				     union iwreq_data *data,
				     char *extra)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	unsigned long flags;
	int mode, err = 0;

	wx_enter();

	mode = *((int *)extra);
	switch (mode) {
	case 0:
		mode = BCM43xx_RADIO_INTERFMODE_NONE;
		break;
	case 1:
		mode = BCM43xx_RADIO_INTERFMODE_NONWLAN;
		break;
	case 2:
		mode = BCM43xx_RADIO_INTERFMODE_MANUALWLAN;
		break;
	case 3:
		mode = BCM43xx_RADIO_INTERFMODE_AUTOWLAN;
		break;
	default:
		printk(KERN_ERR PFX "set_interfmode allowed parameters are: "
				    "0 => None,  1 => Non-WLAN,  2 => WLAN,  "
				    "3 => Auto-WLAN\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&bcm->lock, flags);
	if (bcm->initialized) {
		err = bcm43xx_radio_set_interference_mitigation(bcm, mode);
		if (err) {
			printk(KERN_ERR PFX "Interference Mitigation not "
					    "supported by device\n");
		}
	} else {
		if (mode == BCM43xx_RADIO_INTERFMODE_AUTOWLAN) {
			printk(KERN_ERR PFX "Interference Mitigation mode Auto-WLAN "
					    "not supported while the interface is down.\n");
			err = -ENODEV;
		} else
			bcm->current_core->radio->interfmode = mode;
	}
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
}

static int bcm43xx_wx_get_interfmode(struct net_device *net_dev,
				     struct iw_request_info *info,
				     union iwreq_data *data,
				     char *extra)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	unsigned long flags;
	int mode;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	mode = bcm->current_core->radio->interfmode;
	spin_unlock_irqrestore(&bcm->lock, flags);

	switch (mode) {
	case BCM43xx_RADIO_INTERFMODE_NONE:
		strncpy(extra, "0 (No Interference Mitigation)", MAX_WX_STRING);
		break;
	case BCM43xx_RADIO_INTERFMODE_NONWLAN:
		strncpy(extra, "1 (Non-WLAN Interference Mitigation)", MAX_WX_STRING);
		break;
	case BCM43xx_RADIO_INTERFMODE_MANUALWLAN:
		strncpy(extra, "2 (WLAN Interference Mitigation)", MAX_WX_STRING);
		break;
	default:
		assert(0);
	}
	data->data.length = strlen(extra) + 1;

	return 0;
}

static int bcm43xx_wx_set_shortpreamble(struct net_device *net_dev,
					struct iw_request_info *info,
					union iwreq_data *data,
					char *extra)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	unsigned long flags;
	int on;

	wx_enter();

	on = *((int *)extra);
	spin_lock_irqsave(&bcm->lock, flags);
	bcm->short_preamble = !!on;
	spin_unlock_irqrestore(&bcm->lock, flags);

	return 0;
}

static int bcm43xx_wx_get_shortpreamble(struct net_device *net_dev,
					struct iw_request_info *info,
					union iwreq_data *data,
					char *extra)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	unsigned long flags;
	int on;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	on = bcm->short_preamble;
	spin_unlock_irqrestore(&bcm->lock, flags);

	if (on)
		strncpy(extra, "1 (Short Preamble enabled)", MAX_WX_STRING);
	else
		strncpy(extra, "0 (Short Preamble disabled)", MAX_WX_STRING);
	data->data.length = strlen(extra) + 1;

	return 0;
}

static int bcm43xx_wx_dummy(struct net_device *net_dev,
			    struct iw_request_info *info,
			    union iwreq_data *data,
			    char *extra)
{
	return 0;
}

/* Enough buffer to hold a hexdump of the sprom data. */
#define SPROM_BUFFERSIZE	512

static int sprom2hex(const u16 *sprom, char *dump)
{
	int i, pos = 0;

	for (i = 0; i < BCM43xx_SPROM_SIZE; i++) {
		pos += snprintf(dump + pos, SPROM_BUFFERSIZE - pos - 1,
				"%04X", swab16(sprom[i]) & 0xFFFF);
	}

	return pos + 1;
}

static int hex2sprom(u16 *sprom, const char *dump, unsigned int len)
{
	char tmp[5] = { 0 };
	int cnt = 0;
	unsigned long parsed;
	u8 crc, expected_crc;

	if (len < BCM43xx_SPROM_SIZE * sizeof(u16) * 2)
		return -EINVAL;
	while (cnt < BCM43xx_SPROM_SIZE) {
		memcpy(tmp, dump, 4);
		dump += 4;
		parsed = simple_strtoul(tmp, NULL, 16);
		sprom[cnt++] = swab16((u16)parsed);
	}

	crc = bcm43xx_sprom_crc(sprom);
	expected_crc = (sprom[BCM43xx_SPROM_VERSION] & 0xFF00) >> 8;
	if (crc != expected_crc) {
		printk(KERN_ERR PFX "SPROM input data: Invalid CRC\n");
		return -EINVAL;
	}

	return 0;
}

static int bcm43xx_wx_sprom_read(struct net_device *net_dev,
				 struct iw_request_info *info,
				 union iwreq_data *data,
				 char *extra)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	int err = -EPERM, i;
	u16 *sprom;
	unsigned long flags;

	if (!capable(CAP_SYS_RAWIO))
		goto out;

	err = -ENOMEM;
	sprom = kmalloc(BCM43xx_SPROM_SIZE * sizeof(*sprom),
			GFP_KERNEL);
	if (!sprom)
		goto out;

	spin_lock_irqsave(&bcm->lock, flags);
	err = -ENODEV;
	if (!bcm->initialized) {
		spin_unlock_irqrestore(&bcm->lock, flags);
		goto out_kfree;
	}
	for (i = 0; i < BCM43xx_SPROM_SIZE; i++)
		sprom[i] = bcm43xx_read16(bcm, BCM43xx_SPROM_BASE + (i * 2));
	spin_unlock_irqrestore(&bcm->lock, flags);

	data->data.length = sprom2hex(sprom, extra);

	err = 0;
out_kfree:
	kfree(sprom);
out:
	return err;
}

static int bcm43xx_wx_sprom_write(struct net_device *net_dev,
				  struct iw_request_info *info,
				  union iwreq_data *data,
				  char *extra)
{
	struct bcm43xx_private *bcm = bcm43xx_priv(net_dev);
	int err = -EPERM;
	u16 *sprom;
	unsigned long flags;
	char *input;
	unsigned int len;
	u32 spromctl;
	int i;

	if (!capable(CAP_SYS_RAWIO))
		goto out;

	err = -ENOMEM;
	sprom = kmalloc(BCM43xx_SPROM_SIZE * sizeof(*sprom),
			GFP_KERNEL);
	if (!sprom)
		goto out;

	len = data->data.length;
	extra[len - 1] = '\0';
	input = strchr(extra, ':');
	if (input) {
		input++;
		len -= input - extra;
	} else
		input = extra;
	err = hex2sprom(sprom, input, len);
	if (err)
		goto out_kfree;

	spin_lock_irqsave(&bcm->lock, flags);
	err = -ENODEV;
	if (!bcm->initialized) {
		spin_unlock_irqrestore(&bcm->lock, flags);
		goto out_kfree;
	}

	printk(KERN_INFO PFX "Writing SPROM. Do NOT turn off the power! Please stand by...\n");
	err = bcm43xx_pci_read_config32(bcm, BCM43xx_PCICFG_SPROMCTL, &spromctl);
	if (err) {
		printk(KERN_ERR PFX "Could not access SPROM control register.\n");
		goto out_unlock;
	}
	spromctl |= 0x10; /* SPROM WRITE enable. */
	bcm43xx_pci_write_config32(bcm, BCM43xx_PCICFG_SPROMCTL, spromctl);
	if (err) {
		printk(KERN_ERR PFX "Could not access SPROM control register.\n");
		goto out_unlock;
	}
	/* We must burn lots of CPU cycles here, but that does not
	 * really matter as one does not write the SPROM every other minute...
	 */
	printk(KERN_INFO PFX "[ 0%%");
	mdelay(500);
	for (i = 0; i < BCM43xx_SPROM_SIZE; i++) {
		if (i == 16)
			printk("25%%");
		else if (i == 32)
			printk("50%%");
		else if (i == 48)
			printk("75%%");
		else if (i % 2)
			printk(".");
//TODO		bcm43xx_write16(bcm, BCM43xx_SPROM_BASE + (i * 2), sprom[i]);
		mdelay(20);
	}
	spromctl &= ~0x10; /* SPROM WRITE enable. */
	bcm43xx_pci_write_config32(bcm, BCM43xx_PCICFG_SPROMCTL, spromctl);
	if (err) {
		printk(KERN_ERR PFX "Could not access SPROM control register.\n");
		goto out_unlock;
	}
	mdelay(500);
	printk("100%% ]\n");
	printk(KERN_INFO PFX "SPROM written.\n");
	err = 0;
out_unlock:
	spin_unlock_irqrestore(&bcm->lock, flags);
out_kfree:
	kfree(sprom);
out:
	return err;
}


#ifdef WX
# undef WX
#endif
#define WX(ioctl)  [(ioctl) - SIOCSIWCOMMIT]
static const iw_handler bcm43xx_wx_handlers[] = {
	WX(SIOCSIWNICKN)	= bcm43xx_wx_set_nick,
	WX(SIOCGIWNICKN)	= bcm43xx_wx_get_nick,
};
#undef WX

static const iw_handler bcm43xx_priv_wx_handlers[] = {
	/* Set Interference Mitigation Mode. */
	bcm43xx_wx_set_interfmode,
	/* Get Interference Mitigation Mode. */
	bcm43xx_wx_get_interfmode,
	/* Enable/Disable Short Preamble mode. */
	bcm43xx_wx_set_shortpreamble,
	/* Get Short Preamble mode. */
	bcm43xx_wx_get_shortpreamble,
	/* Enable/Disable Software Encryption mode (SoftMAC only). */
	bcm43xx_wx_dummy,
	/* Get Software Encryption mode (SoftMAC only). */
	bcm43xx_wx_dummy,
	/* Write SRPROM data. */
	bcm43xx_wx_sprom_write,
	/* Read SPROM data. */
	bcm43xx_wx_sprom_read,
};

#define PRIV_WX_SET_INTERFMODE		(SIOCIWFIRSTPRIV + 0)
#define PRIV_WX_GET_INTERFMODE		(SIOCIWFIRSTPRIV + 1)
#define PRIV_WX_SET_SHORTPREAMBLE	(SIOCIWFIRSTPRIV + 2)
#define PRIV_WX_GET_SHORTPREAMBLE	(SIOCIWFIRSTPRIV + 3)
#define PRIV_WX_SET_SWENCRYPTION	(SIOCIWFIRSTPRIV + 4)
#define PRIV_WX_GET_SWENCRYPTION	(SIOCIWFIRSTPRIV + 5)
#define PRIV_WX_SPROM_WRITE		(SIOCIWFIRSTPRIV + 6)
#define PRIV_WX_SPROM_READ		(SIOCIWFIRSTPRIV + 7)

#define PRIV_WX_DUMMY(ioctl)	\
	{					\
		.cmd		= (ioctl),	\
		.name		= "__unused"	\
	}

static const struct iw_priv_args bcm43xx_priv_wx_args[] = {
	{
		.cmd		= PRIV_WX_SET_INTERFMODE,
		.set_args	= IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name		= "set_interfmode",
	},
	{
		.cmd		= PRIV_WX_GET_INTERFMODE,
		.get_args	= IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | MAX_WX_STRING,
		.name		= "get_interfmode",
	},
	{
		.cmd		= PRIV_WX_SET_SHORTPREAMBLE,
		.set_args	= IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
		.name		= "set_shortpreambl",
	},
	{
		.cmd		= PRIV_WX_GET_SHORTPREAMBLE,
		.get_args	= IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | MAX_WX_STRING,
		.name		= "get_shortpreambl",
	},
	PRIV_WX_DUMMY(PRIV_WX_SET_SWENCRYPTION),
	PRIV_WX_DUMMY(PRIV_WX_GET_SWENCRYPTION),
	{
		.cmd		= PRIV_WX_SPROM_WRITE,
		.set_args	= IW_PRIV_TYPE_CHAR | SPROM_BUFFERSIZE,
		.name		= "write_sprom",
	},
	{
		.cmd		= PRIV_WX_SPROM_READ,
		.get_args	= IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | SPROM_BUFFERSIZE,
		.name		= "read_sprom",
	},
};

const struct iw_handler_def bcm43xx_wx_handlers_def = {
	.standard		= bcm43xx_wx_handlers,
	.num_standard		= ARRAY_SIZE(bcm43xx_wx_handlers),
	.num_private		= ARRAY_SIZE(bcm43xx_priv_wx_handlers),
	.num_private_args	= ARRAY_SIZE(bcm43xx_priv_wx_args),
	.private		= bcm43xx_priv_wx_handlers,
	.private_args		= bcm43xx_priv_wx_args,
};

/* vim: set ts=8 sw=8 sts=8: */
