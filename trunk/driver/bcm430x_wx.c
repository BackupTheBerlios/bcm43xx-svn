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
#include <net/ieee80211softmac_wx.h>

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

#define MAX_WX_STRING		80


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
	if (channel == 0xFF)
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
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	struct iw_range *range = (struct iw_range *)extra;
	const struct ieee80211_geo *geo;
	unsigned long flags;
	int i, j;

	wx_enter();

	data->data.length = sizeof(*range);
	memset(range, 0, sizeof(*range));

	//TODO: What about 802.11b?
	/* 54Mb/s == ~27Mb/s payload throughput (802.11g) */
	range->throughput = 27 * 1000 * 1000;

	range->max_qual.qual = 100;
	/* TODO: Real max RSSI */
	range->max_qual.level = 0;
	range->max_qual.noise = 0;
	range->max_qual.updated = 7;

	range->avg_qual.qual = 70;
	range->avg_qual.level = 0;
	range->avg_qual.noise = 0;
	range->avg_qual.updated = 7;

	range->min_rts = BCM430x_MIN_RTS_THRESHOLD;
	range->max_rts = BCM430x_MAX_RTS_THRESHOLD;
	range->min_frag = MIN_FRAG_THRESHOLD;
	range->max_frag = MAX_FRAG_THRESHOLD;

	range->encoding_size[0] = 5;
	range->encoding_size[1] = 13;
	range->num_encoding_sizes = 2;
	range->max_encoding_tokens = WEP_KEYS;

	range->we_version_compiled = WIRELESS_EXT;
	range->we_version_source = 16;

	spin_lock_irqsave(&bcm->lock, flags);

	range->num_bitrates = 0;
	i = 0;
	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
	case BCM430x_PHYTYPE_G:
		range->num_bitrates += 4;
		range->bitrate[i++] = IEEE80211_CCK_RATE_1MB;
		range->bitrate[i++] = IEEE80211_CCK_RATE_2MB;
		range->bitrate[i++] = IEEE80211_CCK_RATE_5MB;
		range->bitrate[i++] = IEEE80211_CCK_RATE_11MB;
	case BCM430x_PHYTYPE_B:
		range->num_bitrates += 8;
		range->bitrate[i++] = IEEE80211_OFDM_RATE_6MB;
		range->bitrate[i++] = IEEE80211_OFDM_RATE_9MB;
		range->bitrate[i++] = IEEE80211_OFDM_RATE_12MB;
		range->bitrate[i++] = IEEE80211_OFDM_RATE_18MB;
		range->bitrate[i++] = IEEE80211_OFDM_RATE_24MB;
		range->bitrate[i++] = IEEE80211_OFDM_RATE_36MB;
		range->bitrate[i++] = IEEE80211_OFDM_RATE_48MB;
		range->bitrate[i++] = IEEE80211_OFDM_RATE_54MB;
	}

	geo = ieee80211_get_geo(bcm->ieee);
	range->num_channels = geo->a_channels + geo->bg_channels;
	j = 0;
	for (i = 0; i < geo->a_channels; i++) {
		if (j == IW_MAX_FREQUENCIES)
			break;
		range->freq[j].i = j + 1;
		range->freq[j].m = geo->a[i].freq;//FIXME?
		range->freq[j].e = 1;
		j++;
	}
	for (i = 0; i < geo->bg_channels; i++) {
		if (j == IW_MAX_FREQUENCIES)
			break;
		range->freq[j].i = j + 1;
		range->freq[j].m = geo->bg[i].freq;//FIXME?
		range->freq[j].e = 1;
		j++;
	}
	range->num_frequency = j;

	spin_unlock_irqrestore(&bcm->lock, flags);

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

static int bcm430x_wx_get_scanresults(struct net_device *net_dev,
				      struct iw_request_info *info,
				      union iwreq_data *data,
				      char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);

	wx_enter();

	return ieee80211_wx_get_scan(bcm->ieee, info, data, extra);
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
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	int err = -ENODEV;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	if (!bcm->initialized)
		goto out_unlock;
	if (data->power.disabled != (!(bcm->current_core->radio->enabled))) {
		if (data->power.disabled)
			err = bcm430x_radio_turn_off(bcm);
		else
			err = bcm430x_radio_turn_on(bcm);
		if (err)
			goto out_unlock;
	}
	//TODO: set txpower.
	err = 0;

out_unlock:
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
}

