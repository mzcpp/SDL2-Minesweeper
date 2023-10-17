#include "Game.hpp"
#include "Constants.hpp"
#include "Texture.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <string>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <stack>

Game::Game() : 
	initialized_(false), 
	running_(false), 
	mouse_pressed_down_(false), 
	game_started_(false), 
	mines_left_(10), 
	seconds_elapsed_(0), 
	ticks_elapsed_(0), 
	window_(nullptr), 
	game_over_(false), 
	renderer_(nullptr), 
	font_(nullptr),
	explosion_sfx_(nullptr), 
	small_board_button_(nullptr), 
	medium_board_button_(nullptr), 
	large_board_button_(nullptr), 
	reset_board_button_(nullptr), 
	sprites_texture_(std::make_unique<Texture>()), 
	mines_left_texture_(std::make_unique<Texture>()), 
	seconds_texture_(std::make_unique<Texture>())
{
	initialized_ = Initialize();

	if (!initialized_)
	{
		return;
	}

	board_size_ = BoardSize::SMALL;

	constexpr int info_viewport_height = 100;

	info_viewport_.x = 0;
	info_viewport_.y = 0;
	info_viewport_.w = constants::screen_width;
	info_viewport_.h = info_viewport_height;

	board_viewport_.x = 0;
	board_viewport_.y = info_viewport_height;
	board_viewport_.w = constants::screen_width;
	board_viewport_.h = board_viewport_.w;

	GenerateBoard();
	SetBoardCellFlags();

	constexpr int button_padding = 10;

	small_board_button_ = std::make_unique<Button>(this, font_, "Small");
	small_board_button_->SetPosition(button_padding, 0);

	medium_board_button_ = std::make_unique<Button>(this, font_, "Medium");
	medium_board_button_->SetPosition((info_viewport_.w / 2) - (medium_board_button_->GetTexture()->width_ / 2), 0);

	large_board_button_ = std::make_unique<Button>(this, font_, "Large");
	large_board_button_->SetPosition(info_viewport_.w - large_board_button_->GetTexture()->width_ - button_padding, 0);

	reset_board_button_ = std::make_unique<Button>(this, font_, "Reset");
	reset_board_button_->SetPosition((info_viewport_.w / 2) - (reset_board_button_->GetTexture()->width_ / 2), (info_viewport_.h / 1.5) - (reset_board_button_->GetTexture()->height_ / 2));

	sprites_texture_->LoadFromPath(renderer_, "res/gfx/sprites.png");

	UpdateMinesLeftTexture();
	UpdateSecondsElapsedTexture();

	SDL_Color numbers_colors[8] = { { 0x00, 0x00, 0xFF, 0xFF }, { 0x00, 0xFF, 0x00, 0xFF }, { 0xFF, 0x00, 0x00, 0xFF },
									{ 0x00, 0x61, 0x76, 0xFF }, { 0xA1, 0x61, 0x76, 0xFF }, { 0xC4, 0xBA, 0x07, 0xFF },
									{ 0xA7, 0x14, 0x9F, 0xFF }, { 0x00, 0x00, 0x00, 0xFF } };

	for (std::size_t i = 0; i < 8; ++i)
	{
		std::unique_ptr<Texture> texture = std::make_unique<Texture>();
		texture->LoadFromText(renderer_, font_, std::to_string(i + 1).c_str(), numbers_colors[i], -1);

		mine_numbers_textures_.push_back(std::move(texture));
	}
}

Game::~Game()
{
	Finalize();
}

