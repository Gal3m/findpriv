CC = gcc
LDLIBS = -lcap
FILE = findpriv

$(FILE): $(FILE).c
all: $(FILE)
	$(CC) $(LDLIBS) $(FILE).c -o $(FILE) 

clean:
	rm -f  $(FILE)

.PHONY: all clean

