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
#include "bcm430x_main.h"
#include "bcm430x_radio.h"

/* Define to enable a printk on each wx handler function invocation */
//#define BCM430x_WX_DEBUG


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
	unsigned long flags;
	int i, nr_80211;
	struct bcm430x_phyinfo *phy;
	char suffix[7] = { 0 };
	int have_a = 0, have_b = 0, have_g = 0;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	spin_lock_irqsave(&bcm->lock, flags);
	nr_80211 = bcm430x_num_80211_cores(bcm);
	for (i = 0; i < nr_80211; i++) {
		phy = bcm->phy + i;
		switch (phy->type) {
		case BCM430x_PHYTYPE_A:
			have_a = 1;
			break;
		case BCM430x_PHYTYPE_G:
			have_g = 1;
		case BCM430x_PHYTYPE_B:
			have_b = 1;
			break;
		default:
			assert(0);
		}
	}
	spin_unlock_irqrestore(&bcm->lock, flags);

	i = 0;
	if (have_a) {
		suffix[i++] = 'a';
		suffix[i++] = '/';
	}
	if (have_b) {
		suffix[i++] = 'b';
		suffix[i++] = '/';
	}
	if (have_g) {
		suffix[i++] = 'g';
		suffix[i++] = '/';
	}
	if (i != 0) 
		suffix[i - 1] = '\0';

	snprintf(data->name, IFNAMSIZ, "IEEE 802.11%s", suffix);

	return 0;
}

//TODO: Use the ieee80211 equivalents for the following three functions in 2.6.15.
static u8 freq_to_channel(struct bcm430x_private *bcm,
			  int freq)
{
	u8 channel;

	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A) {
		channel = (freq - 5000) / 5;
	} else {
		if (freq == 2484)
			channel = 14;
		else
			channel = (freq - 2407) / 5;
	}

	return channel;
}

static int channel_to_freq(struct bcm430x_private *bcm,
			   u8 channel)
{
	int freq;

	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A) {
		freq = 5000 + (5 * channel);
	} else {
		if (channel == 14)
			freq = 2484;
		else
			freq = 2407 + (5 * channel);
	}

	return freq;
}

static int is_valid_channel(struct bcm430x_private *bcm,
			    u8 channel)
{
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A) {
		if (channel <= 200)
			return 1;
	} else {
		if (channel >= 1 && channel <= 14)
			return 1;
	}

	return 0;
}

static int bcm430x_wx_set_channelfreq(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	u8 channel;
	int freq;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	if ((data->freq.m >= 0) && (data->freq.m <= 1000)) {
		channel = data->freq.m;
		freq = channel_to_freq(bcm, channel);
	} else {
		channel = freq_to_channel(bcm, data->freq.m);
		freq = data->freq.m;
	}
	if (!is_valid_channel(bcm, channel))
		return -EINVAL;

	spin_lock_irqsave(&bcm->lock, flags);
	if (bcm->initialized) {
		bcm430x_disassociate(bcm);
		bcm430x_mac_suspend(bcm);
		bcm430x_radio_selectchannel(bcm, channel, 0);
		bcm430x_mac_enable(bcm);
	} else
		bcm->current_core->radio->initial_channel = channel;
	spin_unlock_irqrestore(&bcm->lock, flags);
	printk_wx(KERN_INFO PFX "Selected channel: %d\n", channel);

	return 0;
}

static int bcm430x_wx_get_channelfreq(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	int err = -ENODEV;
	u16 channel;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	spin_lock_irqsave(&bcm->lock, flags);
	channel = bcm->current_core->radio->channel;
	if (channel == 0xFFFF)
		goto out_unlock;
	assert(channel > 0 && channel <= 1000);
	data->freq.e = 0;
	data->freq.m = channel;

	err = 0;
out_unlock:
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
}

static int bcm430x_wx_set_mode(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	spin_lock_irqsave(&bcm->lock, flags);
	if (data->mode == IW_MODE_AUTO)
		bcm->ieee->iw_mode = BCM430x_INITIAL_IWMODE;
	else
		bcm->ieee->iw_mode = data->mode;
	if (bcm->initialized) {
		/*TODO: commit the new mode to the device (StatusBitField?) */
		printk(KERN_ERR PFX "TODO: mode not committed!\n");
	}
	spin_unlock_irqrestore(&bcm->lock, flags);

	return 0;
}

