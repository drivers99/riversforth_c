#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#define VERSION 1
#define DEBUG 1

typedef intptr_t Cell; // 64 bits or 8 bytes
typedef void (*CodeFn)(void);
typedef struct Word Word;
struct Word {
    Word *link;
    Cell flags;
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
//#define RETURN_STACK_SIZE 1024
#define RETURN_STACK_SIZE 100 // debug easier
#define DICTIONARY_SIZE 16384
#define TOKEN_SIZE 64
#define INPUT_BUFFER_SIZE 4096

Cell data_stack[DATA_STACK_SIZE];
Cell *sp = data_stack + DATA_STACK_SIZE; // grows down. C pointer arithmetic adds DATA_STACK_SIZE*sizeof(Cell)

Cell return_stack[RETURN_STACK_SIZE];
Cell *rp = return_stack + RETURN_STACK_SIZE; // I'm thinking return_stack is also made of Cells

Cell *ip = NULL; // address of next "instruction"

void push(Cell x) { *--sp = x; } // TODO add bounds checking
Cell pop(void) { return *sp++; } // TODO add bounds checking

// TODO distinguish dictionary and user data space, this is actually user data space
char dictionary[DICTIONARY_SIZE];
Word *latest = NULL; // head of dictionary linked list
Word *current_word = NULL;

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

void do_drop(void) {
    // drop top of stack
    pop();
}

void do_swap(void) {
    // swap top two elements on stack
    Cell a = pop();
    Cell b = pop();
    push(a);
    push(b);
}

void do_dup(void) {
    // duplicate top of stack
    Cell a = sp[0]; // wow that's cool you can use [0] on a pointer like that
    push(a);
}

void do_over(void) {
    // get the second element of the stack and push it on top
    push(sp[1]);
}

void do_rot(void) {
    // we could pop a, b, c
    // push b, a, c
    // in other words ( c b a -- b a c )
    // or reassign elements directly with one temp
    // or do some crazy xor logic just to avoid using a temp
    Cell a = pop();
    Cell b = pop();
    Cell c = pop();
    push(b);
    push(a);
    push(c);
    // having to remember semicolons and declaring variable types is
    // easy to forget when you're used to writing in Python
}

void do_nrot(void) {
    Cell a = pop();
    Cell b = pop();
    Cell c = pop();
    push(a);
    push(c);
    push(b); // ( c b a - a c b )
}

void do_twodrop(void) {
    // drop top two elements of stack
    pop();
    pop();
}

void do_twodup(void) {
    // duplicate top two elements of stack
    Cell a = sp[0];
    Cell b = sp[1];
    push(b);
    push(a);
}

void do_twoswap(void) {
    // swap top two pairs of elements of stack
    // ( d c b a -- b a d c )
    Cell a = pop();
    Cell b = pop();
    Cell c = pop();
    Cell d = pop();
    push(b);
    push(a);
    push(d);
    push(c);
}

void do_qdup(void) {
    // duplicate top of stack if non-zero
    Cell a = sp[0];
    if (a) {
        push(a);        
    }
}

void do_incr(void) {
    // increment top of stack
    sp[0]++;
}

void do_decr(void) {
    // decrement top of stack
    sp[0]--;
}

void do_incr8(void) {
    // add 8 (size of a Cell / pointer) to top of stack
    sp[0] += 8;
}

void do_decr8(void) {
    // subtract 8 (size of a Cell / pointer) from top of stack
    sp[0] -= 8;
}

void do_add(void) {
    Cell a = pop();
    sp[0] += a;
    //push(pop() + pop());
}

void do_sub(void) {
    Cell a = pop();
    sp[0] -= a;
}

void do_mul(void) {
    Cell a = pop();
    sp[0] *= a; // ignores overflow
}

void do_div(void) {
    Cell a = pop();
    sp[0] /= a;
}

void do_mod(void) {
    Cell a = pop();
    sp[0] %= a;
}

void do_divmod(void) {
    Cell a = pop();
    Cell b = pop();
    push(b % a); // push remainder
    push(b / a); // push quotient
}

// TODO determine if we want to change value of False for Forth standards compliance
void do_equ(void) {
    // top two words are equal?
    Cell a = pop();
    Cell b = pop();
    push (a == b);
}

void do_nequ(void) {
    // top two words are not equal?
    Cell a = pop();
    Cell b = pop();
    push (a != b);
}

void do_lt(void) {
    Cell a = pop();
    Cell b = pop();
    push (b < a);
}

void do_gt(void) {
    Cell a = pop();
    Cell b = pop();
    push (b > a);
}

void do_le(void) {
    Cell a = pop();
    Cell b = pop();
    push (b <= a);
}

void do_ge(void) {
    Cell a = pop();
    Cell b = pop();
    push (b >= a);
}

// comparisons with 0
void do_zequ(void) {
    // top of stack equals 0?
    Cell b = pop();
    push (b == 0);
}

void do_znequ(void) {
    // top of stack not 0?
    Cell b = pop();
    push (b != 0);
}

void do_zlt(void) {
    Cell b = pop();
    push (b < 0);
}

void do_zgt(void) {
    Cell b = pop();
    push (b > 0);
}

void do_zle(void) {
    Cell b = pop();
    push (b <= 0);
}

void do_zge(void) {
    Cell b = pop();
    push (b >= 0);
}

void do_and(void) {
    // bitwise AND
    Cell a = pop();
    sp[0] &= a;
}

void do_or(void) {
    // bitwise OR
    Cell a = pop();
    sp[0] |= a;
}

void do_xor(void) {
    // bitwise XOR
    Cell a = pop();
    sp[0] ^= a;
}

// TODO anything that manipulates the stack directly instead of abstracting over push/pop
// is also going to need to have bounds checking added anyway
// so we may just want to not do that
// We could potentially have fast and slow versions
// You could have words that are verified to use fast versions
void do_invert(void) {
    // this is the FORTH bitwise "NOT" function (cf. NEGATE and NOT)
    sp[0] = ~sp[0];
}

void do_lit (void) {
    // get the next Cell in the params (colon defined code body)
    // as a literal value and push it on the stack
    // then advance ip as if it had never been there
    // so we don't try to execute it
    // TODO wouldn't this do something undefined if you ran it interactively?
    push(*ip++);
}

void do_store(void) {
    // !
    Cell *addr = (Cell *)pop();
    Cell data = pop();
    *addr = data;
}

void do_fetch(void) {
    // @
    Cell *addr = (Cell *)pop();
    Cell data = *addr;
    push(data);
}

void do_addstore(void) {
    // +!
    Cell *addr = (Cell *)pop();
    Cell amount = pop();
    *addr += amount;
}

void do_substore(void) {
    // -!
    Cell *addr = (Cell *)pop();
    Cell amount = pop();
    *addr -= amount;
}

// TODO make sure this truncates >255
void do_storebyte(void) {
    // C!
    uint8_t *addr = (uint8_t *)pop();
    uint8_t data = (uint8_t)pop();
    *addr = data;
}

void do_fetchbyte(void) {
    // C@
    uint8_t *addr = (uint8_t *)pop();
    uint8_t data = *addr;
    push((Cell)data);
}

void do_ccopy(void) {
    // C@C!
    uint8_t *dst = (uint8_t *)pop();
    uint8_t *src = (uint8_t *)pop();
    *dst++ = *src++;
    push((Cell)src);
    push((Cell)dst);
}

void do_cmove(void) {
    Cell length = pop();
    uint8_t *dst = (uint8_t *)pop();
    uint8_t *src = (uint8_t *)pop();
    for (int i = 0; i < length; i++) {
        dst[i] = src[i];
    }
}


void docol(void) {
    rp--; // Cell *
    *rp = (Cell)ip; // rp is Cell *.  *rp is Cell.  ip is Cell *.  Cast to Cell.
    ip = (Cell *)current_word->params; // ip is Cell *. current_word is Word *. params is... void *!
}

//                      link  fl name      code          params (e.g.docol code body)
Word word_drop      = { NULL, 0, "DROP",   do_drop,      NULL };
Word word_swap      = { NULL, 0, "SWAP",   do_swap,      NULL };
Word word_dup       = { NULL, 0, "DUP",    do_dup,       NULL };
Word word_over      = { NULL, 0, "OVER",   do_over,      NULL };
Word word_rot       = { NULL, 0, "ROT",    do_rot,       NULL };
Word word_nrot      = { NULL, 0, "-ROT",   do_nrot,      NULL };
Word word_twodrop   = { NULL, 0, "2DROP",  do_twodrop,   NULL };
Word word_twodup    = { NULL, 0, "2DUP" ,  do_twodup,    NULL };
Word word_twoswap   = { NULL, 0, "2SWAP",  do_twoswap,   NULL };
Word word_qdup      = { NULL, 0, "?DUP",   do_qdup,      NULL };
Word word_incr      = { NULL, 0, "1+",     do_incr,      NULL };
Word word_decr      = { NULL, 0, "1-",     do_decr,      NULL };
Word word_incr8     = { NULL, 0, "8+",     do_incr8,     NULL };
Word word_decr8     = { NULL, 0, "8-",     do_decr8,     NULL };
Word word_add       = { NULL, 0, "+",      do_add,       NULL };
Word word_sub       = { NULL, 0, "-",      do_sub,       NULL };
Word word_mul       = { NULL, 0, "*",      do_mul,       NULL };
Word word_div       = { NULL, 0, "/",      do_div,       NULL };
Word word_mod       = { NULL, 0, "%",      do_mod,       NULL };
Word word_divmod    = { NULL, 0, "/MOD",   do_divmod,    NULL };
Word word_equ       = { NULL, 0, "=",      do_equ,       NULL };
Word word_nequ      = { NULL, 0, "<>",     do_nequ,      NULL };
Word word_lt        = { NULL, 0, "<",      do_lt,        NULL };
Word word_gt        = { NULL, 0, ">",      do_gt,        NULL };
Word word_le        = { NULL, 0, "<=",     do_le,        NULL };
Word word_ge        = { NULL, 0, ">=",     do_ge,        NULL };
Word word_zequ      = { NULL, 0, "0=",     do_zequ,      NULL };
Word word_znequ     = { NULL, 0, "0<>",    do_znequ,     NULL };
Word word_zlt       = { NULL, 0, "0<",     do_zlt,       NULL };
Word word_zgt       = { NULL, 0, "0>",     do_zgt,       NULL };
Word word_zle       = { NULL, 0, "0<=",    do_zle,       NULL };
Word word_zge       = { NULL, 0, "0>=",    do_zge,       NULL };
Word word_and       = { NULL, 0, "AND",    do_and,       NULL };
Word word_or        = { NULL, 0, "OR",     do_or,        NULL };
Word word_xor       = { NULL, 0, "XOR",    do_xor,       NULL };
Word word_invert    = { NULL, 0, "INVERT", do_invert,    NULL };
Word word_exit      = { NULL, 0, "EXIT",   do_exit,      NULL };
Word word_lit       = { NULL, 0, "LIT",    do_lit,       NULL };
Word word_store     = { NULL, 0, "!",      do_store,     NULL };
Word word_fetch     = { NULL, 0, "@",      do_fetch,     NULL };
Word word_addstore  = { NULL, 0, "+!",     do_addstore,  NULL };
Word word_substore  = { NULL, 0, "-!",     do_substore,  NULL };
Word word_storebyte = { NULL, 0, "C!",     do_storebyte, NULL };
Word word_fetchbyte = { NULL, 0, "C@",     do_fetchbyte, NULL };
Word word_ccopy     = { NULL, 0, "C@C!",   do_ccopy,     NULL };
Word word_cmove     = { NULL, 0, "CMOVE",  do_cmove,     NULL };

Word word_dot     = { NULL, 0, ".",      do_dot,     NULL };

Word *double_body[] = { &word_dup, &word_add, &word_exit };
Word word_double =    { NULL, 0, "DOUBLE",    docol, double_body };

Word *quadruple_body[] = { &word_double, &word_double, &word_exit };
Word word_quadruple = { NULL, 0, "QUADRUPLE", docol, quadruple_body };

Word *testlit_body[] = { &word_lit, (void *)21, &word_double, &word_exit };
Word word_testlit =   { NULL, 0, "TESTLIT",   docol, testlit_body };

// built-in variables. var needs to return the address of the variable, not the value!

Cell state = 0;
void do_var_state(void) {
    // Is the interpreter executing code (0) or compiling a word (non-zero)?
    push((Cell)&state);
}

void do_var_latest(void) {
    // Points to the latest (most recently defined) word in the dictionary.
    push((Cell)&latest);
}

void *here = dictionary;
void do_var_here(void) {
    // Points to the next free byte of memory.  When compiling, compiled words go here.
    push((Cell)&here);
}

Cell *s0; // initial value of sp
void do_var_s0(void) {
    // Stores the address of the top of the parameter stack.
    push((Cell)&s0);
}

Cell base = 10;
void do_var_base(void) {
    // The current base for printing and reading numbers.
    push((Cell)&base);
}

Word word_var_state  = { NULL, 0, "STATE",  do_var_state,  NULL };
Word word_var_latest = { NULL, 0, "LATEST", do_var_latest, NULL };
Word word_var_here   = { NULL, 0, "HERE",   do_var_here,   NULL };
Word word_var_s0     = { NULL, 0, "S0",     do_var_s0,     NULL };
Word word_var_base   = { NULL, 0, "BASE",   do_var_base,   NULL };

// built-in constants:

void do_con_version(void) {
    // VERSION, the current version of this FORTH
    push(VERSION);
}

Cell *r0;
void do_con_r0(void) {
    // R0, The address of the top of the return stack.
    push((Cell)r0);
}

void do_con_docol(void) {
    // DOCOL, Pointer to DOCOL.
    push((Cell)docol);
}

#define F_IMMED 1
void do_con_f_immed(void) {
    // F_IMMED, The IMMEDIATE flag's actual value.
    push(F_IMMED);
}

#define F_HIDDEN 2
void do_con_f_hidden(void) {
    // F_HIDDEN, The HIDDEN flag's actual value.
    push(F_HIDDEN);
}

Word word_do_con_version  = { NULL, 0, "VERSION",  do_con_version, NULL };
Word word_do_con_r0       = { NULL, 0, "R0",       do_con_r0,      NULL };
Word word_do_con_docol    = { NULL, 0, "DOCOL",    do_con_docol,   NULL };
Word word_do_con_f_immed  = { NULL, 0, "F_IMMED",  do_con_f_immed, NULL };
Word word_do_con_f_hidden = { NULL, 0, "F_HIDDEN", do_con_f_hidden, NULL };

// return stack related words

void do_tor(void) {
    // >R
    Cell a = pop();
    --rp;
    *rp = a;
}

void do_fromr(void) {
    // R>
    Cell a = rp[0];
    rp++;
    push(a);
}

void do_rspfetch(void) {
    // RSP@
    push((Cell)rp);
}

void do_rspstore(void) {
    // RSP!
    rp = (Cell *)pop();
}

void do_rdrop(void) {
    // RDROP
    rp++;
}

Word word_tor      = { NULL, 0, ">R",    do_tor,      NULL };
Word word_fromr    = { NULL, 0, "R>",    do_fromr,    NULL };
Word word_rspfetch = { NULL, 0, "RSP@",  do_rspfetch, NULL };
Word word_rspstore = { NULL, 0, "RSP!",  do_rspstore, NULL };
Word word_rdrop    = { NULL, 0, "RDROP", do_rdrop,    NULL };

// data stack related words

void do_dspfetch(void) {
    // DSP@
    push((Cell)sp);
}

void do_dspstore(void) {
    // DSP!
    sp = (Cell *)pop();
}

Word word_dspfetch = { NULL, 0, "DSP@",  do_dspfetch, NULL };
Word word_dspstore = { NULL, 0, "DSP!",  do_dspstore, NULL };

// input / output

// we could probably just use getchar or something here instead
// TODO try that out
// moved to a function since it's also used by WORD
char input_buffer[INPUT_BUFFER_SIZE];
char currkey = 0;
char bufftop = 0;
char get_key(void) {
    while (currkey >= bufftop) {
        // refill buffer
        if (!fgets(input_buffer, sizeof(input_buffer), stdin)) {
            // stdin has closed
            exit(0);
        }
        currkey = 0;
        bufftop = strlen(input_buffer);
    }
    return input_buffer[currkey++];
}

void do_key(void) {
    push(get_key());
}

void do_emit(void) {
    putchar(pop());
}

char word_buffer[TOKEN_SIZE]; // TODO add bounds checking
void do_word(void) {
    int length = 0;
    char c = get_key();
    if (c == '\\') {
        // start of comment, skip until after next newline
        while (c != '\n') {
            c = get_key();
        }
    }
    // skip whitespace
    // JonesForth only checks for space ' ' so I'm not sure how it's handling newlines
    // it only seems to deal with it after comments
    // but that's clearly not the case.
    // Perhaps it will become clear later.
    while (isspace(c)) {
        c = get_key();
    }
    while (!isspace(c)) {
        word_buffer[length++] = c;
        c = get_key();
    }
    word_buffer[length] = '\0';
    push((Cell)word_buffer);
    push(length);
}

void do_number(void) {
    // clearly C has functions to do this already
    // we could just use that, at least for certain standard bases
    int length = pop();
    char *s = (char *)pop();
    int unparsed = length;
    int number = 0;
    int negative = 0;

    if (length == 0) {
        // trying to parse a zero-length string is an error, but will return 0.
        push(0);
        push(1); // setting this to 1 to indicate an error. Not sure if this will cause a problem.
        return;
    }
    for (int i = 0; i < length; i++) {
        if (i == 0 && s[i] == '-') {
            if (length == 1) {
                // only "-" as a number is an error
                push(0);
                push(1); // unparsed non-zero indicates error
                return;
            }
            negative = 1; // negative is true
            unparsed--;
            continue;
        }
        int digit = s[i];
        // convert digit, regardless of base (up to base 36 anyway)
        if (isdigit(digit)) { // 0 to 9
            digit -= '0';
        }
        else if (digit >= 'A' && digit <= 'Z') {
            digit -= 'A';
            digit += 10;
        }
        else if (digit >= 'a' && digit <= 'z') {
            digit -= 'a';
            digit += 10;
        }
        // check if it fits within base
        if (digit >= base) {
            push(number);
            push(unparsed);
            return;
        }
        number *= base;
        number += digit;
        unparsed--;
    }

    if (negative) {
        push(-number);
    } else {
        push(number);
    }
    push(unparsed);
}

Word word_key    = { NULL, 0, "KEY",    do_key,    NULL };
Word word_emit   = { NULL, 0, "EMIT",   do_emit,   NULL };
Word word_word   = { NULL, 0, "WORD",   do_word,   NULL };
Word word_number = { NULL, 0, "NUMBER", do_number, NULL };

void do_find(void) {
    int length = pop(); // not used, we use a struct with a pointer to a null terminated name
    char *name = (char *)pop();
    for (Word *w = latest; w != NULL; w = w->link) {
        if (strcasecmp(w->name, name) == 0 && !(w->flags & F_HIDDEN)) {
            push((Cell)w);
            return;
        }
    }
    push((Cell)NULL);
    return;
}

void do_tcfa(void) {
    // >CFA
    Word *w = (Word *)pop();
    push((Cell)w->code); // CodeFn code in Word struct (the function pointer itself)
}

void do_tdfa(void) {
    // >DFA
    Word *w = (Word *)pop();
    push((Cell)w->params); // void *params in Word struct (pointer to a code body outside the struct)
}

void do_create(void) {
    int length = pop();
    char *name = (char *)pop();
	// Link pointer.
    Word *new_word = (Word *)here;
    here += sizeof(Word);
    new_word->link = latest;
    latest = new_word;
    new_word->flags = 0;
    new_word->name = here;
    strncpy((char *)here, name, length);
    here += length + 1;
    new_word->code = docol;
    new_word->params = here; // it is expected compilation will come next
}

void do_comma(void) {
    *((Cell *)here) = (Cell)pop();
    here += sizeof(Cell);
}

void do_lbrac(void) {
    // [
    state = 0; // immediate mode
}

void do_rbrac(void) {
    // ]
    state =1; // compile mode
}

void do_immediate(void) {
    latest->flags ^= F_IMMED; // toggle the immediate bit
}

void do_hidden(void) {
    latest->flags ^= F_HIDDEN; // toggle the hidden bit
}

Word word_find      = { NULL, 0,       "FIND",      do_find,      NULL };
Word word_tcfa      = { NULL, 0,       ">CFA",      do_tcfa,      NULL };
Word word_tdfa      = { NULL, 0,       ">DFA",      do_tdfa,      NULL };
Word word_create    = { NULL, 0,       "CREATE",    do_create,    NULL };
Word word_comma     = { NULL, 0,       "COMMA",     do_comma,     NULL };
Word word_lbrac     = { NULL, F_IMMED, "[",         do_lbrac,     NULL };
Word word_rbrac     = { NULL, 0,       "]",         do_rbrac,     NULL };
Word word_immediate = { NULL, F_IMMED, "IMMEDIATE", do_immediate, NULL };
Word word_hidden    = { NULL, 0,       "HIDDEN",    do_hidden,    NULL };

Word *colon_body[] = {
    &word_word, // Get the name of the new word
    &word_create, // CREATE the dictionary entry / header
    &word_var_latest, &word_fetch, &word_hidden, // Make the word hidden.
    &word_rbrac, // Go into compile mode.
    &word_exit // Return from the function.
};
Word word_colon  = { NULL, 0, ":", docol, colon_body };

Word *hide_body[] = { &word_word, &word_find, &word_hidden, &word_exit };
Word word_hide = { NULL, 0, "HIDE", docol, hide_body };

Word *semicolon_body[] = {
    &word_lit, &word_exit, &word_comma, // Append EXIT (so the word will return).
    &word_var_latest, &word_fetch, &word_hidden, // Toggle hidden flag -- unhide the word.
    &word_lbrac, // Go back to IMMEDIATE mode.
    &word_exit // Return from the function.
};
Word word_semicolon = { NULL, F_IMMED, ";", docol, semicolon_body };

void do_tick(void) {
    // '
    // only works in compiled code. All it does in this version is
    // take the next Cell (i.e. Word *, at least in my version because
    // run() works that way) in the params and put it on the stack,
    // skipping execution of it, following JonesForth example.
    push(*ip++); // same thing as LIT, here and in JonesForth
}

Word word_tick = { NULL, 0, "'", do_tick, NULL };

// Note: built in words don't live in the actual dictionary / user data space
void add_word(Word *w) {
    w->link = latest;
    latest = w;
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
    for (Word *w = latest; w != NULL; w = w->link) {
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
// Converted this function to use WORD and work like it does in JonesForth
// TODO make this a word as well (INTERPRET)

int words_remain(void) {
    // determine if there are any words left in the input_buffer
    // to prevent do_interpret from trying to get more words unnecessarily

    int offset = currkey;
    while (offset < bufftop) {
        if (!isspace(input_buffer[offset])) {
            return(1);
        }
        offset++;
    }
    return(0);
}

void do_interpret(void) {
    while (words_remain()) {
        do_word();
        do_find();
        Word *w = (Word *)pop();
        if (w) {
            if ((w->flags & F_IMMED) || state == 0) {
                // run it now
                if (w->code == docol) {
                    run(w);
                } else {
                    current_word = w;
                    w->code();
                }
            } else {
                push((Cell)w);
                do_comma();
            }
        } else {
            // Not found — try to parse number
            push((Cell)word_buffer);
            push(strlen(word_buffer));
            do_number();
            int unparsed = pop();
            int number = pop();
            if (unparsed == 0) {
                if (state == 0) {
                    push(number);
                } else {
                    push((Cell)&word_lit);
                    do_comma();
                    push(number);
                    do_comma();
                }
            } else {
                printf("Unknown word: %s\n", word_buffer);
            }
        }
    }
}

void interpret(char *s) {
    strncpy(input_buffer, s, INPUT_BUFFER_SIZE);
    currkey = 0;
    bufftop = strlen(input_buffer);
    do_interpret();
}

int main(void)
{
    s0 = sp; // initial value of sp. We must assign in main (or any function) because it's not a constant
    r0 = rp; // initial value of rp. ditto.

    // DOT at the top so we can use it in unit tests
    // but I think it will also get converted to native Forth once we get to that point
    add_word(&word_dot); 
    // no test for dot

    add_word(&word_drop);
#if DEBUG
    Cell *save = sp;
    interpret("1 2 drop ");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_swap);
#if DEBUG
    interpret("2 1 swap drop ");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_dup);
#if DEBUG
    interpret("42 dup ");
    assert(pop() == 42);
    assert(pop() == 42);
    assert(save == sp);
#endif

    add_word(&word_over);
#if DEBUG
    interpret("10 11 over ");
    assert(pop() == 10);
    assert(pop() == 11);
    assert(pop() == 10);
    assert(save == sp);
#endif

    add_word(&word_rot);
#if DEBUG
    interpret("1 2 3 rot ");
    assert(pop() == 1);
    assert(pop() == 3);
    assert(pop() == 2);
    assert(save == sp);
#endif

    add_word(&word_nrot);
#if DEBUG
    interpret("1 2 3 -rot ");
    assert(pop() == 2);
    assert(pop() == 1);
    assert(pop() == 3);
    assert(save == sp);
#endif

    add_word(&word_twodrop);
#if DEBUG
    interpret("1 2 3 2drop ");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_twodup);
#if DEBUG
    interpret("5 6 2dup ");
    assert(pop() == 6);
    assert(pop() == 5);
    assert(pop() == 6);
    assert(pop() == 5);
    assert(save == sp);
#endif

