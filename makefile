CC = gcc
FLAGS = -Wall -Werror

a2: B09A2.c
	$(CC) $(FLAGS) $< -o $@