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

Cell *ip = NULL; // address of next "instruction", TODO make sure of data type depending on dictionary

void push(Cell x) { *--sp = x; } // TODO add bounds checking
Cell pop(void) { return *sp++; } // TODO add bounds checking

Cell dictionary[DICTIONARY_SIZE];
Cell *here = 0;
Word *last = NULL; // head of dictionary linked list
Word *current_word = NULL; // latest and current word really are Word pointers though

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

/*

	defcode "-ROT",4,,NROT
	pop %eax
	pop %ebx
	pop %ecx
	push %eax
	push %ecx
	push %ebx
	NEXT

	defcode "2DROP",5,,TWODROP // drop top two elements of stack
	pop %eax
	pop %eax
	NEXT

	defcode "2DUP",4,,TWODUP // duplicate top two elements of stack
	mov (%esp),%eax
	mov 4(%esp),%ebx
	push %ebx
	push %eax
	NEXT

	defcode "2SWAP",5,,TWOSWAP // swap top two pairs of elements of stack
	pop %eax
	pop %ebx
	pop %ecx
	pop %edx
	push %ebx
	push %eax
	push %edx
	push %ecx
	NEXT

	defcode "?DUP",4,,QDUP	// duplicate top of stack if non-zero
	movl (%esp),%eax
	test %eax,%eax
	jz 1f
	push %eax
1:	NEXT

	defcode "1+",2,,INCR
	incl (%esp)		// increment top of stack
	NEXT

	defcode "1-",2,,DECR
	decl (%esp)		// decrement top of stack
	NEXT

	defcode "4+",2,,INCR4
	addl $4,(%esp)		// add 4 to top of stack
	NEXT

	defcode "4-",2,,DECR4
	subl $4,(%esp)		// subtract 4 from top of stack
	NEXT

	defcode "+",1,,ADD
	pop %eax		// get top of stack
	addl %eax,(%esp)	// and add it to next word on stack
	NEXT
*/
void do_plus(void) {
    Cell a = pop();
    sp[0] += a;
    //push(pop() + pop());
}
/*
	defcode "-",1,,SUB
	pop %eax		// get top of stack
	subl %eax,(%esp)	// and subtract it from next word on stack
	NEXT

	defcode "*",1,,MUL
	pop %eax
	pop %ebx
	imull %ebx,%eax
	push %eax		// ignore overflow
	NEXT

// 
// 	In this FORTH, only /MOD is primitive.  Later we will define the / and MOD words in
// 	terms of the primitive /MOD.  The design of the i386 assembly instruction idiv which
// 	leaves both quotient and remainder makes this the obvious choice.
// 

	defcode "/MOD",4,,DIVMOD
	xor %edx,%edx
	pop %ebx
	pop %eax
	idivl %ebx
	push %edx		// push remainder
	push %eax		// push quotient
	NEXT

// 
// 	Lots of comparison operations like =, <, >, etc..

// 	ANS FORTH says that the comparison words should return all (binary) 1's for
// 	TRUE and all 0's for FALSE.  However this is a bit of a strange convention
// 	so this FORTH breaks it and returns the more normal (for C programmers ...)
// 	1 meaning TRUE and 0 meaning FALSE.
// 

	defcode "=",1,,EQU	// top two words are equal?
	pop %eax
	pop %ebx
	cmp %ebx,%eax
	sete %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "<>",2,,NEQU	// top two words are not equal?
	pop %eax
	pop %ebx
	cmp %ebx,%eax
	setne %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "<",1,,LT
	pop %eax
	pop %ebx
	cmp %eax,%ebx
	setl %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode ">",1,,GT
	pop %eax
	pop %ebx
	cmp %eax,%ebx
	setg %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "<=",2,,LE
	pop %eax
	pop %ebx
	cmp %eax,%ebx
	setle %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode ">=",2,,GE
	pop %eax
	pop %ebx
	cmp %eax,%ebx
	setge %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "0=",2,,ZEQU	// top of stack equals 0?
	pop %eax
	test %eax,%eax
	setz %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "0<>",3,,ZNEQU	// top of stack not 0?
	pop %eax
	test %eax,%eax
	setnz %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "0<",2,,ZLT	// comparisons with 0
	pop %eax
	test %eax,%eax
	setl %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "0>",2,,ZGT
	pop %eax
	test %eax,%eax
	setg %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "0<=",3,,ZLE
	pop %eax
	test %eax,%eax
	setle %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "0>=",3,,ZGE
	pop %eax
	test %eax,%eax
	setge %al
	movzbl %al,%eax
	pushl %eax
	NEXT

	defcode "AND",3,,AND	// bitwise AND
	pop %eax
	andl %eax,(%esp)
	NEXT

	defcode "OR",2,,OR	// bitwise OR
	pop %eax
	orl %eax,(%esp)
	NEXT

	defcode "XOR",3,,XOR	// bitwise XOR
	pop %eax
	xorl %eax,(%esp)
	NEXT

	defcode "INVERT",6,,INVERT // this is the FORTH bitwise "NOT" function (cf. NEGATE and NOT)
	notl (%esp)
	NEXT

*/


