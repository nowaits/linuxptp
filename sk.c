/**
 * @file sk.c
 * @brief Implements protocol independent socket methods.
 * @note Copyright (C) 2012 Richard Cochran <richardcochran@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <errno.h>
#include <time.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <poll.h>

#include "address.h"
#include "ether.h"
#include "missing.h"
#include "print.h"
#include "sk.h"
#if USE_KTIME
#include <ktime.h>
#endif

/* globals */

int sk_tx_timeout = 1;
int sk_check_fupsync;
enum hwts_filter_mode sk_hwts_filter_mode = HWTS_FILTER_NORMAL;

/* private methods */

static void init_ifreq(struct ifreq *ifreq, struct hwtstamp_config *cfg,
	const char *device)
{
	memset(ifreq, 0, sizeof(*ifreq));
	memset(cfg, 0, sizeof(*cfg));

	strncpy(ifreq->ifr_name, device, sizeof(ifreq->ifr_name) - 1);

	ifreq->ifr_data = (void *) cfg;
}

static int hwts_init(int fd, const char *device, int rx_filter,
	int rx_filter2, int tx_type)
{
	struct ifreq ifreq;
	struct hwtstamp_config cfg;
	int orig_rx_filter;
	int err;

	init_ifreq(&ifreq, &cfg, device);

	/* Test if VLAN over bond is supported. */
	cfg.flags = HWTSTAMP_FLAG_BONDED_PHC_INDEX;
	err = ioctl(fd, SIOCGHWTSTAMP, &ifreq);
	if (err < 0 && errno != EINVAL && errno != EOPNOTSUPP) {
		pr_err("ioctl SIOCGHWTSTAMP failed: %m");
		return err;
	}

	init_ifreq(&ifreq, &cfg, device);
	/* If VLAN over bond supported in kernel, configure flag in driver. */
	if (err == 0) {
		cfg.flags = HWTSTAMP_FLAG_BONDED_PHC_INDEX;
	}

	switch (sk_hwts_filter_mode) {
	case HWTS_FILTER_CHECK:
		err = ioctl(fd, SIOCGHWTSTAMP, &ifreq);
		if (err < 0) {
			pr_err("ioctl SIOCGHWTSTAMP failed: %m");
			return err;
		}
		break;
	case HWTS_FILTER_FULL:
		cfg.tx_type   = tx_type;
		cfg.rx_filter = HWTSTAMP_FILTER_ALL;
		err = ioctl(fd, SIOCSHWTSTAMP, &ifreq);
		if (err < 0) {
			pr_err("ioctl SIOCSHWTSTAMP failed: %m");
			return err;
		}
		break;
	case HWTS_FILTER_NORMAL:
		cfg.tx_type   = tx_type;
		cfg.rx_filter = orig_rx_filter = rx_filter;
		err = ioctl(fd, SIOCSHWTSTAMP, &ifreq);
		if (err < 0) {
			pr_info("driver rejected most general HWTSTAMP filter");

			init_ifreq(&ifreq, &cfg, device);
			cfg.tx_type   = tx_type;
			cfg.rx_filter = orig_rx_filter = rx_filter2;

			err = ioctl(fd, SIOCSHWTSTAMP, &ifreq);
			if (err < 0) {
				pr_err("ioctl SIOCSHWTSTAMP failed: %m");
				return err;
			}
		}
		if (cfg.rx_filter == HWTSTAMP_FILTER_SOME)
			cfg.rx_filter = orig_rx_filter;
		break;
	}

	if (cfg.tx_type != tx_type ||
	    (cfg.rx_filter != rx_filter &&
	     cfg.rx_filter != rx_filter2 &&
	     cfg.rx_filter != HWTSTAMP_FILTER_ALL)) {
		pr_debug("tx_type   %d not %d", cfg.tx_type, tx_type);
		pr_debug("rx_filter %d not %d or %d", cfg.rx_filter, rx_filter,
			 rx_filter2);
		pr_err("The current filter does not match the required");
		return -1;
	}

	return 0;
}

