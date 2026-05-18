// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - CAN (SocketCAN) helper implementation
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/can/dev.h>
#include <linux/can/skb.h>

#include "nv_can.h"

struct nv_can_netdev_priv *nv_can_ndpriv(struct net_device *dev)
{
	return (struct nv_can_netdev_priv *)((char *)netdev_priv(dev) +
					     sizeof(struct can_priv));
}
EXPORT_SYMBOL_GPL(nv_can_ndpriv);

struct can_priv *nv_can_cpriv(struct net_device *netdev)
{
	return netdev_priv(netdev);
}
EXPORT_SYMBOL_GPL(nv_can_cpriv);

struct nv_can_dev *nv_can_dev_from_netdev(struct net_device *netdev)
{
	struct nv_can_netdev_priv *np;

	if (!netdev)
		return NULL;

	np = nv_can_ndpriv(netdev);
	return np ? np->ncan : NULL;
}
EXPORT_SYMBOL_GPL(nv_can_dev_from_netdev);

int nv_can_rx_skb(struct nv_can_dev *dev, struct sk_buff *skb)
{
	if (!dev || !dev->netdev || !skb)
		return -EINVAL;

	skb->dev = dev->netdev;
	return netif_rx(skb);
}
EXPORT_SYMBOL_GPL(nv_can_rx_skb);

int nv_can_rx_frame(struct nv_can_dev *dev, struct can_frame *cf)
{
	struct sk_buff *skb;
	struct can_frame *rxcf;
	int ret;

	if (!dev || !dev->netdev || !cf)
		return -EINVAL;

	skb = alloc_can_skb(dev->netdev, &rxcf);
	if (!skb)
		return -ENOMEM;

	memcpy(rxcf, cf, sizeof(*cf));
	ret = nv_can_rx_skb(dev, skb);
	if (ret)
		kfree_skb(skb);

	return ret;
}
EXPORT_SYMBOL_GPL(nv_can_rx_frame);

static int nv_can_do_set_mode(struct net_device *netdev, enum can_mode mode)
{
	struct nv_can_dev *ncan = nv_can_dev_from_netdev(netdev);

	if (ncan && ncan->ops && ncan->ops->do_set_mode)
		return ncan->ops->do_set_mode(ncan, mode);

	if (mode == CAN_MODE_START) {
		netif_carrier_on(netdev);
		netif_start_queue(netdev);
	} else {
		netif_carrier_off(netdev);
		netif_stop_queue(netdev);
	}

	return 0;
}

static int nv_can_do_set_bittiming(struct net_device *netdev)
{
	struct nv_can_dev *ncan = nv_can_dev_from_netdev(netdev);

	if (ncan && ncan->ops && ncan->ops->do_set_bittiming)
		return ncan->ops->do_set_bittiming(ncan);

	return 0;
}

static int nv_can_open(struct net_device *netdev)
{
	return open_candev(netdev);
}

static int nv_can_close(struct net_device *netdev)
{
	close_candev(netdev);
	return 0;
}

static netdev_tx_t nv_can_start_xmit(struct sk_buff *skb,
				     struct net_device *netdev)
{
	struct nv_can_dev *ncan = nv_can_dev_from_netdev(netdev);

	if (ncan && ncan->ops && ncan->ops->start_xmit)
		return ncan->ops->start_xmit(ncan, skb);

	kfree_skb(skb);
	return NETDEV_TX_OK;
}

static const struct net_device_ops nv_can_netdev_ops = {
	.ndo_open	= nv_can_open,
	.ndo_stop	= nv_can_close,
	.ndo_start_xmit	= nv_can_start_xmit,
	.ndo_change_mtu	= can_change_mtu,
};

int nv_can_netdev_register(struct nv_can_dev *dev, const char *ifname,
			   const struct nv_can_ops *ops, void *priv,
			   const struct can_bittiming_const *btc,
			   unsigned int echo_skb_max)
{
	struct net_device *netdev;
	struct can_priv *cpriv;
	struct nv_can_netdev_priv *np;
	int ret;

	if (!dev || !ifname || !ops || !ops->start_xmit)
		return -EINVAL;

	if (dev->registered)
		return -EBUSY;

	netdev = alloc_candev(sizeof(struct nv_can_netdev_priv), echo_skb_max);
	if (!netdev)
		return -ENOMEM;

	dev->netdev = netdev;
	dev->ops = ops;
	dev->priv = priv;
	dev->bittiming_const = btc;
	dev->echo_skb_max = echo_skb_max;

	np = nv_can_ndpriv(netdev);
	np->ncan = dev;

	strscpy(netdev->name, ifname, IFNAMSIZ);
	netdev->netdev_ops = &nv_can_netdev_ops;

	cpriv = netdev_priv(netdev);
	cpriv->bittiming_const = btc;
	cpriv->do_set_mode = nv_can_do_set_mode;
	cpriv->do_set_bittiming = nv_can_do_set_bittiming;
	cpriv->ctrlmode_supported = CAN_CTRLMODE_LOOPBACK;

	ret = register_candev(netdev);
	if (ret) {
		free_candev(netdev);
		dev->netdev = NULL;
		return ret;
	}

	dev->registered = true;
	netdev_info(netdev, "nv_can: %s registered\n", ifname);
	return 0;
}
EXPORT_SYMBOL_GPL(nv_can_netdev_register);

void nv_can_netdev_unregister(struct nv_can_dev *dev)
{
	if (!dev || !dev->registered || !dev->netdev)
		return;

	unregister_candev(dev->netdev);
	free_candev(dev->netdev);
	dev->netdev = NULL;
	dev->registered = false;
}
EXPORT_SYMBOL_GPL(nv_can_netdev_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("CAN bus framework");
MODULE_LICENSE("GPL");
