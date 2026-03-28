#pragma once

#include "games/Game.hpp"
#include "games/fetris/Fetromino.hpp"
#include "games/fetris/Board.hpp"
#include "common/Font.hpp"


class Fetris : public Game {
public:
    Fetris();


private:
    void Start() override;
    void ProcessEvent(SDL_Event& event) override;
    void Update(float dt) override;
    void Draw() override;

    void DrawGameOverMenu() override;


    uint32_t GetSoftdropCount(float dt);
    void HandleFetrominoPlacement();

    int32_t ReadHighscore();
    void WriteHighscore();

    void DrawLineCounter();
    void DrawStatistics();
    void DrawScore();
    void DrawNextFetromino();
    void DrawLevel();



private:
    static constexpr Color k_text_color {0.9f, 0.9f, 0.9f, 1.0f};
    static constexpr char k_highscore_filepath[]{"fetris_highscore.txt"};

    Font m_font;
    Board m_board;
    Fetromino m_active_fetromino;
    Fetromino::Id m_next_fetromino_id;

    int32_t m_fetromino_counters[Fetromino::id_count] {};
    int32_t m_score = 0;
    int32_t m_line_counter = 0;
    int32_t m_starting_level = 0;
    int32_t m_level = 0;
    int32_t m_softdrop_counter = 0;
    int32_t m_highscore = 0;
};


