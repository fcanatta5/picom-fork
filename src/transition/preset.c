// SPDX-License-Identifier: MPL-2.0
// Copyright (c) Yuxuan Shui <yshuiv7@gmail.com>

#include <libconfig.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "config.h"
#include "preset.h"
#include "script.h"
#include "utils/misc.h"
#include "utils/str.h"
#include "wm/win.h"

extern struct {
	const char *name;
	bool (*func)(struct win_script *output, config_setting_t *setting);
} win_script_presets[];

static bool compile_inline_win_script(struct win_script *output, const char *script_text) {
	config_t cfg;
	config_init(&cfg);
	config_set_auto_convert(&cfg, true);

	if (!config_read_string(&cfg, script_text)) {
		log_error("Failed to parse built-in animation preset at line %d: %s",
		          config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		return false;
	}

	struct script_output_info outputs[ARR_SIZE(win_script_outputs)];
	memcpy(outputs, win_script_outputs, sizeof(win_script_outputs));

	struct script_parse_config parse_config = {
	    .context_info = win_script_context_info,
	    .output_info = outputs,
	};

	char *err = NULL;
	output->script = script_compile(config_root_setting(&cfg), parse_config, &err);
	if (output->script == NULL) {
		log_error("Failed to compile built-in animation preset: %s",
		          err == NULL ? "see previous error message" : err);
		free(err);
		config_destroy(&cfg);
		return false;
	}
	free(err);

	for (int i = 0; i < NUM_OF_WIN_SCRIPT_OUTPUTS; i++) {
		output->output_indices[i] = outputs[i].slot;
	}

	config_destroy(&cfg);
	return true;
}

static double preset_float(config_setting_t *setting, const char *name, double def) {
	double value = def;
	config_setting_lookup_float(setting, name, &value);
	return value;
}

static const char *preset_string(config_setting_t *setting, const char *name,
                                 const char *def) {
	const char *value = def;
	config_setting_lookup_string(setting, name, &value);
	return value;
}

static bool hypr_direction_vector(const char *direction, double *x, double *y) {
	if (strcasecmp(direction, "left") == 0) {
		*x = -1.0;
		*y = 0.0;
		return true;
	}
	if (strcasecmp(direction, "right") == 0) {
		*x = 1.0;
		*y = 0.0;
		return true;
	}
	if (strcasecmp(direction, "up") == 0) {
		*x = 0.0;
		*y = -1.0;
		return true;
	}
	if (strcasecmp(direction, "down") == 0) {
		*x = 0.0;
		*y = 1.0;
		return true;
	}
	log_error("Invalid direction \"%s\" for Hyprland-like animation preset. "
	          "Valid values are: left, right, up, down.",
	          direction);
	return false;
}

static bool win_script_preset__hypr_scale(struct win_script *output,
                                          config_setting_t *setting,
                                          bool opening) {
	const double duration = preset_float(setting, "duration", opening ? 0.22 : 0.16);
	const double scale = preset_float(setting, "scale", opening ? 0.84 : 0.82);
	const char *curve = opening ? "cubic-bezier(0.05, 0.90, 0.10, 1.05)"
	                            : "cubic-bezier(0.32, 0.00, 0.67, 0.00)";

	char *script = NULL;
	if (opening) {
		casprintf(&script,
		          "opacity = { curve = \"%s\"; duration = %.17g; "
		          "start = \"window-raw-opacity-before\"; end = \"window-raw-opacity\"; };"
		          "blur-opacity = \"opacity\";"
		          "shadow-opacity = \"opacity\";"
		          "scale-x = { curve = \"%s\"; duration = %.17g; start = %.17g; end = 1.0; };"
		          "scale-y = \"scale-x\";"
		          "offset-x = \"(1 - scale-x) / 2 * window-width\";"
		          "offset-y = \"(1 - scale-y) / 2 * window-height\";"
		          "shadow-scale-x = \"scale-x\";"
		          "shadow-scale-y = \"scale-y\";"
		          "shadow-offset-x = \"offset-x\";"
		          "shadow-offset-y = \"offset-y\";",
		          curve, duration, curve, duration, scale);
	} else {
		casprintf(&script,
		          "opacity = { curve = \"%s\"; duration = %.17g; "
		          "start = \"window-raw-opacity-before\"; end = \"window-raw-opacity\"; };"
		          "blur-opacity = \"opacity\";"
		          "shadow-opacity = \"opacity\";"
		          "scale-x = { curve = \"%s\"; duration = %.17g; start = 1.0; end = %.17g; };"
		          "scale-y = \"scale-x\";"
		          "offset-x = \"(1 - scale-x) / 2 * window-width\";"
		          "offset-y = \"(1 - scale-y) / 2 * window-height\";"
		          "shadow-scale-x = \"scale-x\";"
		          "shadow-scale-y = \"scale-y\";"
		          "shadow-offset-x = \"offset-x\";"
		          "shadow-offset-y = \"offset-y\";",
		          curve, duration, curve, duration, scale);
	}

	bool ok = compile_inline_win_script(output, script);
	free(script);
	return ok;
}

static bool win_script_preset__hypr_open(struct win_script *output,
                                         config_setting_t *setting) {
	return win_script_preset__hypr_scale(output, setting, true);
}

static bool win_script_preset__hypr_close(struct win_script *output,
                                          config_setting_t *setting) {
	return win_script_preset__hypr_scale(output, setting, false);
}

static bool win_script_preset__hypr_fade(struct win_script *output,
                                         config_setting_t *setting,
                                         bool fade_in) {
	const double duration = preset_float(setting, "duration", 0.13);
	const char *curve = fade_in ? "cubic-bezier(0.05, 0.90, 0.10, 1.00)"
	                            : "cubic-bezier(0.32, 0.00, 0.67, 0.00)";
	char *script = NULL;
	casprintf(&script,
	          "opacity = { curve = \"%s\"; duration = %.17g; "
	          "start = \"window-raw-opacity-before\"; end = \"window-raw-opacity\"; };"
	          "blur-opacity = { curve = \"%s\"; duration = %.17g; "
	          "start = \"window-blur-opacity-before\"; end = \"window-blur-opacity\"; };"
	          "shadow-opacity = \"opacity\";",
	          curve, duration, curve, duration);
	bool ok = compile_inline_win_script(output, script);
	free(script);
	return ok;
}

static bool win_script_preset__hypr_fade_in(struct win_script *output,
                                            config_setting_t *setting) {
	return win_script_preset__hypr_fade(output, setting, true);
}

static bool win_script_preset__hypr_fade_out(struct win_script *output,
                                             config_setting_t *setting) {
	return win_script_preset__hypr_fade(output, setting, false);
}

static bool win_script_preset__hypr_geometry(struct win_script *output,
                                             config_setting_t *setting) {
	const double duration = preset_float(setting, "duration", 0.20);
	const char *curve = "cubic-bezier(0.05, 0.90, 0.10, 1.05)";
	char *script = NULL;
	casprintf(&script,
	          "scale-x = { curve = \"%s\"; duration = %.17g; "
	          "start = \"window-width-before / window-width\"; end = 1.0; };"
	          "scale-y = { curve = \"%s\"; duration = %.17g; "
	          "start = \"window-height-before / window-height\"; end = 1.0; };"
	          "shadow-scale-x = \"scale-x\";"
	          "shadow-scale-y = \"scale-y\";"
	          "offset-x = { curve = \"%s\"; duration = %.17g; "
	          "start = \"window-x-before - window-x\"; end = 0.0; };"
	          "offset-y = { curve = \"%s\"; duration = %.17g; "
	          "start = \"window-y-before - window-y\"; end = 0.0; };"
	          "saved-image-blend = { curve = \"linear\"; duration = %.17g; start = 1.0; end = 0.0; };"
	          "shadow-offset-x = \"offset-x\";"
	          "shadow-offset-y = \"offset-y\";",
	          curve, duration, curve, duration, curve, duration, curve, duration,
	          duration);
	bool ok = compile_inline_win_script(output, script);
	free(script);
	return ok;
}

static bool win_script_preset__hypr_workspace(struct win_script *output,
                                              config_setting_t *setting,
                                              bool entering) {
	const double duration = preset_float(setting, "duration", 0.24);
	const double distance = preset_float(setting, "distance", 0.16);
	const double scale = preset_float(setting, "scale", entering ? 0.985 : 0.985);
	const char *direction = preset_string(setting, "direction", entering ? "right" : "left");
	double dx = 0.0, dy = 0.0;
	if (!hypr_direction_vector(direction, &dx, &dy)) {
		return false;
	}

	const double start_x = entering ? dx : 0.0;
	const double start_y = entering ? dy : 0.0;
	const double end_x = entering ? 0.0 : dx;
	const double end_y = entering ? 0.0 : dy;
	const double opacity_start = entering ? 0.78 : 1.0;
	const double opacity_end = entering ? 1.0 : 0.78;
	const double scale_start = entering ? scale : 1.0;
	const double scale_end = entering ? 1.0 : scale;
	const char *curve = "cubic-bezier(0.05, 0.90, 0.10, 1.05)";

	char *script = NULL;
	casprintf(&script,
	          "offset-x = { curve = \"%s\"; duration = %.17g; "
	          "start = \"window-monitor-width * %.17g * %.17g\"; "
	          "end = \"window-monitor-width * %.17g * %.17g\"; };"
	          "offset-y = { curve = \"%s\"; duration = %.17g; "
	          "start = \"window-monitor-height * %.17g * %.17g\"; "
	          "end = \"window-monitor-height * %.17g * %.17g\"; };"
	          "shadow-offset-x = \"offset-x\";"
	          "shadow-offset-y = \"offset-y\";"
	          "opacity = { curve = \"linear\"; duration = %.17g; "
	          "start = \"window-raw-opacity-before * %.17g\"; "
	          "end = \"window-raw-opacity * %.17g\"; };"
	          "blur-opacity = \"opacity\";"
	          "shadow-opacity = \"opacity\";"
	          "scale-x = { curve = \"%s\"; duration = %.17g; start = %.17g; end = %.17g; };"
	          "scale-y = \"scale-x\";"
	          "shadow-scale-x = \"scale-x\";"
	          "shadow-scale-y = \"scale-y\";"
	          "crop-x = \"window-x\";"
	          "crop-y = \"window-y\";"
	          "crop-width = \"window-width\";"
	          "crop-height = \"window-height\";",
	          curve, duration, start_x, distance, end_x, distance, curve, duration,
	          start_y, distance, end_y, distance, duration, opacity_start,
	          opacity_end, curve, duration, scale_start, scale_end);
	bool ok = compile_inline_win_script(output, script);
	free(script);
	return ok;
}

static bool win_script_preset__hypr_workspace_in(struct win_script *output,
                                                 config_setting_t *setting) {
	return win_script_preset__hypr_workspace(output, setting, true);
}

static bool win_script_preset__hypr_workspace_out(struct win_script *output,
                                                  config_setting_t *setting) {
	return win_script_preset__hypr_workspace(output, setting, false);
}

static bool win_script_parse_hypr_preset(const char *preset, struct win_script *output,
                                         config_setting_t *setting) {
	if (strcmp(preset, "hypr-open") == 0 || strcmp(preset, "hypr-popin") == 0) {
		return win_script_preset__hypr_open(output, setting);
	}
	if (strcmp(preset, "hypr-close") == 0 || strcmp(preset, "hypr-popout") == 0) {
		return win_script_preset__hypr_close(output, setting);
	}
	if (strcmp(preset, "hypr-fade-in") == 0) {
		return win_script_preset__hypr_fade_in(output, setting);
	}
	if (strcmp(preset, "hypr-fade-out") == 0) {
		return win_script_preset__hypr_fade_out(output, setting);
	}
	if (strcmp(preset, "hypr-geometry") == 0 || strcmp(preset, "hypr-move") == 0) {
		return win_script_preset__hypr_geometry(output, setting);
	}
	if (strcmp(preset, "hypr-workspace-in") == 0) {
		return win_script_preset__hypr_workspace_in(output, setting);
	}
	if (strcmp(preset, "hypr-workspace-out") == 0) {
		return win_script_preset__hypr_workspace_out(output, setting);
	}
	return false;
}

bool win_script_parse_preset(struct win_script *output, config_setting_t *setting) {
	const char *preset = NULL;
	if (!config_setting_lookup_string(setting, "preset", &preset)) {
		log_error("Missing preset name in script");
		return false;
	}

	if (strncmp(preset, "hypr-", strlen("hypr-")) == 0) {
		log_debug("Using Hyprland-like animation preset: %s", preset);
		if (win_script_parse_hypr_preset(preset, output, setting)) {
			return true;
		}
		log_error("Unknown Hyprland-like animation preset: %s", preset);
		return false;
	}

	for (unsigned i = 0; win_script_presets[i].name; i++) {
		if (strcmp(preset, win_script_presets[i].name) == 0) {
			log_debug("Using animation preset: %s", preset);
			return win_script_presets[i].func(output, setting);
		}
	}
	log_error("Unknown preset: %s", preset);
	return false;
}
