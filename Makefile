CC = gcc
CFLAGS = -Wall -Wextra -g -DDEBUG

SRCDIR = src
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BINDIR)/%.o, $(SOURCES))
EXECUTABLE = sim

ARTIFACTDIR = artifacts
INPUTFILE = "testing.csv"
LOGFILE = "switchlog.csv"

all: $(EXECUTABLE)
	./$(BINDIR)/$(EXECUTABLE) $(ARTIFACTDIR)/$(INPUTFILE) $(ARTIFACTDIR)/$(LOGFILE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $(BINDIR)/$@

$(BINDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BINDIR)/*.o $(BINDIR)/$(EXECUTABLE)
	rm -f *.csv