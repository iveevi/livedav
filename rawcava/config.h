#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum mono_option { LEFT, RIGHT, AVERAGE };

struct config_params {
        double monstercat;
	double integral;
	double gravity;
	double ignore;
	double sens;
	double noise_reduction;

        unsigned int lower_cut_off;
	unsigned int upper_cut_off;

	enum mono_option mono_opt;

	int stereo;
	int framerate;
	int autosens;
        int overshoot;
	int waves;
	int sleep_timer;
	int reverse;
};

bool load_config(char configPath[PATH_MAX], struct config_params *p);
