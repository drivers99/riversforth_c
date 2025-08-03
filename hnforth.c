// implements the concatenative language mentioned in https://news.ycombinator.com/item?id=13082825

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// data stack
int stack[1000];
int SP = 0;
void push(int value) {
	stack[SP] = value;
	SP++;
}
int pop(void) {
	if (SP == 0) {
		fprintf(stderr, "ERROR: Stack underflow\n");
		exit(1);
	}
	SP--;
	return(stack[SP]);
}

void add(void) {
	push(pop() + pop());
}
void sub(void) {
	int a = pop();
	int b = pop();
	push(b - a);
}
void mul(void) {
	push(pop() * pop());
}
void divide(void) { // "devide" because "div" conflicts with div from stdlib
	int a = pop();
	int b = pop();
	push(b / a);
}
void clr(void) {
	SP = 0;
}
void print(void) {
	printf("%d ",pop());
}
void dup(void) {
	int a = pop();
	push(a);
	push(a);
}
void drop(void) {
	pop();
}
void swap(void) {
	int a = pop();
	int b = pop();
	push(a);
	push(b);
}
void colon(void);
void semicolon(void);
void imm_do(void);
void imm_while(void);

//int sq_def[] = {6, 2}; // : sq dup * ;
//int test_def[] = {6, 2, -1, 10, 0}; // : test dup * 10 + ;

#define DICT_SIZE 1000
int dict_len = 13;
struct DictEntry {
	char * word;
	int c_flag;
	void (*func)(void);
	int * def;
	int deflen;
	int immediate;
} dict[DICT_SIZE] = {
	{"+",     1, add,       NULL,     0, 0}, // 0
	{"-",     1, sub,       NULL,     0, 0}, // 1
	{"*",     1, mul,       NULL,     0, 0}, // 2
	{"/",     1, divide,    NULL,     0, 0}, // 3
	{"clr",   1, clr,       NULL,     0, 0}, // 4
	{".",     1, print,     NULL,     0, 0}, // 5
	{"dup",   1, dup,       NULL,     0, 0}, // 6
	{"drop",  1, drop,      NULL,     0, 0}, // 7
	{"swap",  1, swap,      NULL,     0, 0}, // 8
//	{"sq",    0, NULL,      sq_def,   2, 0}, // ...
//	{"test",  0, NULL,      test_def, 5, 0}, // ...
	{":",     1, colon,     NULL,     0, 0}, // 9
	{";",     1, semicolon, NULL,     0, 1}, // 10
	{"do",    1, imm_do,    NULL,     0, 1}, // 11
	{"while", 1, imm_while, NULL,     0, 1}, // 12 (set dict_len to 13 above)
};

int user_space[4096];
int HERE = 0; // next available space in user_space
char line[4096] = "";
char token[64] = "";

char *NextToken(void) {
	static int offset = 0;

	while(1) {
		// get another line of data whenever we reach the end
		while(!line[offset]) {
			char *s = fgets(line, sizeof(line), stdin);
			if (s == NULL) { // no more input, return NULL
				return(s);
			}
			offset = 0;
		}
		// skip past while space or end of line
		while (line[offset] && isspace(line[offset])) {
			offset++;
		}
		// copy into token until we find another space or end of line
		int token_offset = 0;
		while (line[offset] && !isspace(line[offset])) {
			token[token_offset++] = line[offset++];
		}
		// null terminate the token
		token[token_offset] = 0;
		// if we got something, return it
		// else we loop back to get more data, if any
		if (token_offset) {
			return(token);
		}
	}
}

int COMPILING = 0;