/* public methods */

int sk_interface_fd(void)
{
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		pr_err("socket failed: %m");
		return -1;
	}
	return fd;
}

int sk_interface_index(int fd, const char *name)
{
	struct ifreq ifreq;
	int err;

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name) - 1);
	err = ioctl(fd, SIOCGIFINDEX, &ifreq);
	if (err < 0) {
		pr_err("ioctl SIOCGIFINDEX failed: %m");
		return err;
	}
	return ifreq.ifr_ifindex;
}

int sk_general_init(int fd)
{
	int on = sk_check_fupsync ? 1 : 0;
	if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPNS, &on, sizeof(on)) < 0) {
		pr_err("ioctl SO_TIMESTAMPNS failed: %m");
		return -1;
	}
	return 0;
}

int sk_get_ts_info(const char *name, struct sk_ts_info *sk_info)
{
#ifdef ETHTOOL_GET_TS_INFO
	struct ethtool_ts_info info;
	struct ifreq ifr;
	int fd, err;

	memset(&ifr, 0, sizeof(ifr));
	memset(&info, 0, sizeof(info));
	info.cmd = ETHTOOL_GET_TS_INFO;
	strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
	ifr.ifr_data = (char *) &info;
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0) {
		pr_err("socket failed: %m");
		goto failed;
	}

	err = ioctl(fd, SIOCETHTOOL, &ifr);
	if (err < 0) {
		pr_err("ioctl SIOCETHTOOL failed: %m");
		close(fd);
		goto failed;
	}

	close(fd);

	/* copy the necessary data to sk_info */
	memset(sk_info, 0, sizeof(struct sk_ts_info));
	sk_info->valid = 1;
	sk_info->phc_index = info.phc_index;
	sk_info->so_timestamping = info.so_timestamping;
	sk_info->tx_types = info.tx_types;
	sk_info->rx_filters = info.rx_filters;

	return 0;
failed:
#endif
	/* clear data and ensure it is not marked valid */
	memset(sk_info, 0, sizeof(struct sk_ts_info));
	return -1;
}

int sk_get_if_info(const char *name, struct sk_if_info *if_info)
{
#ifdef ETHTOOL_GLINKSETTINGS
	struct ifreq ifr;
	int fd, err;

	struct {
		struct ethtool_link_settings req;
		/*
		 * link_mode_data consists of supported[], advertising[],
		 * lp_advertising[] with size up to 127 each.
		 * The actual size is provided by the kernel.
		 */
		__u32 link_mode_data[3 * 127];
	} ecmd;

	memset(&ifr, 0, sizeof(ifr));
	memset(&ecmd, 0, sizeof(ecmd));

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		goto failed;
	}

	ecmd.req.cmd = ETHTOOL_GLINKSETTINGS;

	strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
	ifr.ifr_data = (char *) &ecmd;

	/* Handshake with kernel to determine number of words for link
	 * mode bitmaps. When requested number of bitmap words is not
	 * the one expected by kernel, the latter returns the integer
	 * opposite of what it is expecting. We request length 0 below
	 * (aka. invalid bitmap length) to get this info.
	 */
	err = ioctl(fd, SIOCETHTOOL, &ifr);
	if (err < 0) {
		pr_err("ioctl SIOCETHTOOL failed: %m");
		close(fd);
		goto failed;
	}

	if (ecmd.req.link_mode_masks_nwords >= 0 ||
			ecmd.req.cmd != ETHTOOL_GLINKSETTINGS) {
		close(fd);
		goto failed;
	}
	ecmd.req.link_mode_masks_nwords = -ecmd.req.link_mode_masks_nwords;

	err = ioctl(fd, SIOCETHTOOL, &ifr);
	if (err < 0) {
		pr_err("ioctl SIOCETHTOOL failed: %m");
		close(fd);
		goto failed;
	}

	close(fd);

	/* copy the necessary data to sk_info */
	memset(if_info, 0, sizeof(struct sk_if_info));
	if_info->valid = 1;
	if_info->speed = ecmd.req.speed;

	/* Megabits per second converted to attoseconds per bit */
	if_info->iface_bit_period = (1000000000000ULL/if_info->speed);
	return 0;
