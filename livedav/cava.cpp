#include <locale.h>

#include <stdlib.h>
#include <fcntl.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "cavacore.hpp"
#include "config.hpp"
#include "common.hpp"
#include "pulse.hpp"
#include "util.hpp"

// these variables are used only in main, but making them global
// will allow us to not free them on exit without ASan complaining
struct config_params p;

// general: entry point
// TODO: create a library module, then use this main function as an example
int main(int argc, char **argv)
{
	// general: handle command-line arguments
	char configPath[PATH_MAX];
	configPath[0] = '\0';

	// config: load
	if (!load_config(configPath, &p))
		exit(EXIT_FAILURE);

	// input: init
	struct audio_data audio;
	memset(&audio, 0, sizeof(audio));

	// TODO: this is an actual option
	audio.source = (char *) malloc(5);
	strcpy(audio.source, "auto");

	audio.format = -1;
	audio.rate = 0;
	audio.samples_counter = 0;
	audio.channels = 2;

	audio.input_buffer_size = BUFFER_SIZE * audio.channels;
	audio.cava_buffer_size = audio.input_buffer_size * 8;

	audio.cava_in = (double *)malloc(audio.cava_buffer_size * sizeof(double));
	memset(audio.cava_in, 0, sizeof(int) * audio.cava_buffer_size);

	audio.terminate = 0;

	pthread_t p_thread;

	// configuring pulseaudio
	if (strcmp(audio.source, "auto") == 0)
		getPulseDefaultSink((void *) &audio);

	// TODO: use C++ threading instead of pthreads
	// starting pulsemusic listener
	pthread_create(&p_thread, NULL, input_pulse, (void *) &audio);
	audio.rate = 44100;
	// end of input init

	// TODO: use microlog for logging...
	if (p.upper_cut_off > audio.rate / 2) {
		fprintf(stderr, "higher cuttoff frequency can't be higher than sample rate / 2");
		exit(EXIT_FAILURE);
	}

	double *cava_out;

	// getting numbers of bars
	int number_of_bars = 200;
	printf("number of bars: %d\n", number_of_bars);

	if (number_of_bars <= 1) {
		number_of_bars = 1; // must have at least 1 bars
		if (p.stereo)
			number_of_bars = 2; // stereo have at least 2 bars
	}

	int output_channels = 1;
	if (p.stereo) { // stereo must have even numbers of bars
		if (audio.channels == 1) {
			fprintf(stderr,
					"stereo output configured, but only one channel in audio input.\n");
			exit(1);
		}
		output_channels = 2;
		if (number_of_bars % 2 != 0)
			number_of_bars--;
	}

	struct cava_plan *plan = cava_init(number_of_bars / output_channels,
		audio.rate, audio.channels, p.autosens,
		p.noise_reduction, p.lower_cut_off, p.upper_cut_off
	);

	cava_out = (double *) malloc(number_of_bars * audio.channels / output_channels * sizeof(double));

	memset(cava_out, 0, sizeof(double) * number_of_bars * audio.channels / output_channels);
	// NOTE: cava out is the real deal...

	int frame_time_msec = (1 / (float)p.framerate) * 1000;
	struct timespec framerate_timer = {.tv_sec = 0, .tv_nsec = 0};
	if (p.framerate <= 1) {
		framerate_timer.tv_sec = frame_time_msec / 1000;
	} else {
		framerate_timer.tv_sec = 0;
		framerate_timer.tv_nsec = frame_time_msec * 1e6;
	}

	int sleep_counter = 0;
	bool silence = false;

	struct timespec sleep_mode_timer = {.tv_sec = 1, .tv_nsec = 0};

	while (1) {
		// FIXME: silence does not fully work...
		silence = true;
		for (int n = 0; n < audio.input_buffer_size * 4; n++) {
			if (audio.cava_in[n]) {
				silence = false;
				break;
			}
		}

		if (p.sleep_timer) {
			printf("valid sleep timer\n");
			if (silence && sleep_counter <= p.framerate * p.sleep_timer) {
				printf("silence, adding to counter\n");
				sleep_counter++;
			} else if (!silence) {
				printf("sound on, resetting counter\n");
				sleep_counter = 0;
			}

			if (sleep_counter > p.framerate * p.sleep_timer) {
				printf("sleep mode\n");
				nanosleep(&sleep_mode_timer, NULL);
				continue;
			}
		}

		// process: execute cava
		// TODO: openmp parallelize?
		pthread_mutex_lock(&audio.lock);
		cava_execute(audio.cava_in, audio.samples_counter, cava_out, plan);
		if (audio.samples_counter > 0)
			audio.samples_counter = 0;
		pthread_mutex_unlock(&audio.lock);

		// NOTE: this is where things are printed
		// TODO: instead keep in a queue?
		printf("-- cava_out -- [%d]\n", number_of_bars);
		for (int n = 0; n < number_of_bars; n++) {
			printf("%.2f ", cava_out[n]);
			if (n < number_of_bars - 1)
				printf("| ");
		}
		printf("\n");

		// checking if audio thread has exited unexpectedly
		if (audio.terminate == 1) {
			fprintf(stderr, "Audio thread exited unexpectedly. %s\n", audio.error_message);
			exit(EXIT_FAILURE);
		}

		// TODO: custom framerate control...

		// NOTE: larger values will result in lower load and also provides larger
		// impact (for fluid sim)
		nanosleep(&framerate_timer, NULL);
	}

	// Cleanup
	cava_destroy(plan);
	free(plan);
	free(cava_out);

	audio.terminate = 1;
	pthread_join(p_thread, NULL);

	free(audio.source);
	free(audio.cava_in);

	return EXIT_SUCCESS;
}
