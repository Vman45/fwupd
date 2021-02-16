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
	gchar			*name;
	gchar			*address;
	gchar			*adapter;
} FuBleDevicePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (FuBleDevice, fu_ble_device, FU_TYPE_DEVICE)

#define GET_PRIVATE(o) (fu_ble_device_get_instance_private (o))

/**
 * fu_ble_device_get_address:
 * @self: A #FuBleDevice
 *
 * Gets the address of the device.
 *
 * Returns: The address of the device, e.g. `F2:EC:98:FF:03:C6`
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
 * @address: (nullable): The address, e.g. `F2:EC:98:FF:03:C6`
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
 * Returns: The adapter of the device, e.g. `/org/bluez/hci0`
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
 * @adapter: (nullable): The adapter, e.g. `/org/bluez/hci0`
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
 * fu_ble_device_read:
 * @self: A #FuBleDevice
 * @uuid: The UUID, e.g. `00cde35c-7062-11eb-9439-0242ac130002`
 * @error: A #GError, or %NULL
 *
 * Reads from a UUID on the device.
 *
 * Returns: (transfer full): data, or %NULL for error
 *
 * Since: 1.5.7
 **/
GByteArray *
fu_ble_device_read (FuBleDevice *self, const gchar *uuid, GError **error)
{
	FuBleDeviceClass *klass = FU_BLE_DEVICE_GET_CLASS (self);
	if (klass->read == NULL) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_NOT_SUPPORTED,
				     "not supported");
		return NULL;
	}
	return klass->read (self, uuid, error);
}

/**
 * fu_ble_device_read_string:
 * @self: A #FuBleDevice
 * @uuid: The UUID, e.g. `FIXME:better-example-please`
 * @error: A #GError, or %NULL
 *
 * Reads a string from a UUID on the device.
 *
 * Returns: (transfer full): data, or %NULL for error
 *
 * Since: 1.5.7
 **/
gchar *
fu_ble_device_read_string (FuBleDevice *self, const gchar *uuid, GError **error)
{
	GByteArray *buf = fu_ble_device_read (self, uuid, error);
	if (buf == NULL)
		return NULL;
	return (gchar *) g_byte_array_free (buf, FALSE);
}

/**
 * fu_ble_device_write:
 * @self: A #FuBleDevice
 * @uuid: The UUID, e.g. `00cde35c-7062-11eb-9439-0242ac130002`
 * @error: A #GError, or %NULL
 *
 * Writes to a UUID on the device.
 *
 * Returns: (transfer full): data, or %NULL for error
 *
 * Since: 1.5.7
 **/
gboolean
fu_ble_device_write (FuBleDevice *self,
			 const gchar *uuid,
			 GByteArray *buf,
			 GError **error)
{
	FuBleDeviceClass *klass = FU_BLE_DEVICE_GET_CLASS (self);
	if (klass->write == NULL) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_NOT_SUPPORTED,
				     "not supported");
		return FALSE;
	}
	return klass->write (self, uuid, buf, error);
}

static void
fu_ble_device_to_string (FuDevice *device, guint idt, GString *str)
{
	FuBleDevice *self = FU_BLE_DEVICE (device);
	FuBleDevicePrivate *priv = GET_PRIVATE (self);

	if (priv->name != NULL)
		fu_common_string_append_kv (str, idt, "Name", priv->name);
	if (priv->address != NULL)
		fu_common_string_append_kv (str, idt, "Address", priv->address);
	if (priv->adapter != NULL)
		fu_common_string_append_kv (str, idt, "Adapter", priv->adapter);
}

static void
fu_ble_device_finalize (GObject *object)
{
	FuBleDevice *self = FU_BLE_DEVICE (object);
	FuBleDevicePrivate *priv = GET_PRIVATE (self);

	g_free (priv->name);
	g_free (priv->address);
	g_free (priv->adapter);
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