static int bcm430x_wx_get_mode(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	spin_lock_irqsave(&bcm->lock, flags);
	data->mode = bcm->ieee->iw_mode;
	spin_unlock_irqrestore(&bcm->lock, flags);

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
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	s32 in_rate = data->bitrate.value;
	u8 rate;
	int is_ofdm = 0;
	int err = -EINVAL;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	if (in_rate == -1) {
		/* automatic detect */
		switch (bcm->current_core->phy->type) {
		case BCM430x_PHYTYPE_A:
		case BCM430x_PHYTYPE_G:
			in_rate = 54000000;
			break;
		case BCM430x_PHYTYPE_B:
			in_rate = 11000000;
			break;
		default:
			assert(0);
		}
	}

	switch (in_rate) {
	case 1000000:
		rate = IEEE80211_CCK_RATE_1MB;
		break;
	case 2000000:
		rate = IEEE80211_CCK_RATE_2MB;
		break;
	case 5500000:
		rate = IEEE80211_CCK_RATE_5MB;
		break;
	case 11000000:
		rate = IEEE80211_CCK_RATE_11MB;
		break;
	case 6000000:
		rate = IEEE80211_OFDM_RATE_6MB;
		is_ofdm = 1;
		break;
	case 9000000:
		rate = IEEE80211_OFDM_RATE_9MB;
		is_ofdm = 1;
		break;
	case 12000000:
		rate = IEEE80211_OFDM_RATE_12MB;
		is_ofdm = 1;
		break;
	case 18000000:
		rate = IEEE80211_OFDM_RATE_18MB;
		is_ofdm = 1;
		break;
	case 24000000:
		rate = IEEE80211_OFDM_RATE_24MB;
		is_ofdm = 1;
		break;
	case 36000000:
		rate = IEEE80211_OFDM_RATE_36MB;
		is_ofdm = 1;
		break;
	case 48000000:
		rate = IEEE80211_OFDM_RATE_48MB;
		is_ofdm = 1;
		break;
	case 54000000:
		rate = IEEE80211_OFDM_RATE_54MB;
		is_ofdm = 1;
		break;
	default:
		goto out;
	}

	spin_lock_irqsave(&bcm->lock, flags);

	if (is_ofdm) {
		/* Check if correct modulation for this PHY. */
		if (bcm->current_core->phy->type != BCM430x_PHYTYPE_A &&
		    bcm->current_core->phy->type != BCM430x_PHYTYPE_G)
			goto out_unlock;
		bcm->ieee->modulation = IEEE80211_OFDM_MODULATION;
	} else {
		bcm->ieee->modulation = IEEE80211_CCK_MODULATION;
	}
	/* finally set the rate to be used by TX code. */
	bcm->current_core->phy->default_bitrate = rate;

	err = 0;
out_unlock:	
	spin_unlock_irqrestore(&bcm->lock, flags);
out:
	return err;
}

static int bcm430x_wx_get_defaultrate(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	int err = -EINVAL;

	printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);

	spin_lock_irqsave(&bcm->lock, flags);
	switch (bcm->current_core->phy->default_bitrate) {
	case IEEE80211_CCK_RATE_1MB:
		data->bitrate.value = 1000000;
		break;
	case IEEE80211_CCK_RATE_2MB:
		data->bitrate.value = 2000000;
		break;
	case IEEE80211_CCK_RATE_5MB:
		data->bitrate.value = 5500000;
		break;
	case IEEE80211_CCK_RATE_11MB:
		data->bitrate.value = 11000000;
		break;
	case IEEE80211_OFDM_RATE_6MB:
		data->bitrate.value = 6000000;
		break;
	case IEEE80211_OFDM_RATE_9MB:
		data->bitrate.value = 9000000;
		break;
	case IEEE80211_OFDM_RATE_12MB:
		data->bitrate.value = 12000000;
		break;
	case IEEE80211_OFDM_RATE_18MB:
		data->bitrate.value = 18000000;
		break;
	case IEEE80211_OFDM_RATE_24MB:
		data->bitrate.value = 24000000;
		break;
	case IEEE80211_OFDM_RATE_36MB:
		data->bitrate.value = 36000000;
		break;
	case IEEE80211_OFDM_RATE_48MB:
		data->bitrate.value = 48000000;
		break;
	case IEEE80211_OFDM_RATE_54MB:
		data->bitrate.value = 54000000;
		break;
	default:
		assert(0);
		goto out_up;
	}
	err = 0;
out_up:
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
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

	err = ieee80211_wx_set_encode(bcm->ieee, info, data, extra);

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

	err = ieee80211_wx_get_encode(bcm->ieee, info, data, extra);

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
/* 0x8B04 */	bcm430x_wx_set_channelfreq,
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
/* 0x8B20 */	bcm430x_wx_set_defaultrate,
/* 0x8B21 */	bcm430x_wx_get_defaultrate,
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
