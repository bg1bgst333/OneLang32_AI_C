# One Compiler - Win32 + C++03
CXX     = g++
CXXFLAGS = -std=c++03 -Wall -Wextra -Isrc
LDFLAGS  =

TARGET  = onec.exe
SRCDIR  = src
SRCS    = $(SRCDIR)/main.cpp \
          $(SRCDIR)/lexer.cpp \
          $(SRCDIR)/parser.cpp \
          $(SRCDIR)/codegen.cpp

OBJS    = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	del /Q src\*.o $(TARGET) 2>NUL || true

.PHONY: all clean
