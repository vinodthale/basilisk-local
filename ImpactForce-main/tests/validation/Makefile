# Makefile for Sharp VOF Validation Tests

# Compiler
QCC = qcc
QCCFLAGS = -O2 -Wall -D_FORTIFY_SOURCE=0
LIBS = -lm

# Test files
TESTS = test_mass_conservation test_contact_angle

# Default target
all: $(TESTS)

# Compile each test
test_%: test_%.c
	$(QCC) $(QCCFLAGS) $< -o $@ $(LIBS)

# Run all tests
run: $(TESTS)
	@echo "Running validation tests..."
	@for test in $(TESTS); do \
		echo "Running $$test..."; \
		./$$test 2>&1 | tee ../results/$$test.log; \
	done

# Clean
clean:
	rm -f $(TESTS)
	rm -f _test_*.c
	rm -f *.o

# Clean results
clean-results:
	rm -f ../results/test_*.txt
	rm -f ../results/test_*.log
	rm -f ../results/*.json

.PHONY: all run clean clean-results
