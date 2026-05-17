// SPDX-License-Identifier: GPL-2.0
/*
 * Example: minimal Ethernet-like netdev using nv_net framework
 *
 * TX packets are dropped; useful to bring the interface up and test
 * registration. Default interface name: nv%d (nv0).
 *
 * Load:   insmod nv_net_dummy.ko
 * Test:   ip link show nv0
 *         sudo ip link set nv0 up
 *         ping -c1 -I nv0 127.0.0.1   # will not receive replies
 * Unload: rmmod nv_net_dummy
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

#include "nv_net.h"

#define DUMMY_IFNAME	"nv%d"

static struct nv_net_driver dummy_driver;
static struct nv_net_dev dummy_dev;

static int dummy_open(struct nv_net_dev *dev, struct net_device *netdev)
{
	netif_start_queue(netdev);
	return 0;
}

static int dummy_stop(struct nv_net_dev *dev, struct net_device *netdev)
{
	netif_stop_queue(netdev);
	return 0;
}

static netdev_tx_t dummy_xmit(struct nv_net_dev *dev, struct sk_buff *skb,
			      struct net_device *netdev)
{
	struct net_device_stats *stats = &netdev->stats;

	stats->tx_packets++;
	stats->tx_bytes += skb->len;
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static void dummy_get_stats64(struct nv_net_dev *dev,
			      struct net_device *netdev,
			      struct rtnl_link_stats64 *stats)
{
	netdev_stats_to_stats64(stats, &netdev->stats);
}

static const struct nv_net_ops dummy_ops = {
	.open		= dummy_open,
	.stop		= dummy_stop,
	.start_xmit	= dummy_xmit,
	.get_stats64	= dummy_get_stats64,
};

static int __init dummy_init(void)
{
	int ret;

	dummy_driver.name = "nv_net_dummy";
	dummy_driver.owner = THIS_MODULE;
	dummy_driver.count = 1;

	ret = nv_net_driver_register(&dummy_driver);
	if (ret)
		return ret;

	ret = nv_net_device_register(&dummy_driver, &dummy_dev, 0, DUMMY_IFNAME,
				      &dummy_ops, NULL, NULL, ETH_DATA_LEN);
	if (ret)
		goto err_drv;

	pr_info("nv_net_dummy: ready (%s)\n", dummy_dev.netdev->name);
	return 0;

err_drv:
	nv_net_driver_unregister(&dummy_driver);
	return ret;
}

static void __exit dummy_exit(void)
{
	nv_net_device_unregister(&dummy_dev);
	nv_net_driver_unregister(&dummy_driver);
	pr_info("nv_net_dummy: unloaded\n");
}

module_init(dummy_init);
module_exit(dummy_exit);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("Dummy network device example");
MODULE_LICENSE("GPL");