    add_word(&word_twoswap);
#if DEBUG
    interpret("5 6 7 8 2swap ");
    assert(pop() == 6);
    assert(pop() == 5);
    assert(pop() == 8);
    assert(pop() == 7);
    assert(save == sp);
#endif

    add_word(&word_qdup);
#if DEBUG
    interpret("0 ?DUP ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("1 ?DUP ");
    assert(pop() == 1);
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_incr);
#if DEBUG
    interpret("5 1+ ");
    assert(pop() == 6);
    assert(save == sp);
#endif

    add_word(&word_decr);
#if DEBUG
    interpret("5 1- ");
    assert(pop() == 4);
    assert(save == sp);
#endif

    add_word(&word_incr8);
#if DEBUG
    interpret("5 8+ ");
    assert(pop() == 13);
    assert(save == sp);
#endif

    add_word(&word_decr8);
#if DEBUG
    interpret("5 8- ");
    assert(pop() == -3);
    assert(save == sp);
#endif

    add_word(&word_add);
#if DEBUG
    interpret("11 22 + ");
    assert(pop() == 33);
    assert(save == sp);

    interpret("5 -8 + ");
    assert(pop() == -3);
    assert(save == sp);
#endif

    add_word(&word_sub);
#if DEBUG
    interpret("11 22 - ");
    assert(pop() == -11);
    assert(save == sp);

