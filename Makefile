CXXFLAGS=-Wall -g
OBJECTS=compiler.o
fcc: $(OBJECTS)
	$(CXX) -o fcc $(OBJECTS) $(LDFLAGS)

all: regc