static int bcm430x_wx_get_xmitpower(struct net_device *net_dev,
				    struct iw_request_info *info,
				    union iwreq_data *data,
				    char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
//TODO	data->power.value = ???
	data->power.fixed = 1;
	data->power.flags = IW_TXPOW_DBM;
	data->power.disabled = !(bcm->current_core->radio->enabled);
	spin_unlock_irqrestore(&bcm->lock, flags);

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

static int bcm430x_wx_set_interfmode(struct net_device *net_dev,
				     struct iw_request_info *info,
				     union iwreq_data *data,
				     char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	int mode, err = 0;

	wx_enter();

	mode = *((int *)extra);
	switch (mode) {
	case 0:
		mode = BCM430x_RADIO_INTERFMODE_NONE;
		break;
	case 1:
		mode = BCM430x_RADIO_INTERFMODE_NONWLAN;
		break;
	case 2:
		mode = BCM430x_RADIO_INTERFMODE_MANUALWLAN;
		break;
	case 3:
		mode = BCM430x_RADIO_INTERFMODE_AUTOWLAN;
		break;
	default:
		printk(KERN_ERR PFX "set_interfmode allowed parameters are: "
				    "0 => None,  1 => Non-WLAN,  2 => WLAN,  "
				    "3 => Auto-WLAN\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&bcm->lock, flags);
	if (bcm->initialized) {
		err = bcm430x_radio_set_interference_mitigation(bcm, mode);
		if (err) {
			printk(KERN_ERR PFX "Interference Mitigation not "
					    "supported by device\n");
		}
	} else {
		if (mode == BCM430x_RADIO_INTERFMODE_AUTOWLAN) {
			printk(KERN_ERR PFX "Interference Mitigation mode Auto-WLAN "
					    "not supported while the interface is down.\n");
			err = -ENODEV;
		} else
			bcm->current_core->radio->interfmode = mode;
	}
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
}

static int bcm430x_wx_get_interfmode(struct net_device *net_dev,
				     struct iw_request_info *info,
				     union iwreq_data *data,
				     char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	int mode;

	wx_enter();

	spin_lock_irqsave(&bcm->lock, flags);
	mode = bcm->current_core->radio->interfmode;
	spin_unlock_irqrestore(&bcm->lock, flags);

	switch (mode) {
	case BCM430x_RADIO_INTERFMODE_NONE:
		strncpy(extra, "0 (No Interference Mitigation)", MAX_WX_STRING);
		break;
	case BCM430x_RADIO_INTERFMODE_NONWLAN:
		strncpy(extra, "1 (Non-WLAN Interference Mitigation)", MAX_WX_STRING);
		break;
	case BCM430x_RADIO_INTERFMODE_MANUALWLAN:
		strncpy(extra, "2 (WLAN Interference Mitigation)", MAX_WX_STRING);
		break;
	default:
		assert(0);
	}
	data->data.length = strlen(extra) + 1;

	return 0;
}

static int bcm430x_wx_set_shortpreamble(struct net_device *net_dev,
					struct iw_request_info *info,
					union iwreq_data *data,
					char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;
	int on;

	wx_enter();

	on = *((int *)extra);
	spin_lock_irqsave(&bcm->lock, flags);
	bcm->short_preamble = !!on;
	spin_unlock_irqrestore(&bcm->lock, flags);

	return 0;
}

static int bcm430x_wx_get_shortpreamble(struct net_device *net_dev,
					struct iw_request_info *info,
					union iwreq_data *data,
					char *extra)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
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


#ifdef WX
# undef WX
#endif
#define WX(ioctl)  [(ioctl) - SIOCSIWCOMMIT]
static const iw_handler bcm430x_wx_handlers[] = {
	/* Wireless Identification */
	WX(SIOCGIWNAME)		= bcm430x_wx_get_name,
	/* Basic operations */
	WX(SIOCSIWFREQ)		= bcm430x_wx_set_channelfreq,
	WX(SIOCGIWFREQ)		= bcm430x_wx_get_channelfreq,
	WX(SIOCSIWMODE)		= bcm430x_wx_set_mode,
	WX(SIOCGIWMODE)		= bcm430x_wx_get_mode,
	/* Informative stuff */
	WX(SIOCGIWRANGE)	= bcm430x_wx_get_rangeparams,
	/* Access Point manipulation */
//TODO	WX(SIOCSIWAP)		= bcm430x_wx_set_apmac,
//TODO	WX(SIOCGIWAP)		= bcm430x_wx_get_apmac,
	WX(SIOCSIWSCAN)		= ieee80211softmac_wx_trigger_scan,
	WX(SIOCGIWSCAN)		= bcm430x_wx_get_scanresults,
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
	WX(SIOCSIWTXPOW)	= bcm430x_wx_set_xmitpower,
	WX(SIOCGIWTXPOW)	= bcm430x_wx_get_xmitpower,
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

static const iw_handler bcm430x_priv_wx_handlers[] = {
	/* Set Interference Mitigation Mode. */
	bcm430x_wx_set_interfmode,
	/* Get Interference Mitigation Mode. */
	bcm430x_wx_get_interfmode,
	/* Enable/Disable Short Preamble mode. */
	bcm430x_wx_set_shortpreamble,
	/* Get Short Preamble mode. */
	bcm430x_wx_get_shortpreamble,
};

#define PRIV_WX_SET_INTERFMODE		(SIOCIWFIRSTPRIV + 0)
#define PRIV_WX_GET_INTERFMODE		(SIOCIWFIRSTPRIV + 1)
#define PRIV_WX_SET_SHORTPREAMBLE	(SIOCIWFIRSTPRIV + 2)
#define PRIV_WX_GET_SHORTPREAMBLE	(SIOCIWFIRSTPRIV + 3)

static const struct iw_priv_args bcm430x_priv_wx_args[] = {
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
};

const struct iw_handler_def bcm430x_wx_handlers_def = {
	.standard		= bcm430x_wx_handlers,
	.num_standard		= ARRAY_SIZE(bcm430x_wx_handlers),
	.num_private		= ARRAY_SIZE(bcm430x_priv_wx_handlers),
	.num_private_args	= ARRAY_SIZE(bcm430x_priv_wx_args),
	.private		= bcm430x_priv_wx_handlers,
	.private_args		= bcm430x_priv_wx_args,
};

/* vim: set ts=8 sw=8 sts=8: */