bool Game::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not be initialized! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"))
	{
		printf("%s\n", "Warning: Texture filtering is not enabled!");
	}

	window_ = SDL_CreateWindow(constants::game_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, constants::screen_width, constants::screen_height, SDL_WINDOW_SHOWN);

	if (window_ == nullptr)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

	if (renderer_ == nullptr)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	constexpr int img_flags = IMG_INIT_PNG;

	if (!(IMG_Init(img_flags) & img_flags))
	{
		printf("SDL_image could not be initialized! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}

	if (TTF_Init() == -1)
	{
		printf("SDL_ttf could not be initialized! SDL_ttf Error: %s\n", TTF_GetError());
		return false;
	}

	font_ = TTF_OpenFont("res/font/font.ttf", 28);

	if (font_ == nullptr)
	{
		printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
		return false;
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		printf("SDL_mixer could not be initialized! SDL_mixer Error: %s\n", Mix_GetError());
		return false;
	}

	explosion_sfx_ = Mix_LoadWAV("res/sfx/explosion.wav");

	return true;
}

void Game::Finalize()
{
	SDL_DestroyWindow(window_);
	window_ = nullptr;

	SDL_DestroyRenderer(renderer_);
	renderer_ = nullptr;

	TTF_CloseFont(font_);
	font_ = nullptr;

	Mix_FreeChunk(explosion_sfx_);
	explosion_sfx_ = nullptr;

	IMG_Quit();
	SDL_Quit();
	TTF_Quit();
	Mix_Quit();
}

void Game::Run()
{
	if (!initialized_)
	{
		return;
	}

	running_ = true;

	constexpr long double ms = 1.0 / 60.0;
	std::uint64_t last_time = SDL_GetPerformanceCounter();
	long double delta = 0.0;

	double timer = SDL_GetTicks();

	int frames = 0;
	int ticks = 0;

	while (running_)
	{
		const std::uint64_t now = SDL_GetPerformanceCounter();
		const long double elapsed = static_cast<long double>(now - last_time) / static_cast<long double>(SDL_GetPerformanceFrequency());

		last_time = now;
		delta += elapsed;

		HandleEvents();

		while (delta >= ms)
		{
			Tick();
			delta -= ms;
			++ticks;
		}

		//printf("%Lf\n", delta / ms);
		Render();
		++frames;

		if (SDL_GetTicks() - timer > 1000)
		{
			timer += 1000;
			//printf("Frames: %d, Ticks: %d\n", frames, ticks);
			frames = 0;
			ticks = 0;
		}
	}
}

void Game::HandleEvents()
{
	SDL_Event e;

	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			running_ = false;
			return;
		}

		if (e.type == SDL_MOUSEBUTTONUP)
		{
			if (small_board_button_->MouseOverlapsButton())
			{
				ResizeWindow(BoardSize::SMALL);
			}
			else if (medium_board_button_->MouseOverlapsButton())
			{
				ResizeWindow(BoardSize::MEDIUM);
			}
			else if (large_board_button_->MouseOverlapsButton())
			{
				ResizeWindow(BoardSize::LARGE);
			}
			else if (reset_board_button_->MouseOverlapsButton())
			{
				ResetBoard();
			}
		}
		else if (e.type == SDL_MOUSEMOTION)
		{
			small_board_button_->HandleEvent(&e);
			medium_board_button_->HandleEvent(&e);
			large_board_button_->HandleEvent(&e);
			reset_board_button_->HandleEvent(&e);
		}

		if (game_over_)
		{
			continue;
		}

		if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
		{
			std::size_t mouse_index = 0;

			if (!GetMousePositionIndex(&mouse_index))
			{
				continue;
			}

			if (e.button.button == SDL_BUTTON_LEFT)
			{
				if (e.type == SDL_MOUSEBUTTONDOWN)
				{
					mouse_pressed_down_ = true;
				}
				else if (e.type == SDL_MOUSEBUTTONUP)
				{
					if (!game_started_)
					{
						game_started_ = true;
					}

					mouse_pressed_down_ = false;
					
					if (!board_[mouse_index].uncovered_)
					{
						UncoverCells(mouse_index);
					}
					else
					{
						UncoverAvailableNeighbourCells(mouse_index);
						ResetRenderCellFlags(mouse_index);
					}
				}
			}
			else if (e.button.button == SDL_BUTTON_RIGHT)
			{
				if (e.type == SDL_MOUSEBUTTONDOWN)
				{
					if (!board_[mouse_index].uncovered_)
					{
						board_[mouse_index].render_cell_ = true;
						
						if (board_[mouse_index].flag_)
						{
							++mines_left_;
							board_[mouse_index].flag_ = false;
						}
						else
						{
							--mines_left_;
							board_[mouse_index].flag_ = true;
						}
						
						UpdateMinesLeftTexture();
					}
				}
			}
		}
	}
}
	
