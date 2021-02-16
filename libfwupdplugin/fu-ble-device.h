/*
 * Copyright (C) 2021 Ricardo Cañuelo <ricardo.canuelo@collabora.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <glib-object.h>

#include "fu-device.h"

#define FU_TYPE_BLE_DEVICE (fu_ble_device_get_type ())
G_DECLARE_DERIVABLE_TYPE (FuBleDevice, fu_ble_device, FU, BLE_DEVICE, FuDevice)

struct _FuBleDeviceClass
{
	FuDeviceClass	parent_class;
	void		 (*to_string)			(FuBleDevice	*self,
							 guint		 indent,
							 GString	*str);
	gpointer	__reserved[28];
};

FuBleDevice		*fu_ble_device_new		(const gchar	*name,
							 const gchar	*addr,
							 GHashTable	*characteristics);
const gchar		*fu_ble_device_get_name 	(FuBleDevice	*self);
const gchar		*fu_ble_device_get_address 	(FuBleDevice	*self);