failed:
#endif
	/* clear data and ensure it is not marked valid */
	memset(if_info, 0, sizeof(struct sk_if_info));
	return -1;
}


static int sk_interface_guidaddr(const char *name, unsigned char *guid)
{
	char file_name[64], buf[64], addr[8];
	FILE *f;
	char *err;
	int res;

	snprintf(file_name, sizeof buf, "/sys/class/net/%s/address", name);
	f = fopen(file_name, "r");
	if (!f) {
		pr_err("failed to open %s: %m", buf);
		return -1;
	}

	/* Set the file position to the beginning of the GUID */
	res = fseek(f, GUID_OFFSET, SEEK_SET);
	if (res) {
		pr_err("fseek failed: %m");
		goto error;
	}

	err = fgets(buf, sizeof buf, f);
	if (err == NULL) {
		pr_err("fseek failed: %m");
		goto error;
	}

	res = sscanf(buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
			   &addr[0], &addr[1], &addr[2], &addr[3],
			   &addr[4], &addr[5], &addr[6], &addr[7]);
	if (res != GUID_LEN) {
		pr_err("sscanf failed: %m");
		goto error;
	}

	memcpy(guid, addr, GUID_LEN);
	fclose(f);

	return 0;

error:
	fclose(f);
	return -1;
}

int sk_interface_macaddr(const char *name, struct address *mac)
{
	struct ifreq ifreq;
	int err, fd, type;

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name) - 1);

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		pr_err("socket failed: %m");
		return -1;
	}

	err = ioctl(fd, SIOCGIFHWADDR, &ifreq);
	if (err < 0) {
		pr_err("ioctl SIOCGIFHWADDR failed: %m");
		close(fd);
		return -1;
	}

	close(fd);

	/* Get interface type */
	type = ifreq.ifr_hwaddr.sa_family;
	switch (type) {
		case ARPHRD_INFINIBAND:
			err = sk_interface_guidaddr(name, mac->sll.sll_addr);
			if (err) {
				pr_err("fail to get address using sysfs: %m");
				return -1;
			}
			mac->sll.sll_halen = EUI64;
			break;
		default:
			memcpy(mac->sll.sll_addr, &ifreq.ifr_hwaddr.sa_data, MAC_LEN);
			mac->sll.sll_halen = EUI48;
	}

	mac->sll.sll_family = AF_PACKET;
	mac->len = sizeof(mac->sll);
	return 0;
}

int sk_interface_addr(const char *name, int family, struct address *addr)
{
	struct ifaddrs *ifaddr, *i;
	int result = -1;

	if (getifaddrs(&ifaddr) == -1) {
		pr_err("getifaddrs failed: %m");
		return -1;
	}
	for (i = ifaddr; i; i = i->ifa_next) {
		if (i->ifa_addr && family == i->ifa_addr->sa_family &&
			strcmp(name, i->ifa_name) == 0)
		{
			switch (family) {
			case AF_INET:
				addr->len = sizeof(addr->sin);
				memcpy(&addr->sin, i->ifa_addr, addr->len);
				break;
			case AF_INET6:
				addr->len = sizeof(addr->sin6);
				memcpy(&addr->sin6, i->ifa_addr, addr->len);
				break;
			default:
				continue;
			}
			result = 0;
			break;
		}
	}
	freeifaddrs(ifaddr);
	return result;
}

static short sk_events = POLLPRI;
static short sk_revents = POLLPRI;

