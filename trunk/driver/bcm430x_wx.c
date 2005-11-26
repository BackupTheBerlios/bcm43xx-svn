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
#define wx_enter()		printk_wx(KERN_INFO PFX "WX handler called: %s\n", __FUNCTION__);


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

	wx_enter();

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

	wx_enter();

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

	wx_enter();

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

	wx_enter();

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

	wx_enter();

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
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_sensitivity(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_rangeparams(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_apmac(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_apmac(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_trigger_scan(struct net_device *net_dev,
				   struct iw_request_info *info,
				   union iwreq_data *data,
				   char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_scanresults(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_essid(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_essid(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_nick(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
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

static int bcm430x_wx_get_nick(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
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

	wx_enter();

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

	wx_enter();

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
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	int err = -EINVAL;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	if (data->rts.disabled) {
		bcm->rts_threshold = BCM430x_MAX_RTS_THRESHOLD;
		err = 0;
	} else {
		if (data->rts.value >= BCM430x_MIN_RTS_THRESHOLD &&
		    data->rts.value <= BCM430x_MAX_RTS_THRESHOLD) {
			bcm->rts_threshold = data->rts.value;
			err = 0;
		}
	}
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
}

static int bcm430x_wx_get_rts(struct net_device *net_dev,
			      struct iw_request_info *info,
			      union iwreq_data *data,
			      char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	data->rts.value = bcm->rts_threshold;
	data->rts.fixed = 0;
	data->rts.disabled = (bcm->rts_threshold == BCM430x_MAX_RTS_THRESHOLD);
	spin_unlock_irqrestore(&bcm->lock, flags);

	return 0;
}

static int bcm430x_wx_set_frag(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	int err = -EINVAL;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	if (data->frag.disabled) {
		bcm->ieee->fts = MAX_FRAG_THRESHOLD;
		err = 0;
	} else {
		if (data->frag.value >= MIN_FRAG_THRESHOLD &&
		    data->frag.value <= MAX_FRAG_THRESHOLD) {
			bcm->ieee->fts = data->frag.value & ~0x1;
			err = 0;
		}
	}
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
}

static int bcm430x_wx_get_frag(struct net_device *net_dev,
			       struct iw_request_info *info,
			       union iwreq_data *data,
			       char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	data->frag.value = bcm->ieee->fts;
	data->frag.fixed = 0;
	data->frag.disabled = (bcm->ieee->fts == MAX_FRAG_THRESHOLD);
	spin_unlock_irqrestore(&bcm->lock, flags);

	return 0;
}

static int bcm430x_wx_set_xmitpower(struct net_device *net_dev,
				    struct iw_request_info *info,
				    union iwreq_data *data,
				    char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_xmitpower(struct net_device *net_dev,
				    struct iw_request_info *info,
				    union iwreq_data *data,
				    char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_set_retry(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_retry(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	wx_enter();
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

	wx_enter();

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

	wx_enter();

	err = ieee80211_wx_get_encode(bcm->ieee, info, data, extra);

	return err;
}

static int bcm430x_wx_set_power(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}

static int bcm430x_wx_get_power(struct net_device *net_dev,
				struct iw_request_info *info,
				union iwreq_data *data,
				char *extra)
{
	wx_enter();
	/*TODO*/
	return 0;
}


#ifdef WX
# undef WX
#endif
#define WX(ioctl)  [(ioctl) - SIOCSIWCOMMIT]
static iw_handler bcm430x_wx_handlers[] = {
	/* Wireless Identification */
	WX(SIOCGIWNAME)		= bcm430x_wx_get_name,
	/* Basic operations */
	WX(SIOCSIWFREQ)		= bcm430x_wx_set_channelfreq,
	WX(SIOCGIWFREQ)		= bcm430x_wx_get_channelfreq,
	WX(SIOCSIWMODE)		= bcm430x_wx_set_mode,
	WX(SIOCGIWMODE)		= bcm430x_wx_get_mode,
	/* Informative stuff */
//TODO	WX(SIOCGIWRANGE)	= bcm430x_wx_get_rangeparams,
	/* Access Point manipulation */
//TODO	WX(SIOCSIWAP)		= bcm430x_wx_set_apmac,
//TODO	WX(SIOCGIWAP)		= bcm430x_wx_get_apmac,
//TODO	WX(SIOCSIWSCAN)		= bcm430x_wx_trigger_scan,
//TODO	WX(SIOCGIWSCAN)		= bcm430x_wx_get_scanresults,
	/* 802.11 specific support */
//TODO	WX(SIOCSIWESSID)	= bcm430x_wx_set_essid,
//TODO	WX(SIOCGIWESSID)	= bcm430x_wx_get_essid,
	WX(SIOCSIWNICKN)	= bcm430x_wx_set_nick,
	WX(SIOCGIWNICKN)	= bcm430x_wx_get_nick,
	/* Other parameters */
	WX(SIOCSIWRATE)		= bcm430x_wx_set_defaultrate,
	WX(SIOCGIWRATE)		= bcm430x_wx_get_defaultrate,
	WX(SIOCSIWRTS)		= bcm430x_wx_set_rts,
	WX(SIOCGIWRTS)		= bcm430x_wx_get_rts,
	WX(SIOCSIWFRAG)		= bcm430x_wx_set_frag,
	WX(SIOCGIWFRAG)		= bcm430x_wx_get_frag,
//TODO	WX(SIOCSIWTXPOW)	= bcm430x_wx_set_xmitpower,
//TODO	WX(SIOCGIWTXPOW)	= bcm430x_wx_get_xmitpower,
//TODO	WX(SIOCSIWRETRY)	= bcm430x_wx_set_retry,
//TODO	WX(SIOCGIWRETRY)	= bcm430x_wx_get_retry,
	/* Encoding */
	WX(SIOCSIWENCODE)	= bcm430x_wx_set_encoding,
	WX(SIOCGIWENCODE)	= bcm430x_wx_get_encoding,
	/* Power saving */
//TODO	WX(SIOCSIWPOWER)	= bcm430x_wx_set_power,
//TODO	WX(SIOCGIWPOWER)	= bcm430x_wx_get_power,
};
#undef WX

const struct iw_handler_def bcm430x_wx_handlers_def = {
	.standard = bcm430x_wx_handlers,
	.num_standard = ARRAY_SIZE(bcm430x_wx_handlers),
};
