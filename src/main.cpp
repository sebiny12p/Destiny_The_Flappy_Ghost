#include "GhostGame.hpp"
#include "raylib.h"

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
#if defined(PLATFORM_ANDROID)
    InitWindow(0, 0, "Destiny: The Flappy Ghost | Sebiny Labs");
#else
    InitWindow(FlappyGhost::VIRTUAL_WIDTH, FlappyGhost::VIRTUAL_HEIGHT,
               "Destiny: The Flappy Ghost | Sebiny Labs");
    {
        int mon = GetCurrentMonitor();
        int mw = GetMonitorWidth(mon);
        int mh = GetMonitorHeight(mon);
        if (mw >= 1600 && mh >= 900) {
            int winH = ((mh * 84) / 100) & ~1;
            int winW = ((winH * 16) / 9) & ~1;
            if (winW > (mw * 92) / 100) {
                winW = ((mw * 88) / 100) & ~1;
                winH = ((winW * 9) / 16) & ~1;
            }
            SetWindowSize(winW, winH);
            SetWindowPosition((mw - winW) / 2, (mh - winH) / 2);
        }
    }
#endif
    SetWindowMinSize(640, 360);
    SetExitKey(KEY_NULL); // Let ESC navigate in-game Director menus & Pause screen
    SetTargetFPS(120);

    Image appIcon = LoadImage("assets/icon.png");
    if (appIcon.data == nullptr) {
        appIcon = LoadImage("build/assets/icon.png");
    }
    if (appIcon.data != nullptr) {
        SetWindowIcon(appIcon);
        UnloadImage(appIcon);
    }

    FlappyGhost::GhostGame game;
    game.Init();

    while (!WindowShouldClose()) {
        game.RunFrame();
    }

    game.Shutdown();
    CloseWindow();
    return 0;
}
