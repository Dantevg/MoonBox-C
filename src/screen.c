#include <SDL2/SDL.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "util.h"
#include "screen.h"
#include "font.h"

/* C library definitions */

/* Lua API definitions */

// Returns the window width
int screen_getWidth(lua_State *L){
	lua_pushinteger(L, window.rect.w / window.scale);
	
	return 1;
}

// Returns the window height
int screen_getHeight(lua_State *L){
	lua_pushinteger(L, window.rect.h / window.scale);
	
	return 1;
}

// Returns the rendering scale
int screen_getScale(lua_State *L){
	lua_pushinteger(L, window.scale);
	
	return 1;
}

// Sets the rendering scale
int screen_setScale(lua_State *L){
	window.scale = luaL_checkinteger(L, 1);
	
	return 0;
}

// Sets drawing colour
int screen_colour(lua_State *L){
	int r = luaL_checkinteger(L, 1);
	int g = luaL_optinteger(L, 2, r);
	int b = luaL_optinteger(L, 3, r);
	int a = luaL_optinteger(L, 4, 255);
	
	SDL_SetRenderDrawColor(window.renderer, r, g, b, a);
	
	return 0;
}

// Sets pixel
int screen_pixel(lua_State *L){
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	
	SDL_RenderDrawPoint(window.renderer, x, y);
	
	return 0;
}

// Draws a rectangle
int screen_rect(lua_State *L){
	SDL_Rect rect;
	rect.x = luaL_checkinteger(L, 1);
	rect.y = luaL_checkinteger(L, 2);
	rect.w = luaL_checkinteger(L, 3);
	rect.h = luaL_checkinteger(L, 4);
	int fill = lua_toboolean(L, 5);
	
	if(fill){
		SDL_RenderFillRect(window.renderer, &rect);
	}else{
		SDL_RenderDrawRect(window.renderer, &rect);
	}
	
	return 0;
}

// Clears the screen using the current colour
int screen_clear(lua_State *L){
	SDL_RenderClear(window.renderer);
	
	return 0;
}

int screen_char(lua_State *L){
	const char *str = luaL_checkstring(L, 1);
	SDL_Rect rect;
	rect.x = luaL_checkinteger(L, 2);
	rect.y = luaL_checkinteger(L, 3);
	font_char(&window.font, window.renderer, &rect, str[0]);
	
	return 0;
}

int screen_write(lua_State *L){
	const char *str = luaL_checkstring(L, 1);
	
	/* Get string length */
	lua_len(L, 1);
	int n = lua_tointeger(L, -1);
	lua_pop(L, 1);
	
	/* Set initial position rect */
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	SDL_Rect rect;
	
	/* Draw characters */
	for(int i = 0; i < n; i++){
		rect.x = x;
		rect.y = y;
		x += font_char(&window.font, window.renderer, &rect, str[i]);
	}
	return 0;
}

int screen_loadFont(lua_State *L){
	window.font = font_load(L, window.renderer);
	return 0;
}

// Resizes the canvas
// Intended to be used as callback (ignores first argument, event name)
int screen_resize(lua_State *L){
	window.rect.w = luaL_checkinteger(L, 2);
	window.rect.h = luaL_checkinteger(L, 3);
	
	/* Create a new texture */
	SDL_Texture *newtexture = SDL_CreateTexture(window.renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		window.rect.w, window.rect.h);
	checkSDL(window.texture, "could not initialize texture: %s\n");
	
	/* Set the source and destination rect */
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	SDL_QueryTexture(window.texture, NULL, NULL, &rect.w, &rect.h);
	rect.w = (window.rect.w < rect.w) ? window.rect.w : rect.w;
	rect.h = (window.rect.h < rect.h) ? window.rect.h : rect.h;
	
	/* Copy over texture data */
	SDL_SetRenderTarget(window.renderer, newtexture);
	SDL_RenderCopy(window.renderer, window.texture, &rect, &rect);
	SDL_SetRenderTarget(window.renderer, NULL);
	SDL_DestroyTexture(window.texture);
	window.texture = newtexture;
	
	return 0;
}

// Presents the buffer on screen
// Can block if vsync enabled
// FIXME: results in segfault / realloc invalid next size / malloc assertion failed
// when this function doesn't get called often enough (less than 10 times per second)
// and there is mouse movement (?)
int screen_present(lua_State *L){
	/* Display */
	SDL_SetRenderTarget(window.renderer, NULL);
	SDL_RenderCopy(window.renderer, window.texture, &window.rect, &window.rect);
	SDL_RenderPresent(window.renderer);
	
	/* Reset render target and set scale */
	SDL_SetRenderTarget(window.renderer, window.texture);
	SDL_RenderSetScale(window.renderer, window.scale, window.scale);
	
	return 0;
}

int luaopen_screen(lua_State *L){
	lua_newtable(L);
	luaL_setfuncs(L, screen, 0);
	
	/* Init */
	window.rect.x = 0;
	window.rect.y = 0;
	window.rect.w = 600;
	window.rect.h = 400;
	window.scale = 2;
	
	/* Create window */
	window.window = SDL_CreateWindow("SDL2 Window",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		window.rect.w, window.rect.h,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	checkSDL(window.window, "could not initialize window: %s\n");
	
	/* Create renderer */
	window.renderer = SDL_CreateRenderer(window.window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	checkSDL(window.renderer, "could not initialize renderer: %s\n");
	
	/* Create texture */
	window.texture = SDL_CreateTexture(window.renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		window.rect.w, window.rect.h);
	checkSDL(window.texture, "could not initialize texture: %s\n");
	
	/* Set default colour to white */
	SDL_SetRenderDrawColor(window.renderer, 255, 255, 255, 255);
	
	return 1;
}