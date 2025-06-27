# riversforth_c
writing a forth in C

jonesforth.S and jonesforth.f are for reference since I'm using them for how things should work and what order to implement things.

I'm also planning to refer to other resources such as:
- Threaded Interpretive Languages
- Forth Encyclopedia
- 2014 Forth Standard (electronic version)
- Starting Forth
- Thinking Forth

## Are we Forth yet?

Checklist of words to be implemented.

### Easy FORTH primitives

- [x] DROP
- [x] SWAP
- [x] DUP
- [x] OVER
- [x] ROT
- [x] -ROT
- [x] 2DROP
- [x] 2DUP
- [x] 2SWAP
- [x] ?DUP
- [x] 1+
- [x] 1-
- [x] 4+ (made it 8+ since it's 64-bit on my machine)
- [x] 4- (made it 8- since it's 64-bit on my machine)
- [x] +
- [x] -
- [x] *
- [x] /MOD (also did / (DIV) and % (MOD) as primitives since it's not using an assembly instruction for /MOD like JonesForth)
- [x] /
- [x] % (modulo)
- [x] =
- [x] <>
- [x] <
- [x] >
- [x] <=
- [x] >=
- [x] 0=
- [x] 0<>
- [x] 0<
- [x] 0>
- [x] 0<=
- [x] 0>=
- [x] AND
- [x] OR
- [x] XOR
- [x] INVERT

### More FORTH primitives

- [x] EXIT
- [x] LIT

### FORTH Memory primitives

- [x] !
- [x] @
- [x] +!
- [x] -!
- [x] C!
- [x] C@
- [x] C@C!
- [x] CMOVE

### Built in variables
- [x] STATE
- [x] HERE
- [x] LATEST
- [x] S0
- [x] BASE

### Built in constants

- [x] VERSION
- [x] R0
- [x] DOCOL (written in C currently)
- [x] F_IMMED
- [x] F_HIDDEN

### Return stack words

- [x] >R
- [x] R>
- [x] RSP@
- [x] RSP!
- [x] RDROP

### Parameter (data) stack words

- [x] DSP@
- [x] DSP!

### Input and Output

- [x] KEY
- [x] EMIT
- [x] WORD (get_token C function does this currently)
- [x] NUMBER

### Dictionary, compiling primitives

- [x] FIND
- [x] >CFA
- [x] >DFA (compound word)
- [x] CREATE
- [x] ,
- [x] [
- [x] ]
- [x] IMMEDIATE
- [x] HIDDEN
- [x] HIDE (compound word)
- [x] : (compound word)
- [x] ; (compound word)
- [x] '

### Branching

- [ ] BRANCH
- [ ] 0BRANCH

### Literal strings

- [ ] LITSTRING
- [ ] TELL

### QUIT/INTERPRET

- [ ] QUIT
- [ ] INTERPRET

### Odds and ends

- [ ] CHAR
- [ ] EXECUTE
- [ ] SYSCALL*

### Everything in jonesforth.f (which is in FORTH itself)

- . (DOT) is written in C using printf (for now?)
- tbd
