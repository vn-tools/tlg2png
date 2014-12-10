SRCDIR := src/
OBJDIR := obj/
BINDIR := bin/
EXECUTABLE := tlg2png

SOURCES := $(wildcard $(SRCDIR)*.cc)
OBJECTS := $(SOURCES:$(SRCDIR)%.cc=$(OBJDIR)%.o)
CXXFLAGS := -Wall -pedantic -std=c++11
LFLAGS := -lpng

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(BINDIR)$(EXECUTABLE) $(LFLAGS) $(LIBS)

$(OBJECTS): $(OBJDIR)%.o : $(SRCDIR)%.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(BINDIR)tlg2png
	rm -f $(OBJDIR)*.o
