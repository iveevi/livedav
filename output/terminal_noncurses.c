#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

wchar_t *frame_buffer;
wchar_t *barstring[8];
wchar_t *spacestring;
int buf_length;
char *ttyframe_buffer;
char *ttybarstring[8];
char *ttyspacestring;
int ttybuf_length;

int setecho(int fd, int onoff) {

    struct termios t;

    if (tcgetattr(fd, &t) == -1)
        return -1;

    if (onoff == 0)
        t.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON);
    else
        t.c_lflag |= (ECHO | ECHOE | ECHOK | ECHONL | ICANON);

    if (tcsetattr(fd, TCSANOW, &t) == -1)
        return -1;

    return 0;
}

struct colors {
    short unsigned int rgb[3];
};

struct colors *gradient_colors;

struct colors parse_color(char *color_string) {
    struct colors color;
    sscanf(++color_string, "%02hx%02hx%02hx", &color.rgb[0], &color.rgb[1], &color.rgb[2]);
    return color;
}

// general: cleanup
void free_terminal_noncurses(void) {
    free(frame_buffer);
    free(ttyframe_buffer);
    free(spacestring);
    free(ttyspacestring);
    for (int i = 0; i < 8; i++) {
        free(barstring[i]);
        free(ttybarstring[i]);
    }
    free(gradient_colors);
}

int init_terminal_noncurses(int tty, char *const fg_color_string, char *const bg_color_string,
                            int col, int bgcol, int gradient, int gradient_count,
                            char **gradient_color_strings, int width, int lines, int bar_width) {

    free_terminal_noncurses();

    if (tty) {

        ttybuf_length = sizeof(char) * width * lines * 10;
        ttyframe_buffer = (char *)malloc(ttybuf_length);
        ttyspacestring = (char *)malloc(sizeof(char) * (bar_width + 1));

        // clearing barstrings
        for (int n = 0; n < 8; n++) {
            ttybarstring[n] = (char *)malloc(sizeof(char) * (bar_width + 1));
            ttybarstring[n][0] = '\0';
        }
        ttyspacestring[0] = '\0';
        ttyframe_buffer[0] = '\0';

        // creating barstrings for drawing
        for (int n = 0; n < bar_width; n++) {
            strcat(ttybarstring[0], "H");
            strcat(ttybarstring[1], "A");
            strcat(ttybarstring[2], "B");
            strcat(ttybarstring[3], "C");
            strcat(ttybarstring[4], "D");
            strcat(ttybarstring[5], "E");
            strcat(ttybarstring[6], "F");
            strcat(ttybarstring[7], "G");
            strcat(ttyspacestring, " ");
        }
    } else if (!tty) {

        buf_length = sizeof(wchar_t) * width * lines * 10;
        frame_buffer = (wchar_t *)malloc(buf_length);
        spacestring = (wchar_t *)malloc(sizeof(wchar_t) * (bar_width + 1));

        // clearing barstrings
        for (int n = 0; n < 8; n++) {
            barstring[n] = (wchar_t *)malloc(sizeof(wchar_t) * (bar_width + 1));
            barstring[n][0] = '\0';
        }
        spacestring[0] = '\0';
        frame_buffer[0] = '\0';

        // creating barstrings for drawing
        for (int n = 0; n < bar_width; n++) {
            wcscat(barstring[0], L"\u2588");
            wcscat(barstring[1], L"\u2581");
            wcscat(barstring[2], L"\u2582");
            wcscat(barstring[3], L"\u2583");
            wcscat(barstring[4], L"\u2584");
            wcscat(barstring[5], L"\u2585");
            wcscat(barstring[6], L"\u2586");
            wcscat(barstring[7], L"\u2587");
            wcscat(spacestring, L" ");
        }
    }

    col += 30;

    system("setterm -cursor off");
    system("setterm -blank 0");

    // output: reset console
    printf("\033[0m\n");
    system("clear");

    if (col == 38) {
        struct colors fg_color = parse_color(fg_color_string);
        printf("\033[38;2;%d;%d;%dm", fg_color.rgb[0], fg_color.rgb[1], fg_color.rgb[2]);
    } else {
        printf("\033[%dm", col); // setting color
    }

    if (gradient) {
        struct colors gradient_color_defs[gradient_count];
        for (int i = 0; i < gradient_count; i++) {
            gradient_color_defs[i] = parse_color(gradient_color_strings[i]);
        }

        gradient_colors = (struct colors *)malloc((lines * 2) * sizeof(struct colors));

        int individual_size = lines / (gradient_count - 1);

        float rest = lines / (float)(gradient_count - 1) - individual_size;

        float rest_total = 0;
        int gradient_lines = 0;

        for (int i = 0; i < gradient_count - 1; i++) {
            individual_size = lines / (gradient_count - 1);
            if (rest_total > 1.0) {
                individual_size++;
                rest_total = rest_total - 1.0;
            }
            for (int n = 0; n < individual_size; n++) {
                for (int c = 0; c < 3; c++) {
                    float next_color =
                        gradient_color_defs[i + 1].rgb[c] - gradient_color_defs[i].rgb[c];
                    next_color *= n / (float)individual_size;
                    gradient_colors[gradient_lines].rgb[c] =
                        gradient_color_defs[i].rgb[c] + next_color;
                }
                gradient_lines++;
            }
            rest_total = rest_total + rest;
        }
        gradient_colors[lines - 1] = gradient_color_defs[gradient_count - 1];
    }

    if (bgcol != 0) {

        bgcol += 40;

        if (bgcol == 48) {
            struct colors bg_color = parse_color(bg_color_string);
            printf("\033[48;2;%d;%d;%dm", bg_color.rgb[0], bg_color.rgb[1], bg_color.rgb[2]);
        } else {
            printf("\033[%dm", bgcol);
        }

        for (int n = lines; n >= 0; n--) {
            for (int i = 0; i < width; i++) {
                printf(" "); // setting backround color
            }
            if (n != 0)
                printf("\n");
            else
                printf("\r");
        }
        printf("\033[%dA", lines); // moving cursor back up
    }

    setecho(STDIN_FILENO, 0);

    return 0;
}

