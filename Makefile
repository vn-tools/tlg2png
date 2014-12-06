SRCDIR := src/
OBJDIR := obj/
BINDIR := bin/
EXECUTABLE := tlg2png

SOURCES := $(wildcard $(SRCDIR)*.cc)
OBJECTS := $(SOURCES:$(SRCDIR)%.cc=$(OBJDIR)%.o)
CXXFLAGS := -O3 -Wall -pedantic -std=c++11 $(shell GraphicsMagick++-config --cppflags --cxxflags)
LFLAGS := $(shell GraphicsMagick++-config --libs --ldflags)

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(BINDIR)$(EXECUTABLE) $(LFLAGS) $(LIBS)

$(OBJECTS): $(OBJDIR)%.o : $(SRCDIR)%.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(BINDIR)tlg2png
	rm -f $(OBJDIR)*.o