int sk_receive(int fd, void *buf, int buflen,
	       struct address *addr, struct hw_timestamp *hwts, int flags)
{
	char control[256];
	int cnt = 0, res = 0, level, type;
	struct cmsghdr *cm;
	struct iovec iov = { buf, buflen };
	struct msghdr msg;
	struct timespec *sw, *ts = NULL;

	memset(control, 0, sizeof(control));
	memset(&msg, 0, sizeof(msg));
	if (addr) {
		msg.msg_name = &addr->ss;
		msg.msg_namelen = sizeof(addr->ss);
	}
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);

	if (flags == MSG_ERRQUEUE) {
		if (hwts) {
			struct timespec now;

			do_clock_gettime(CLOCK_REALTIME, &now);
			hwts->ts = timespec_to_tmv(now);
			return 1;
		}

		struct pollfd pfd = { fd, sk_events, 0 };
		res = poll(&pfd, 1, sk_tx_timeout);
		/* Retry once on EINTR to avoid logging errors before exit */
		if (res < 0 && errno == EINTR)
			res = poll(&pfd, 1, sk_tx_timeout);
		if (res < 0) {
			pr_err("poll for tx timestamp failed: %m");
			return -errno;
		} else if (!res) {
			pr_err("timed out while polling for tx timestamp");
			pr_err("increasing tx_timestamp_timeout or increasing "
			       "kworker priority may correct this issue, "
			       "but a driver bug likely causes it");
			errno = ETIME;
			return -1;
		} else if (!(pfd.revents & sk_revents)) {
			pr_err("poll for tx timestamp woke up on non ERR event");
			return -1;
		}
	}

	cnt = recvmsg(fd, &msg, flags);
	if (cnt < 0) {
		pr_err("recvmsg%sfailed: %m",
		       flags == MSG_ERRQUEUE ? " tx timestamp " : " ");
	}
	
#if !USE_KTIME
	for (cm = CMSG_FIRSTHDR(&msg); cm != NULL; cm = CMSG_NXTHDR(&msg, cm)) {
		level = cm->cmsg_level;
		type  = cm->cmsg_type;
		if (SOL_SOCKET == level && SO_TIMESTAMPING == type) {
			if (cm->cmsg_len < sizeof(*ts) * 3) {
				pr_warning("short SO_TIMESTAMPING message");
				return -EMSGSIZE;
			}
			ts = (struct timespec *) CMSG_DATA(cm);
		}
		if (SOL_SOCKET == level && SO_TIMESTAMPNS == type) {
			if (cm->cmsg_len < sizeof(*sw)) {
				pr_warning("short SO_TIMESTAMPNS message");
				return -EMSGSIZE;
			}
			sw = (struct timespec *) CMSG_DATA(cm);
			hwts->sw = timespec_to_tmv(*sw);
		}
	}

	if (addr)
		addr->len = msg.msg_namelen;

	if (!ts) {
		memset(&hwts->ts, 0, sizeof(hwts->ts));
		return cnt < 0 ? -errno : cnt;
	}

	switch (hwts->type) {
	case TS_SOFTWARE:
		hwts->ts = timespec_to_tmv(ts[0]);
		break;
	case TS_HARDWARE:
	case TS_ONESTEP:
	case TS_P2P1STEP:
		hwts->ts = timespec_to_tmv(ts[2]);
		break;
	case TS_LEGACY_HW:
		hwts->ts = timespec_to_tmv(ts[1]);
		break;
	}
#else
	{
		struct timespec now;

		do_clock_gettime(CLOCK_REALTIME, &now);
		hwts->ts = hwts->sw = timespec_to_tmv(now);
	}
#endif

	return cnt < 0 ? -errno : cnt;
}

int sk_get_error(int fd)
{
	socklen_t len;
	int error;

	len = sizeof (error);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		pr_err("getsockopt SO_ERROR failed: %m");
		return -1;
	}

	return error;
}

