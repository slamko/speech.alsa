all: alsa.c
	gcc $< -lasound -lpthread -lm -o alsa
