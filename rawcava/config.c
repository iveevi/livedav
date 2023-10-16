#include "config.h"
#include "util.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>

#include <stdarg.h>
#include <stdbool.h>
#include <sys/stat.h>

char *channels;
char *monoOption;

bool validate_config(struct config_params *p)
{
	// validate: output channels
	p->stereo = -1;
	if (strcmp(channels, "mono") == 0) {
		p->stereo = 0;
		if (strcmp(monoOption, "average") == 0) {
			p->mono_opt = AVERAGE;
		} else if (strcmp(monoOption, "left") == 0) {
			p->mono_opt = LEFT;
		} else if (strcmp(monoOption, "right") == 0) {
			p->mono_opt = RIGHT;
		} else {
			fprintf(stderr, "mono option %s is not supported, supported "
					"options are: 'average', "
					"'left' or 'right'\n",
					monoOption);
			return false;
		}
	}
	if (strcmp(channels, "stereo") == 0)
		p->stereo = 1;
	if (p->stereo == -1) {
		fprintf(stderr, "output channels %s is not supported, supported "
				"channelss are: 'mono' and 'stereo'\n",
				channels);
		return false;
	}

	// validate: framerate
	if (p->framerate < 0) {
		fprintf(stderr, "framerate can't be negative!\n");
		return false;
	}

	// validate: gravity
	p->gravity = p->gravity / 100;
	if (p->gravity < 0) {
		p->gravity = 0;
	}

	// validate: integral
	p->integral = p->integral / 100;
	if (p->integral < 0) {
		p->integral = 0;
	} else if (p->integral > 1) {
		p->integral = 1;
	}

	// validate: noise_reduction
	if (p->noise_reduction < 0) {
		p->noise_reduction = 0;
	} else if (p->noise_reduction > 1) {
		p->noise_reduction = 1;
	}

	// validate: cutoff
	if (p->lower_cut_off == 0)
		p->lower_cut_off++;
	if (p->lower_cut_off > p->upper_cut_off) {
		fprintf(stderr, "lower cutoff frequency can't be higher than "
				"higher cutoff frequency\n");
		return false;
	}

	// setting sens
	p->sens = p->sens / 100;

	return true;
}

bool load_config(char configPath[PATH_MAX], struct config_params *p)
{
	// TODO: most of these will become library options...
	p->monstercat = 1.5 * 0;
	p->waves = 0;
	p->integral = 77;
	p->gravity = 100;
	p->ignore = 0;
	p->noise_reduction = 0.77;
	p->framerate = 60;
	p->sens = 100;
	p->autosens = 1;
	p->overshoot = 20;
	p->lower_cut_off = 50;
	p->upper_cut_off = 10000;
	p->sleep_timer = 1;

	// config: output
	free(channels);
	free(monoOption);

	channels = strdup("stereo");
	monoOption = strdup("average");
	p->reverse = 0;

	bool result = validate_config(p);

	return result;
}
