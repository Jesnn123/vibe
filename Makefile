CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
LDFLAGS =

TARGET = vibe_example
SOURCES = vibe.c examples/example.c
OBJECTS = vibe.o examples/example.o
HEADERS = vibe.h

# Example files
EXAMPLES = examples/simple.vibe examples/config.vibe examples/web_server.vibe examples/database.vibe

.PHONY: all clean test run demo help install

all: $(TARGET)

$(TARGET): vibe.o examples/example.o
	$(CC) $(LDFLAGS) -o $@ $^

vibe.o: vibe.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

examples/example.o: examples/example.c $(HEADERS)
	$(CC) $(CFLAGS) -I. -c -o $@ $<

test: $(TARGET)
	@echo "=== Running VIBE Parser Tests ==="
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
	@echo "✓ All tests passed!"

run: $(TARGET)
	./$(TARGET) examples/simple.vibe

demo: all
	@echo "=== VIBE Parser Demo ==="
	@echo ""
	@./$(TARGET) examples/simple.vibe

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f *.gcov *.gcda *.gcno
	rm -f examples/*.o

help:
	@echo "VIBE Parser Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        Build the parser (default)"
	@echo "  test       Run all example tests"
	@echo "  demo       Run quick demo"
	@echo "  run        Run with simple.vibe"
	@echo "  clean      Remove build artifacts"
	@echo "  install    Install to /usr/local (requires sudo)"
	@echo "  help       Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make"
	@echo "  make test"
	@echo "  ./vibe_example examples/config.vibe"

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
