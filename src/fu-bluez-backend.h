/*
 * Copyright (C) 2021
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include "fu-backend.h"

#define FU_TYPE_BLUEZ_BACKEND (fu_bluez_backend_get_type ())
G_DECLARE_FINAL_TYPE (FuBluezBackend, fu_bluez_backend, FU, BLUEZ_BACKEND, FuBackend)

FuBackend	*fu_bluez_backend_new			(void);

//FIXME: would these be better as vfuncs on FuBleDevice?
GByteArray	*fu_bluez_characteristic_read_buf	(const gchar	*obj_path,
							 GError		**error);
gchar		*fu_bluez_characteristic_read_string 	(const gchar	*obj_path,
							 GError		**error);
gboolean	 fu_bluez_characteristic_write_buf 	(const gchar	*obj_path,
							 GByteArray	*buf,
							 GError		**error);
