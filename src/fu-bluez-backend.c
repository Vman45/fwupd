/*
 * Copyright (C) 2021 Ricardo Cañuelo <ricardo.canuelo@collabora.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define G_LOG_DOMAIN				"FuBackend"

#include "config.h"

#include "fu-ble-device.h"
#include "fu-bluez-backend.h"

#define DEFAULT_PROXY_TIMEOUT	5000

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
static FuBleDevice *
fu_bluez_load_device_properties (GVariant *properties)
{
	const gchar *prop_name;
	GVariantIter it;
	g_autoptr(GVariant) prop_val = NULL;
	FuBleDevice *dev = NULL;
	g_autofree gchar *address = NULL;
	g_autofree gchar *name = NULL;
	gboolean connected = FALSE;
	GHashTable *characteristics = g_hash_table_new_full (g_str_hash, g_str_equal,
							     g_free, g_free);

	g_variant_iter_init (&it, properties);
	while (g_variant_iter_next (&it, "{&sv}", &prop_name, &prop_val)) {
		if (g_strcmp0 (prop_name, "Address") == 0) {
			address = g_strdup (g_variant_get_string(prop_val, NULL));
			continue;
		}
		if (g_strcmp0 (prop_name, "Name") == 0) {
			name = g_strdup (g_variant_get_string(prop_val, NULL));
			continue;
		}
		if (g_strcmp0 (prop_name, "Connected") == 0) {
			connected = g_variant_get_boolean(prop_val);
			continue;
		}
	}
	dev = fu_ble_device_new (name, address, characteristics);
	if (connected)
		fu_device_add_flag (FU_DEVICE (dev), FWUPD_DEVICE_FLAG_CONNECTED);

	return dev;
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
			g_autoptr(FuBleDevice) dev = NULL;

			if (g_strcmp0 (if_name, "org.bluez.Device1")) {
				continue;
			}

			dev = fu_bluez_load_device_properties (properties);
			if (dev != NULL) {
				g_hash_table_insert (self->devices,
						     g_strdup (fu_ble_device_get_address (dev)),
						     g_object_ref (dev));
				fu_backend_device_added (FU_BACKEND (self),
							 FU_DEVICE (dev));
			}
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
	/*
	 * TODO: Needed?
	 *
	 * klass_backend->recoldplug = fu_bluez_backend_recoldplug;
	 */
}

FuBackend *
fu_bluez_backend_new (void)
{
	return FU_BACKEND (g_object_new (FU_TYPE_BLUEZ_BACKEND, "name", "bluez", NULL));
}

/*
 * Calls the ReadValue method of the characteristic implemented by
 * obj_path.
 *
 * This creates a new ad hoc connection every time and runs
 * synchronously (ie. it blocks until it gets the result or until the
 * request times out).
 *
 * Returns a GByteArray that must be freed after use, NULL in case of
 * error.
 */
static GByteArray *
fu_bluez_characteristic_read_value (const gchar *obj_path)
{
	g_autoptr(GDBusProxy) proxy = NULL;
	g_autoptr(GVariant) val = NULL;
	GError *error = NULL;
	g_autoptr(GVariantBuilder) builder = NULL;
	g_autoptr(GVariantIter) iter = NULL;
	GByteArray *val_bytes = NULL;
	guint8 byte;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
					       G_DBUS_PROXY_FLAGS_NONE,
					       NULL,
					       "org.bluez",
					       obj_path,
					       "org.bluez.GattCharacteristic1",
					       NULL,
					       &error);
	if (proxy == NULL) {
		/* TODO: Error handling and logging */
		return NULL;
	}
	g_dbus_proxy_set_default_timeout (proxy, DEFAULT_PROXY_TIMEOUT);

	/*
	 * Call the "ReadValue" method through the proxy synchronously.
	 *
	 * The method takes one argument: an array of dicts of
	 * {string:value} specifing the options (here the option is
	 * "offset":0.
	 * The result is a byte array.
	 */
	builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
	g_variant_builder_add (builder, "{sv}", "offset",
			       g_variant_new("q", 0));

	val = g_dbus_proxy_call_sync (proxy,
				      "ReadValue",
				      g_variant_new ("(a{sv})", builder),
				      G_DBUS_CALL_FLAGS_NONE,
				      -1,
				      NULL,
				      &error);
	if (val == NULL) {
		/* TODO: Error handling and logging */
		return NULL;
	}

	val_bytes = g_byte_array_new ();
	g_variant_get(val, "(ay)", &iter);
	while (g_variant_iter_loop (iter, "y", &byte)) {
		g_byte_array_append (val_bytes, &byte, 1);
	}

	return val_bytes;
}

/*
 * Calls the WriteValue method of the characteristic implemented by
 * obj_path and writes a byte array to it.
 *
 * This creates a new ad hoc connection every time and runs
 * synchronously (ie. it blocks until it gets the result or until the
 * request times out).
 *
 * Returns EXIT_SUCCESS if the call ran successfully, EXIT_FAILURE
 * otherwise.
 */
static int
fu_bluez_characteristic_write_value (const gchar *obj_path, const guint8 *val, int len)
{
	g_autoptr(GDBusProxy) proxy = NULL;
	g_autoptr(GVariant) val_variant = NULL;
	g_autoptr(GVariant) opt_variant = NULL;
	g_autoptr(GVariantBuilder) val_builder = NULL;
	g_autoptr(GVariantBuilder) opt_builder = NULL;
	g_autoptr(GVariant) ret = NULL;
	GError *error = NULL;
	int i;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
					       G_DBUS_PROXY_FLAGS_NONE,
					       NULL,
					       "org.bluez",
					       obj_path,
					       "org.bluez.GattCharacteristic1",
					       NULL,
					       &error);
	if (proxy == NULL) {
		/* TODO: Error handling and logging */
		return EXIT_FAILURE;
	}
	/* Build the value variant */
	val_builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
	for (i = 0; i < len; i++) {
		g_variant_builder_add (val_builder, "y", val[i]);
	}
	val_variant = g_variant_new ("ay", val_builder);

	/* Build the options variant (offset = 0) */
	opt_builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add (opt_builder, "{sv}", "offset",
			       g_variant_new_uint16 (0));
	opt_variant = g_variant_new("a{sv}", opt_builder);

	ret = g_dbus_proxy_call_sync (proxy,
				      "WriteValue",
				      g_variant_new ("(@ay@a{sv})",
						     val_variant,
						     opt_variant),
				      G_DBUS_CALL_FLAGS_NONE,
				      -1,
				      NULL,
				      &error);
	if (ret == NULL) {
		/* TODO: Error handling and logging */
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

gchar *
fu_bluez_characteristic_read_string (const gchar *obj_path)
{
	GByteArray *val_bytes = fu_bluez_characteristic_read_value (obj_path);

	if (val_bytes == NULL)
		return NULL;

	return (gchar *) g_byte_array_free (val_bytes, FALSE);
}

int
fu_bluez_characteristic_write_bytes (const gchar *obj_path, const guint8 *val, int len)
{
	return fu_bluez_characteristic_write_value (obj_path, val, len);
}
