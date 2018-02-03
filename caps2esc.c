#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <linux/input.h>
#include <time.h>

// clang-format off
const struct input_event
esc_up          = {.type = EV_KEY, .code = KEY_ESC,      .value = 0},
ctrl_up         = {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = 0},
capslock_up     = {.type = EV_KEY, .code = KEY_CAPSLOCK, .value = 0},
esc_down        = {.type = EV_KEY, .code = KEY_ESC,      .value = 1},
ctrl_down       = {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = 1},
capslock_down   = {.type = EV_KEY, .code = KEY_CAPSLOCK, .value = 1},
esc_repeat      = {.type = EV_KEY, .code = KEY_ESC,      .value = 2},
ctrl_repeat     = {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = 2},
capslock_repeat = {.type = EV_KEY, .code = KEY_CAPSLOCK, .value = 2},
syn             = {.type = EV_SYN, .code = SYN_REPORT,   .value = 0};
// clang-format on

int equal(const struct input_event *first, const struct input_event *second) {
    return first->type == second->type && first->code == second->code &&
        first->value == second->value;
}

int read_event(struct input_event *event) {
    return fread(event, sizeof(struct input_event), 1, stdin) == 1;
}

void write_event(const struct input_event *event) {
    if (fwrite(event, sizeof(struct input_event), 1, stdout) != 1)
        exit(EXIT_FAILURE);
}

int main(void) {
    int capslock_is_down = 0, esc_give_up = 0;
    int esc_timeout = 250;

    struct input_event input;
    struct timespec start, end;

    setbuf(stdin, NULL), setbuf(stdout, NULL);

    while (read_event(&input)) {
        if (input.type == EV_MSC && input.code == MSC_SCAN)
            continue;

        if (input.type != EV_KEY) {
            write_event(&input);
            continue;
        }
        if (equal(&input, &capslock_down)) {
            capslock_is_down = 1;
            clock_gettime(CLOCK_MONOTONIC_RAW, &start);
            continue;
        }
        if (capslock_is_down) {
            write_event(&ctrl_down);
            write_event(&syn);
            if (!esc_give_up && input.value) {
                esc_give_up = 1;
            }
            if (equal(&input, &capslock_down) ||
                equal(&input, &capslock_repeat))
                continue;

            if (equal(&input, &capslock_up)) {
                clock_gettime(CLOCK_MONOTONIC_RAW, &end);
                write_event(&ctrl_up);
                capslock_is_down = 0;
                if (esc_give_up) {
                    esc_give_up = 0;
                    continue;
                }
                if (((end.tv_sec - start.tv_sec) / 1.0e6) <= esc_timeout) {
                    write_event(&esc_down);
                    write_event(&syn);
                    write_event(&esc_up);
                    continue;
                }
                usleep(20000);
                /* write_event(&esc_up); */
                write_event(&syn);
                continue;
            }

        }
        if (input.code == KEY_ESC)
            input.code = KEY_CAPSLOCK;
        write_event(&input);
    }
}
