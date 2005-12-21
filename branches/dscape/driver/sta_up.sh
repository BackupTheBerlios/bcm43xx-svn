#!/bin/bash

if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
	echo "$0, an ugly script to configure and bring up a STA (802.11 station)"
	echo "device for the linux devicescape 802.11 stack."
	echo
	echo "Usage:"
	echo "$0 [ wlan_device  local_ip  wpasupplicant_config ]"
	echo
	echo "Examples:"
	echo "Run with default parameters:  $0"
	echo "Manually define parameters:   $0 wlan0 192.168.1.1 ./wpasupp.conf"
	echo
	echo "Default parameters are:  $0 wlan0 192.168.1.101 /etc/wpa_supplicant.conf"
	exit 1
fi

wlan_dev="$1"
ip_addr="$2"
wpasupp_conf="$3"

if [ -z "$wlan_dev" ]; then
	wlan_dev="wlan0"
fi
if [ -z "$sta_dev" ]; then
	sta_dev="sta0"
fi
if [ -z "$ip_addr" ]; then
	ip_addr="192.168.1.101"
fi
if [ -z "$wpasupp_conf" ]; then
	wpasupp_conf="/etc/wpa_supplicant.conf"
fi

idx=$(echo $wlan_dev | awk '{ gsub("[^0-9]", "", $0); printf($0); }')
if [ -z "$idx" ]; then
	echo "Invalid wlan_device parameter \"$wlan_dev\".  Example: wlan0"
	exit 1
fi
sta_dev="sta$idx"

function run()
{
	echo "$@"
	$@
	res=$?
	if [ $res -ne 0 ]; then
		echo "FAILED ($res)"
		exit 1
	fi
}

if [ -z "$(grep -e bcm43xx /proc/modules)" ]; then
	echo "ERROR: bcm43xx module not loaded."
	exit 1
fi

add_sta_source="./.add_sta.c"
add_sta="./.add_sta"
cat > $add_sta_source <<__EOF__
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#define PRISM2_HOSTAPD_ADD_IF	1006
#define HOSTAP_IF_STA		4
#define PRISM2_IOCTL_HOSTAPD	(0x8BE0 + 3)
#define IFNAMSIZ		16
#define ETH_ALEN		6
struct iwreq {
	union { char ifrn_name[IFNAMSIZ]; } ifr_ifrn;
	union {
		char name[IFNAMSIZ];
		struct {
			void *pointer;
			uint16_t length;
			uint16_t flags;
		} data;
	} u;
};
struct prism2_hostapd_param {
	uint32_t cmd;
	unsigned char sta_addr[ETH_ALEN];
	unsigned char pad[2];
	union {
		struct {
			unsigned char type;
			unsigned char name[IFNAMSIZ];
			unsigned char data[0] __attribute__((aligned));
		} if_info;
		struct { unsigned char d[80]; } dummy;
	} u;
};

int main(int argc, char **argv)
{
	struct iwreq w;
	struct prism2_hostapd_param parm;
	int hnd, res;

	strncpy(w.ifr_ifrn.ifrn_name, argv[1], IFNAMSIZ);
	w.ifr_ifrn.ifrn_name[IFNAMSIZ - 1] = 0;
	w.u.data.pointer = &parm;
	w.u.data.length = sizeof(parm);
	parm.cmd = PRISM2_HOSTAPD_ADD_IF;
	parm.u.if_info.type = HOSTAP_IF_STA;
	strncpy((char *)parm.u.if_info.name, argv[2], IFNAMSIZ);
	hnd = socket(PF_INET, SOCK_DGRAM, 0);
	if (hnd < 0)
		return 1;
	res = ioctl(hnd, PRISM2_IOCTL_HOSTAPD, &w);
	close(hnd);
	if (res < 0)
		return 2;
	return 0;
}
__EOF__
run gcc -O0 -o $add_sta $add_sta_source

killall wpa_supplicant 2>/dev/null
run $add_sta $wlan_dev $sta_dev
run iwconfig $wlan_dev.11 mode managed
run ifconfig $wlan_dev.11 up

hwaddr="$(ifconfig | grep $wlan_dev.11 | awk '{print $NF}')"
run ifconfig $sta_dev hw ether $hwaddr
run ifconfig $sta_dev $ip_addr
run ifconfig $sta_dev up
run iwconfig $sta_dev mode managed

run wpa_supplicant -B -Ddscape -i$sta_dev -c$wpasupp_conf

echo
echo "You may want to set the default route, now:"
echo " route add default gw GATEWAY_IP_ADDRESS"

exit 0
