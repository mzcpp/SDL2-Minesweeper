#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "Texture.hpp"

#include <SDL2/SDL.h>

#include <memory>
#include <string>

class Game;

class Button
{
private:
	Game* game_;
	TTF_Font* font_;
	SDL_Point top_left_;
	std::unique_ptr<Texture> button_texture_;
	std::string button_text_;

	bool highlighted_;
	bool redraw_;
	bool enabled_;

	void LoadText();

public:
	Button(Game* game, TTF_Font* font, const std::string& text, int x = 0, int y = 0);

	~Button();
	
	void UpdateButtonFlags();

	void SetPosition(int x, int y);

	void SetText(const std::string& text);

	Texture* GetTexture();

	void HandleEvent(SDL_Event* e);

	void Tick();

	void Render();
	
	bool MouseOverlapsButton();
};

#endif
