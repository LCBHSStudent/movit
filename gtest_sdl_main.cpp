#define GTEST_HAS_EXCEPTIONS 0

#include <SDL/SDL.h>
#include <SDL/SDL_error.h>
#include <SDL/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>

#include "gtest/gtest.h"

int main(int argc, char **argv) {
	// Set up an OpenGL context using SDL.
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetVideoMode(32, 32, 0, SDL_OPENGL);
	SDL_WM_SetCaption("OpenGL window for unit test", NULL);

	testing::InitGoogleTest(&argc, argv);
	int err = RUN_ALL_TESTS();
	SDL_Quit();
	exit(err);
}
