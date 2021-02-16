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
 * fu_ble_device_set_name:
 * @self: A #FuBleDevice
 * @name: (nullable): The name, e.g. `FIXME:better-example-please`
 *
 * Sets the name of the device.
 *
 * Since: 1.5.7
 **/
void
fu_ble_device_set_name (FuBleDevice *self, const gchar *name)
{
	FuBleDevicePrivate *priv = GET_PRIVATE (self);
	g_return_if_fail (FU_IS_BLE_DEVICE (self));

	/* not changed */
	if (g_strcmp0 (priv->name, name) == 0)
		return;

	g_free (priv->name);
	priv->name = g_strdup (name);
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

/**
 * fu_ble_device_set_address:
 * @self: A #FuBleDevice
 * @address: (nullable): The address, e.g. `FIXME:better-example-please`
 *
 * Sets the address of the device.
 *
 * Since: 1.5.7
 **/
void
fu_ble_device_set_address (FuBleDevice *self, const gchar *address)
{
	FuBleDevicePrivate *priv = GET_PRIVATE (self);
	g_return_if_fail (FU_IS_BLE_DEVICE (self));

	/* not changed */
	if (g_strcmp0 (priv->address, address) == 0)
		return;

	g_free (priv->address);
	priv->address = g_strdup (address);
}

/**
 * fu_ble_device_get_adapter:
 * @self: A #FuBleDevice
 *
 * Gets the adapter of the device.
 *
 * Returns: The adapter of the device.
 *
 * Since: 1.5.7
 **/
const gchar *
fu_ble_device_get_adapter (FuBleDevice *self)
{
	FuBleDevicePrivate *priv = GET_PRIVATE (self);
	g_return_val_if_fail (FU_IS_BLE_DEVICE (self), NULL);
	return priv->adapter;
}

/**
 * fu_ble_device_set_adapter:
 * @self: A #FuBleDevice
 * @adapter: (nullable): The adapter, e.g. `FIXME:better-example-please`
 *
 * Sets the adapter of the device.
 *
 * Since: 1.5.7
 **/
void
fu_ble_device_set_adapter (FuBleDevice *self, const gchar *adapter)
{
	FuBleDevicePrivate *priv = GET_PRIVATE (self);
	g_return_if_fail (FU_IS_BLE_DEVICE (self));

	/* not changed */
	if (g_strcmp0 (priv->adapter, adapter) == 0)
		return;

	g_free (priv->adapter);
	priv->adapter = g_strdup (adapter);
}

/**
 * fu_ble_device_add_characteristic:
 * @self: A #FuBleDevice
 * @uuid: (nullable): The UUID, e.g. `FIXME:better-example-please`
 * @uuid: (nullable): The path, e.g. `FIXME:better-example-please`
 *
 * Adds a characteristic to the device.
 *
 * Since: 1.5.7
 **/
void
fu_ble_device_add_characteristic (FuBleDevice *self,
				  const gchar *uuid,
				  const gchar *path)
{
	FuBleDevicePrivate *priv = GET_PRIVATE (self);
	g_return_if_fail (FU_IS_BLE_DEVICE (self));
	g_return_if_fail (uuid != NULL);
	g_return_if_fail (path != NULL);
	g_hash_table_insert (priv->characteristics,
			     g_strdup (uuid),
			     g_strdup (path));
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
	FuBleDevicePrivate *priv = GET_PRIVATE (self);
	priv->characteristics = g_hash_table_new_full (g_str_hash, g_str_equal,
						       g_free, g_free);
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
fu_ble_device_new (void)
{
	FuBleDevice *self = g_object_new (FU_TYPE_BLE_DEVICE, NULL);
	return FU_BLE_DEVICE (self);
}
