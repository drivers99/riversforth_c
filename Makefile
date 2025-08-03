CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = riversforth
SOURCE = riversforth.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running built-in tests..."
	@echo "1 2 + ." | ./$(TARGET) | grep -q "3" && echo "Basic arithmetic test passed" || echo "Basic arithmetic test failed"
	@echo "42 dup . ." | ./$(TARGET) | grep -q "42 42" && echo "DUP test passed" || echo "DUP test failed"

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET) 