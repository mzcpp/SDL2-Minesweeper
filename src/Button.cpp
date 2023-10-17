#include "Game.hpp"
#include "Button.hpp"

#include <iostream>

Button::Button(Game* game, TTF_Font* font, const std::string& text, int x, int y) : 
	game_(game), 
	font_(font), 
	top_left_({ x, y }), 
	button_texture_(std::make_unique<Texture>()), 
	button_text_(text), 
	highlighted_(false), 
	redraw_(false), 
	enabled_(true)
{
	UpdateButtonFlags();
	LoadText();
}

Button::~Button()
{
}

void Button::UpdateButtonFlags()
{
	const bool mouse_overlaps_button = MouseOverlapsButton();

	if (highlighted_ != mouse_overlaps_button)
	{
		highlighted_ = mouse_overlaps_button;
		redraw_ = enabled_;
	}
}

void Button::LoadText()
{
	SDL_Color text_color = { 0x00, 0x00, 0x00, 0x00 };

	if (highlighted_)
	{
		text_color = { 0xFF, 0x00, 0x00, 0xFF };
	}

	if (!enabled_)
	{
		text_color = { 0x00, 0x00, 0x00, 0x19 };
	}

	button_texture_->FreeTexture();
	button_texture_->LoadFromText(game_->renderer_, font_, button_text_.c_str(), text_color, -1);
	redraw_ = false;
}


void Button::SetPosition(int x, int y)
{
	top_left_.x = x;
	top_left_.y = y;
}

void Button::SetText(const std::string& text)
{
	button_text_ = text;
}

Texture* Button::GetTexture()
{
	return button_texture_.get();
}


void Button::HandleEvent(SDL_Event* e)
{
	if (e->type == SDL_MOUSEMOTION)
	{
		UpdateButtonFlags();
	}
}

void Button::Tick()
{
	if (redraw_)
	{
		LoadText();
	}
}

void Button::Render()
{
	button_texture_->Render(game_->renderer_, top_left_.x, top_left_.y);
}

bool Button::MouseOverlapsButton()
{
	SDL_Point mouse_position;
	SDL_GetMouseState(&mouse_position.x, &mouse_position.y);
	
	SDL_Rect button_bounding_box = { top_left_.x, top_left_.y, button_texture_->width_, button_texture_->height_ };

	return SDL_PointInRect(&mouse_position, &button_bounding_box);
}