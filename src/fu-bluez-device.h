/*
 * Copyright (C) 2021 Ricardo Cañuelo <ricardo.canuelo@collabora.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <glib-object.h>

#include "fu-ble-device.h"

#define FU_TYPE_BLUEZ_DEVICE (fu_bluez_device_get_type ())
G_DECLARE_FINAL_TYPE (FuBluezDevice, fu_bluez_device, FU, BLUEZ_DEVICE, FuBleDevice)

FuBluezDevice		*fu_bluez_device_new		(void);
void			 fu_bluez_device_add_uuid_path	(FuBluezDevice	*self,
							 const gchar	*uuid,
							 const gchar	*path);
void			 fu_bluez_device_set_modalias	(FuBluezDevice	*self,
							 const gchar	*modalias);
