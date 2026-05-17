/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LinuxDriverFramework - USB device helper layer
 */
#ifndef _NV_USB_H_
#define _NV_USB_H_

#include <linux/module.h>
#include <linux/types.h>
#include <linux/usb.h>

struct nv_usb_dev;
struct nv_usb_driver;

/**
 * struct nv_usb_endpoints - cached bulk/interrupt endpoints
 */
struct nv_usb_endpoints {
	struct usb_endpoint_descriptor *bulk_in;
	struct usb_endpoint_descriptor *bulk_out;
	struct usb_endpoint_descriptor *int_in;
	u8 bulk_in_addr;
	u8 bulk_out_addr;
	u8 int_in_addr;
	u16 bulk_in_mps;
	u16 bulk_out_mps;
	u16 int_in_mps;
};

/**
 * struct nv_usb_ops - driver callbacks invoked by the framework
 */
struct nv_usb_ops {
	int (*probe)(struct nv_usb_dev *dev, const struct usb_device_id *id);
	void (*disconnect)(struct nv_usb_dev *dev);
	int (*suspend)(struct nv_usb_dev *dev, pm_message_t state);
	int (*resume)(struct nv_usb_dev *dev);
	int (*reset_resume)(struct nv_usb_dev *dev);
	void (*shutdown)(struct nv_usb_dev *dev);
};

/**
 * struct nv_usb_dev - per-interface context (usb_set_intfdata)
 */
struct nv_usb_dev {
	struct usb_interface *intf;
	struct usb_device *udev;
	struct nv_usb_driver *driver;
	const struct usb_device_id *id;
	void *priv;

	struct nv_usb_endpoints eps;
	int altsetting;
	bool autosuspend_enabled;
};

/**
 * struct nv_usb_driver - USB interface driver registration wrapper
 */
struct nv_usb_driver {
	const char *name;
	struct module *owner;
	const struct usb_device_id *id_table;
	const struct nv_usb_ops *ops;
	unsigned int flags;

	struct usb_driver usb_driver;
	bool registered;
};

#define NV_USB_FLAG_SKIP_ALTSETTING		BIT(0)
#define NV_USB_FLAG_ENABLE_AUTOSUSPEND		BIT(1)

int nv_usb_driver_register(struct nv_usb_driver *drv);
void nv_usb_driver_unregister(struct nv_usb_driver *drv);

struct nv_usb_dev *nv_usb_dev_from_intf(struct usb_interface *intf);

int nv_usb_select_altsetting(struct nv_usb_dev *dev, int alt);
int nv_usb_find_endpoints(struct nv_usb_dev *dev);

int nv_usb_bulk_in(struct nv_usb_dev *dev, void *buf, int len, int timeout_ms);
int nv_usb_bulk_out(struct nv_usb_dev *dev, const void *buf, int len,
		    int timeout_ms);
int nv_usb_control(struct nv_usb_dev *dev, u8 request_type, u8 request,
		   u16 value, u16 index, void *buf, u16 size, int timeout_ms);

static inline struct device *nv_usb_dev_device(struct nv_usb_dev *dev)
{
	return dev && dev->intf ? &dev->intf->dev : NULL;
}

#endif /* _NV_USB_H_ */
