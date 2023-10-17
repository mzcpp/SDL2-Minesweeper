#ifndef GAME_HPP
#define GAME_HPP

#include "Texture.hpp"
#include "Cell.hpp"
#include "Button.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <memory>
#include <array>
#include <vector>

enum class BoardSize
{
	SMALL, MEDIUM, LARGE
};

class Game
{
private:
	bool initialized_;
	bool running_;
	bool mouse_pressed_down_;
	bool game_started_;
	int mines_left_;
	int seconds_elapsed_;
	int ticks_elapsed_;

	SDL_Window* window_;

public:
	bool game_over_;
	SDL_Renderer* renderer_;
	TTF_Font* font_;

private:
	Mix_Chunk* explosion_sfx_;

	BoardSize board_size_;
	SDL_Rect info_viewport_;
	SDL_Rect board_viewport_;

	std::vector<Cell> board_;

	std::unique_ptr<Button> small_board_button_;
	std::unique_ptr<Button> medium_board_button_;
	std::unique_ptr<Button> large_board_button_;
	std::unique_ptr<Button> reset_board_button_;

public:
	std::unique_ptr<Texture> sprites_texture_;
	std::unique_ptr<Texture> mines_left_texture_;
	std::unique_ptr<Texture> seconds_texture_;

	std::vector<std::unique_ptr<Texture>> mine_numbers_textures_;

	Game();

	~Game();

	bool Initialize();

	void Finalize();

	void Run();

	void HandleEvents();
	
	void Tick();

	void Render();

	void RenderInfo();

	void RenderBoard();

	void ResizeWindow(BoardSize board_size);

	void DebugBoard();

	void ResetRenderCellFlags(std::size_t current_mouse_index);

	void UpdateMinesLeftTexture();

	void UpdateSecondsElapsedTexture();

	void ResetBoard();
	
	void GenerateBoard();

	bool GetMousePositionIndex(std::size_t* index);

	void SetBoardCellFlags();

	void UncoverCells(std::size_t start_index);

	void UncoverAvailableNeighbourCells(std::size_t start_index);

	std::vector<std::size_t> GetNeighboursIndices(std::size_t cell_index);
};

#endif