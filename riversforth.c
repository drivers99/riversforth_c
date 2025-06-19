#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef intptr_t Cell; // 64 bits or 8 bytes
typedef void (*CodeFn)(void);
typedef struct Word Word;
struct Word {
    Word *link;
    const char *name;
    CodeFn code;
    void *params;
};

// 1024 should actually be way too big.
/* 
  Quote from Chuck Moore found at https://www.hpmuseum.org/forum/thread-22655-post-195090.html

  "This stack which people have so much trouble manipulating should 
  never be more than three or four deep."
*/
// made these powers of 2 for no real reason
//#define DATA_STACK_SIZE 1024
#define DATA_STACK_SIZE 100 // debug easier
#define RETURN_STACK_SIZE 1024
#define DICTIONARY_SIZE 16384
#define TOKEN_SIZE 64

Cell data_stack[DATA_STACK_SIZE];
Cell *sp = data_stack + DATA_STACK_SIZE; // grows down. C pointer arithmetic adds DATA_STACK_SIZE*sizeof(Cell)

Cell return_stack[RETURN_STACK_SIZE];
Cell *rp = return_stack + RETURN_STACK_SIZE; // I'm thinking return_stack is also made of Cells

Cell *ip = NULL; // address of next "instruction", TODO make sure of data type depending on dictionary

void push(Cell x) { *--sp = x; } // TODO add bounds checking
Cell pop(void) { return *sp++; } // TODO add bounds checking

Cell dictionary[DICTIONARY_SIZE];
Cell *here = 0;
Word *last = NULL; // head of dictionary linked list
Word *current_word = NULL; // latest and current word really are Word pointers though

void do_dup(void) {
    Cell x = sp[0]; // wow that's cool you can use [0] on a pointer like that
    push(x);
}

void do_plus(void) {
    push(pop() + pop());
}

void do_dot(void) {
    printf("%ld ", pop());
}

void do_exit(void) {
    ip = (Cell *)rp[0];
    rp++;
    // ip is Cell *
    // rp is Cell * but it points at the value we want to put back in ip... so use rp[0]
    // which is a Cell, but I guess we'd need to cast it into a Cell *
}

void docol(void) {
    rp--; // Cell *
    *rp = (Cell)ip; // rp is Cell *.  *rp is Cell.  ip is Cell *.  Cast to Cell.
    ip = (Cell *)current_word->params; // ip is Cell *. current_word is Word *. params is... void *!
}

//                 link  name    code     params (e.g.docol code body)
Word word_dot  = { NULL, ".",    do_dot,  NULL };
Word word_plus = { NULL, "+",    do_plus, NULL };
Word word_dup  = { NULL, "DUP",  do_dup,  NULL };
Word word_exit = { NULL, "EXIT", do_exit, NULL };

Word *double_body[] = { &word_dup, &word_plus, &word_exit };
Word word_double = { NULL, "DOUBLE", docol, double_body };

Word *quadruple_body[] = { &word_double, &word_double, &word_exit };
Word word_quadruple = { NULL, "QUADRUPLE", docol, quadruple_body };

// interesting... built in words don't live in the actual dictionary
void add_word(Word *w) {
    w->link = last;
    last = w;
}

void run(Word *start) {
    // push NULL (current IP which is initially NULL or NULL itself?)
    // docol only pushes what we set ip to here which is the next word
    // in the params we're running
    rp--;
    *rp = (Cell)ip;
    ip = (Cell *)start->params;
    while (ip != NULL) {
        current_word = (Word *)*ip;
        ip++;
        current_word->code();
    }
}

Word *find(const char *name) {
    for (Word *w = last; w != NULL; w = w->link) {
        if (strcasecmp(w->name, name) == 0) {
            return w;
        }
    }
    return NULL;
}

char *get_token(char **input_ptr) {
    static char token[TOKEN_SIZE];
    char *in = *input_ptr;
    char *out = token;

    // Skip whitespace
    while (*in && isspace(*in)) in++;

    // Copy next word
    while (*in && !isspace(*in)) {
        *out++ = *in++;
    }
    *out = '\0';
    *input_ptr = in; // interesting... updates the pointer itself

    return token;
}

// TODO I think if we finish converting JonesForth then
// interpret will run in Forth when you initially run QUIT
// instead of having this C function (TBD)
void interpret(char *line) {
    char *ptr = line;

    while (1) {
        char *tok = get_token(&ptr);
        if (*tok == '\0') break;

        Word *w = find(tok);
        if (w) {
            if (w->code == docol) {
                run(w);
            } else {
                current_word = w;
                w->code();
            }
        } else {
            // Not found — try to parse number
            char *end;
            long val = strtol(tok, &end, 10);
            if (*end == '\0') {
                push(val);
            } else {
                printf("Unknown word: %s\n", tok);
            }
        }
    }
    printf("\n");
}

int main(void)
{
    // interesting that the words can be added in another order
    // we probably shouldn't do that
    // going to arrange it in an order that could happen
    add_word(&word_dup);
    add_word(&word_plus);
    add_word(&word_dot);
    add_word(&word_exit);
    add_word(&word_double);
    add_word(&word_quadruple);

    char line[256];

    while (1) {
        printf("ok\n");
        if (!fgets(line, sizeof(line), stdin)) break;
        interpret(line);
    }

    // push(3);
    // run(find("quadruple"));
    // run(find("double"));
    // do_dot();

    // char *input = " this is Jeopardy! ";
    // char **in = &input;
    // printf("[%s]\n",get_token(in));
    // printf("[%s]\n",get_token(in));
    // printf("[%s]\n",get_token(in));
    // printf("[%s]\n",get_token(in));
    // printf("[%s]\n",get_token(in));

    // interpret("100 QUADRUPLE .");
    return 0;
}