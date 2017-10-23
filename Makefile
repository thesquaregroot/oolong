all: oolong

clean:
	rm -f oolong src/parser.cpp src/parser.hpp src/tokens.cpp

src/parser.cpp: src/parser.y
	bison -d -o $@ $^
    
src/parser.hpp: src/parser.cpp

src/tokens.cpp: src/tokens.l src/parser.hpp
	flex -o $@ $^

oolong: src/parser.cpp src/node.cpp src/code-generation.cpp src/oolong.cpp src/tokens.cpp
	g++ -o $@ `llvm-config --libs core native --cxxflags --ldflags` src/*.cpp
