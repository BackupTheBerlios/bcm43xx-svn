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

#include <linux/wireless.h>
#include <net/iw_handler.h>

#include "bcm430x.h"
#include "bcm430x_wx.h"

/* Define to enable a printk on each wx handler function invocation */
#define BCM430x_WX_DEBUG


#ifdef BCM430x_WX_DEBUG
# define printk_wx		printk
#else
# define printk_wx(x...)	do { /* nothing */ } while (0)
#endif


static int bcm430x_wx_get_name(struct net_device *net_dev,
                               struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	int i = 0;
	char suffix[6] = { 0 };

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	down(&bcm->sem);

	if ((bcm->phy[0].type == BCM430x_PHYTYPE_A) || (bcm->phy[0].type == BCM430x_PHYTYPE_A)) {
		suffix[i++] = 'a';
		suffix[i++] = '/';
	}
	if ((bcm->phy[0].type == BCM430x_PHYTYPE_G) || (bcm->phy[0].type == BCM430x_PHYTYPE_G)) {
		suffix[i++] = 'b';
		suffix[i++] = '/';
		suffix[i++] = 'g';
	} else {
		suffix[i++] = 'b';
	}
	snprintf(data->name, IFNAMSIZ, "IEEE 802.11%s", suffix);
	up(&bcm->sem);

	return 0;
}

static int bcm430x_wx_set_channelfreq(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_channelfreq(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	static const u16 frequencies_bg[14] = {
	        12, 17, 22, 27,
		32, 37, 42, 47,
		52, 57, 62, 67,
		72, 84,
	};

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	down(&bcm->sem);

	//XXX: returning current_core's channel...
	data->freq.e = 6;
	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		data->freq.m = 5000 + 5 * bcm->current_core->radio->channel;
		break;
	case BCM430x_PHYTYPE_B:
	case BCM430x_PHYTYPE_G:
		data->freq.m = 2400 + frequencies_bg[bcm->current_core->radio->channel];
		break;
	}

	up(&bcm->sem);
	
	return 0;
}

static int bcm430x_wx_set_mode(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	down(&bcm->sem);
	spin_lock_irqsave(&bcm->lock, flags);
	if (data->mode == IW_MODE_AUTO)
		bcm->ieee->iw_mode = BCM430x_INITIAL_IWMODE;
	else
		bcm->ieee->iw_mode = data->mode;
	if (bcm->status & BCM430x_STAT_BOARDINITDONE) {
		/*TODO: commit the new mode to the device (StatusBitField?) */
		printk(KERN_ERR PFX "TODO: mode not committed!\n");
	}
	spin_unlock_irqrestore(&bcm->lock, flags);
	up(&bcm->sem);

	return 0;
}

static int bcm430x_wx_get_mode(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	down(&bcm->sem);
	data->mode = bcm->ieee->iw_mode;
	up(&bcm->sem);

	return 0;
}

