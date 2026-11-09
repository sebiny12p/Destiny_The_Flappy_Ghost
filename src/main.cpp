#include "GhostGame.hpp"
#include "raylib.h"

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(FlappyGhost::VIRTUAL_WIDTH, FlappyGhost::VIRTUAL_HEIGHT,
               "Destiny: The Flappy Ghost | Sebiny Labs");
    SetWindowMinSize(640, 360);
    SetExitKey(KEY_NULL); // Let ESC navigate in-game Director menus & Pause screen
    SetTargetFPS(120);

    FlappyGhost::GhostGame game;
    game.Init();

    while (!WindowShouldClose()) {
        game.RunFrame();
    }

    game.Shutdown();
    CloseWindow();
    return 0;
}
