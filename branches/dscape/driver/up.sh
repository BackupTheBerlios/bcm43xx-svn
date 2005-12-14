#!/bin/bash

if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
	echo "Usage: $0 wlan0 sta0 path_to__add_sta__binary"
	exit 1
fi

wlan_dev="$1"
sta_dev="$2"
add_sta_bin="$3"

if [ -z "$wlan_dev" ]; then
	wlan_dev="wlan0"
fi
if [ -z "$sta_dev" ]; then
	sta_dev="sta0"
fi
if [ -z "$add_sta_bin" ]; then
	add_sta_bin="../add_sta/add_sta"
	if ! [ -x "$add_sta_bin" ]; then
		add_sta_bin="add_sta"
	fi
fi

function run()
{
	echo "$@"
	$@
	[ $? -eq 0 ] || exit 1
}


run $add_sta_bin $wlan_dev $sta_dev
run iwconfig $wlan_dev.11 mode managed
run ifconfig $wlan_dev.11 up

run ifconfig $sta_dev hw ether 00:11:24:A0:DE:8B
run ifconfig $sta_dev up
run iwconfig $sta_dev mode managed

run iwconfig $sta_dev channel 1
run iwconfig $sta_dev essid OpenWrt
run iwconfig $sta_dev ap 00:90:4C:67:04:00

exit 0


run iwconfig $wlan_dev mode managed
run $add_sta_bin $wlan_dev $sta_dev
run ifconfig $sta_dev up
run ifconfig "${wlan_dev}.11" up

#run iwlist $sta_dev scan

