# Rubik's Cube Solver — build file
#
# Targets:
#   make            build the solver CLI
#   make tests      build and run the test suite
#   make benchmark  build the benchmark harness
#   make db         generate the corner pattern database (needed for IDA*)
#   make clean      remove build artifacts

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall

# Source files compiled into every binary.
CORE := Model/RubiksCube.cpp \
        PatternDatabases/CornerPatternDatabase.cpp \
        PatternDatabases/PatternDatabase.cpp \
        PatternDatabases/NibbleArray.cpp \
        PatternDatabases/math.cpp

# The CornerDBMaker is only needed by the solver CLI (for `makedb`).
SOLVER_SRC := main.cpp $(CORE) PatternDatabases/CornerDBMaker.cpp

.PHONY: all tests benchmark db clean

all: solver

solver: $(SOLVER_SRC)
	$(CXX) $(CXXFLAGS) $(SOLVER_SRC) -o $@

tests: tests.cpp $(CORE)
	$(CXX) $(CXXFLAGS) tests.cpp $(CORE) -o $@
	./tests

benchmark: benchmark.cpp $(CORE)
	$(CXX) $(CXXFLAGS) benchmark.cpp $(CORE) -o $@

# Generate the corner pattern database (~40 MB, BFS over corner states).
db: solver
	mkdir -p Databases
	./solver makedb

clean:
	rm -f solver tests benchmark solver.exe tests.exe benchmark.exe