int sk_set_priority(int fd, int family, uint8_t dscp)
{
	int level, optname, tos;
	socklen_t tos_len;

	switch (family) {
	case AF_INET:
		level = IPPROTO_IP;
		optname = IP_TOS;
		break;
	case AF_INET6:
		level = IPPROTO_IPV6;
		optname = IPV6_TCLASS;
		break;
	default:
		return -1;
	}

	tos_len = sizeof(tos);
	if (getsockopt(fd, level, optname, &tos, &tos_len) < 0) {
		tos = 0;
	}

	/* clear old DSCP value */
	tos &= ~0xFC;

	/* set new DSCP value */
	tos |= dscp<<2;
	tos_len = sizeof(tos);
	if (setsockopt(fd, level, optname, &tos, tos_len) < 0) {
		return -1;
	}

	return 0;
}

int sk_timestamping_init(int fd, const char *device, enum timestamp_type type,
			 enum transport_type transport, int vclock)
{
	int err, filter1, filter2 = 0, flags, tx_type = HWTSTAMP_TX_ON;
	struct so_timestamping timestamping;

#if USE_KTIME
	return 0;
#endif
	switch (type) {
	case TS_SOFTWARE:
		flags = SOF_TIMESTAMPING_TX_SOFTWARE |
			SOF_TIMESTAMPING_RX_SOFTWARE |
			SOF_TIMESTAMPING_SOFTWARE;
		break;
	case TS_HARDWARE:
	case TS_ONESTEP:
	case TS_P2P1STEP:
		flags = SOF_TIMESTAMPING_TX_HARDWARE |
			SOF_TIMESTAMPING_RX_HARDWARE |
			SOF_TIMESTAMPING_RAW_HARDWARE;
		break;
	case TS_LEGACY_HW:
		flags = SOF_TIMESTAMPING_TX_HARDWARE |
			SOF_TIMESTAMPING_RX_HARDWARE |
			SOF_TIMESTAMPING_SYS_HARDWARE;
		break;
	default:
		return -1;
	}

	if (type != TS_SOFTWARE) {
		filter1 = HWTSTAMP_FILTER_PTP_V2_EVENT;
		switch (type) {
		case TS_SOFTWARE:
			tx_type = HWTSTAMP_TX_OFF;
			break;
		case TS_HARDWARE:
		case TS_LEGACY_HW:
			tx_type = HWTSTAMP_TX_ON;
			break;
		case TS_ONESTEP:
			tx_type = HWTSTAMP_TX_ONESTEP_SYNC;
			break;
		case TS_P2P1STEP:
			tx_type = HWTSTAMP_TX_ONESTEP_P2P;
			break;
		}
		switch (transport) {
		case TRANS_UDP_IPV4:
		case TRANS_UDP_IPV6:
			filter2 = HWTSTAMP_FILTER_PTP_V2_L4_EVENT;
			break;
		case TRANS_IEEE_802_3:
			filter2 = HWTSTAMP_FILTER_PTP_V2_L2_EVENT;
			break;
		case TRANS_DEVICENET:
		case TRANS_CONTROLNET:
		case TRANS_PROFINET:
		case TRANS_UDS:
			return -1;
		}
		err = hwts_init(fd, device, filter1, filter2, tx_type);
		if (err)
			return err;
	}

	if (vclock >= 0)
		flags |= SOF_TIMESTAMPING_BIND_PHC;

	timestamping.flags = flags;
	timestamping.bind_phc = vclock;

	if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING,
		       &timestamping, sizeof(timestamping)) < 0) {
		pr_err("ioctl SO_TIMESTAMPING failed: %m");
		return -1;
	}

	flags = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_SELECT_ERR_QUEUE,
		       &flags, sizeof(flags)) < 0) {
		pr_warning("%s: SO_SELECT_ERR_QUEUE: %m", device);
		sk_events = 0;
		sk_revents = POLLERR;
	}

	/* Enable the sk_check_fupsync option, perhaps. */
	if (sk_general_init(fd)) {
		return -1;
	}

	return 0;
}
