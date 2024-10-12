# Copyright Plopeanu Teodora-Anca 312CA 2023-2024

# Compiler Setup
CC=gcc
CFLAGS=-Wall -Wextra -std=c99

# Define Targets
TARGETS=image_editor

build: $(TARGETS)

image_editor: image_editor.c
	$(CC) $(CFLAGS) image_editor.c -o image_editor -lm

clean:
	rm -f $(TARGETS)

pack:
	zip -FSr 312CA_PlopeanuTeodoraAnca_Tema3.zip README Makefile *.c *.h

.PHONY: pack clean