void Game::Tick()
{
	small_board_button_->Tick();
	medium_board_button_->Tick();
	large_board_button_->Tick();
	reset_board_button_->Tick();

	if (game_over_)
	{
		return;
	}

	if (game_started_)
	{
		++ticks_elapsed_;

		/* This is bad. */
		if (ticks_elapsed_ % 60 == 0)
		{
			++seconds_elapsed_;
		}

		UpdateSecondsElapsedTexture();
	}

	std::size_t mouse_index = 0;

	if (!GetMousePositionIndex(&mouse_index))
	{
		return;
	}

	if (mouse_pressed_down_)
	{
		ResetRenderCellFlags(mouse_index);
		
		if (!board_[mouse_index].uncovered_)
		{
			if (board_[mouse_index].render_cell_ && !board_[mouse_index].flag_)
			{
				board_[mouse_index].render_cell_ = false;
			}
		}
		else
		{
			std::vector<std::size_t> neighbour_indices = GetNeighboursIndices(mouse_index);

			for (std::size_t neighbour_index : neighbour_indices)
			{
				if (board_[neighbour_index].render_cell_ && !board_[neighbour_index].flag_)
				{
					board_[neighbour_index].render_cell_ = false;
				}
			}
		}
	}
}

void Game::Render()
{
	SDL_SetRenderDrawColor(renderer_, 0xC6, 0xC6, 0xC6, 0xFF);
	SDL_RenderClear(renderer_);

	RenderInfo();
	RenderBoard();

	std::for_each(board_.begin(), board_.end(), [](const Cell& cell)
	{
		cell.Render();
	});

	SDL_RenderPresent(renderer_);
}

void Game::RenderInfo()
{
	SDL_RenderSetViewport(renderer_, &info_viewport_);	
	SDL_SetRenderDrawColor(renderer_, 0x80, 0x80, 0x80, 0xFF);
	SDL_RenderDrawLine(renderer_, 0, info_viewport_.h - 1, board_viewport_.w, info_viewport_.h - 1);

	small_board_button_->Render();
	medium_board_button_->Render();
	large_board_button_->Render();
	reset_board_button_->Render();

	mines_left_texture_->Render(renderer_, (info_viewport_.w / 3) - ((info_viewport_.w / 3) / 2) - (mines_left_texture_->width_ / 2), (info_viewport_.h / 1.5) - (mines_left_texture_->height_ / 2));
	seconds_texture_->Render(renderer_, (info_viewport_.w * 2 / 3) + (info_viewport_.w / 3 / 2) - (seconds_texture_->width_ / 2), (info_viewport_.h / 1.5) - (seconds_texture_->height_ / 2));
}
	
void Game::RenderBoard()
{
	SDL_RenderSetViewport(renderer_, &board_viewport_);
	SDL_SetRenderDrawColor(renderer_, 0x80, 0x80, 0x80, 0xFF);

	int increment = 32;

	for (int i = 0; i < board_viewport_.w / 32; ++i)
	{
		SDL_RenderDrawLine(renderer_, increment, 0, increment, board_viewport_.h);
		SDL_RenderDrawLine(renderer_, 0, increment, board_viewport_.w, increment);

		increment += 32;
	}
}

void Game::DebugBoard()
{
	int width_in_cells = 0;

	switch (board_size_)
	{
	case BoardSize::SMALL:
		width_in_cells = 10;
		break;
	case BoardSize::MEDIUM:
		width_in_cells = 16;
		break;
	case BoardSize::LARGE:
		width_in_cells = 32;
		break;
	}

	for (std::size_t i = 0; i < board_.size(); ++i)
	{
		if (board_[i].mine_)
		{
			std::cout << "X ";
		}
		else
		{
			std::cout << std::to_string(board_[i].mines_in_vicinity_) << " ";
		}

		if ((i + 1) % width_in_cells == 0)
		{
			std::cout << std::endl;
		}
	}
}

void Game::ResizeWindow(BoardSize new_board_size)
{
	if (new_board_size == board_size_)
	{
		return;
	}

	int new_info_viewport_width = 0;
	constexpr int info_viewport_height = 100;

	int new_board_viewport_width = 0;
	int new_board_viewport_height = 0;

	switch (new_board_size)
	{
	case BoardSize::SMALL:
		new_info_viewport_width = 320;
		new_board_viewport_width = 320;
		new_board_viewport_height = 320;
		mines_left_ = 10;
		break;
	case BoardSize::MEDIUM:
		new_info_viewport_width = 512;
		new_board_viewport_width = 512;
		new_board_viewport_height = 512;
		mines_left_ = 40;
		break;
	case BoardSize::LARGE:
		new_info_viewport_width = 1024;
		new_board_viewport_width = 1024;
		new_board_viewport_height = 512;
		mines_left_ = 99;
		break;
	}

	info_viewport_.w = new_info_viewport_width;
	info_viewport_.h = info_viewport_height;

	board_viewport_.w = new_board_viewport_width;
	board_viewport_.h = new_board_viewport_height;

	SDL_SetWindowSize(window_, new_board_viewport_width, info_viewport_height + new_board_viewport_height);
	SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	board_size_ = new_board_size;

	ResetBoard();

	constexpr int button_padding = 10;

	small_board_button_->SetPosition(button_padding, 0);
	medium_board_button_->SetPosition((info_viewport_.w / 2) - (large_board_button_->GetTexture()->width_ / 2), 0);
	large_board_button_->SetPosition(info_viewport_.w - medium_board_button_->GetTexture()->width_ - button_padding, 0);
	reset_board_button_->SetPosition((info_viewport_.w / 2) - (reset_board_button_->GetTexture()->width_ / 2), (info_viewport_.h / 1.5) - (reset_board_button_->GetTexture()->height_ / 2));
}