    interpret("5 -8 - ");
    assert(pop() == 13);
    assert(save == sp);
#endif

    add_word(&word_mul);
#if DEBUG
    interpret("11 22 * ");
    assert(pop() == 242);
    assert(save == sp);

    interpret("5 -8 * ");
    assert(pop() == -40);
    assert(save == sp);
#endif

    add_word(&word_div);
#if DEBUG
    interpret("20 5 / ");
    assert(pop() == 4);
    assert(save == sp);

    interpret("21 5 / ");
    assert(pop() == 4);
    assert(save == sp);
#endif

    add_word(&word_mod);
#if DEBUG
    interpret("20 5 % ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("21 5 % ");
    assert(pop() == 1);
    assert(save == sp);

    // TODO how SHOULD this work if either number is negative?
#endif

    add_word(&word_divmod);
#if DEBUG
    interpret("20 5 /MOD ");
    assert(pop() == 4);
    assert(pop() == 0);
    assert(save == sp);

    interpret("21 5 /MOD ");
    assert(pop() == 4);
    assert(pop() == 1);
    assert(save == sp);

    // TODO how SHOULD this work if either number is negative?
#endif

    add_word(&word_equ);
#if DEBUG
    interpret("5 5 = ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 6 = ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("6 5 = ");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_nequ);
#if DEBUG
    interpret("5 5 <> ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 6 <> ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("6 5 <> ");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_lt);
#if DEBUG
    interpret("5 5 < ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 6 < ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("6 5 < ");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_gt);
#if DEBUG
    interpret("5 5 > ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 6 > ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("6 5 > ");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_le);
#if DEBUG
    interpret("5 5 <= ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 6 <= ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("6 5 <= ");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_ge);
#if DEBUG
    interpret("5 5 >= ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 6 >= ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("6 5 >= ");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_zequ);
#if DEBUG
    interpret("0 0= ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("-5 0= ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 0= ");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_znequ);
#if DEBUG
    interpret("0 0<> ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("-5 0<> ");
    assert(pop() == 1);
    assert(save == sp);
    
    interpret("5 0<> ");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_zlt);
#if DEBUG
    interpret("0 0< ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("-5 0< ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 0< ");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_zgt);
#if DEBUG
    interpret("0 0> ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("-5 0> ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 0> ");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_zle);
#if DEBUG
    interpret("0 0<= ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("-5 0<= ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 0<= ");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_zge);
#if DEBUG
    interpret("0 0>= ");
    assert(pop() == 1);
    assert(save == sp);

    interpret("-5 0>= ");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 0>= ");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_and);
#if DEBUG
    interpret("4 6 AND ");
    assert(pop() == 4);
    assert(save == sp);
#endif

    add_word(&word_or);
#if DEBUG
    interpret("4 6 OR ");
    assert(pop() == 6);
    assert(save == sp);
#endif

    add_word(&word_xor);
#if DEBUG
    interpret("4 6 XOR ");
    assert(pop() == 2);
    assert(save == sp);
#endif

    add_word(&word_invert);
#if DEBUG
    interpret("99 INVERT ");
    assert(pop() == -100);
    assert(save == sp);    
#endif

    add_word(&word_store);
#if DEBUG
    Cell testbuff1[1] = {0};
    push(1000);
    push((Cell)testbuff1);
    interpret("! ");
    assert(testbuff1[0] == 1000);
    assert(save == sp);
#endif

    add_word(&word_fetch);
#if DEBUG
    push((Cell)testbuff1);
    interpret("@ ");
    assert(pop() == 1000);
    assert(save == sp);
#endif

    add_word(&word_addstore);
#if DEBUG
    push(1);
    push((Cell)testbuff1);
    interpret("+! ");
    assert(testbuff1[0] == 1001);
    assert(save == sp);
#endif

    add_word(&word_substore);
#if DEBUG
    push(2);
    push((Cell)testbuff1);
    interpret("-! ");
    assert(testbuff1[0] == 999);
    assert(save == sp);
#endif

    add_word(&word_storebyte);
#if DEBUG
    char testbuff2[10] = "abcdefghi";
    char testbuff3[10] = "ABCDEFGHI";
    push('x');
    push((Cell)testbuff2);
    interpret("C! ");
    assert(testbuff2[0] == 'x');
    assert(testbuff2[1] == 'b');
    assert(save == sp);
#endif

    add_word(&word_fetchbyte);
#if DEBUG
    push((Cell)testbuff2);
    interpret("C@ ");
    assert(pop() == 'x');
    assert(save == sp);
#endif

    add_word(&word_ccopy);
#if DEBUG
    push((Cell)testbuff3); // src
    push((Cell)testbuff2); // dst
    assert(testbuff2[0] == 'x');
    assert(testbuff2[1] == 'b');
    assert(testbuff3[0] == 'A');
    assert(testbuff3[1] == 'B');
    interpret("C@C! ");
    assert(testbuff2[0] == 'A');
    assert(testbuff2[1] == 'b');
    assert(testbuff3[0] == 'A');
    assert(testbuff3[1] == 'B');
    interpret("C@C! ");
    assert(testbuff2[0] == 'A');
    assert(testbuff2[1] == 'B');
    assert(testbuff2[2] == 'c');
    assert(testbuff3[0] == 'A');
    assert(testbuff3[1] == 'B');
    assert(testbuff3[2] == 'C');
    pop();
    pop();
    assert(save == sp);
#endif

    add_word(&word_cmove);
#if DEBUG
    push((Cell)testbuff3); // src
    push((Cell)testbuff2); // dst
    push(5); // length
    interpret("CMOVE ");
    assert(strcmp("ABCDEfghi",testbuff2) == 0);
    assert(save == sp);
#endif


    add_word(&word_exit);
    add_word(&word_double);
    add_word(&word_quadruple);
    add_word(&word_testlit);
#if DEBUG
    // test docolon/exit
    interpret("42 double ");
    assert(pop() == 84);
    assert(save == sp);

    // test nested docolon/exit
    interpret("9 quadruple ");
    assert(pop() == 36);
    assert(save == sp);

    // test LIT
    interpret("testlit ");
    assert(pop() == 42);
    assert(save == sp);
#endif

    add_word(&word_var_state);
    add_word(&word_var_latest);
    add_word(&word_var_here);
    add_word(&word_var_s0);
    add_word(&word_var_base);
#if DEBUG
    interpret("state @ ");
    assert(pop() == 0);
    assert(save == sp);
    interpret("latest @ ");
    assert(pop() == (Cell)latest);
    assert(save == sp);
    interpret("here @ ");
    assert(pop() == (Cell)dictionary);
    assert(save == sp);
    interpret("s0 @ ");
    assert(pop() == (Cell)s0);
    assert(save == s0); // note: also checking if save and s0 are the same
    interpret("base @ ");
    assert(pop() == 10);
    assert(save == sp);
#endif

    add_word(&word_do_con_version);
    add_word(&word_do_con_r0);
    add_word(&word_do_con_docol);
    add_word(&word_do_con_f_immed);
    add_word(&word_do_con_f_hidden);
#if DEBUG
    interpret("VERSION R0 DOCOL F_IMMED F_HIDDEN ");
    assert(pop() == 2);
    assert(pop() == 1);
    assert(pop() == (Cell)docol);
    assert(pop() == (Cell)r0);
    assert(pop() == VERSION);
    assert(save == sp);
#endif

    add_word(&word_tor);
    add_word(&word_fromr);
    add_word(&word_rspfetch);
    add_word(&word_rspstore);
    add_word(&word_rdrop);
#if DEBUG
    Cell *save_rp = rp;
    interpret("7 >R R> ");
    assert(pop() == 7);
    assert(save == sp);
    assert(save_rp == rp);
    assert(save_rp == r0);
    interpret("8 >R RDROP ");
    assert(save == sp);
    assert(save_rp == rp);
    assert(save_rp == r0);
    interpret("RSP@ ");
    assert(pop() == (Cell)rp);
    // not sure how to fully test things here right now
    assert(save == sp);
    assert(save_rp == rp);
    assert(save_rp == r0);
#endif

    add_word(&word_dspfetch);
    add_word(&word_dspstore);
#if DEBUG
    interpret("DSP@ 7 OVER 8 OVER 9 OVER DSP! ");
    assert(save == sp); // stack pointer should be restored due to DSP@ and DSP!
#endif

    add_word(&word_key);
    add_word(&word_emit);
    add_word(&word_word); // lol
    // tested interactively

    add_word(&word_number);
    // TODO add automated testing after interpret etc is converted to use WORD/KEY/NUMBER

    // compiling words
    add_word(&word_find);
    add_word(&word_tcfa);
#if DEBUG
    char *test_plus = "+";
    push((Cell)test_plus);
    push(1);
    interpret("find >cfa ");
    assert(*((CodeFn) pop()) == do_add);
    assert(save == sp);
#endif

    add_word(&word_tdfa);
#if DEBUG
    char *test_double = "double";
    push((Cell)test_double);
    push(6);
    interpret("find ");
    interpret(">dfa ");
    void *test_params = (void *)pop();
    assert(test_params == double_body);
    assert(save == sp);
#endif

    add_word(&word_create);
    add_word(&word_comma);
    add_word(&word_lbrac);
    add_word(&word_rbrac);
    add_word(&word_immediate);
    add_word(&word_hidden);
    add_word(&word_hide);
    add_word(&word_colon);
    add_word(&word_semicolon);
    add_word(&word_tick);
    // TODO add automated tests

    char line[256];

    while (1) {
        printf("ok\n");
        if (!fgets(line, sizeof(line), stdin)) break;
        interpret(line);
    }

    return 0;
}
