dSim: main.o
	c++ -Wextra -o dSim main.o -lX11 -lGL -lpthread -lpng -lstdc++fs
		
main.o: main.cpp
	c++ -Wextra -c -o main.o -std=c++17 main.cpp
	
	
