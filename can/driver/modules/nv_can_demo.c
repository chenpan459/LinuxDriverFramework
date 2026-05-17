// SPDX-License-Identifier: GPL-2.0
/*
 * Example: virtual CAN device using nv_can framework (SocketCAN loopback)
 *
 * Creates interface nvcan0 — transmitted frames are echoed to RX.
 *
 * Load:  sudo insmod nv_can_demo.ko
 *        sudo ip link set nvcan0 up type can bitrate 1000000
 * Test:  can/app/bin/nv_test_can
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/can/dev.h>
#include <linux/can/skb.h>

#include "nv_can.h"

#define DRV_NAME	"nv_can_demo"
#define IFNAME		"nvcan0"

static const struct can_bittiming_const demo_btc = {
	.name		= "nvcan",
	.tseg1_min	= 1,
	.tseg1_max	= 16,
	.tseg2_min	= 1,
	.tseg2_max	= 8,
	.sjw_max	= 4,
	.brp_min	= 1,
	.brp_max	= 512,
	.brp_inc	= 1,
};

static struct nv_can_dev demo_dev;

static int demo_set_mode(struct nv_can_dev *dev, enum can_mode mode)
{
	struct net_device *netdev = dev->netdev;

	if (!netdev)
		return -EINVAL;

	if (mode == CAN_MODE_START) {
		netif_carrier_on(netdev);
		netif_start_queue(netdev);
	} else {
		netif_carrier_off(netdev);
		netif_stop_queue(netdev);
	}

	return 0;
}

static netdev_tx_t demo_start_xmit(struct nv_can_dev *dev, struct sk_buff *skb)
{
	struct net_device *netdev = dev->netdev;
	struct sk_buff *rxskb;
	struct can_frame *cf, *rxf;
	unsigned int len;

	if (!netdev)
		goto drop;

	if (can_dev_dropped_skb(netdev, skb))
		return NETDEV_TX_OK;

	len = skb->len;
	if (len > CAN_MTU)
		goto drop;

	rxskb = alloc_can_skb(netdev, &rxf);
	if (!rxskb)
		goto drop;

	cf = (struct can_frame *)skb->data;
	memcpy(rxf, cf, len);
	rxf->len = cf->len;

	if (nv_can_rx_skb(dev, rxskb) != NET_RX_SUCCESS)
		kfree_skb(rxskb);

	netdev->stats.tx_packets++;
	netdev->stats.tx_bytes += cf->len;
	kfree_skb(skb);
	return NETDEV_TX_OK;

drop:
	kfree_skb(skb);
	return NETDEV_TX_OK;
}

static const struct nv_can_ops demo_ops = {
	.do_set_mode	= demo_set_mode,
	.start_xmit	= demo_start_xmit,
};

static int __init demo_init(void)
{
	return nv_can_netdev_register(&demo_dev, IFNAME, &demo_ops, NULL,
				      &demo_btc, 0);
}

static void __exit demo_exit(void)
{
	nv_can_netdev_unregister(&demo_dev);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("CAN demo driver (nvcan0 loopback)");
MODULE_LICENSE("GPL");
