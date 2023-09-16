all: alsa.c
	gcc $< -lasound -lpthread -lm -g -o alsa
