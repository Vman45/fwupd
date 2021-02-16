/*
 * Copyright (C) 2021 Ricardo Cañuelo <ricardo.canuelo@collabora.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define G_LOG_DOMAIN				"FuBackend"

#include "config.h"

#include "fu-bluez-device.h"
#include "fu-bluez-backend.h"

struct _FuBluezBackend {
	FuBackend		 parent_instance;
	GDBusConnection		*connection;
	GHashTable		*devices;	/* platform_id : * FuDevice */
};

G_DEFINE_TYPE (FuBluezBackend, fu_bluez_backend, FU_TYPE_BACKEND)

/*
 * Returns a new FuBleDevice populated with the properties passed
 * in the properties variant (@a{sv}).
 *
 * TODO: Search device characteristics and populate the hash table.
 */
static FuBluezDevice *
fu_bluez_load_device_properties (GVariant *properties)
{
	const gchar *prop_name;
	GVariantIter it;
	g_autoptr(FuBluezDevice) dev = fu_bluez_device_new ();
	g_autoptr(GVariant) prop_val = NULL;

	g_variant_iter_init (&it, properties);
	while (g_variant_iter_next (&it, "{&sv}", &prop_name, &prop_val)) {
		if (g_getenv ("FU_BLUEZ_BACKEND_DEBUG") != NULL) {
			g_autofree gchar *str = g_variant_print (prop_val, TRUE);
			g_debug ("%s: %s", prop_name, str);
		}
		if (g_strcmp0 (prop_name, "Address") == 0) {
			fu_ble_device_set_address (FU_BLE_DEVICE (dev),
						   g_variant_get_string (prop_val, NULL));
			continue;
		}
		if (g_strcmp0 (prop_name, "Adapter") == 0) {
			fu_ble_device_set_adapter (FU_BLE_DEVICE (dev),
						   g_variant_get_string (prop_val, NULL));
			continue;
		}
		if (g_strcmp0 (prop_name, "Name") == 0) {
			fu_device_set_name (FU_DEVICE (dev),
					    g_variant_get_string (prop_val, NULL));
			continue;
		}
		if (g_strcmp0 (prop_name, "Icon") == 0) {
			fu_device_add_icon (FU_DEVICE (dev),
					    g_variant_get_string (prop_val, NULL));
			continue;
		}
		if (g_strcmp0 (prop_name, "Modalias") == 0) {
			fu_bluez_device_set_modalias (dev, g_variant_get_string (prop_val, NULL));
			continue;
		}
		if (g_strcmp0 (prop_name, "Connected") == 0) {
			if (g_variant_get_boolean (prop_val))
				fu_device_add_flag (FU_DEVICE (dev), FWUPD_DEVICE_FLAG_CONNECTED);
			continue;
		}
	}
	return g_steal_pointer (&dev);
}


static gboolean
fu_bluez_backend_setup (FuBackend *backend, GError **error)
{
	FuBluezBackend *self = FU_BLUEZ_BACKEND (backend);

	self->connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, NULL);
	if (self->connection == NULL) {
		g_prefix_error (error, "Failed to connect to bluez dbus: ");
		return FALSE;
	}
	/*
	 * TODO: Subscribe DBus signals.
	 */

	return TRUE;
}

static gboolean
fu_bluez_backend_coldplug (FuBackend *backend, GError **error)
{
	FuBluezBackend *self = FU_BLUEZ_BACKEND (backend);
	g_autoptr(GDBusProxy) proxy = NULL;
	g_autoptr(GVariant) value = NULL;
	g_autoptr(GVariant) obj_info = NULL;
	const gchar *obj_path;
	GVariantIter i;

	/*
	 * Bluez publishes all the object info through the
	 * "GetManagedObjects" method
	 * ("org.freedesktop.DBus.ObjectManager" interface).
	 *
	 * Look for objects that implement "org.bluez.Device1".
	 */

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
					       G_DBUS_PROXY_FLAGS_NONE,
					       NULL,
					       "org.bluez",
					       "/",
					       "org.freedesktop.DBus.ObjectManager",
					       NULL,
					       error);
	if (proxy == NULL) {
		g_prefix_error (error, "Failed to connect to bluez dbus: ");
		return FALSE;
	}
	value = g_dbus_proxy_call_sync (proxy,
					"GetManagedObjects",
					NULL,
					G_DBUS_CALL_FLAGS_NONE,
					-1,
					NULL,
					error);
	if (value == NULL) {
		g_prefix_error (error, "Failed to call GetManagedObjects: ");
		return FALSE;
	}

	value = g_variant_get_child_value (value, 0);
	g_variant_iter_init (&i, value);
	while (g_variant_iter_next (&i, "{&o@a{sa{sv}}}", &obj_path, &obj_info)) {
		const gchar *if_name;
		g_autoptr(GVariant) properties = NULL;
		GVariantIter j;

		g_variant_iter_init (&j, obj_info);
		while (g_variant_iter_next (&j, "{&s@a{sv}}", &if_name, &properties)) {
			g_autoptr(FuBluezDevice) dev = NULL;
			if (g_strcmp0 (if_name, "org.bluez.Device1") != 0)
				continue;
			dev = fu_bluez_load_device_properties (properties);
			if (dev == NULL)
				continue;
			if (g_getenv ("FU_BLUEZ_BACKEND_DEBUG") != NULL)
				g_debug ("%s", fu_device_to_string (FU_DEVICE (dev)));
			g_hash_table_insert (self->devices,
					     g_strdup (fu_ble_device_get_address (FU_BLE_DEVICE (dev))),
					     g_object_ref (dev));
			fu_backend_device_added (FU_BACKEND (self), FU_DEVICE (dev));
		}
	}

	return TRUE;
}

static void
fu_bluez_backend_finalize (GObject *object)
{
	FuBluezBackend *self = FU_BLUEZ_BACKEND (object);

	g_hash_table_unref (self->devices);
	g_object_unref (self->connection);
	G_OBJECT_CLASS (fu_bluez_backend_parent_class)->finalize (object);
}

static void
fu_bluez_backend_init (FuBluezBackend *self)
{
	self->devices = g_hash_table_new_full (g_str_hash, g_str_equal,
					       g_free,
					       (GDestroyNotify) g_object_unref);
}

static void
fu_bluez_backend_class_init (FuBluezBackendClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	FuBackendClass *klass_backend = FU_BACKEND_CLASS (klass);

	object_class->finalize = fu_bluez_backend_finalize;
	klass_backend->setup = fu_bluez_backend_setup;
	klass_backend->coldplug = fu_bluez_backend_coldplug;
	klass_backend->recoldplug = fu_bluez_backend_coldplug;
}

FuBackend *
fu_bluez_backend_new (void)
{
	return FU_BACKEND (g_object_new (FU_TYPE_BLUEZ_BACKEND, "name", "bluez", NULL));
}
