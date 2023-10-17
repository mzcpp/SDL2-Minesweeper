#ifndef CELL_HPP
#define CELL_HPP

#include "Texture.hpp"

#include <SDL2/SDL.h>

#include <memory>

class Game;

class Cell
{
public:
	Game* game_;
	SDL_Rect rect_;

	bool render_cell_;
	bool flag_;

	bool mine_;
	bool mine_exploded_;
	bool uncovered_;
	int mines_in_vicinity_;
	Texture* mines_in_vicinity_texture_;

	Cell(Game* game);

	~Cell();

	void Uncover();

	void HandleEvents();
	
	void Tick();

	void Render() const;
};

#endif
