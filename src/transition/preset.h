// SPDX-License-Identifier: MPL-2.0
// Copyright (c) Yuxuan Shui <yshuiv7@gmail.com>

#pragma once

#include <stdbool.h>

typedef struct config_setting_t config_setting_t;
struct win_script;

struct win_script_preset {
	const char *name;
	bool (*func)(struct win_script *output, config_setting_t *setting);
};

extern const struct win_script_preset win_script_presets[];

/// Parse an animation preset definition into a win_script.
bool win_script_parse_preset(struct win_script *output, config_setting_t *setting);
