/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - CAN (SocketCAN) device helper layer
 */
#ifndef _NV_CAN_H_
#define _NV_CAN_H_

#include <linux/can/dev.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/types.h>

struct nv_can_dev;

/**
 * struct nv_can_ops - CAN controller callbacks
 */
struct nv_can_ops {
	int (*do_set_mode)(struct nv_can_dev *dev, enum can_mode mode);
	int (*do_set_bittiming)(struct nv_can_dev *dev);
	int (*do_get_state)(struct nv_can_dev *dev, enum can_state *state);
	netdev_tx_t (*start_xmit)(struct nv_can_dev *dev, struct sk_buff *skb);
};

/**
 * struct nv_can_netdev_priv - driver private after struct can_priv
 */
struct nv_can_netdev_priv {
	struct nv_can_dev *ncan;
};

/**
 * struct nv_can_dev - one CAN network interface
 */
struct nv_can_dev {
	struct net_device *netdev;
	const struct nv_can_ops *ops;
	void *priv;

	const struct can_bittiming_const *bittiming_const;
	unsigned int echo_skb_max;
	bool registered;
};

int nv_can_netdev_register(struct nv_can_dev *dev, const char *ifname,
			   const struct nv_can_ops *ops, void *priv,
			   const struct can_bittiming_const *btc,
			   unsigned int echo_skb_max);
void nv_can_netdev_unregister(struct nv_can_dev *dev);

struct nv_can_netdev_priv *nv_can_ndpriv(struct net_device *netdev);
struct nv_can_dev *nv_can_dev_from_netdev(struct net_device *netdev);
struct can_priv *nv_can_cpriv(struct net_device *netdev);

int nv_can_rx_frame(struct nv_can_dev *dev, struct can_frame *cf);
int nv_can_rx_skb(struct nv_can_dev *dev, struct sk_buff *skb);

#endif /* _NV_CAN_H_ */
