all: alsa.c
	gcc $< -lasound -o alsa
