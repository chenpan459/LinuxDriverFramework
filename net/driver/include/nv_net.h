/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - network device helper layer
 */
#ifndef _NV_NET_H_
#define _NV_NET_H_

#include <linux/if_ether.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/types.h>

struct nv_net_dev;
struct nv_net_driver;

/**
 * struct nv_net_ops - per-interface net_device operation callbacks
 *
 * start_xmit is required when the interface can transmit. open/stop are
 * optional; the framework provides defaults that start/stop the TX queue.
 */
struct nv_net_ops {
	int (*open)(struct nv_net_dev *dev, struct net_device *netdev);
	int (*stop)(struct nv_net_dev *dev, struct net_device *netdev);
	netdev_tx_t (*start_xmit)(struct nv_net_dev *dev, struct sk_buff *skb,
				  struct net_device *netdev);
	int (*set_mac_address)(struct nv_net_dev *dev,
			       struct net_device *netdev, void *addr);
	int (*change_mtu)(struct nv_net_dev *dev, struct net_device *netdev,
			  int new_mtu);
	void (*set_rx_mode)(struct nv_net_dev *dev, struct net_device *netdev);
	int (*ioctl)(struct nv_net_dev *dev, struct net_device *netdev,
		     struct ifreq *ifr, int cmd);
	void (*get_stats64)(struct nv_net_dev *dev, struct net_device *netdev,
			    struct rtnl_link_stats64 *stats);
};

/**
 * struct nv_net_dev - one network interface instance
 */
struct nv_net_dev {
	struct nv_net_driver *driver;
	unsigned int index;
	const char *name;
	const struct nv_net_ops *ops;
	void *priv;

	struct net_device *netdev;
	bool registered;
};

/**
 * struct nv_net_driver - driver-wide bookkeeping
 */
struct nv_net_driver {
	const char *name;
	struct module *owner;
	unsigned int count;

	struct nv_net_dev **devices;
};

int nv_net_driver_register(struct nv_net_driver *drv);
void nv_net_driver_unregister(struct nv_net_driver *drv);

int nv_net_device_register(struct nv_net_driver *drv,
			    struct nv_net_dev *dev, unsigned int index,
			    const char *name, const struct nv_net_ops *ops,
			    void *priv, const unsigned char *mac_addr, int mtu);
void nv_net_device_unregister(struct nv_net_dev *dev);

#endif /* _NV_NET_H_ */
