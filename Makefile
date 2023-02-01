COMPILER=gcc
VERSION=-std=gnu11
OUTPUT=a.out

all: main.c
	$(COMPILER) $(VERSION) main.c -o $(OUTPUT)