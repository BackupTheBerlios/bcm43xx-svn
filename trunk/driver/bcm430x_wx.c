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

static int bcm430x_wx_get_name(struct net_device *dev,
                               struct iw_request_info *info,
			       union iwreq_data *data, char *extra)
{
	snprintf(data->name, IFNAMSIZ, "IEEE 802.11");
	return 0;
}

static int bcm430x_wx_get_mode(struct net_device *dev,
			       struct iw_request_info *info,
			       union iwreq_data *data, char *extra)
{
	struct bcm430x_private *priv = ieee80211_priv(dev);
	data->mode = priv->ieee->iw_mode;
	return 0;
}

static iw_handler bcm430x_wx_handlers[] = {
	NULL,
	bcm430x_wx_get_name,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	bcm430x_wx_get_mode,

};

const struct iw_handler_def bcm430x_wx_handlers_def = {
	.standard = bcm430x_wx_handlers,
	.num_standard = sizeof(bcm430x_wx_handlers)/sizeof(iw_handler),
};