void Game::ResetRenderCellFlags(std::size_t current_mouse_index)
{
	for (std::size_t i = 0; i < board_.size(); ++i)
	{
		if (i != current_mouse_index && !board_[i].uncovered_)
		{
			board_[i].render_cell_ = true;
		}
	}
}

void Game::UpdateMinesLeftTexture()
{
	SDL_Color text_color = { 0x00, 0x00, 0x00, 0xFF };
	mines_left_texture_->LoadFromText(renderer_, font_, std::to_string(mines_left_).c_str(), text_color, -1);
}

void Game::UpdateSecondsElapsedTexture()
{
	const SDL_Color text_color = { 0x00, 0x00, 0x00, 0xFF };
	seconds_texture_->LoadFromText(renderer_, font_, std::to_string(seconds_elapsed_).c_str(), text_color, -1);
}

void Game::ResetBoard()
{
	game_over_ = false;
	game_started_ = false;
	seconds_elapsed_ = 0;
	UpdateSecondsElapsedTexture();

	switch (board_size_)
	{
	case BoardSize::SMALL:
		mines_left_ = 10;
		break;
	case BoardSize::MEDIUM:
		mines_left_ = 40;
		break;
	case BoardSize::LARGE:
		mines_left_ = 99;
		break;
	}

	UpdateMinesLeftTexture();

	GenerateBoard();
	SetBoardCellFlags();
}

void Game::GenerateBoard()
{
	constexpr int sprite_size = 32;

	board_.clear();

	for (int i = 0; i < (board_viewport_.w / sprite_size) * (board_viewport_.h / sprite_size); ++i)
	{
		board_.emplace_back(this);
	}

	int rect_x = 0;
	int rect_y = 0;

	for (std::size_t i = 0; i < board_.size(); ++i)
	{
		if (i != 0 && (i * sprite_size) % board_viewport_.w == 0)
		{
			rect_y += sprite_size;
			rect_x = 0;
		}

		board_[i].rect_.x = rect_x;
		board_[i].rect_.y = rect_y;
		board_[i].rect_.w = sprite_size;
		board_[i].rect_.h = board_[i].rect_.w;

		rect_x += sprite_size; 
	}
}

bool Game::GetMousePositionIndex(std::size_t* index)
{
	SDL_Point mouse_position = { 0, 0 };
	SDL_GetMouseState(&mouse_position.x, &mouse_position.y);

	if (mouse_position.x < 0 || mouse_position.x > board_viewport_.w || 
		mouse_position.y - info_viewport_.h < 0 || mouse_position.y - info_viewport_.h > board_viewport_.h)
	{
		return false;
	}

	constexpr std::size_t sprite_size = 32;
	const std::size_t x = mouse_position.x / sprite_size;
	const std::size_t y = (mouse_position.y - info_viewport_.h) / sprite_size;
	const std::size_t mouse_index = (y * (board_viewport_.w / sprite_size)) + x;

	*index = mouse_index;
	return true;
}


void Game::SetBoardCellFlags()
{
	std::mt19937_64 mt{ std::random_device{}() };
	std::uniform_int_distribution<int> random_index{ 0, static_cast<int>(board_.size() - 1) };

	for (int i = 0; i < mines_left_; ++i)
	{
		int mine_index = random_index(mt);

		while (board_[mine_index].mine_)
		{
			mine_index = random_index(mt);
		}

		board_[mine_index].mine_ = true;
	}

	for (std::size_t i = 0; i < board_.size(); ++i)
	{
		for (std::size_t neighbour_index : GetNeighboursIndices(i))
		{
			if (board_[neighbour_index].mine_)
			{
				++board_[i].mines_in_vicinity_;
			}
		}
	}
	
	//DebugBoard();
}

