#!/bin/bash

echo "Testing basic arithmetic..."
echo "1 2 + ." | timeout 3s ./riversforth | grep -q "3" && echo "✓ Addition works" || echo "✗ Addition failed"

echo "Testing DUP..."
echo "42 dup . ." | timeout 3s ./riversforth | grep -q "42 42" && echo "✓ DUP works" || echo "✗ DUP failed"

echo "Testing negative number handling..."
echo "-21 5 % ." | timeout 3s ./riversforth | grep -q "4" && echo "✓ Negative modulo works" || echo "✗ Negative modulo failed"

echo "Testing CHAR..."
echo "CHAR A ." | timeout 3s ./riversforth | grep -q "65" && echo "✓ CHAR works" || echo "✗ CHAR failed"

echo "All tests completed!" 