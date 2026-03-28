#pragma once

#include "games/Game.hpp"
#include "common/math.hpp"
#include "common/Font.hpp"


class Finesweeper : public Game {
public:
    enum Difficulty {
        beginner,
        intermediate,
        expert
    };


public:
    Finesweeper();
    ~Finesweeper() = default;


private:
    void Start() override;
    void ProcessEvent(SDL_Event& event) override;
    void Update(float dt) override;
    void Draw() override;

    void DrawGameStartMenu() override;
    void DrawGameOverMenu() override;


    void InitIsMineBitmap();
    void InitAdjacentMineCounters();
    bool IsWon();

    void UncoverMines();
    void Uncover(int32_t x, int32_t y);
    void ToggleFlag(int32_t x, int32_t y);

    bool IsCovered(int32_t x, int32_t y);
    bool IsFlagged(int32_t x, int32_t y);
    bool IsMine(int32_t x, int32_t y);


private:
    static constexpr int32_t k_max_grid_width = 32;
    static constexpr int32_t k_max_grid_height = 32;


private:
    Difficulty m_difficulty = beginner;
    int32_t m_cells_uncovered;
    int32_t m_mine_count;

    float m_world_width = 4.0f;
    float m_world_height = 3.0f;

    int32_t m_grid_width;
    int32_t m_grid_height;
    V2F32 m_grid_pos;
    V2F32 m_cell_outer_size;
    V2F32 m_cell_inner_size;

    uint32_t m_is_covered_bitmap[k_max_grid_height] {};
    uint32_t m_is_flagged_bitmap[k_max_grid_height] {};
    uint32_t m_is_mine_bitmap[k_max_grid_height] {};
    int32_t m_adjacent_mine_counts[k_max_grid_width * k_max_grid_height] {};

    Font m_font;
};


