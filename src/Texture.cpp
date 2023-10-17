#include "Texture.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

Texture::Texture() : texture_(nullptr), width_(0), height_(0)
{
}

Texture::~Texture()
{
	FreeTexture();
}

void Texture::FreeTexture()
{
	if (texture_ != nullptr)
	{
		SDL_DestroyTexture(texture_);
		texture_ = nullptr;
		width_ = 0;
		height_ = 0;
	}
}

bool Texture::LoadFromPath(SDL_Renderer* renderer, const char* path)
{
	FreeTexture();

	SDL_Texture* tmp_texture = nullptr;
	SDL_Surface* loaded_surface = IMG_Load(path);

	if (loaded_surface == nullptr)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
		return false;
	}

	SDL_SetColorKey(loaded_surface, SDL_TRUE, SDL_MapRGB(loaded_surface->format, 0xFF, 0x00, 0xFF));

	tmp_texture = SDL_CreateTextureFromSurface(renderer, loaded_surface);

	if (tmp_texture == nullptr)
	{
		printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
	}
	else
	{
		width_ = loaded_surface->w;
		height_ = loaded_surface->h;
	}

	SDL_FreeSurface(loaded_surface);
	texture_ = tmp_texture;

	return texture_ != nullptr;
}

bool Texture::LoadFromText(SDL_Renderer* renderer, TTF_Font* font, const char* text, const SDL_Color& text_color, int text_length)
{
	FreeTexture();

	SDL_Surface* text_surface = text_length == -1 ? TTF_RenderText_Blended(font, text, text_color) : TTF_RenderText_Blended_Wrapped(font, text, text_color, text_length);

	if (text_surface == nullptr)
	{
		printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
		return false;
	}

	texture_ = SDL_CreateTextureFromSurface(renderer, text_surface);

	if (texture_ == nullptr)
	{
		printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
		SDL_FreeSurface(text_surface);
		return false;
	}

	width_ = text_surface->w;
	height_ = text_surface->h;
	SDL_FreeSurface(text_surface);
	return true;
}

void Texture::Render(SDL_Renderer* renderer, int x, int y, float scale, SDL_Rect* clip)
{
	SDL_Rect render_rect = { x, y, static_cast<int>(width_ * scale), static_cast<int>(height_ * scale) };

	if (clip != nullptr)
	{
		render_rect.w = clip->w * scale;
		render_rect.h = clip->h * scale;
	}

	SDL_RenderCopy(renderer, texture_, clip, &render_rect);
}