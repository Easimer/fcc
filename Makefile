CXXFLAGS=-Wall -g
OBJECTS=compiler.o shunting_yard.o
fcc: $(OBJECTS)
	$(CXX) -o fcc $(OBJECTS) $(LDFLAGS)

all: regc
