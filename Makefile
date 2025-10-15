CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
LDFLAGS =

# Targets
TARGET = vibe_example
TEST_TARGET = vibe_test

# Source files
VIBE_SRC = vibe.c
EXAMPLE_SRC = examples/example.c
TEST_SRC = tests/test.c

# Object files
VIBE_OBJ = vibe.o
EXAMPLE_OBJ = examples/example.o
TEST_OBJ = tests/test.o

# Headers
HEADERS = vibe.h

# Example VIBE files
EXAMPLES = examples/simple.vibe examples/config.vibe examples/web_server.vibe examples/database.vibe

.PHONY: all clean test test-suite demo run help install

all: $(TARGET)

# Build example program
$(TARGET): $(VIBE_OBJ) $(EXAMPLE_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Build test suite
$(TEST_TARGET): $(VIBE_OBJ) $(TEST_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Object files
vibe.o: $(VIBE_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

examples/example.o: $(EXAMPLE_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

tests/test.o: $(TEST_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

# Run example files
test: $(TARGET)
	@echo "=== Running Example File Tests ==="
	@echo ""
	@echo "Testing simple.vibe..."
	@./$(TARGET) examples/simple.vibe || exit 1
	@echo ""
	@echo "Testing config.vibe..."
	@./$(TARGET) examples/config.vibe || exit 1
	@echo ""
	@echo "Testing web_server.vibe..."
	@./$(TARGET) examples/web_server.vibe || exit 1
	@echo ""
	@echo "Testing database.vibe..."
	@./$(TARGET) examples/database.vibe || exit 1
	@echo ""
	@echo "✓ All example tests passed!"

# Run comprehensive test suite
test-suite: $(TEST_TARGET)
	@echo "=== Running Comprehensive Test Suite ==="
	@echo ""
	@./$(TEST_TARGET)

# Run all tests
test-all: test test-suite
	@echo ""
	@echo "✓ All tests completed successfully!"

run: $(TARGET)
	./$(TARGET) examples/simple.vibe

demo: all
	@echo "=== VIBE Parser Demo ==="
	@echo ""
	@./$(TARGET) examples/simple.vibe

clean:
	rm -f $(VIBE_OBJ) $(EXAMPLE_OBJ) $(TEST_OBJ)
	rm -f examples/*.o
	rm -f $(TARGET) $(TEST_TARGET)
	rm -f *.gcov *.gcda *.gcno
	rm -f libvibe.a

help:
	@echo "VIBE Parser Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          Build the example program (default)"
	@echo "  test         Run example file tests"
	@echo "  test-suite   Run comprehensive test suite"
	@echo "  test-all     Run all tests (examples + suite)"
	@echo "  demo         Run quick demo"
	@echo "  run          Run with simple.vibe"
	@echo "  clean        Remove build artifacts"
	@echo "  install      Install to /usr/local (requires sudo)"
	@echo "  help         Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build"
	@echo "  make test               # Test with example files"
	@echo "  make test-suite         # Run unit tests"
	@echo "  make test-all           # Run all tests"
	@echo "  ./vibe_example examples/config.vibe"
	@echo "  ./vibe_test             # Run test suite directly"

# Install to system (optional)
install: all
	@echo "Installing VIBE parser..."
	install -d $(DESTDIR)/usr/local/lib
	install -d $(DESTDIR)/usr/local/include
	install -m 644 vibe.h $(DESTDIR)/usr/local/include/
	ar rcs libvibe.a vibe.o
	install -m 644 libvibe.a $(DESTDIR)/usr/local/lib/
	@echo "✓ Installation complete!"
	@echo "  Header: /usr/local/include/vibe.h"
	@echo "  Library: /usr/local/lib/libvibe.a"
