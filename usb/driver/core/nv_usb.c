// SPDX-License-Identifier: GPL-2.0
/*
 * LinuxDriverFramework - USB device helper implementation
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#include "nv_usb.h"

static struct nv_usb_driver *nv_usb_drv_from_intf(struct usb_interface *intf)
{
	if (!intf || !intf->dev.driver)
		return NULL;

	return container_of(to_usb_driver(intf->dev.driver),
			    struct nv_usb_driver, usb_driver);
}

static struct nv_usb_driver *nv_usb_drv_from_id(const struct usb_device_id *id)
{
	if (!id || !id->driver_info)
		return NULL;

	return (struct nv_usb_driver *)(unsigned long)id->driver_info;
}

struct nv_usb_dev *nv_usb_dev_from_intf(struct usb_interface *intf)
{
	return intf ? usb_get_intfdata(intf) : NULL;
}
EXPORT_SYMBOL_GPL(nv_usb_dev_from_intf);

int nv_usb_select_altsetting(struct nv_usb_dev *dev, int alt)
{
	int ret;

	if (!dev || !dev->intf)
		return -EINVAL;

	if (dev->altsetting == alt)
		return 0;

	ret = usb_set_interface(dev->udev, dev->intf->cur_altsetting->desc.bInterfaceNumber,
				alt);
	if (ret)
		return ret;

	dev->altsetting = alt;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_usb_select_altsetting);

int nv_usb_find_endpoints(struct nv_usb_dev *dev)
{
	struct usb_host_interface *alt;
	int i;

	if (!dev || !dev->intf)
		return -EINVAL;

	memset(&dev->eps, 0, sizeof(dev->eps));
	alt = dev->intf->cur_altsetting;

	for (i = 0; i < alt->desc.bNumEndpoints; i++) {
		struct usb_endpoint_descriptor *ep = &alt->endpoint[i].desc;
		unsigned int etype = usb_endpoint_type(ep);

		if (etype == USB_ENDPOINT_XFER_BULK) {
			if (usb_endpoint_dir_in(ep) && !dev->eps.bulk_in) {
				dev->eps.bulk_in = ep;
				dev->eps.bulk_in_addr = ep->bEndpointAddress;
				dev->eps.bulk_in_mps = usb_endpoint_maxp(ep);
			} else if (!usb_endpoint_dir_in(ep) && !dev->eps.bulk_out) {
				dev->eps.bulk_out = ep;
				dev->eps.bulk_out_addr = ep->bEndpointAddress;
				dev->eps.bulk_out_mps = usb_endpoint_maxp(ep);
			}
		} else if (etype == USB_ENDPOINT_XFER_INT &&
			   usb_endpoint_dir_in(ep) && !dev->eps.int_in) {
			dev->eps.int_in = ep;
			dev->eps.int_in_addr = ep->bEndpointAddress;
			dev->eps.int_in_mps = usb_endpoint_maxp(ep);
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(nv_usb_find_endpoints);

int nv_usb_bulk_in(struct nv_usb_dev *dev, void *buf, int len, int timeout_ms)
{
	int ret;

	if (!dev || !dev->udev || !dev->eps.bulk_in_addr || !buf || len <= 0)
		return -EINVAL;

	ret = usb_bulk_msg(dev->udev, usb_rcvbulkpipe(dev->udev,
						      dev->eps.bulk_in_addr),
			   buf, len, NULL, timeout_ms);
	return ret < 0 ? ret : ret;
}
EXPORT_SYMBOL_GPL(nv_usb_bulk_in);

int nv_usb_bulk_out(struct nv_usb_dev *dev, const void *buf, int len,
		    int timeout_ms)
{
	int ret;

	if (!dev || !dev->udev || !dev->eps.bulk_out_addr || !buf || len <= 0)
		return -EINVAL;

	ret = usb_bulk_msg(dev->udev, usb_sndbulkpipe(dev->udev,
						      dev->eps.bulk_out_addr),
			   (void *)buf, len, NULL, timeout_ms);
	return ret < 0 ? ret : ret;
}
EXPORT_SYMBOL_GPL(nv_usb_bulk_out);

int nv_usb_control(struct nv_usb_dev *dev, u8 request_type, u8 request,
		   u16 value, u16 index, void *buf, u16 size, int timeout_ms)
{
	if (!dev || !dev->udev)
		return -EINVAL;

	return usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			       request, request_type, value, index, buf, size,
			       timeout_ms);
}
EXPORT_SYMBOL_GPL(nv_usb_control);

static int nv_usb_probe(struct usb_interface *intf,
			const struct usb_device_id *id)
{
	struct nv_usb_driver *drv;
	struct nv_usb_dev *dev;
	int ret;

	drv = nv_usb_drv_from_id(id);
	if (!drv)
		drv = nv_usb_drv_from_intf(intf);
	if (!drv || !drv->ops || !drv->ops->probe)
		return -ENODEV;

	dev = devm_kzalloc(&intf->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->intf = intf;
	dev->udev = interface_to_usbdev(intf);
	dev->driver = drv;
	dev->id = id;
	dev->altsetting = intf->cur_altsetting->desc.bAlternateSetting;
	usb_set_intfdata(intf, dev);

	if (!(drv->flags & NV_USB_FLAG_SKIP_ALTSETTING)) {
		ret = nv_usb_select_altsetting(dev, 0);
		if (ret)
			return ret;
	}

	if (drv->flags & NV_USB_FLAG_ENABLE_AUTOSUSPEND) {
		usb_enable_autosuspend(dev->udev);
		dev->autosuspend_enabled = true;
	}

	ret = drv->ops->probe(dev, id);
	if (ret) {
		if (dev->autosuspend_enabled)
			usb_disable_autosuspend(dev->udev);
		usb_set_intfdata(intf, NULL);
		return ret;
	}

	dev_info(&intf->dev, "nv_usb: %s bound (%04x:%04x if %d)\n",
		 drv->name, le16_to_cpu(dev->udev->descriptor.idVendor),
		 le16_to_cpu(dev->udev->descriptor.idProduct),
		 intf->cur_altsetting->desc.bInterfaceNumber);
	return 0;
}

static void nv_usb_disconnect(struct usb_interface *intf)
{
	struct nv_usb_driver *drv = nv_usb_drv_from_intf(intf);
	struct nv_usb_dev *dev = nv_usb_dev_from_intf(intf);

	if (!dev)
		return;

	if (drv && drv->ops && drv->ops->disconnect)
		drv->ops->disconnect(dev);

	if (dev->autosuspend_enabled)
		usb_disable_autosuspend(dev->udev);

	usb_set_intfdata(intf, NULL);
}

static int nv_usb_suspend(struct usb_interface *intf, pm_message_t state)
{
	struct nv_usb_driver *drv = nv_usb_drv_from_intf(intf);
	struct nv_usb_dev *dev = nv_usb_dev_from_intf(intf);

	if (drv && drv->ops && drv->ops->suspend && dev)
		return drv->ops->suspend(dev, state);

	return 0;
}

static int nv_usb_resume(struct usb_interface *intf)
{
	struct nv_usb_driver *drv = nv_usb_drv_from_intf(intf);
	struct nv_usb_dev *dev = nv_usb_dev_from_intf(intf);

	if (drv && drv->ops && drv->ops->resume && dev)
		return drv->ops->resume(dev);

	return 0;
}

static int nv_usb_reset_resume(struct usb_interface *intf)
{
	struct nv_usb_driver *drv = nv_usb_drv_from_intf(intf);
	struct nv_usb_dev *dev = nv_usb_dev_from_intf(intf);

	if (drv && drv->ops && drv->ops->reset_resume && dev)
		return drv->ops->reset_resume(dev);

	return 0;
}

static void nv_usb_shutdown(struct usb_interface *intf)
{
	struct nv_usb_driver *drv = nv_usb_drv_from_intf(intf);
	struct nv_usb_dev *dev = nv_usb_dev_from_intf(intf);

	if (drv && drv->ops && drv->ops->shutdown && dev)
		drv->ops->shutdown(dev);
}

int nv_usb_driver_register(struct nv_usb_driver *drv)
{
	int ret;

	if (!drv || !drv->name || !drv->owner || !drv->id_table || !drv->ops)
		return -EINVAL;

	if (drv->registered)
		return -EBUSY;

	drv->usb_driver.name = drv->name;
	drv->usb_driver.id_table = drv->id_table;
	drv->usb_driver.probe = nv_usb_probe;
	drv->usb_driver.disconnect = nv_usb_disconnect;
	drv->usb_driver.suspend = nv_usb_suspend;
	drv->usb_driver.resume = nv_usb_resume;
	drv->usb_driver.reset_resume = nv_usb_reset_resume;
	drv->usb_driver.shutdown = nv_usb_shutdown;
	drv->usb_driver.supports_autosuspend =
		!!(drv->flags & NV_USB_FLAG_ENABLE_AUTOSUSPEND);

	ret = usb_register_driver(&drv->usb_driver, drv->owner, drv->name);
	if (ret)
		return ret;

	drv->registered = true;
	return 0;
}
EXPORT_SYMBOL_GPL(nv_usb_driver_register);

void nv_usb_driver_unregister(struct nv_usb_driver *drv)
{
	if (!drv || !drv->registered)
		return;

	usb_deregister(&drv->usb_driver);
	drv->registered = false;
}
EXPORT_SYMBOL_GPL(nv_usb_driver_unregister);

MODULE_AUTHOR("LinuxDriverFramework");
MODULE_DESCRIPTION("USB device framework");
MODULE_LICENSE("GPL");
