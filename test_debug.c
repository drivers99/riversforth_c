#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple test to verify the interpreter works
int main() {
    printf("Testing Forth interpreter...\n");
    
    // Test basic arithmetic
    printf("Testing: 1 2 + .\n");
    system("echo '1 2 + .' | timeout 2s ./riversforth");
    
    printf("Testing: 42 dup . .\n");
    system("echo '42 dup . .' | timeout 2s ./riversforth");
    
    printf("Testing: -21 5 % .\n");
    system("echo '-21 5 % .' | timeout 2s ./riversforth");
    
    return 0;
} 