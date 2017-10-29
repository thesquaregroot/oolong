all: oolong

clean:
	rm -f oolong a.out *.ll *.o vgcore.* src/parser.cpp src/parser.hpp src/tokens.cpp

src/parser.cpp: src/parser.y
	bison -d -o $@ $^
    
src/parser.hpp: src/parser.cpp

src/tokens.cpp: src/tokens.l src/parser.hpp
	flex -o $@ $^

oolong: Makefile src/parser.cpp src/node.cpp src/code-generation.cpp src/oolong.cpp src/tokens.cpp packages
	g++ -o $@ `llvm-config --libs core native --cxxflags --ldflags` -O0 -g src/*.cpp

packages: src/package/io.c
	gcc -c src/package/*.c

