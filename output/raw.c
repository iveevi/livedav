#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int print_raw_out(int bars_count, int fd, int is_binary, int bit_format, int ascii_range,
		char bar_delim, char frame_delim, int const f[]) {
	int16_t buf_16;
	int8_t buf_8;

	for (int i = 0; i < bars_count; i++) {
		int f_ranged = f[i];
		//if (f_ranged > ascii_range)
		//    f_ranged = ascii_range;

		// finding size of number-string in byte
		int bar_height_size = 2; // a number + \0
		if (f_ranged != 0)
			bar_height_size += floor(log10(f_ranged));

		char bar_height[bar_height_size];
		snprintf(bar_height, bar_height_size, "%d", f_ranged);

		write(fd, bar_height, bar_height_size - 1);
		write(fd, &bar_delim, sizeof(bar_delim));
	}
	write(fd, &frame_delim, sizeof(frame_delim));
	return 0;
}