void Game::UncoverCells(std::size_t start_index)
{
	if (board_[start_index].mine_)
	{
		game_over_ = true;
		Mix_PlayChannel(-1, explosion_sfx_, 0);
		board_[start_index].mine_exploded_ = true;

		for (std::size_t i = 0; i < board_.size(); ++i)
		{
			if (board_[i].mine_ && !board_[i].flag_)
			{
				board_[i].Uncover();
			}
		}

		return;
	}

	board_[start_index].Uncover();

	int free_cells_remaining = std::count_if(board_.begin(), board_.end(), [](const Cell& cell)
	{
		return !cell.mine_ && !cell.uncovered_;
	});

	if (free_cells_remaining == 0)
	{
		game_over_ = true;
		mines_left_ = 0;
		UpdateMinesLeftTexture();

		for (std::size_t i = 0; i < board_.size(); ++i)
		{
			if (board_[i].mine_)
			{
				board_[i].flag_ = true;
			}
		}

		return;
	}

	if (board_[start_index].mines_in_vicinity_ != 0)
	{
		return;
	}

	std::stack<std::size_t> indices_stack;
	std::vector<std::size_t> neighbour_indices = GetNeighboursIndices(start_index);

	for (std::size_t neighbour_index : neighbour_indices)
	{
		if (!board_[neighbour_index].uncovered_)
		{
			indices_stack.push(neighbour_index);
		}
	}

	while (!indices_stack.empty())
	{
		std::size_t top_index = indices_stack.top();
		indices_stack.pop();

		if (board_[top_index].uncovered_)
		{
			continue;
		}

		board_[top_index].Uncover();

		if (board_[top_index].mines_in_vicinity_ != 0)
		{
			continue;
		}

		neighbour_indices = GetNeighboursIndices(top_index);

		for (std::size_t neighbour_index : neighbour_indices)
		{
			if (!board_[neighbour_index].uncovered_)
			{			
				indices_stack.push(neighbour_index);
			}
		}
	}
}

void Game::UncoverAvailableNeighbourCells(std::size_t start_index)
{
	std::vector<std::size_t> neighbour_indices = GetNeighboursIndices(start_index);

	int flagged_mines = std::count_if(neighbour_indices.begin(), neighbour_indices.end(), [this](std::size_t neighbour_index)
		{
			return board_[neighbour_index].flag_;
		});

	if (flagged_mines == board_[start_index].mines_in_vicinity_)
	{
		for (std::size_t neighbour_index : neighbour_indices)
		{
			if (!board_[neighbour_index].flag_)
			{
				UncoverCells(neighbour_index);
			}
		}
	}
}


std::vector<std::size_t> Game::GetNeighboursIndices(std::size_t cell_index)
{
	std::vector<std::size_t> result_indices;
	
	constexpr int sprite_size = 32;
	const SDL_Rect cell_rect = board_[cell_index].rect_;

	bool left_cell_available = cell_rect.x - sprite_size >= 0;
	bool right_cell_available = cell_rect.x + sprite_size < board_viewport_.w;
	bool upper_cell_available = cell_rect.y - sprite_size >= 0;
	bool lower_cell_available = cell_rect.y + sprite_size < board_viewport_.h;

	if (left_cell_available)
	{
		result_indices.emplace_back(cell_index - 1);
	}

	if (right_cell_available)
	{
		result_indices.emplace_back(cell_index + 1);
	}

	if (upper_cell_available)
	{
		result_indices.emplace_back(cell_index - (board_viewport_.w / sprite_size));
	}

	if (lower_cell_available)
	{
		result_indices.emplace_back(cell_index + (board_viewport_.w / sprite_size));
	}

	if (left_cell_available && upper_cell_available)
	{
		result_indices.emplace_back(cell_index - (board_viewport_.w / sprite_size) - 1);
	}

	if (left_cell_available && lower_cell_available)
	{
		result_indices.emplace_back(cell_index + (board_viewport_.w / sprite_size) - 1);
	}

	if (right_cell_available && upper_cell_available)
	{
		result_indices.emplace_back(cell_index - (board_viewport_.w / sprite_size) + 1);
	}

	if (right_cell_available && lower_cell_available)
	{
		result_indices.emplace_back(cell_index + (board_viewport_.w / sprite_size) + 1);
	}

	return result_indices;
}