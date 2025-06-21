#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#define DEBUG 1

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

Cell *ip = NULL; // address of next "instruction"

void push(Cell x) { *--sp = x; } // TODO add bounds checking
Cell pop(void) { return *sp++; } // TODO add bounds checking

Cell dictionary[DICTIONARY_SIZE];
Cell *here = 0;
Word *last = NULL; // head of dictionary linked list
Word *current_word = NULL; // last and current word really are Word pointers though

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

//                      link  name      code          params (e.g.docol code body)
Word word_drop      = { NULL, "DROP",   do_drop,      NULL };
Word word_swap      = { NULL, "SWAP",   do_swap,      NULL };
Word word_dup       = { NULL, "DUP",    do_dup,       NULL };
Word word_over      = { NULL, "OVER",   do_over,      NULL };
Word word_rot       = { NULL, "ROT",    do_rot,       NULL };
Word word_nrot      = { NULL, "-ROT",   do_nrot,      NULL };
Word word_twodrop   = { NULL, "2DROP",  do_twodrop,   NULL };
Word word_twodup    = { NULL, "2DUP" ,  do_twodup,    NULL };
Word word_twoswap   = { NULL, "2SWAP",  do_twoswap,   NULL };
Word word_qdup      = { NULL, "?DUP",   do_qdup,      NULL };
Word word_incr      = { NULL, "1+",     do_incr,      NULL };
Word word_decr      = { NULL, "1-",     do_decr,      NULL };
Word word_incr8     = { NULL, "8+",     do_incr8,     NULL };
Word word_decr8     = { NULL, "8-",     do_decr8,     NULL };
Word word_add       = { NULL, "+",      do_add,       NULL };
Word word_sub       = { NULL, "-",      do_sub,       NULL };
Word word_mul       = { NULL, "*",      do_mul,       NULL };
Word word_div       = { NULL, "/",      do_div,       NULL };
Word word_mod       = { NULL, "%",      do_mod,       NULL };
Word word_divmod    = { NULL, "/MOD",   do_divmod,    NULL };
Word word_equ       = { NULL, "=",      do_equ,       NULL };
Word word_nequ      = { NULL, "<>",     do_nequ,      NULL };
Word word_lt        = { NULL, "<",      do_lt,        NULL };
Word word_gt        = { NULL, ">",      do_gt,        NULL };
Word word_le        = { NULL, "<=",     do_le,        NULL };
Word word_ge        = { NULL, ">=",     do_ge,        NULL };
Word word_zequ      = { NULL, "0=",     do_zequ,      NULL };
Word word_znequ     = { NULL, "0<>",    do_znequ,     NULL };
Word word_zlt       = { NULL, "0<",     do_zlt,       NULL };
Word word_zgt       = { NULL, "0>",     do_zgt,       NULL };
Word word_zle       = { NULL, "0<=",    do_zle,       NULL };
Word word_zge       = { NULL, "0>=",    do_zge,       NULL };
Word word_and       = { NULL, "AND",    do_and,       NULL };
Word word_or        = { NULL, "OR",     do_or,        NULL };
Word word_xor       = { NULL, "XOR",    do_xor,       NULL };
Word word_invert    = { NULL, "INVERT", do_invert,    NULL };
Word word_exit      = { NULL, "EXIT",   do_exit,      NULL };
Word word_lit       = { NULL, "LIT",    do_lit,       NULL };
Word word_store     = { NULL, "!",      do_store,     NULL };
Word word_fetch     = { NULL, "@",      do_fetch,     NULL };
Word word_addstore  = { NULL, "+!",     do_addstore,  NULL };
Word word_substore  = { NULL, "-!",     do_substore,  NULL };
Word word_storebyte = { NULL, "C!",     do_storebyte, NULL };
Word word_fetchbyte = { NULL, "C@",     do_fetchbyte, NULL };
Word word_ccopy     = { NULL, "C@C!",   do_ccopy,     NULL };
Word word_cmove     = { NULL, "CMOVE",  do_cmove,     NULL };

Word word_dot     = { NULL, ".",      do_dot,     NULL };

Word *double_body[] = { &word_dup, &word_add, &word_exit };
Word word_double =    { NULL, "DOUBLE",    docol, double_body };

Word *quadruple_body[] = { &word_double, &word_double, &word_exit };
Word word_quadruple = { NULL, "QUADRUPLE", docol, quadruple_body };

Word *testlit_body[] = { &word_lit, (void *)21, &word_double, &word_exit };
Word word_testlit =   { NULL, "TESTLIT",   docol, testlit_body };

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
}

