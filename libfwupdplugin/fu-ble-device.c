/*
 * Copyright (C) 2021 Ricardo Cañuelo <ricardo.canuelo@collabora.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define G_LOG_DOMAIN				"FuBleDevice"

#include "config.h"

#include "fu-common.h"
#include "fu-ble-device.h"
#include "fu-device-private.h"

/**
 * SECTION:fu-ble-device
 * @short_description: a Bluetooth LE device
 *
 * An object that represents a Bluetooth LE device.
 *
 * See also: #FuDevice
 */

typedef struct {
	gchar		*name;
	gchar		*address;
	gchar		*adapter;
	GHashTable	*characteristics;	/* utf8 : utf8 */
} FuBleDevicePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (FuBleDevice, fu_ble_device, FU_TYPE_DEVICE)

#define GET_PRIVATE(o) (fu_ble_device_get_instance_private (o))

/**
 * fu_ble_device_get_name:
 * @self: A #FuBleDevice
 *
 * Gets the name of the device.
 *
 * Returns: The name of the device.
 *
 * Since: 1.5.7
 **/
const gchar *
fu_ble_device_get_name (FuBleDevice *self)
{
	FuBleDevicePrivate *priv = GET_PRIVATE (self);
	g_return_val_if_fail (FU_IS_BLE_DEVICE (self), NULL);
	return priv->name;
}

/**
 * fu_ble_device_get_address:
 * @self: A #FuBleDevice
 *
 * Gets the address of the device.
 *
 * Returns: The address of the device.
 *
 * Since: 1.5.7
 **/
const gchar *
fu_ble_device_get_address (FuBleDevice *self)
{
	FuBleDevicePrivate *priv = GET_PRIVATE (self);
	g_return_val_if_fail (FU_IS_BLE_DEVICE (self), NULL);
	return priv->address;
}

static void
fu_ble_device_to_string (FuDevice *device, guint idt, GString *str)
{
	FuBleDevice *self = FU_BLE_DEVICE (device);
	FuBleDeviceClass *klass = FU_BLE_DEVICE_GET_CLASS (self);
	FuBleDevicePrivate *priv = GET_PRIVATE (self);

	if (priv->name != NULL)
		fu_common_string_append_kv (str, idt, "Name", priv->name);
	if (priv->address != NULL)
		fu_common_string_append_kv (str, idt, "Address", priv->address);
	if (priv->adapter != NULL)
		fu_common_string_append_kv (str, idt, "Adapter", priv->adapter);
	if (priv->characteristics != NULL) {
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init (&iter, priv->characteristics);
		while (g_hash_table_iter_next (&iter, &key, &value)) {
			fu_common_string_append_kv (str, idt + 1,
						    (const gchar *) key,
						    (const gchar *) value);
		}
	}

	/* subclassed */
	if (klass->to_string != NULL)
		klass->to_string (self, idt, str);
}

static void
fu_ble_device_finalize (GObject *object)
{
	FuBleDevice *self = FU_BLE_DEVICE (object);
	FuBleDevicePrivate *priv = GET_PRIVATE (self);

	g_free (priv->name);
	g_free (priv->address);
	g_free (priv->adapter);
	g_hash_table_unref (priv->characteristics);
	G_OBJECT_CLASS (fu_ble_device_parent_class)->finalize (object);
}

static void
fu_ble_device_init (FuBleDevice *self)
{
}

static void
fu_ble_device_class_init (FuBleDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	FuDeviceClass *device_class = FU_DEVICE_CLASS (klass);

	object_class->finalize = fu_ble_device_finalize;
	device_class->to_string = fu_ble_device_to_string;
}

/**
 * fu_ble_device_new:
 * @name: The name of the ble device.
 * @addr: The 48-bit address of the device.
 * @characteristics: A #GHashTable of utf8:utf8. The characteristics
 *   implemented by the device {uuid : object_path}.
 *
 * Creates a new #FuBleDevice.
 *
 * Returns: (transfer full): a #FuBleDevice
 *
 * Since: 1.5.7
 **/
FuBleDevice *
fu_ble_device_new (const gchar *name, const gchar *addr, GHashTable *characteristics)
{
	FuBleDevice *self = g_object_new (FU_TYPE_BLE_DEVICE, NULL);
	FuBleDevicePrivate *priv = GET_PRIVATE (self);

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (addr != NULL, NULL);
	g_return_val_if_fail (characteristics != NULL, NULL);

	priv->name = g_strdup (name);
	priv->address = g_strdup (addr);
	priv->characteristics = g_hash_table_ref (characteristics);
	return FU_BLE_DEVICE (self);
}
