/*
 * Copyright (C) 2018 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include "fu-plugin-vfuncs.h"

#include "fu-rts54hub-device.h"

void
fu_plugin_init (FuPlugin *plugin)
{
	fu_plugin_set_build_hash (plugin, FU_BUILD_HASH);
	fu_plugin_add_rule (plugin, FU_PLUGIN_RULE_REQUIRES_QUIRK, FU_QUIRKS_PLUGIN);
	fu_plugin_add_rule (plugin, FU_PLUGIN_RULE_SUPPORTS_PROTOCOL, "com.realtek.rts54");
	fu_plugin_set_device_gtype (plugin, FU_TYPE_RTS54HUB_DEVICE);
}