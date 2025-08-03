#!/bin/bash

echo "Testing Forth interpreter with built-in tests..."

# Compile with DEBUG enabled
gcc -Wall -Wextra -std=c99 -g -DDEBUG -o riversforth_test riversforth.c

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
    echo "Running built-in tests..."
    ./riversforth_test
    echo "✓ Built-in tests completed"
else
    echo "✗ Compilation failed"
    exit 1
fi

# Clean up
rm -f riversforth_test 