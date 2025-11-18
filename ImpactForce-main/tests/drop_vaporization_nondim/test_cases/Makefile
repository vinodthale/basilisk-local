# Makefile for Nondimensional Drop Vaporization Test
#
# Usage:
#   make                 # Compile test
#   make clean           # Clean build files
#   make run RE=100      # Compile and run single case
#   make test            # Run quick test
#   make sweep           # Run full parameter sweep
#   make help            # Show this help

# Compiler
QCC = qcc

# Flags
CFLAGS = -O2 -Wall
LDFLAGS = -lm

# Files
SRC = test_drop_vap_nondim.c
EXE = test_drop_vap_nondim

# Default target
all: $(EXE)

# Compile
$(EXE): $(SRC)
	$(QCC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Clean
clean:
	rm -f $(EXE) $(EXE).out $(EXE).tst
	rm -f *.o *.s _*.c
	rm -rf ../results/*
	@echo "Cleaned build files and results"

# Run single case
run: $(EXE)
	@if [ -z "$(RE)" ]; then \
		echo "Error: Please specify RE value. Example: make run RE=100"; \
		exit 1; \
	fi
	@echo "Running case with Re = $(RE)"
	./$(EXE) $(RE) ../results/Re_$(RE)_We_1.50

# Quick test (Re = 100, low refinement)
test: $(EXE)
	@echo "Running quick test (Re = 100)..."
	./$(EXE) 100 ../results/test_Re_100

# Full parameter sweep
sweep: $(EXE)
	@echo "Running full parameter sweep..."
	cd ../utilities && ./run_parameter_sweep.sh

# Quick parameter sweep
sweep-quick: $(EXE)
	@echo "Running quick parameter sweep..."
	cd ../utilities && ./run_parameter_sweep.sh --quick

# Help
help:
	@echo "Nondimensional Drop Vaporization Test - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make              Compile test case"
	@echo "  make clean        Clean build files and results"
	@echo "  make run RE=100   Compile and run single case"
	@echo "  make test         Run quick test (Re = 100)"
	@echo "  make sweep        Run full parameter sweep"
	@echo "  make sweep-quick  Run quick parameter sweep"
	@echo "  make help         Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make"
	@echo "  make run RE=100"
	@echo "  make sweep-quick"

.PHONY: all clean run test sweep sweep-quick help