int main(void)
{
    // DOT at the top so we can use it in unit tests
    // but I think it will also get converted to native Forth once we get to that point
    add_word(&word_dot); 
    // no test for dot

    add_word(&word_drop);
#if DEBUG
    Cell *save = sp;
    interpret("1 2 drop");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_swap);
#if DEBUG
    interpret("2 1 swap drop");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_dup);
#if DEBUG
    interpret("42 dup");
    assert(pop() == 42);
    assert(pop() == 42);
    assert(save == sp);
#endif

    add_word(&word_over);
#if DEBUG
    interpret("10 11 over");
    assert(pop() == 10);
    assert(pop() == 11);
    assert(pop() == 10);
    assert(save == sp);
#endif

    add_word(&word_rot);
#if DEBUG
    interpret("1 2 3 rot");
    assert(pop() == 1);
    assert(pop() == 3);
    assert(pop() == 2);
    assert(save == sp);
#endif

    add_word(&word_nrot);
#if DEBUG
    interpret("1 2 3 -rot");
    assert(pop() == 2);
    assert(pop() == 1);
    assert(pop() == 3);
    assert(save == sp);
#endif

    add_word(&word_twodrop);
#if DEBUG
    interpret("1 2 3 2drop");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_twodup);
#if DEBUG
    interpret("5 6 2dup");
    assert(pop() == 6);
    assert(pop() == 5);
    assert(pop() == 6);
    assert(pop() == 5);
    assert(save == sp);
#endif

    add_word(&word_twoswap);
#if DEBUG
    interpret("5 6 7 8 2swap");
    assert(pop() == 6);
    assert(pop() == 5);
    assert(pop() == 8);
    assert(pop() == 7);
    assert(save == sp);
#endif

    add_word(&word_qdup);
#if DEBUG
    interpret("0 ?DUP");
    assert(pop() == 0);
    assert(save == sp);

    interpret("1 ?DUP");
    assert(pop() == 1);
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_incr);
#if DEBUG
    interpret("5 1+");
    assert(pop() == 6);
    assert(save == sp);
#endif

    add_word(&word_decr);
#if DEBUG
    interpret("5 1-");
    assert(pop() == 4);
    assert(save == sp);
#endif

    add_word(&word_incr8);
#if DEBUG
    interpret("5 8+");
    assert(pop() == 13);
    assert(save == sp);
#endif

    add_word(&word_decr8);
#if DEBUG
    interpret("5 8-");
    assert(pop() == -3);
    assert(save == sp);
#endif

    add_word(&word_add);
#if DEBUG
    interpret("11 22 +");
    assert(pop() == 33);
    assert(save == sp);

    interpret("5 -8 +");
    assert(pop() == -3);
    assert(save == sp);
#endif

    add_word(&word_sub);
#if DEBUG
    interpret("11 22 -");
    assert(pop() == -11);
    assert(save == sp);

    interpret("5 -8 -");
    assert(pop() == 13);
    assert(save == sp);
#endif

    add_word(&word_mul);
#if DEBUG
    interpret("11 22 *");
    assert(pop() == 242);
    assert(save == sp);

    interpret("5 -8 *");
    assert(pop() == -40);
    assert(save == sp);
#endif

    add_word(&word_div);
#if DEBUG
    interpret("20 5 /");
    assert(pop() == 4);
    assert(save == sp);

    interpret("21 5 /");
    assert(pop() == 4);
    assert(save == sp);
#endif

    add_word(&word_mod);
#if DEBUG
    interpret("20 5 %");
    assert(pop() == 0);
    assert(save == sp);

    interpret("21 5 %");
    assert(pop() == 1);
    assert(save == sp);

    // TODO how SHOULD this work if either number is negative?
#endif

    add_word(&word_divmod);
#if DEBUG
    interpret("20 5 /MOD");
    assert(pop() == 4);
    assert(pop() == 0);
    assert(save == sp);

    interpret("21 5 /MOD");
    assert(pop() == 4);
    assert(pop() == 1);
    assert(save == sp);

    // TODO how SHOULD this work if either number is negative?
#endif

    add_word(&word_equ);
#if DEBUG
    interpret("5 5 =");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 6 =");
    assert(pop() == 0);
    assert(save == sp);

    interpret("6 5 =");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_nequ);
#if DEBUG
    interpret("5 5 <>");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 6 <>");
    assert(pop() == 1);
    assert(save == sp);

    interpret("6 5 <>");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_lt);
#if DEBUG
    interpret("5 5 <");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 6 <");
    assert(pop() == 1);
    assert(save == sp);

    interpret("6 5 <");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_gt);
#if DEBUG
    interpret("5 5 >");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 6 >");
    assert(pop() == 0);
    assert(save == sp);

    interpret("6 5 >");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_le);
