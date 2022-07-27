#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <stdbool.h>
#include <sys/time.h>
#include <errno.h>
#include <ctype.h>

#define sectomicro(sec) (int) (sec * 1000000)
#define ETX 3
#define MAX_INPUT 1000
#define NUM_LINES_TO_RETURN "6"

struct termios orig_termios;

void reset_terminal_mode(void) {
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode(void) {
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit(void) {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

int getch(void) {
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof c)) < 0) {
        return r;
    } else {
        return c;
    }
}

int main2(void) {

    struct timeval tval_before, tval_after, tval_result;

    gettimeofday(&tval_before, NULL);
    // Some code you want to time, for example:
    sleep(1);
    gettimeofday(&tval_after, NULL);
    timersub(&tval_after, &tval_before, &tval_result);
    printf("Time elapsed: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);

    return 0;
}


void move_up(int lines) {
    printf("\033[%dA\r", lines);
}

void return_cursor(void) {
    printf("\033["NUM_LINES_TO_RETURN"B\r");
}

char *bycicle1 = "  __o ";
char *bycicle2 = " -\\<,";
char *bycicle3 = "O / O ";
 
int main(void) {
    set_conio_terminal_mode();
    atexit(return_cursor);
    bool quit = false;
    FILE *logfile = fopen("errors.log", "a");

    char input_str[MAX_INPUT] = {0};
    int input_str_p = 0;

    while (!quit) {
        while (!kbhit()) {
            printf("%s\n\r", bycicle1);
            printf("%s\n\r", bycicle2);
            printf("%s\n\r", bycicle3);
            printf("-------------------\n\r%s\r\n-------------------\n\r", input_str);
            move_up(6);
            usleep(sectomicro(0.02));
        }
        char c = getch(); /* consume the character */
        switch (c) {
            case -1:
                fputs(strerror(errno), logfile);
                break;
            case ETX:
                quit = true;
                break;
            case 127:
                //basckspace
                if (input_str_p > 0)
                    input_str[--input_str_p] = ' ';
                break;
            default:
                if (isalpha(c) && input_str_p < MAX_INPUT) {
                input_str[input_str_p++] = c;
                }
                break;
                
        }
    }
}
