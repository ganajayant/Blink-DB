CPP = g++
CPPFLAGS = -std=c++17 -Wall -Wextra

SRC = src/main.cpp
CLI = src/cli.cpp
EXEC = main
CLI_EXEC = main
DATA_DIR = data
BENCHMARK  = benchmark.sh

.PHONY: all run benchmark build prune docs

run:
	$(CPP) $(CPPFLAGS) $(SRC) -o $(EXEC)
	./$(EXEC)

cli:
	$(CPP) $(CPPFLAGS) $(CLI) -o $(CLI_EXEC)
	./$(CLI_EXEC)

benchmark:
	bash $(BENCHMARK)

docs:
	doxygen

prune:
	rm -Rf $(EXEC) $(DATA_DIR) $(CLI_EXEC)