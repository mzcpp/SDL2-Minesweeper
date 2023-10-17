#include "Cell.hpp"
#include "Game.hpp"

#include <iostream>

Cell::Cell(Game* game) : 
	game_(game), 
	render_cell_(true), 
	flag_(false), 
	mine_(false), 
	mine_exploded_(false), 
	uncovered_(false), 
	mines_in_vicinity_(0), 
	mines_in_vicinity_texture_(nullptr)
{
}

Cell::~Cell()
{
}

void Cell::Uncover()
{
	render_cell_ = false;
	uncovered_ = true;
}

void Cell::HandleEvents()
{
}
	
void Cell::Tick()
{
}

void Cell::Render() const
{
	SDL_Rect clip;
	clip.y = 0;
	clip.w = 32;
	clip.h = 32;

	if (render_cell_)
	{
		clip.x = 0;
		
		game_->sprites_texture_->Render(game_->renderer_, rect_.x, rect_.y, 1.0, &clip);

		if (flag_)
		{
			if (game_->game_over_ && !mine_)
			{
				SDL_SetRenderDrawColor(game_->renderer_, 0xFE, 0xA0, 0xA0, 0xFF);
				SDL_RenderFillRect(game_->renderer_, &rect_);
			}

			clip.x = 64;
			game_->sprites_texture_->Render(game_->renderer_, rect_.x, rect_.y, 1.0, &clip);

		}
	}
	else if (uncovered_)
	{
		if (mine_)
		{
			if (mine_exploded_)
			{
				SDL_SetRenderDrawColor(game_->renderer_, 0xFF, 0x00, 0x00, 0xFF);
				SDL_RenderFillRect(game_->renderer_, &rect_);
			}

			clip.x = 32;
			game_->sprites_texture_->Render(game_->renderer_, rect_.x, rect_.y, 1.0, &clip);
		}
		else if (mines_in_vicinity_ != 0)
		{
			game_->mine_numbers_textures_[mines_in_vicinity_ - 1]->Render(game_->renderer_, rect_.x + (rect_.w / 2) - game_->mine_numbers_textures_[mines_in_vicinity_ - 1]->width_ / 2, rect_.y + 3);
		}
	}	
}