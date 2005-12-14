#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include "hostapd_ioctl.h"

int main(int argc, char **argv)
{
	struct iwreq w;
	struct prism2_hostapd_param parm;
	int hnd, res;

	if (argc < 3) {
		printf("Usage: add_sta wlan0 sta0\n");
		return 1;
	}
	strncpy(w.ifr_ifrn.ifrn_name, argv[1], IFNAMSIZ);
	w.ifr_ifrn.ifrn_name[IFNAMSIZ - 1] = 0;
	w.u.data.pointer = &parm;
	w.u.data.length = sizeof(parm);
	parm.cmd = PRISM2_HOSTAPD_ADD_IF;
	parm.u.if_info.type = HOSTAP_IF_STA;
	strncpy((char *)parm.u.if_info.name, argv[2], IFNAMSIZ);
	hnd = socket(PF_INET, SOCK_DGRAM, 0);
	if (hnd < 0) {
		printf("Error opening socket (%d)\n", hnd);
		return 1;
	}
	res = ioctl(hnd, PRISM2_IOCTL_HOSTAPD, &w);
	close(hnd);
	if (res < 0) {
		printf("Error (%d)\n", res);
		return 1;
	}
	return 0;
}
