/* By-C-cle - a terminal typing game about riding a bike. */

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
#include <time.h>
#include <assert.h>

#define sectomicro(sec) (int) ((sec) * 1000000)
#define ETX 3
#define MAX_INPUT 1000
#define NUM_LINES_TO_RETURN "12"
#define SLOWNESS_MIN 0.25
#define SLOWNESS_MAX 1.00
#define NANOSECS_TO_SLOWDOWN 500000000

//log file for errors and debug information
FILE *logfile;

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

//asynchronous input
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

/* move cursor LINES up */
void move_up(int lines) {
    printf("\033[%dA\r", lines);
}

//used for atexit to return cursor where it is supposed to be after program finishes
void return_cursor(void) {
    printf("\033["NUM_LINES_TO_RETURN"B\r");
}

//print last n characters of a string
void print_last_n(char *str, int n) {
    str += strlen(str) - n;
    printf("%s", str);
}

//print first n characters of a string
void print_first_n(char *str, int n) {
    printf("%.*s\r\n", n, str);
}

/*------------------string assets------------------*/
char *bycicle1 = "  __o ";
char *bycicle2 = " -\\<,";
char *bycicle3 = "O / O ";

#define TREE_LENGTH 27
char *tree1 = "          _-_              ";
char *tree2 = "       /~~   ~~\\           ";
char *tree3 = "    /~~         ~~\\        ";
char *tree4 = "   {               }       ";
char *tree5 = "    \\  _-     -_  /        ";
char *tree6 = "      ~  \\\\ //  ~          ";
char *tree7 = "   _- -   | | _- _         ";
char *tree8 = "     _ -  | |   -_         ";
char *tree9 = "         // \\\\             ";
/*------------------string assets------------------*/

/*print background that is moving after each function execution */
void print_background(void) {

            static int len = TREE_LENGTH, first = TREE_LENGTH, last = 0;

            first = (first + 2) % len;
            last = len - first;

            print_last_n(tree1, last);
            print_first_n(tree1, first);
            print_last_n(tree2, last);
            print_first_n(tree2, first);
            print_last_n(tree3, last);
            print_first_n(tree3, first);
            print_last_n(tree4, last);
            print_first_n(tree4, first);
            print_last_n(tree5, last);
            print_first_n(tree5, first);

            //TODO: explore animation possibilities
            print_last_n(tree6, last);
            print_first_n(tree6, first);
            print_last_n(tree7, last);
            print_first_n(tree7, first);

            print_last_n(tree8, last);
            print_first_n(tree8, first);
            print_last_n(tree9, last);
            print_first_n(tree9, first);

}

//125 km/h - 5 km/h
double slowness_to_speed(double slowness) {
    return  40 / slowness - 35;
}

//TODO: better functions for increasing and decreasing slowness
void inc_slowness(double *slowness) {
    if (*slowness < SLOWNESS_MAX)
        *slowness+=0.01;
}

void dec_slowness(double *slowness) {
    if (*slowness > SLOWNESS_MIN)
        *slowness-=0.05;
}

//sample texts
char *text1 = "This is a text. This is gameplay shit. This.";
char *text2 = "Next text is nothing interesting, so okay...";
char *text3 = "This is some text the user will type.";

//get random int between from and to
int randint(int from, int to) {
    return from + rand() % (to - from + 1);
}

//get next word in a line, return 1 if last word, write it to next_word
int get_next_word(const char *line, char *next_word) {
    static int line_pos;
    while(isspace(line[line_pos])) line_pos++;
    while(!isspace(line[line_pos]) && line[line_pos] != '\0') {
        *next_word++ = line[line_pos++];
    }
    *next_word = '\0';
    if (line[line_pos] == '\0') {
        line_pos = 0;
        return 1;
    }
    return 0;
    
}

//get last word in str starting from pos going backwards where str[pos] is in word
char *get_last_word(char *str, int pos) {
    while (pos >= 0 && !isspace(str[pos])) pos--;
    
    return &str[pos + 1];
}

void get_next_line(char *line) {
    (void) line;
    assert(false && "get_next_line is not implemented");
}

void trim_r_from_pos(char *str, int pos) {
    while (pos > 0 && isspace(str[pos])) pos--;
    str[pos + 1] = '\0';
}

int main(void) {

    //for asynchronous input
    set_conio_terminal_mode();
    //set RNG seed to time
    srand(time(NULL));
    //return cursor when exiting the program
    atexit(return_cursor);

    bool quit = false;

    //a file for possible errors
    logfile = fopen("errors.log", "a");

    //user input string - conveniently zero initiatialized
    char input_str[MAX_INPUT] = {0};
    //pointer to last free character spot in the above string
    int input_str_p = 0;

    //instead of speed we have slowness modifier for sleep
    double slowness = 1;
    //variable to hold time
    struct timespec start, current;

    char *textpointers[] = { text1, text2, text3 };
    int num_texts = sizeof textpointers / sizeof(char *);
    char *goal_string = textpointers[randint(0, num_texts - 1)];
   
    clock_gettime(CLOCK_MONOTONIC, &start);

    char next_word[MAX_INPUT] = {0};
    get_next_word(goal_string, next_word);
    char *last_word = "";
    
    while (!quit) {
        while (!kbhit()) {
           clock_gettime(CLOCK_MONOTONIC, &current);
            if (1000000000 * (current.tv_sec - start.tv_sec)+(current.tv_nsec - start.tv_nsec) > NANOSECS_TO_SLOWDOWN) {
                clock_gettime(CLOCK_MONOTONIC, &start);
                inc_slowness(&slowness);
            }

            print_background();
            move_up(3);
            printf("%s\n\r", bycicle1);
            printf("%s\n\r", bycicle2);
            printf("%s\n\r", bycicle3);
            //debug line
            //printf("[%.70s]\n\r%s\r\n----- Speed: %.1lf km/h --- next_word: %s --last word typed: %s------\n\r", goal_string, input_str, slowness_to_speed(slowness), next_word, last_word);
            printf("[%.70s]\n\r%s\r\n----- Speed: %.1lf km/h ---------------------------------------------\n\r", goal_string, input_str, slowness_to_speed(slowness));
            move_up(12);

            usleep(sectomicro(slowness * 0.05));
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
                //basckspace produces a space to overwrite old characters
                if (input_str_p > 0)
                    input_str[--input_str_p] = ' ';
                break;
            default:
                if (isprint(c) && input_str_p < MAX_INPUT) {
                input_str[input_str_p++] = c;
                }
                trim_r_from_pos(input_str, input_str_p);

                last_word = get_last_word(input_str, input_str_p);
                if (strcmp(next_word, last_word) == 0) {
                    if (get_next_word(goal_string, next_word))
                        get_next_line(goal_string);
                    dec_slowness(&slowness);
                }
                break;
        }
    }
}
