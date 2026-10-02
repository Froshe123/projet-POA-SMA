CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

BASE = json.o reconfort_io.o
<<<<<<< Updated upstream
ROBOT = types.o carte_robot.o graphe.o dictionnaire.o roue.o choix_casier.o
=======
ROBOT = types.o carte_robot.o graphe.o dictionnaire.o roue.o environnement.o
>>>>>>> Stashed changes

all: demo test_graphe $(ROBOT)

demo: $(BASE) demo.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test_graphe: types.o carte_robot.o graphe.o test_graphe.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(wildcard *.hpp)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f demo test_graphe *.o

tests: types.o carte_robot.o graphe.o dictionnaire.o roue.o choix_casier.o reconfort_io.o json.o tests.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test: tests
	./tests

.PHONY: all clean test
