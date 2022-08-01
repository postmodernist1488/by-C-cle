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
#include <sys/ioctl.h>

#define sectomicro(sec) (int) ((sec) * 1000000)
#define ETX 3
#define NUM_LINES_TO_RETURN "12"
#define SLOWNESS_MIN 0.25
#define SLOWNESS_MAX 1.00
#define NANOSECS_TO_SLOWDOWN 500000000
#define MAX_LINES_IN_TEXT 100
#define MAX_CHARS_IN_LINE 100
#define MAX_TEXTS 100

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

/* move cursor LINES down */
void move_down(int lines) {
    printf("\033[%dB\r", lines);
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


//get random int between from and to
int randint(int from, int to) {
    return from + rand() % (to - from + 1);
}

//get next word in a line, return true if last word, write it to whatever next_word points to
bool get_next_word(const char *line, char *next_word) {
    static int line_pos;
    while(isspace(line[line_pos])) line_pos++;
    if (line[line_pos] == '\0') {
        line_pos = 0;
        return false;
    }
    while(!isspace(line[line_pos]) && line[line_pos] != '\0') {
        *next_word++ = line[line_pos++];
    }
    *next_word = '\0';
    return true;
    
}

//get last word in str starting from pos going backwards where str[pos] is in word
char *get_last_word(char *str, int pos) {
    while (pos >= 0 && !isspace(str[pos])) pos--;
    
    return &str[pos + 1];
}

//TODO: dynamic allocation for lines in a text
typedef struct {
    char *lines[MAX_LINES_IN_TEXT];
    int lines_number;
} Text;


//TODO: dynamic allocation for texts
Text *texts[MAX_TEXTS];
//push text on the text stack and return the number of texts
int push_text(Text *text) {
    static size_t text_sp;
    if (text_sp < MAX_TEXTS)
        texts[text_sp++] = text; 
    else
        fprintf(stderr, "Warning: texts limit reached");
    return text_sp;
}

char *slurp_file(const char *filename);

//create text structs from str and return the number of texts added
int texts_from_string(char *str) {

    int texts_number = 0;

    size_t i = 0;
    while (str[i]) {
        Text *text = malloc(sizeof(Text));
        if (text == NULL) {
            fprintf(stderr, "Error: unable to allocate memory %d\n", texts_number);
            exit(1);
        }
        text->lines_number = 0;
        while (!(str[i] == '\n' && str[i + 1] == '\n') && str[i]) {
            while (isspace(str[i])) i++;
            if (str[i] == '\0') break;
            char *line = malloc(MAX_CHARS_IN_LINE * sizeof(char));
            if (line == NULL) {
                fprintf(stderr, "Error: unable to allocate memory %d\n", texts_number);
                exit(1);
            }
            int j = 0;

            //TODO: smarter cutting of long lines
            while(str[i] != '\n' && str[i] && j < MAX_CHARS_IN_LINE - 1) {
                line[j++] = str[i++];
            }
            line[j] = '\0';
            if (text->lines_number < MAX_LINES_IN_TEXT)
                text->lines[text->lines_number++] = line;
            else
                fprintf(stderr, "Error: too many lines in text %d\n", texts_number);
        }
        while (isspace(str[i])) i++;
        texts_number = push_text(text);
    }

    return texts_number;
}


char *get_next_line(int texts_number) {
    assert(texts_number > 0);
    static int lines_read;
    static Text *text;
    if (text == NULL || lines_read >= text->lines_number) {
        //get new text
        lines_read = 0;
        int n = randint(0, texts_number - 1); 
        text = texts[n];
    }
    return text->lines[lines_read++];
}

void trim_r_from_pos(char *str, int pos) {
    while (pos > 0 && isspace(str[pos])) pos--;
    str[pos + 1] = '\0';
}

int main(void) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    //max input is the size of the terminal
    int max_input = w.ws_col;

    //for asynchronous input
    set_conio_terminal_mode();
    //set RNG seed to time
    srand(time(NULL));
    //return cursor when exiting the program
    atexit(return_cursor);
    //default file for texts
    //TODO: get file from command line arguments
    char *file = "texts.txt";
    bool quit = false;
    //variable to hold time
    struct timespec start, current;
    //user input string - conveniently zero initiatialized
    char input_str[max_input];
    memset(input_str, 0, max_input);

    //pointer to last free character spot in the above string
    int input_str_p = 0;

    //instead of speed we have slowness modifier for sleep
    double slowness = 1;

    //read the file
    char *str;
    if ((str = slurp_file(file)) == NULL) {
        return -1;
    }
    int texts_number = texts_from_string(str);
    free(str);
    if (texts_number == -1) {
        fprintf(stderr, "Unable to open the texts file");
        return 1;
    }


    char *goal_string = get_next_line(texts_number);
    clock_gettime(CLOCK_MONOTONIC, &start);

    char next_word[max_input];
    memset(next_word, 0, max_input);
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
            //TODO: don't overwrite blanks and display background behind the cyclist
            printf("%s\n\r", bycicle1);
            printf("%s\n\r", bycicle2);
            printf("%s\n\r", bycicle3);
            //debug line
            //printf("[%-*s]\n\r%s\r\n----- Speed: %.1lf km/h --- next_word: %s --last word typed: %s------\n\r", max_input - 2, goal_string, input_str, slowness_to_speed(slowness), next_word, last_word);
            printf("[%.70s]\n\r%s\r\n----- Speed: %.1lf km/h ---------------------------------------------\n\r", goal_string, input_str, slowness_to_speed(slowness));
            move_up(12);

            usleep(sectomicro(slowness * 0.05));
        }
        char c = getch();
        switch (c) {
            case -1:
                fprintf(stderr, "read failed: %s\n", strerror(errno));
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
                if (isprint(c) && input_str_p < max_input) {
                input_str[input_str_p++] = c;
                }
                trim_r_from_pos(input_str, input_str_p);

                last_word = get_last_word(input_str, input_str_p);
                if (strcmp(next_word, last_word) == 0) {
                    if (!get_next_word(goal_string, next_word)) {
                        //no more words in the current line
                        goal_string = get_next_line(texts_number);
                        memset((void *)input_str, 0, max_input);
                        input_str_p = 0;
                        //clear the stdout
                        move_down(9);
                        for (int i = 0; i < max_input; i++) putchar(' ');
                        putchar('\n');
                        putchar('\r');
                        for (int i = 0; i < max_input; i++) putchar(' ');
                        move_up(10);
                        get_next_word(goal_string, next_word);
                    }
                    dec_slowness(&slowness);
                }
                break;
        }
    }
    return 0;
}

/*stolen from Tsoding and modified to NULL-terminate, though it's easy to understand this code
 * it just mallocs the size of a file + 1 and puts the data into the allocated buffer
 * + a null character
 */
char *slurp_file(const char *file_path)
{
    char *buffer = NULL;

    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        goto error;
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        goto error;
    }

    long m = ftell(f);
    if (m < 0) {
        goto error;
    }

    buffer = malloc(sizeof(char) * m + 1);
    if (buffer == NULL) {
        goto error;
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        goto error;
    }

    size_t n = fread(buffer, 1, m, f);
    assert(n == (size_t) m);

    if (ferror(f)) {
        goto error;
    }

    buffer[n] = '\0';

    fclose(f);

    return buffer;

error:
    if (f) {
        fclose(f);
    }

    if (buffer) {
        free(buffer);
    }

    return NULL;
}

