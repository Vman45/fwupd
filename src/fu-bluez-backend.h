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
gchar *		fu_bluez_characteristic_read_string 	(const gchar *obj_path);
int		fu_bluez_characteristic_write_bytes 	(const gchar *obj_path,
							 const guint8 *val, int len);
