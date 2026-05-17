// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - network device helper implementation
 */

#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/slab.h>

#include "nv_net.h"

struct nv_net_netdev_priv {
	struct nv_net_dev *nv_dev;
};

static struct nv_net_dev *nv_net_dev_from_netdev(struct net_device *netdev)
{
	struct nv_net_netdev_priv *np = netdev_priv(netdev);

	return np->nv_dev;
}

static int nv_net_open(struct net_device *netdev)
{
	struct nv_net_dev *dev = nv_net_dev_from_netdev(netdev);

	if (dev->ops && dev->ops->open)
		return dev->ops->open(dev, netdev);

	netif_start_queue(netdev);
	return 0;
}

static int nv_net_stop(struct net_device *netdev)
{
	struct nv_net_dev *dev = nv_net_dev_from_netdev(netdev);

	if (dev->ops && dev->ops->stop)
		return dev->ops->stop(dev, netdev);

	netif_stop_queue(netdev);
	return 0;
}

static netdev_tx_t nv_net_start_xmit(struct sk_buff *skb,
				      struct net_device *netdev)
{
	struct nv_net_dev *dev = nv_net_dev_from_netdev(netdev);

	if (dev->ops && dev->ops->start_xmit)
		return dev->ops->start_xmit(dev, skb, netdev);

	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static int nv_net_set_mac_address(struct net_device *netdev, void *addr)
{
	struct nv_net_dev *dev = nv_net_dev_from_netdev(netdev);

	if (dev->ops && dev->ops->set_mac_address)
		return dev->ops->set_mac_address(dev, netdev, addr);

	return eth_mac_addr(netdev, addr);
}

static int nv_net_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct nv_net_dev *dev = nv_net_dev_from_netdev(netdev);

	if (dev->ops && dev->ops->change_mtu)
		return dev->ops->change_mtu(dev, netdev, new_mtu);

	return dev_set_mtu(netdev, new_mtu);
}

static void nv_net_set_rx_mode(struct net_device *netdev)
{
	struct nv_net_dev *dev = nv_net_dev_from_netdev(netdev);

	if (dev->ops && dev->ops->set_rx_mode)
		dev->ops->set_rx_mode(dev, netdev);
}

static int nv_net_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	struct nv_net_dev *dev = nv_net_dev_from_netdev(netdev);

	if (dev->ops && dev->ops->ioctl)
		return dev->ops->ioctl(dev, netdev, ifr, cmd);

	return -EOPNOTSUPP;
}

static void nv_net_get_stats64(struct net_device *netdev,
				struct rtnl_link_stats64 *stats)
{
	struct nv_net_dev *dev = nv_net_dev_from_netdev(netdev);

	if (dev->ops && dev->ops->get_stats64) {
		dev->ops->get_stats64(dev, netdev, stats);
		return;
	}

	netdev_stats_to_stats64(stats, &netdev->stats);
}

static const struct net_device_ops nv_net_netdev_ops = {
	.ndo_open		= nv_net_open,
	.ndo_stop		= nv_net_stop,
	.ndo_start_xmit		= nv_net_start_xmit,
	.ndo_set_mac_address	= nv_net_set_mac_address,
	.ndo_change_mtu		= nv_net_change_mtu,
	.ndo_set_rx_mode	= nv_net_set_rx_mode,
	.ndo_do_ioctl		= nv_net_ioctl,
	.ndo_get_stats64	= nv_net_get_stats64,
};

int nv_net_driver_register(struct nv_net_driver *drv)
{
	if (!drv || !drv->name || !drv->count || !drv->owner)
		return -EINVAL;

	drv->devices = kcalloc(drv->count, sizeof(*drv->devices), GFP_KERNEL);
	if (!drv->devices)
		return -ENOMEM;

	return 0;
}
EXPORT_SYMBOL_GPL(nv_net_driver_register);

void nv_net_driver_unregister(struct nv_net_driver *drv)
{
	unsigned int i;

	if (!drv)
		return;

	if (drv->devices) {
		for (i = 0; i < drv->count; i++) {
			if (drv->devices[i])
				nv_net_device_unregister(drv->devices[i]);
		}
		kfree(drv->devices);
		drv->devices = NULL;
	}
}
EXPORT_SYMBOL_GPL(nv_net_driver_unregister);

int nv_net_device_register(struct nv_net_driver *drv, struct nv_net_dev *dev,
			    unsigned int index, const char *name,
			    const struct nv_net_ops *ops, void *priv,
			    const unsigned char *mac_addr, int mtu)
{
	struct net_device *netdev;
	struct nv_net_netdev_priv *np;
	int ret;

	if (!drv || !dev || !name || !ops || index >= drv->count)
		return -EINVAL;

	if (dev->registered)
		return -EBUSY;

	dev->driver = drv;
	dev->index = index;
	dev->name = name;
	dev->ops = ops;
	dev->priv = priv;

	netdev = alloc_etherdev_mqs(sizeof(*np), 1, 1);
	if (!netdev)
		return -ENOMEM;

	np = netdev_priv(netdev);
	np->nv_dev = dev;
	dev->netdev = netdev;

	netdev->netdev_ops = &nv_net_netdev_ops;
	if (mac_addr)
		eth_hw_addr_set(netdev, mac_addr);
	else
		eth_hw_addr_random(netdev);

	if (mtu > 0)
		netdev->mtu = mtu;
	else
		netdev->mtu = ETH_DATA_LEN;

	ret = dev_alloc_name(netdev, name);
	if (ret < 0) {
		free_netdev(netdev);
		dev->netdev = NULL;
		return ret;
	}

	ret = register_netdev(netdev);
	if (ret) {
		free_netdev(netdev);
		dev->netdev = NULL;
		return ret;
	}

	drv->devices[index] = dev;
	dev->registered = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_net_device_register);

void nv_net_device_unregister(struct nv_net_dev *dev)
{
	if (!dev || !dev->registered)
		return;

	if (dev->driver && dev->driver->devices)
		dev->driver->devices[dev->index] = NULL;

	if (dev->netdev) {
		unregister_netdev(dev->netdev);
		free_netdev(dev->netdev);
		dev->netdev = NULL;
	}

	dev->registered = false;
}
EXPORT_SYMBOL_GPL(nv_net_device_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("Network device framework for LinuxDriverFramework");
MODULE_LICENSE("GPL");
