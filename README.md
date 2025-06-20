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

Built in primitives are in a linked list using C structs but not using the space inside the user data / dictionary.

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
- [ ] ?DUP
- [ ] 1+
- [ ] 1-
- [ ] 4+
- [ ] 4-
- [x] +
- [ ] -
- [ ] *
- [ ] /MOD
- [ ] =
- [ ] <>
- [ ] <
- [ ] >
- [ ] <=
- [ ] >=
- [ ] 0=
- [ ] 0<>
- [ ] 0<
- [ ] 0>
- [ ] 0<=
- [ ] 0>=
- [ ] AND
- [ ] OR
- [ ] XOR
- [ ] INVERT

### More FORTH primitives

- [x] EXIT
- [ ] LIT

### FORTH Memory primitives

- [ ] !
- [ ] @
- [ ] +!
- [ ] -!
- [ ] C!
- [ ] C@
- [ ] C@C!
- [ ] CMOVE

### Built in variables
- [ ] STATE
- [ ] HERE (not available in FORTH yet)
- [ ] LATEST (accidentally called "last", not available in FORTH yet)
- [ ] S0
- [ ] BASE

### Built in constants

- [ ] VERSION
- [ ] R0
- [ ] DOCOL (written in C currently)
- [ ] F_IMMED
- [ ] F_HIDDEN
- [ ] F_LENMASK
- [ ] SYS_*

### Return stack words

- [ ] >R
- [ ] R>
- [ ] RSP@
- [ ] RSP!
- [ ] RDROP

### Parameter (data) stack words

- [ ] DSP@
- [ ] DSP!

### Input and Output

- [ ] KEY
- [ ] EMIT
- [ ] WORD (get_token C function does this currently)
- [ ] NUMBER

### Dictionary, compiling primitives

- [ ] FIND
- [ ] >CFA
- [ ] >DFA (compound word)
- [ ] CREATE
- [ ] ,
- [ ] [
- [ ] ]
- [ ] : (compound word)
- [ ] ; (compound word)
- [ ] IMMEDIATE
- [ ] HIDDEN
- [ ] HIDE (compound word)
- [ ] `

### Branching

- [ ] BRANCH
- [ ] 0BRANCH

### Literal strings

- [ ] LITSTRING
- [ ] TELL

### QUIT/INTERPRET

- [ ] QUIT
- [ ] INTERPRET (written in C currently)

### Odds and ends

- [ ] CHAR
- [ ] EXECUTE
- [ ] SYSCALL*

### Everything in jonesforth.f (which is in FORTH itself)

- . (DOT) is written in C using printf (for now?)
- tbd