void colon(void) { // start compiling a new word
	char *token;
	if ((token = NextToken()) == NULL) {
		exit(0); // no more input, exit
	}
	// if token is in the dictionary already
	for (int i = 0; i < dict_len; i++) {
		if (!strcmp(token,dict[i].word)) {
			// then bail
			fprintf(stderr, "ERROR: Can't reuse function names in this version of the language.\n");
			exit(1);
		}
	}
	// create new dictionary entry for it
	char *s = strdup(token); // allocates memory permanently since we don't plan to free it
	if (s == NULL) {
		fprintf(stderr,"ERROR: strdup failed\n");
		exit(1);
	}
	dict[dict_len].word = s;
	dict[dict_len].c_flag = 0; // colon defined, not C function
	dict[dict_len].func = NULL; // n/a since it's not a C function
	dict[dict_len].def = &user_space[HERE]; // starts here
	dict[dict_len].deflen = 0; // will increase it as we compile
	dict[dict_len].immediate = 0; // off by default but "immediate" could flip this
	// user_data will be used for the definition of the function
	// flip on the compile flag
	COMPILING = 1;
}

void semicolon(void) { // end compiling
	// turn off compiling mode
	COMPILING = 0;
	// finalize the compilation
	dict_len++;
}

void imm_do(void) {
	if (COMPILING) {
		// save the offset of where the next word will be compiled to
		push(HERE);
	} else {
		fprintf(stderr,"WARN: ignoring \"do\" when not compiling.\n");
	}
}

void imm_while(void) {
	if (COMPILING) {
		int target = pop();
		int offset = target - HERE - 2; // account for the 2 ints of the while statement
		user_space[HERE++] = -3; // conditional goto
		user_space[HERE++] = offset;
		dict[dict_len].deflen += 2;
	} else {
		fprintf(stderr,"WARN: ignoring \"while\" when not compiling.\n");
	}
}

void InterpWords(int *def, int deflen);
void InterpOneWord(struct DictEntry *dp) {
	if (dp->c_flag) {
		(*dp->func)();
	} else {
		InterpWords(dp->def, dp->deflen);
	}
}
void InterpWords(int *def, int deflen) {
	for (int i = 0; i < deflen; i++) {
		if (def[i] >= 0) {
			InterpOneWord(&dict[def[i]]);
		} else if (def[i] == -1) { // literal
			i++;
			push(def[i]);
		} else if (def[i] == -2) { // goto
			i++;
			int offset = def[i];
			i += offset;
		} else if (def[i] == -3) { // conditional goto
			i++;
			int offset = def[i];
			if(pop()) {
				i += offset;
			}
		}
		// TODO else error
	}
}

int main(void) {
	char *token;
	while(1) {
		if ((token = NextToken()) == NULL) {
			return(0); // no more input, exit
		}
		int found = 0;
		// if token is in the dictionary
		for (int i = 0; i < dict_len; i++) {
			if (!strcmp(token,dict[i].word)) {
				found = 1;
				if (COMPILING && !dict[i].immediate) {
					// do not execute the tokens, just add to the list of dictionary offsets it would have executed
					user_space[HERE++] = i;
					dict[dict_len].deflen++;
				} else {
					// if dict entry is marked as C code
					if (dict[i].c_flag) {
						// call function from that dict entry
						dict[i].func();
					} else {
						InterpWords(dict[i].def, dict[i].deflen);
					}
				}
			}
		}
		// else if token is a number
		if (!found) {
			int len = strlen(token);
			int is_num = 1;
			int is_negative = 0;
			int value = 0;
			for (int j = 0; j < len; j++) {
				if (j == 0 && token[j]=='-' && len > 1) { // number starts with "-" but isn't JUST a "-"
					is_negative = 1;
				} else if (isdigit(token[j])) // only works for decimal for now
				{
					value *= 10;
					value += token[j] - '0';
				} else {
					is_num = 0;
				}
			}
			if (is_num) {
				if (is_negative) {
					value = -value;
				}
				if (COMPILING) {
					user_space[HERE++] = -1; // literal
					user_space[HERE++] = value;
					dict[dict_len].deflen += 2;
				} else {
					//push that number onto the data stack
					push(value);
				}
			}
		}
	}
}