static int bcm430x_wx_set_sensitivity(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_sensitivity(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_rangeparams(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_privinfo(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_apmac(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_apmac(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_trigger_scan(struct net_device *net_dev,
				   struct iw_request_info *info,
				   union iwreq_data *data,
				   char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_scanresults(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_essid(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_essid(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_nick(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_nick(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_defaultrate(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_defaultrate(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_rts(struct net_device *net_dev,
			      struct iw_request_info *info,
			      union iwreq_data *data,
			      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_rts(struct net_device *net_dev,
			      struct iw_request_info *info,
			      union iwreq_data *data,
			      char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_frag(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_frag(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_xmitpower(struct net_device *net_dev,
				    struct iw_request_info *info,
				    union iwreq_data *data,
				    char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_xmitpower(struct net_device *net_dev,
				    struct iw_request_info *info,
				    union iwreq_data *data,
				    char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_retry(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_retry(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_encoding(struct net_device *net_dev,
				   struct iw_request_info *info,
				   union iwreq_data *data,
				   char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	int err;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	down(&bcm->sem);
	err = ieee80211_wx_set_encode(bcm->ieee, info, data, extra);
	up(&bcm->sem);

	return err;
}

static int bcm430x_wx_get_encoding(struct net_device *net_dev,
				   struct iw_request_info *info,
				   union iwreq_data *data,
				   char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	int err;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	down(&bcm->sem);
	err = ieee80211_wx_get_encode(bcm->ieee, info, data, extra);
	up(&bcm->sem);

	return err;
}

static int bcm430x_wx_set_power(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_power(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);
	/*TODO*/
	return 0;
}


static iw_handler bcm430x_wx_handlers[] = {
		/* Wireless Identification */
/* 0x8B00 */	NULL, /* commit pending changes */
/* 0x8B01 */	bcm430x_wx_get_name,

		/* Basic operations */
/* 0x8B02 */	NULL, /* set network id (pre-802.11) */
/* 0x8B03 */	NULL, /* get network id (the cell)  FIXME: Implement? */
/* 0x8B04 */	NULL,//TODO bcm430x_wx_set_channelfreq,
/* 0x8B05 */	bcm430x_wx_get_channelfreq,
/* 0x8B06 */	bcm430x_wx_set_mode,
/* 0x8B07 */	bcm430x_wx_get_mode,
/* 0x8B08 */	NULL,//TODO bcm430x_wx_set_sensitivity,
/* 0x8B09 */	NULL,//TODO bcm430x_wx_get_sensitivity,

		/* Informative stuff */
/* 0x8B0A */	NULL, /* unused */
/* 0x8B0B */	NULL,//TODO bcm430x_wx_get_rangeparams,
/* 0x8B0C */	NULL, /* unused */
/* 0x8B0D */	NULL,//TODO bcm430x_wx_get_privinfo,
/* 0x8B0E */	NULL, /* unused */
/* 0x8B0F */	NULL, /* get /proc/net/wireless stats */

		/* Spy support */
/* 0x8B10 */	NULL, /* set spy addresses */
/* 0x8B11 */	NULL, /* get spy info (quality of link) */
/* 0x8B12 */	NULL, /* set spy threshold (spy event) */
/* 0x8B13 */	NULL, /* get spy threshold */

		/* Access Point manipulation */
/* 0x8B14 */	NULL,//TODO bcm430x_wx_set_apmac,
/* 0x8B15 */	NULL,//TODO bcm430x_wx_get_apmac,

		/* WPA: IEEE 802.11 MLME requests */
/* 0x8B16 */	NULL, /* request MLME operation */

		/* Access Point maniputaion (continued) */
/* 0x8B17 */	NULL, /* deprecated in favour of scanning */
/* 0x8B18 */	NULL,//TODO bcm430x_wx_trigger_scan,
/* 0x8B19 */	NULL,//TODO bcm430x_wx_get_scanresults,

		/* 802.11 specific support */
/* 0x8B1A */	NULL,//TODO bcm430x_wx_set_essid,
/* 0x8B1B */	NULL,//TODO bcm430x_wx_get_essid,
/* 0x8B1C */	NULL,//TODO bcm430x_wx_set_nick,
/* 0x8B1D */	NULL,//TODO bcm430x_wx_get_nick,

/* 0x8B1E */	NULL, /* unused */
/* 0x8B1F */	NULL, /* unused */

		/* Other parameters */
/* 0x8B20 */	NULL,//TODO bcm430x_wx_set_defaultrate,
/* 0x8B21 */	NULL,//TODO bcm430x_wx_get_defaultrate,
/* 0x8B22 */	NULL,//TODO bcm430x_wx_set_rts,
/* 0x8B23 */	NULL,//TODO bcm430x_wx_get_rts,
/* 0x8B24 */	NULL,//TODO bcm430x_wx_set_frag,
/* 0x8B25 */	NULL,//TODO bcm430x_wx_get_frag,
/* 0x8B26 */	NULL,//TODO bcm430x_wx_set_xmitpower,
/* 0x8B27 */	NULL,//TODO bcm430x_wx_get_xmitpower,
/* 0x8B28 */	NULL,//TODO bcm430x_wx_set_retry,
/* 0x8B29 */	NULL,//TODO bcm430x_wx_get_retry,

		/* Encoding */
/* 0x8B2A */	bcm430x_wx_set_encoding,
/* 0x8B2B */	bcm430x_wx_get_encoding,

		/* Power saving */
/* 0x8B2C */	NULL,//TODO bcm430x_wx_set_power,
/* 0x8B2D */	NULL,//TODO bcm430x_wx_get_power,

/* 0x8B2E */	NULL, /* unused */
/* 0x8B2F */	NULL, /* unused */

		/* WPA: Generic IEEE 802.11 information element */
/* 0x8B30 */	NULL, /* set generic IE */
/* 0x8B31 */	NULL, /* get generic IE */

		/* WPA: Authentication mode parameters */
/* 0x8B32 */	NULL, /* set authentication mode params */
/* 0x8B33 */	NULL, /* get authentication mode params */

		/* WPA: Extended version of encoding configuration */
/* 0x8B34 */	NULL, /* set encoding token & mode */
/* 0x8B35 */	NULL, /* get encoding token & mode */

		/* WPA2: PMKSA cache management */
/* 0x8B36 */	NULL, /* PMKSA cache operation */
};

const struct iw_handler_def bcm430x_wx_handlers_def = {
	.standard = bcm430x_wx_handlers,
	.num_standard = ARRAY_SIZE(bcm430x_wx_handlers),
};