#if DEBUG
    interpret("5 5 <=");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 6 <=");
    assert(pop() == 1);
    assert(save == sp);

    interpret("6 5 <=");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_ge);
#if DEBUG
    interpret("5 5 >=");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 6 >=");
    assert(pop() == 0);
    assert(save == sp);

    interpret("6 5 >=");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_zequ);
#if DEBUG
    interpret("0 0=");
    assert(pop() == 1);
    assert(save == sp);

    interpret("-5 0=");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 0=");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_znequ);
#if DEBUG
    interpret("0 0<>");
    assert(pop() == 0);
    assert(save == sp);

    interpret("-5 0<>");
    assert(pop() == 1);
    assert(save == sp);
    
    interpret("5 0<>");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_zlt);
#if DEBUG
    interpret("0 0<");
    assert(pop() == 0);
    assert(save == sp);

    interpret("-5 0<");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 0<");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_zgt);
#if DEBUG
    interpret("0 0>");
    assert(pop() == 0);
    assert(save == sp);

    interpret("-5 0>");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 0>");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_zle);
#if DEBUG
    interpret("0 0<=");
    assert(pop() == 1);
    assert(save == sp);

    interpret("-5 0<=");
    assert(pop() == 1);
    assert(save == sp);

    interpret("5 0<=");
    assert(pop() == 0);
    assert(save == sp);
#endif

    add_word(&word_zge);
#if DEBUG
    interpret("0 0>=");
    assert(pop() == 1);
    assert(save == sp);

    interpret("-5 0>=");
    assert(pop() == 0);
    assert(save == sp);

    interpret("5 0>=");
    assert(pop() == 1);
    assert(save == sp);
#endif

    add_word(&word_and);
#if DEBUG
    interpret("4 6 AND");
    assert(pop() == 4);
    assert(save == sp);
#endif

    add_word(&word_or);
#if DEBUG
    interpret("4 6 OR");
    assert(pop() == 6);
    assert(save == sp);
#endif

    add_word(&word_xor);
#if DEBUG
    interpret("4 6 XOR");
    assert(pop() == 2);
    assert(save == sp);
#endif

    add_word(&word_invert);
#if DEBUG
    interpret("99 INVERT");
    assert(pop() == -100);
    assert(save == sp);    
#endif

    add_word(&word_store);
#if DEBUG
    Cell testbuff1[1] = {0};
    push(1000);
    push((Cell)testbuff1);
    interpret("!");
    assert(testbuff1[0] == 1000);
    assert(save == sp);
#endif

    add_word(&word_fetch);
#if DEBUG
    push((Cell)testbuff1);
    interpret("@");
    assert(pop() == 1000);
    assert(save == sp);
#endif

    add_word(&word_addstore);
#if DEBUG
    push(1);
    push((Cell)testbuff1);
    interpret("+!");
    assert(testbuff1[0] == 1001);
    assert(save == sp);
#endif

    add_word(&word_substore);
#if DEBUG
    push(2);
    push((Cell)testbuff1);
    interpret("-!");
    assert(testbuff1[0] == 999);
    assert(save == sp);
#endif

    add_word(&word_storebyte);
#if DEBUG
    char testbuff2[10] = "abcdefghi";
    char testbuff3[10] = "ABCDEFGHI";
    push('x');
    push((Cell)testbuff2);
    interpret("C!");
    assert(testbuff2[0] == 'x');
    assert(testbuff2[1] == 'b');
    assert(save == sp);
#endif

    add_word(&word_fetchbyte);
#if DEBUG
    push((Cell)testbuff2);
    interpret("C@");
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
    interpret("C@C!");
    assert(testbuff2[0] == 'A');
    assert(testbuff2[1] == 'b');
    assert(testbuff3[0] == 'A');
    assert(testbuff3[1] == 'B');
    interpret("C@C!");
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
    interpret("CMOVE");
    assert(strcmp("ABCDEfghi",testbuff2) == 0);
    assert(save == sp);
#endif


    add_word(&word_exit);
    add_word(&word_double);
    add_word(&word_quadruple);
    add_word(&word_testlit);
#if DEBUG
    // test docolon/exit
    interpret("42 double");
    assert(pop() == 84);
    assert(save == sp);

    // test nested docolon/exit
    interpret("9 quadruple");
    assert(pop() == 36);
    assert(save == sp);

    // test LIT
    interpret("testlit");
    assert(pop() == 42);
    assert(save == sp);
#endif

    char line[256];

    while (1) {
        printf("ok\n");
        if (!fgets(line, sizeof(line), stdin)) break;
        interpret(line);
    }

    return 0;
}

// TODO add flags:
	// .set F_IMMED,0x80
	// .set F_HIDDEN,0x20
	// .set F_LENMASK,0x1f	// length mask