void docol(void) {
    rp--; // Cell *
    *rp = (Cell)ip; // rp is Cell *.  *rp is Cell.  ip is Cell *.  Cast to Cell.
    ip = (Cell *)current_word->params; // ip is Cell *. current_word is Word *. params is... void *!
}

//                 link  name    code     params (e.g.docol code body)
Word word_drop = { NULL, "DROP", do_drop, NULL };
Word word_swap = { NULL, "SWAP", do_swap, NULL };
Word word_dup  = { NULL, "DUP",  do_dup,  NULL };
Word word_over = { NULL, "OVER", do_over, NULL };
Word word_rot  = { NULL, "ROT",  do_rot,  NULL };
Word word_nrot = { NULL, "-ROT", do_nrot, NULL };

Word word_dot  = { NULL, ".",    do_dot,  NULL };
Word word_plus = { NULL, "+",    do_plus, NULL };
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
}

int main(void)
{
    // interesting that the words can be added in another order
    // we probably shouldn't do that
    // going to arrange it in an order that could happen
    
    // moving this to the top so we can use it to troubleshoot
    // but I think it will also get converted to native Forth once we get to that point
    add_word(&word_dot); 
    // no test for dot

    add_word(&word_drop);
#if DEBUG
    Cell *save = sp;
    interpret("1 2 drop");
    assert(sp[0] == 1);
    interpret("drop");
    assert(save == sp);
#endif

    add_word(&word_swap);
#if DEBUG
    interpret("2 1 swap drop");
    assert(sp[0] == 1);
    interpret("drop");
    assert(save == sp);
#endif

    add_word(&word_dup);
#if DEBUG
    interpret("42 dup");
    assert(sp[0] == 42);
    interpret("drop");
    assert(sp[0] == 42);
    interpret("drop");
    assert(save == sp);
#endif

    add_word(&word_over);
#if DEBUG
    interpret("10 11 over");
    assert(sp[0] == 10);
    interpret("drop");
    assert(sp[0] == 11);
    interpret("drop");
    assert(sp[0] == 10);
    interpret("drop");
    assert(save == sp);
#endif

    add_word(&word_plus);
#if DEBUG
    interpret("11 22 +");
    assert(sp[0] == 33);
    interpret("drop");
    assert(save == sp);

    interpret("5 -8 +");
    assert(sp[0] == -3);
    interpret("drop");
    assert(save == sp);
#endif

    add_word(&word_rot);
#if DEBUG
    interpret("1 2 3 rot");
    assert(sp[0] == 1);
    interpret("drop");
    assert(sp[0] == 3);
    interpret("drop");
    assert(sp[0] == 2);
    interpret("drop");
    assert(save == sp);
#endif

    add_word(&word_nrot);
#if DEBUG
    interpret("1 2 3 -rot");
    assert(sp[0] == 2);
    interpret("drop");
    assert(sp[0] == 1);
    interpret("drop");
    assert(sp[0] == 3);
    interpret("drop");
    assert(save == sp);
#endif

    add_word(&word_exit);
    add_word(&word_double);
    add_word(&word_quadruple);
#if DEBUG
    // test docolon/exit
    interpret("42 double");
    assert(sp[0] == 84);
    interpret("drop");
    assert(save == sp);

    // test nested docolon/exit
    interpret("9 quadruple");
    assert(sp[0] == 36);
    interpret("drop");
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
