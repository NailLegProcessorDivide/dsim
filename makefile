CXX=clang++
dSim: main.o
	$(CXX) -Wextra -o dSim main.o -lX11 -lGL -lpthread -lpng -lstdc++fs

main.o: main.cpp
	$(CXX) -Wextra -c -o main.o -std=c++17 main.cpp