void get_terminal_dim_noncurses(int *width, int *lines) {

    struct winsize dim;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &dim);

    *lines = (int)dim.ws_row;
    *width = (int)dim.ws_col;

    system("clear"); // clearing in case of resieze
}

int draw_terminal_noncurses(int tty, int lines, int width, int number_of_bars, int bar_width,
                            int bar_spacing, int rest, int bars[], int previous_frame[],
                            int gradient, int x_axis_info) {

    int current_cell, prev_cell, same_line, new_line, cx;

    struct winsize dim;

    same_line = 0;
    new_line = 0;
    cx = 0;

    if (!tty) {
        // output: check if terminal has been resized
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &dim);
        if (x_axis_info)
            lines++;
        if ((int)dim.ws_row != (lines) || (int)dim.ws_col != width)
            return -1;
        if (x_axis_info)
            lines--;
    }

    if (tty)
        ttyframe_buffer[0] = '\0';
    else if (!tty)
        frame_buffer[0] = '\0';

    for (int current_line = lines - 1; current_line >= 0; current_line--) {

        if (gradient) {
            if (tty) {
                cx += snprintf(ttyframe_buffer + cx, ttybuf_length - cx, "\033[38;2;%d;%d;%dm",
                               gradient_colors[current_line].rgb[0],
                               gradient_colors[current_line].rgb[1],
                               gradient_colors[current_line].rgb[2]);
            } else if (!tty) {
                cx += swprintf(frame_buffer + cx, buf_length - cx, L"\033[38;2;%d;%d;%dm",
                               gradient_colors[current_line].rgb[0],
                               gradient_colors[current_line].rgb[1],
                               gradient_colors[current_line].rgb[2]);
            }
        }

        int same_bar = 0;
        int center_adjusted = 0;

        for (int i = 0; i < number_of_bars; i++) {

            current_cell = bars[i] - current_line * 8;
            prev_cell = previous_frame[i] - current_line * 8;

            // same as last frame
            if ((current_cell < 1 && prev_cell < 1) || (current_cell > 7 && prev_cell > 7) ||
                (current_cell == prev_cell)) {
                same_bar++;
            } else {
                if (tty) {
                    if (same_line > 0) {
                        cx += snprintf(ttyframe_buffer + cx, ttybuf_length - cx, "\033[%dB",
                                       same_line); // move down
                        new_line += same_line;
                        same_line = 0;
                    }

                    if (same_bar > 0) {
                        cx += snprintf(ttyframe_buffer + cx, ttybuf_length - cx, "\033[%dC",
                                       (bar_width + bar_spacing) * same_bar); // move forward
                        same_bar = 0;
                    }

                    if (!center_adjusted && rest) {
                        cx += snprintf(ttyframe_buffer + cx, ttybuf_length - cx, "\033[%dC", rest);
                        center_adjusted = 1;
                    }

                    if (current_cell < 1)
                        cx += snprintf(ttyframe_buffer + cx, ttybuf_length - cx, "%s",
                                       ttyspacestring);
                    else if (current_cell > 7)
                        cx += snprintf(ttyframe_buffer + cx, ttybuf_length - cx, "%s",
                                       ttybarstring[0]);
                    else
                        cx += snprintf(ttyframe_buffer + cx, ttybuf_length - cx, "%s",
                                       ttybarstring[current_cell]);

                    if (bar_spacing)
                        cx += snprintf(ttyframe_buffer + cx, ttybuf_length - cx, "\033[%dC",
                                       bar_spacing);
                } else if (!tty) {
                    if (same_line > 0) {
                        cx += swprintf(frame_buffer + cx, buf_length - cx, L"\033[%dB",
                                       same_line); // move down
                        new_line += same_line;
                        same_line = 0;
                    }

                    if (same_bar > 0) {
                        cx += swprintf(frame_buffer + cx, buf_length - cx, L"\033[%dC",
                                       (bar_width + bar_spacing) * same_bar); // move forward
                        same_bar = 0;
                    }

                    if (!center_adjusted && rest) {
                        cx += swprintf(frame_buffer + cx, buf_length - cx, L"\033[%dC", rest);
                        center_adjusted = 1;
                    }

                    if (current_cell < 1)
                        cx += swprintf(frame_buffer + cx, buf_length - cx, spacestring);
                    else if (current_cell > 7)
                        cx += swprintf(frame_buffer + cx, buf_length - cx, barstring[0]);
                    else
                        cx += swprintf(frame_buffer + cx, buf_length - cx, barstring[current_cell]);

                    if (bar_spacing)
                        cx +=
                            swprintf(frame_buffer + cx, buf_length - cx, L"\033[%dC", bar_spacing);
                }
            }
        }

        if (same_bar != number_of_bars) {
            if (current_line != 0) {
                if (tty)
                    cx += snprintf(ttyframe_buffer + cx, ttybuf_length - cx, "\n");
                else if (!tty)
                    cx += swprintf(frame_buffer + cx, buf_length - cx, L"\n");

                new_line++;
            }
        } else {
            same_line++;
        }
    }
    if (same_line != lines) {
        if (tty)
            printf("%s\r\033[%dA", ttyframe_buffer, new_line);
        else if (!tty)
            printf("%ls\r\033[%dA", frame_buffer, new_line);

        fflush(stdout);
    }
    return 0;
}
