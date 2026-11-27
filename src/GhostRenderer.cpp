#include "GhostGame.hpp"

#undef VIRTUAL_WIDTH
#define VIRTUAL_WIDTH virtualWidth_

#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace FlappyGhost {

namespace {
Font g_uiFontSmall{};
Font g_uiFontBody{};
Font g_uiFontBold{};
Font g_uiFontHero{};
float g_builtFontScale = 0.0f;
int g_builtTextSizeMode = -1;
float g_activeUiScale = 1.0f;
int g_uiTextSizeMode = 0; // 0 = Normal, 1 = Large, 2 = Extra Large

float GetAdaptiveTextScale(int fontSize, int mode = g_uiTextSizeMode) {
    if (mode <= 0) return 1.0f;
    if (mode == 1) {
        if (fontSize <= 13) return 1.15f;
        if (fontSize <= 16) return 1.09f;
        if (fontSize <= 20) return 1.04f;
        return 1.0f;
    }
    if (fontSize <= 13) return 1.25f;
    if (fontSize <= 16) return 1.15f;
    if (fontSize <= 20) return 1.07f;
    return 1.0f;
}

Font LoadCrispUiFont(const std::initializer_list<const char*>& paths, int baseSize) {
    int codepoints[236];
    int count = 0;
    for (int cp = 32; cp <= 255; ++cp) codepoints[count++] = cp;
    codepoints[count++] = 8211;
    codepoints[count++] = 8212;
    codepoints[count++] = 8226;
    codepoints[count++] = 8776;
    codepoints[count++] = 10003;
    unsigned int defaultTexId = GetFontDefault().texture.id;
    for (const char* p : paths) {
#if !defined(PLATFORM_ANDROID)
        if (!FileExists(p)) continue;
#endif
        Font f = LoadFontEx(p, baseSize, codepoints, count);
        if (f.texture.id != 0 && f.texture.id != defaultTexId) {
            SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
            return f;
        }
    }
    return Font{};
}

const Font& PickUiFont(int fontSize) {
    int effPx = static_cast<int>(std::round(static_cast<float>(fontSize) * GetAdaptiveTextScale(fontSize)));
    if (effPx <= 14) return (g_uiFontSmall.texture.id != 0) ? g_uiFontSmall : g_uiFontBody;
    if (effPx <= 19) return g_uiFontBody;
    if (effPx <= 34) return (g_uiFontBold.texture.id != 0) ? g_uiFontBold : g_uiFontBody;
    return (g_uiFontHero.texture.id != 0) ? g_uiFontHero : g_uiFontBold;
}

int MeasureGameTextScaled(const char* text, int fontSize, float customBoost) {
    if (!text || !text[0]) return 0;
    const Font& f = PickUiFont(fontSize);
    float effSize = static_cast<float>(fontSize) * customBoost;
    if (f.texture.id == 0) return ::MeasureText(text, static_cast<int>(std::round(effSize)));
    float scaledFs = effSize * 1.10f;
    float spacing = std::max(0.5f, effSize * 0.03f);
    Vector2 sz = MeasureTextEx(f, text, scaledFs, spacing);
    return static_cast<int>(std::ceil(sz.x));
}

int MeasureGameText(const char* text, int fontSize) {
    return MeasureGameTextScaled(text, fontSize, GetAdaptiveTextScale(fontSize));
}

void DrawGameTextScaled(const char* text, int posX, int posY, int fontSize, float customBoost, Color color) {
    if (!text || !text[0]) return;
    const Font& f = PickUiFont(fontSize);
    float effSize = static_cast<float>(fontSize) * customBoost;
    if (f.texture.id == 0) {
        ::DrawText(text, posX, posY, static_cast<int>(std::round(effSize)), color);
        return;
    }
    float sc = std::max(0.5f, g_activeUiScale);
    float scaledFs = effSize * 1.10f;
    float spacing = std::max(0.5f, effSize * 0.03f);
    Vector2 pos = {
        std::round(static_cast<float>(posX) * sc) / sc,
        std::round((static_cast<float>(posY) - scaledFs * 0.06f) * sc) / sc
    };
    if (color.a > 40 && (static_cast<int>(color.r) + static_cast<int>(color.g) + static_cast<int>(color.b)) > 95) {
        unsigned char shA = static_cast<unsigned char>(std::min(220, static_cast<int>(color.a * 0.72f)));
        float px1 = std::max(1.0f, std::round(sc * 0.85f)) / sc;
        DrawTextEx(f, text, { pos.x + px1, pos.y + px1 }, scaledFs, spacing, { 0, 0, 0, shA });
    }
    DrawTextEx(f, text, pos, scaledFs, spacing, color);
}

void DrawGameText(const char* text, int posX, int posY, int fontSize, Color color) {
    DrawGameTextScaled(text, posX, posY, fontSize, GetAdaptiveTextScale(fontSize), color);
}

void DrawUpgradeBadge(Rectangle r, float globalTime) {
    float pulse = 0.5f + 0.5f * std::sin(globalTime * 5.0f);
    Vector2 c = { r.x + r.width - 10.0f, r.y + 10.0f };
    DrawCircleV(c, 8.5f + pulse * 1.5f, Color{ 255, 210, 40, static_cast<unsigned char>(85 + pulse * 80) });
    DrawCircleV(c, 7.5f, GOLD);
    DrawCircleLines(static_cast<int>(c.x), static_cast<int>(c.y), 7.5f, WHITE);
    int ew = MeasureGameText("!", 12);
    DrawGameText("!", static_cast<int>(c.x - ew * 0.5f), static_cast<int>(c.y - 6.0f), 12, BLACK);
}
} // namespace

void GhostGame::RefreshUiFonts(float targetScale) {
    float clampedScale = std::clamp(targetScale, 0.75f, 4.0f);
    int curTextMode = std::clamp(profile_.textSize, 0, 2);
    g_uiTextSizeMode = curTextMode;
    g_activeUiScale = clampedScale;
    if (g_uiFontBody.texture.id != 0 && curTextMode == g_builtTextSizeMode && std::abs(clampedScale - g_builtFontScale) < 0.08f) {
        return;
    }
    ShutdownUiFonts();
    g_builtFontScale = clampedScale;
    g_builtTextSizeMode = curTextMode;
    float atlasBoost = (curTextMode == 2) ? 1.25f : (curTextMode == 1) ? 1.15f : 1.0f;
    int pxSmall = std::clamp(static_cast<int>(std::round(15.0f * clampedScale * atlasBoost)), 18, 112);
    int pxBody  = std::clamp(static_cast<int>(std::round(20.0f * clampedScale * atlasBoost)), 24, 144);
    int pxBold  = std::clamp(static_cast<int>(std::round(32.0f * clampedScale)), 36, 176);
    int pxHero  = std::clamp(static_cast<int>(std::round(66.0f * clampedScale)), 64, 256);
#if defined(PLATFORM_ANDROID)
    g_uiFontSmall = LoadCrispUiFont({ "ui_medium.ttf", "assets/ui_medium.ttf" }, pxSmall);
    g_uiFontBody  = LoadCrispUiFont({ "ui_medium.ttf", "assets/ui_medium.ttf" }, pxBody);
    g_uiFontBold  = LoadCrispUiFont({ "ui_bold.ttf", "assets/ui_bold.ttf" }, pxBold);
    g_uiFontHero  = LoadCrispUiFont({ "ui_bold.ttf", "assets/ui_bold.ttf" }, pxHero);
#else
    g_uiFontSmall = LoadCrispUiFont({ "assets/ui_medium.ttf", "ui_medium.ttf", "../assets/ui_medium.ttf", "build/assets/ui_medium.ttf" }, pxSmall);
    g_uiFontBody  = LoadCrispUiFont({ "assets/ui_medium.ttf", "ui_medium.ttf", "../assets/ui_medium.ttf", "build/assets/ui_medium.ttf" }, pxBody);
    g_uiFontBold  = LoadCrispUiFont({ "assets/ui_bold.ttf", "ui_bold.ttf", "../assets/ui_bold.ttf", "build/assets/ui_bold.ttf" }, pxBold);
    g_uiFontHero  = LoadCrispUiFont({ "assets/ui_bold.ttf", "ui_bold.ttf", "../assets/ui_bold.ttf", "build/assets/ui_bold.ttf" }, pxHero);
#endif
}

void GhostGame::InitUiFonts() {
    RefreshUiFonts(uiScale_);
}

void GhostGame::ShutdownUiFonts() {
    unsigned int defaultTexId = GetFontDefault().texture.id;
    if (g_uiFontSmall.texture.id != 0 && g_uiFontSmall.texture.id != defaultTexId) { UnloadFont(g_uiFontSmall); }
    if (g_uiFontBody.texture.id != 0  && g_uiFontBody.texture.id != defaultTexId)  { UnloadFont(g_uiFontBody);  }
    if (g_uiFontBold.texture.id != 0  && g_uiFontBold.texture.id != defaultTexId)  { UnloadFont(g_uiFontBold);  }
    if (g_uiFontHero.texture.id != 0  && g_uiFontHero.texture.id != defaultTexId)  { UnloadFont(g_uiFontHero);  }
    g_uiFontSmall = Font{};
    g_uiFontBody  = Font{};
    g_uiFontBold  = Font{};
    g_uiFontHero  = Font{};
    g_builtFontScale = 0.0f;
    g_builtTextSizeMode = -1;
}

#define MeasureText MeasureGameText
#define DrawText DrawGameText

namespace {
void DrawTextWrapped(const char* text, int x, int y, int maxWidth, int fontSize, int lineHeight, Color color) {
    if (!text || !text[0]) return;
    std::string input(text);

    auto countLinesAt = [&](float boost) -> int {
        int lines = 0;
        std::string curLine, w;
        for (size_t i = 0; i <= input.size(); ++i) {
            char c = (i < input.size()) ? input[i] : ' ';
            if (c == ' ' || c == '\n') {
                if (!w.empty()) {
                    std::string cand = curLine.empty() ? w : (curLine + " " + w);
                    if (MeasureGameTextScaled(cand.c_str(), fontSize, boost) > maxWidth && !curLine.empty()) {
                        lines++;
                        curLine = w;
                    } else {
                        curLine = cand;
                    }
                    w.clear();
                }
                if (c == '\n' && !curLine.empty()) { lines++; curLine.clear(); }
            } else {
                w.push_back(c);
            }
        }
        if (!curLine.empty()) lines++;
        return lines;
    };

    float boost = GetAdaptiveTextScale(fontSize);
    if (boost > 1.01f) {
        int baseLines = countLinesAt(1.0f);
        if (baseLines <= 2 && countLinesAt(boost) > baseLines) {
            // Prevent short 1-2 line card descriptions from spilling onto an extra vertical line!
            float midBoost = 1.0f + (boost - 1.0f) * 0.52f;
            boost = (countLinesAt(midBoost) <= baseLines) ? midBoost : 1.04f;
        }
    }
    int effLineH = static_cast<int>(std::round(static_cast<float>(lineHeight) * (1.0f + (boost - 1.0f) * 0.72f)));

    std::string currentLine;
    std::string word;
    int curY = y;

    auto flushLine = [&]() {
        if (!currentLine.empty()) {
            DrawGameTextScaled(currentLine.c_str(), x, curY, fontSize, boost, color);
            curY += effLineH;
            currentLine.clear();
        }
    };

    for (size_t i = 0; i <= input.size(); ++i) {
        char c = (i < input.size()) ? input[i] : ' ';
        if (c == ' ' || c == '\n') {
            if (!word.empty()) {
                std::string candidate = currentLine.empty() ? word : (currentLine + " " + word);
                if (MeasureText(candidate.c_str(), fontSize) > maxWidth && !currentLine.empty()) {
                    flushLine();
                    currentLine = word;
                } else {
                    currentLine = candidate;
                }
                word.clear();
            }
            if (c == '\n') {
                flushLine();
            }
        } else {
            word.push_back(c);
        }
    }
    flushLine();
}
} // namespace

void GhostGame::RenderFrame() {
    UpdateVirtualResolution();

    BeginDrawing();
    ClearBackground(BLACK);

    Vector2 shakeOffset = { 0.0f, 0.0f };
    if (screenShakeTrauma_ > 0.0f && profile_.screenShakeScale > 0.0f) {
        float mag = screenShakeTrauma_ * screenShakeTrauma_ * 18.0f * profile_.screenShakeScale;
        shakeOffset.x = std::sin(globalTime_ * 67.0f) * mag;
        shakeOffset.y = std::cos(globalTime_ * 53.0f) * mag;
    }

    Camera2D worldCam{};
    worldCam.offset = { uiOffsetX_ + shakeOffset.x * uiScale_, uiOffsetY_ + shakeOffset.y * uiScale_ };
    worldCam.target = { 0.0f, 0.0f };
    worldCam.rotation = 0.0f;
    worldCam.zoom = uiScale_;

    BeginMode2D(worldCam);

    RenderStarfieldAndBackdrop();

    if (state_ == ScreenState::PLAYING || state_ == ScreenState::PAUSED || state_ == ScreenState::LIGHT_FADED) {
        RenderBiomeEnvironment();
        RenderObstacles();
        RenderCollectibles();
        RenderPatrolEnemies();
        RenderBoss();
        RenderProjectiles();
        RenderParticles();

        // Manual-Fire Combat Drone (Only active & visible in Nightfall Mode when a drone is equipped)
        if (isNightfallMode_ && profile_.equippedDrone >= 0 && profile_.equippedDrone < static_cast<int>(NUM_DRONES)) {
            const auto& droneDef = GetDroneCatalog()[profile_.equippedDrone];
            Vector2 dronePos = {
                ghost_.pos.x + std::cos(ghost_.companionAngle) * 42.0f,
                ghost_.pos.y + std::sin(ghost_.companionAngle) * 32.0f
            };
            float pulseScale = 1.0f + ghost_.dronePulseAnim * 0.45f;
            DrawCircleGradient(static_cast<int>(dronePos.x), static_cast<int>(dronePos.y),
                               18.0f * pulseScale, droneDef.color, BLANK);
            int sides = (profile_.equippedDrone == 0) ? 6 : (profile_.equippedDrone == 1) ? 4 :
                        (profile_.equippedDrone == 2) ? 8 : 3;
            DrawPoly(dronePos, sides, 9.0f * pulseScale, globalTime_ * 180.0f, WHITE);
            DrawPolyLinesEx(dronePos, sides, 12.5f * pulseScale, -globalTime_ * 140.0f, 2.0f, droneDef.color);
        }

        // 1. Voidwalker / Prismatic Spatial Warp Rift & Afterimage Echo
        if (ghost_.blinkEchoTimer > 0.0f) {
            float echoRatio = std::clamp(ghost_.blinkEchoTimer / 0.38f, 0.0f, 1.0f);
            Color riftCol = GetSubclassAccentColor();
            Color beamCol = { riftCol.r, riftCol.g, riftCol.b, static_cast<unsigned char>(210 * echoRatio) };
            DrawLineEx(ghost_.blinkEchoPos, ghost_.pos, 5.0f * echoRatio, beamCol);
            DrawLineEx(ghost_.blinkEchoPos, ghost_.pos, 2.0f * echoRatio, Color{ 255, 255, 255, static_cast<unsigned char>(190 * echoRatio) });
            DrawPolyLinesEx(ghost_.blinkEchoPos, 4, 24.0f * (1.25f - 0.35f * echoRatio), globalTime_ * 220.0f, 2.2f, beamCol);
            RenderGhostRig(ghost_.blinkEchoPos, ghost_.radius * 0.92f, ghost_.rotation, 0.65f,
                           ghost_.shellSpinAngle, ghost_.eyeLookOffset, profile_.equippedShell, echoRatio * 0.50f);
        }

        // 2. Dawnblade Icarus Hover Jet (Radiant Solar Wings + Golden Plume + Magnet Ring)
        if (ghost_.solarHoverTimer > 0.0f) {
            float hovRatio = std::clamp(ghost_.solarHoverTimer / 1.15f, 0.0f, 1.0f);
            float flapWave = std::sin(globalTime_ * 24.0f) * 6.0f;
            unsigned char wingA = static_cast<unsigned char>(185 * hovRatio + 45);
            Color wingOuter = { 255, 140, 30, wingA };
            Color wingInner = { 255, 230, 120, wingA };
            DrawTriangle({ ghost_.pos.x - 6.0f, ghost_.pos.y - 4.0f },
                         { ghost_.pos.x - 46.0f, ghost_.pos.y - 34.0f + flapWave },
                         { ghost_.pos.x - 26.0f, ghost_.pos.y - 8.0f }, wingOuter);
            DrawTriangle({ ghost_.pos.x - 6.0f, ghost_.pos.y + 4.0f },
                         { ghost_.pos.x - 26.0f, ghost_.pos.y + 8.0f },
                         { ghost_.pos.x - 46.0f, ghost_.pos.y + 34.0f - flapWave }, wingOuter);
            DrawRectangleGradientH(static_cast<int>(ghost_.pos.x - 68.0f), static_cast<int>(ghost_.pos.y - 8.0f),
                                   56, 16, BLANK, wingInner);
            DrawCircleLines(static_cast<int>(ghost_.pos.x), static_cast<int>(ghost_.pos.y),
                            42.0f + std::sin(globalTime_ * 16.0f) * 4.0f,
                            Color{ 255, 195, 70, static_cast<unsigned char>(130 * hovRatio) });
        }

        // 3. Stormcaller Ionic Lightning Surge (Flat Bolt Speed Lines & Crackling Arcs)
        if (ghost_.arcDashTimer > 0.0f) {
            float arcRatio = std::clamp(ghost_.arcDashTimer / 0.42f, 0.0f, 1.0f);
            Color boltCol = { 80, 230, 255, static_cast<unsigned char>(235 * arcRatio) };
            for (int b = -1; b <= 1; ++b) {
                Vector2 p0 = { ghost_.pos.x + 18.0f, ghost_.pos.y + b * 11.0f };
                Vector2 p1 = { ghost_.pos.x - 36.0f, ghost_.pos.y + b * 14.0f + std::sin(globalTime_ * 65.0f + b * 2.1f) * 10.0f };
                Vector2 p2 = { ghost_.pos.x - 95.0f, ghost_.pos.y + b * 9.0f  - std::cos(globalTime_ * 53.0f + b * 1.7f) * 11.0f };
                DrawLineEx(p0, p1, 3.0f, WHITE);
                DrawLineEx(p1, p2, 2.5f, boltCol);
            }
            DrawPolyLinesEx(ghost_.pos, 6, 30.0f, globalTime_ * 320.0f, 2.2f, boltCol);
        }

        // 4. Shadebinder Frost Pulse Shockwave & Bounceable Stasis Ice Floor
        if (ghost_.stasisPulseTimer > 0.0f) {
            float prog = 1.0f - std::clamp(ghost_.stasisPulseTimer / 0.50f, 0.0f, 1.0f);
            float pulseR = 24.0f + prog * 235.0f;
            unsigned char pA = static_cast<unsigned char>((1.0f - prog) * 220.0f);
            DrawPolyLinesEx(ghost_.pos, 6, pulseR, prog * 45.0f, 3.0f, Color{ 95, 175, 255, pA });
            DrawCircleLines(static_cast<int>(ghost_.pos.x), static_cast<int>(ghost_.pos.y), pulseR * 0.82f, Color{ 200, 235, 255, pA });
        }
        if (ghost_.stasisFloorTimer > 0.0f) {
            float flRatio = std::clamp(ghost_.stasisFloorTimer / 0.45f, 0.0f, 1.0f);
            unsigned char flA = static_cast<unsigned char>(215 * flRatio);
            float rx0 = ghost_.pos.x - 90.0f;
            float rx1 = ghost_.pos.x + 150.0f;
            DrawRectangle(static_cast<int>(rx0), static_cast<int>(ghost_.stasisFloorY + ghost_.radius * 0.5f),
                          static_cast<int>(rx1 - rx0), 7, Color{ 65, 145, 255, flA });
            DrawLineEx({ rx0, ghost_.stasisFloorY + ghost_.radius * 0.5f },
                       { rx1, ghost_.stasisFloorY + ghost_.radius * 0.5f }, 2.5f, Color{ 215, 242, 255, flA });
            for (int k = 0; k < 5; ++k) {
                Vector2 shardPos = { rx0 + 24.0f + k * 48.0f, ghost_.stasisFloorY + ghost_.radius * 0.5f + 4.0f };
                DrawPoly(shardPos, 4, 7.5f, 0.0f, Color{ 125, 200, 255, flA });
            }
        }

        // 5. Strand Grapple Tether Line
        if (ghost_.grappleTimer > 0.0f) {
            DrawLineEx(ghost_.pos, ghost_.grappleAnchor, 3.5f, { 65, 245, 130, 235 });
            DrawPoly(ghost_.grappleAnchor, 6, 12.0f, globalTime_ * 160.0f, { 65, 245, 130, 255 });
        }

        // Chaos Reach Arc Beam visual when Arc Super is active in Nightfall
        if (isNightfallMode_ && ghost_.superActiveTimer > 0.0f &&
            static_cast<LightSubclass>(profile_.selectedSubclass) == LightSubclass::ARC_CALLER) {
            float beamH = 88.0f + std::sin(globalTime_ * 45.0f) * 12.0f;
            DrawRectangleGradientH(static_cast<int>(ghost_.pos.x + 15.0f),
                                   static_cast<int>(ghost_.pos.y - beamH * 0.5f),
                                   VIRTUAL_WIDTH, static_cast<int>(beamH),
                                   { 80, 230, 255, 210 }, { 190, 250, 255, 150 });
            DrawRectangle(static_cast<int>(ghost_.pos.x + 15.0f),
                          static_cast<int>(ghost_.pos.y - beamH * 0.22f),
                          VIRTUAL_WIDTH, static_cast<int>(beamH * 0.44f), WHITE);
        }

        // Solar Daybreak / Prismatic Transcendent Aura when active in Nightfall
        if (isNightfallMode_ && ghost_.superActiveTimer > 0.0f &&
            (static_cast<LightSubclass>(profile_.selectedSubclass) == LightSubclass::SOLAR_DAWN ||
             static_cast<LightSubclass>(profile_.selectedSubclass) == LightSubclass::PRISMATIC_TRANSCENDENT)) {
            Color auraCol = (static_cast<LightSubclass>(profile_.selectedSubclass) == LightSubclass::SOLAR_DAWN)
                          ? Color{ 255, 180, 40, 140 } : Color{ 255, 120, 235, 140 };
            Color ringCol = (static_cast<LightSubclass>(profile_.selectedSubclass) == LightSubclass::SOLAR_DAWN)
                          ? GOLD : SKYBLUE;
            float r = 74.0f + std::sin(globalTime_ * 14.0f) * 7.0f;
            DrawCircleGradient(static_cast<int>(ghost_.pos.x), static_cast<int>(ghost_.pos.y),
                               r, auraCol, { auraCol.r, auraCol.g, auraCol.b, 0 });
            DrawCircleLines(static_cast<int>(ghost_.pos.x), static_cast<int>(ghost_.pos.y), r, ringCol);
        }

        // Overshield ring around Ghost
        if (ghost_.overshieldCharges > 0) {
            DrawCircleLines(static_cast<int>(ghost_.pos.x), static_cast<int>(ghost_.pos.y),
                            ghost_.radius + 11.0f + std::sin(globalTime_ * 8.0f) * 2.0f,
                            { 100, 225, 255, 210 });
        }

        float ghostAlpha = (ghost_.phaseTimer > 0.0f) ? 0.45f :
                           (ghost_.invulnerableTimer > 0.0f && std::fmod(globalTime_, 0.14f) < 0.07f ? 0.5f : 1.0f);
        RenderGhostRig(ghost_.pos, ghost_.radius, ghost_.rotation, ghost_.shellExpand,
                       ghost_.shellSpinAngle, ghost_.eyeLookOffset, profile_.equippedShell, ghostAlpha);

        RenderHud();
    }

    EndMode2D();

    Camera2D uiCam{};
    uiCam.offset = { uiOffsetX_, uiOffsetY_ };
    uiCam.target = { 0.0f, 0.0f };
    uiCam.rotation = 0.0f;
    uiCam.zoom = uiScale_;

    BeginMode2D(uiCam);

    if (flashAlpha_ > 0.0f && profile_.screenFlashes) {
        Color fc = flashColor_;
        fc.a = static_cast<unsigned char>(std::clamp(flashAlpha_ * 255.0f, 0.0f, 255.0f));
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, fc);
    }

    if (state_ == ScreenState::SPLASH_INTRO) {
        RenderSplashIntro();
    } else if (state_ == ScreenState::TITLE_FRONT_PAGE) {
        RenderParticles();
        RenderTitleFrontPage();
    } else if (state_ == ScreenState::TITLE_DIRECTOR) {
        RenderParticles();
        RenderTitleDirector();
    } else if (state_ == ScreenState::SUBCLASS_ARMORY) {
        RenderSubclassArmory();
    } else if (state_ == ScreenState::SHELL_HANGAR) {
        RenderShellHangar();
    } else if (state_ == ScreenState::TRIUMPHS_LOG) {
        RenderTriumphsLog();
    } else if (state_ == ScreenState::SETTINGS_VIEW) {
        RenderSettingsView();
    } else if (state_ == ScreenState::CAREER_STATS) {
        RenderCareerStats();
    } else if (state_ == ScreenState::PAUSED) {
        RenderPauseOverlay();
    } else if (state_ == ScreenState::LIGHT_FADED) {
        RenderLightFadedOverlay();
    }

    RenderTriumphToast();

    EndMode2D();

    if (uiOffsetY_ > 0.5f) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        int bar = static_cast<int>(std::ceil(uiOffsetY_));
        DrawRectangle(0, 0, sw, bar, BLACK);
        DrawRectangle(0, sh - bar, sw, bar + 2, BLACK);
    }

    EndDrawing();
}

void GhostGame::RenderStarfieldAndBackdrop() const {
    BiomeSector b = (state_ == ScreenState::PLAYING || state_ == ScreenState::PAUSED || state_ == ScreenState::LIGHT_FADED)
                    ? GetCurrentBiome() : BiomeSector::COSMODROME;

    Color topBg = { 10, 15, 28, 255 };
    Color botBg = { 22, 34, 58, 255 };
    if (b == BiomeSector::VAULT_OF_GLASS) {
        topBg = { 8, 22, 32, 255 };
        botBg = { 18, 52, 68, 255 };
    } else if (b == BiomeSector::HELLMOUTH) {
        topBg = { 10, 20, 16, 255 };
        botBg = { 24, 48, 34, 255 };
    } else if (b == BiomeSector::ASCENDANT_PLANE) {
        topBg = { 14, 10, 24, 255 };
        botBg = { 36, 18, 48, 255 };
    } else if (b == BiomeSector::DREAMING_CITY) {
        topBg = { 24, 16, 48, 255 };
        botBg = { 62, 34, 92, 255 };
    } else if (b == BiomeSector::EUROPA_BRAYTECH) {
        topBg = { 10, 28, 52, 255 };
        botBg = { 42, 92, 138, 255 };
    } else if (b == BiomeSector::SAVATHUNS_THRONE_WORLD) {
        topBg = { 16, 30, 32, 255 };
        botBg = { 44, 78, 72, 255 };
    } else if (b == BiomeSector::PALE_HEART) {
        topBg = { 32, 22, 54, 255 };
        botBg = { 96, 62, 88, 255 };
    }

    DrawRectangleGradientV(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, topBg, botBg);

    for (const auto& s : stars_) {
        DrawCircleV(s.pos, s.size, s.color);
    }

    Vector2 travelerCenter = { 980.0f, 220.0f };
    DrawCircleLines(static_cast<int>(travelerCenter.x), static_cast<int>(travelerCenter.y), 240.0f, { 255, 255, 255, 18 });
    DrawCircleLines(static_cast<int>(travelerCenter.x), static_cast<int>(travelerCenter.y), 185.0f, { 255, 255, 255, 25 });

    DrawCircleGradient(static_cast<int>(travelerCenter.x), static_cast<int>(travelerCenter.y),
                       115.0f, { 235, 242, 255, 75 }, { 160, 195, 230, 10 });
    DrawCircle(static_cast<int>(travelerCenter.x), static_cast<int>(travelerCenter.y), 92.0f, { 215, 225, 238, 42 });
    DrawCircleLines(static_cast<int>(travelerCenter.x), static_cast<int>(travelerCenter.y), 92.0f, { 255, 255, 255, 50 });
}

void GhostGame::RenderBiomeEnvironment() const {
    BiomeSector b = GetCurrentBiome();
    Color horizonColor = { 30, 44, 68, 180 };
    if (b == BiomeSector::VAULT_OF_GLASS) horizonColor = { 20, 75, 95, 180 };
    if (b == BiomeSector::HELLMOUTH)      horizonColor = { 28, 70, 48, 180 };
    if (b == BiomeSector::ASCENDANT_PLANE)        horizonColor = { 65, 25, 85, 180 };
    if (b == BiomeSector::DREAMING_CITY)          horizonColor = { 92, 48, 128, 180 };
    if (b == BiomeSector::EUROPA_BRAYTECH)        horizonColor = { 75, 145, 205, 180 };
    if (b == BiomeSector::SAVATHUNS_THRONE_WORLD) horizonColor = { 88, 122, 108, 180 };
    if (b == BiomeSector::PALE_HEART)             horizonColor = { 175, 118, 135, 180 };

    float scrollOffset = std::fmod(distanceMeters_ * 14.0f, 160.0f);
    for (int i = -1; i < 10; ++i) {
        float x = i * 160.0f - scrollOffset;
        float h = 95.0f + ((i * 53) % 85);
        DrawRectangle(static_cast<int>(x), static_cast<int>(VIRTUAL_HEIGHT - h), 90, static_cast<int>(h), horizonColor);
    }

    DrawRectangle(0, 0, VIRTUAL_WIDTH, 18, { 12, 16, 24, 240 });
    DrawLineEx({ 0.0f, 18.0f }, { static_cast<float>(VIRTUAL_WIDTH), 18.0f }, 2.0f, GetSubclassAccentColor());

    DrawRectangle(0, VIRTUAL_HEIGHT - 24, VIRTUAL_WIDTH, 24, { 12, 16, 24, 240 });
    DrawLineEx({ 0.0f, VIRTUAL_HEIGHT - 24.0f }, { static_cast<float>(VIRTUAL_WIDTH), VIRTUAL_HEIGHT - 24.0f }, 2.0f, GetSubclassAccentColor());
}

void GhostGame::RenderObstacles() const {
    for (const auto& o : obstacles_) {
        if (!o.active) continue;

        float gapTop = o.gapCenterY - o.gapHeight * 0.5f;
        float gapBot = o.gapCenterY + o.gapHeight * 0.5f;

        Color pillarMain = { 52, 64, 82, 255 };
        Color pillarEdge = { 220, 115, 45, 255 };
        if (o.biome == BiomeSector::VAULT_OF_GLASS) {
            pillarMain = { 34, 62, 74, 255 };
            pillarEdge = { 70, 235, 255, 255 };
        } else if (o.biome == BiomeSector::HELLMOUTH) {
            pillarMain = { 42, 54, 46, 255 };
            pillarEdge = { 90, 250, 130, 255 };
        } else if (o.biome == BiomeSector::ASCENDANT_PLANE) {
            pillarMain = { 32, 24, 46, 255 };
            pillarEdge = { 215, 90, 255, 255 };
        } else if (o.biome == BiomeSector::DREAMING_CITY) {
            pillarMain = { 54, 38, 86, 255 };
            pillarEdge = { 235, 185, 255, 255 };
        } else if (o.biome == BiomeSector::EUROPA_BRAYTECH) {
            pillarMain = { 44, 78, 112, 255 };
            pillarEdge = { 165, 238, 255, 255 };
        } else if (o.biome == BiomeSector::SAVATHUNS_THRONE_WORLD) {
            pillarMain = { 112, 118, 104, 255 };
            pillarEdge = { 75, 235, 195, 255 };
        } else if (o.biome == BiomeSector::PALE_HEART) {
            pillarMain = { 82, 54, 88, 255 };
            pillarEdge = { 255, 205, 120, 255 };
        }

        if (o.frozenByStasis || ghost_.stasisFreezeTimer > 0.0f) {
            pillarEdge = { 90, 165, 255, 255 };
        }

        DrawRectangle(static_cast<int>(o.x), 18, static_cast<int>(o.width), static_cast<int>(gapTop - 18.0f), pillarMain);
        DrawRectangleLinesEx({ o.x, 18.0f, o.width, std::max(0.0f, gapTop - 18.0f) }, 2.5f, pillarEdge);
        DrawRectangle(static_cast<int>(o.x - 5.0f), static_cast<int>(gapTop - 18.0f), static_cast<int>(o.width + 10.0f), 18, pillarEdge);

        float botH = (VIRTUAL_HEIGHT - 24.0f) - gapBot;
        DrawRectangle(static_cast<int>(o.x), static_cast<int>(gapBot), static_cast<int>(o.width), static_cast<int>(botH), pillarMain);
        DrawRectangleLinesEx({ o.x, gapBot, o.width, std::max(0.0f, botH) }, 2.5f, pillarEdge);
        DrawRectangle(static_cast<int>(o.x - 5.0f), static_cast<int>(gapBot), static_cast<int>(o.width + 10.0f), 18, pillarEdge);

        Vector2 mid = { o.x + o.width * 0.5f, o.gapCenterY };
        if (o.kind == ObstacleKind::MOVING_VEX_LASER) {
            DrawLineEx({ mid.x, 18.0f }, { mid.x, gapTop }, 3.0f, RED);
            DrawLineEx({ mid.x, gapBot }, { mid.x, VIRTUAL_HEIGHT - 24.0f }, 3.0f, RED);
        } else if (o.kind == ObstacleKind::ROTATING_ORACLE && !o.oracleDestroyed) {
            DrawPoly(mid, 6, o.hazardRadius, globalTime_ * 90.0f, { 255, 200, 50, 220 });
            DrawPolyLinesEx(mid, 6, o.hazardRadius + 4.0f, -globalTime_ * 70.0f, 2.5f, WHITE);
            DrawCircleV(mid, 8.0f, WHITE);
        } else if (o.kind == ObstacleKind::TAKEN_BLIGHT_WELL && !o.oracleDestroyed) {
            float pulse = std::fmod(globalTime_ * 65.0f, 55.0f);
            DrawCircleLines(static_cast<int>(mid.x), static_cast<int>(mid.y), 65.0f - pulse, { 180, 120, 255, 120 });
            DrawCircleGradient(static_cast<int>(mid.x), static_cast<int>(mid.y), o.hazardRadius + 8.0f, BLACK, { 140, 50, 230, 180 });
            DrawCircleLines(static_cast<int>(mid.x), static_cast<int>(mid.y), o.hazardRadius, WHITE);
        }
    }
}

void GhostGame::RenderCollectibles() const {
    for (const auto& c : collectibles_) {
        if (!c.active) continue;
        if (c.kind == CollectibleKind::MOTE_OF_LIGHT) {
            DrawCircleGradient(static_cast<int>(c.pos.x), static_cast<int>(c.pos.y), c.radius * 1.7f,
                               { 130, 230, 255, 160 }, { 80, 180, 255, 0 });
            DrawPoly(c.pos, 4, c.radius, globalTime_ * 120.0f, WHITE);
            DrawPolyLinesEx(c.pos, 4, c.radius + 3.0f, globalTime_ * 120.0f, 2.0f, SKYBLUE);
        } else if (c.kind == CollectibleKind::EXOTIC_ENGRAM) {
            DrawCircleGradient(static_cast<int>(c.pos.x), static_cast<int>(c.pos.y), c.radius * 2.1f,
                               { 255, 215, 40, 190 }, { 255, 140, 0, 0 });
            DrawPoly(c.pos, 6, c.radius, globalTime_ * 80.0f, GOLD);
            DrawPolyLinesEx(c.pos, 6, c.radius + 2.0f, globalTime_ * 80.0f, 2.5f, WHITE);
        }
    }
}

void GhostGame::RenderPatrolEnemies() const {
    for (const auto& e : enemies_) {
        if (!e.active) continue;
        Vector2 ep = e.pos;

        if (e.type == EnemyType::FALLEN_SHANK) {
            // Hovering orange Fallen Shank drone with twin plasma vents
            DrawRectangle(static_cast<int>(ep.x - 18), static_cast<int>(ep.y - 10), 36, 20, { 42, 46, 56, 255 });
            DrawRectangleLines(static_cast<int>(ep.x - 18), static_cast<int>(ep.y - 10), 36, 20, e.color);
            DrawCircleV({ ep.x, ep.y }, 6.0f, RED);
            DrawRectangle(static_cast<int>(ep.x - 22), static_cast<int>(ep.y + 4), 6, 8, SKYBLUE);
            DrawRectangle(static_cast<int>(ep.x + 16), static_cast<int>(ep.y + 4), 6, 8, SKYBLUE);
        } else if (e.type == EnemyType::VEX_SUPPLICANT) {
            // Explosive Vex Supplicant harpy with pulsing cyan chassis
            DrawPoly(ep, 3, 20.0f, 0.0f, { 35, 50, 65, 255 });
            DrawPolyLinesEx(ep, 3, 22.0f, 0.0f, 2.5f, e.color);
            DrawCircleGradient(static_cast<int>(ep.x), static_cast<int>(ep.y), 8.0f, WHITE, e.color);
        } else if (e.type == EnemyType::HIVE_THRALL) {
            // Hive Thrall leaping shadow claw
            DrawPoly(ep, 5, 17.0f, globalTime_ * 120.0f, { 25, 38, 28, 255 });
            DrawPolyLinesEx(ep, 5, 19.0f, globalTime_ * 120.0f, 2.0f, e.color);
            DrawCircleV({ ep.x - 4, ep.y - 3 }, 3.5f, e.color);
            DrawCircleV({ ep.x + 4, ep.y - 3 }, 3.5f, e.color);
            DrawCircleV({ ep.x, ep.y + 4 }, 3.5f, WHITE);
        } else {
            // Taken Psion with shadowy cyan warping aura
            DrawCircleGradient(static_cast<int>(ep.x), static_cast<int>(ep.y), 24.0f, { 220, 245, 255, 120 }, BLANK);
            DrawPoly(ep, 6, 17.0f, -globalTime_ * 60.0f, { 18, 22, 34, 255 });
            DrawPolyLinesEx(ep, 6, 19.0f, -globalTime_ * 60.0f, 2.0f, e.color);
            DrawCircleV(ep, 5.0f, WHITE);
        }
    }
}

void GhostGame::RenderBoss() const {
    if (!boss_.active) return;

    Vector2 bp = boss_.pos;
    Color bossCol = PURPLE;
    switch (boss_.bossType) {
        case 0: bossCol = PURPLE; break;
        case 1: bossCol = GOLD; break;
        case 2: bossCol = GREEN; break;
        case 3: bossCol = MAGENTA; break;
        case 4: bossCol = { 235, 185, 255, 255 }; break;
        case 5: bossCol = SKYBLUE; break;
        case 6: bossCol = { 75, 235, 195, 255 }; break;
        default: bossCol = { 255, 205, 120, 255 }; break;
    }

    if (boss_.shieldImmune) {
        DrawCircleLines(static_cast<int>(bp.x), static_cast<int>(bp.y), 76.0f + std::sin(globalTime_ * 8.0f) * 4.0f, SKYBLUE);
        DrawCircle(static_cast<int>(bp.x), static_cast<int>(bp.y), 74.0f, { 100, 210, 255, 45 });
    }

    if (boss_.bossType == 0) {
        // Sepiks Prime: Round Fallen Servitor chassis + orbital ring + cyclops eye
        DrawCircleV(bp, 52.0f, { 34, 24, 48, 255 });
        DrawCircleLines(static_cast<int>(bp.x), static_cast<int>(bp.y), 52.0f, PURPLE);
        DrawRing(bp, 58.0f, 65.0f, globalTime_ * 45.0f, globalTime_ * 45.0f + 290.0f, 24, { 175, 85, 255, 190 });
        DrawCircleGradient(static_cast<int>(bp.x - 10.0f), static_cast<int>(bp.y), 24.0f, WHITE, PURPLE);
    } else if (boss_.bossType == 1) {
        // The Templar: Vex Hydra square-brass frame + rotating 3-arc shield + red optical eye
        DrawPoly(bp, 4, 52.0f, 45.0f, { 42, 48, 44, 255 });
        DrawPolyLinesEx(bp, 4, 52.0f, 45.0f, 3.5f, GOLD);
        for (int a = 0; a < 3; ++a) {
            float startA = globalTime_ * 65.0f + a * 120.0f;
            DrawRing(bp, 58.0f, 65.0f, startA, startA + 72.0f, 12, { 70, 235, 255, 210 });
        }
        DrawCircleGradient(static_cast<int>(bp.x - 8.0f), static_cast<int>(bp.y), 20.0f, WHITE, RED);
    } else if (boss_.bossType == 2) {
        // Savathun's Song: 6 opening Hive Shrieker petals + glowing Soulfire core
        float open = 8.0f + std::sin(globalTime_ * 4.5f) * 6.0f;
        for (int p = 0; p < 6; ++p) {
            float ang = (p * 60.0f + globalTime_ * 22.0f) * DEG2RAD;
            Vector2 pc = { bp.x + std::cos(ang) * (28.0f + open), bp.y + std::sin(ang) * (28.0f + open) };
            DrawPoly(pc, 3, 22.0f, p * 60.0f + globalTime_ * 22.0f, { 42, 54, 46, 255 });
            DrawPolyLinesEx(pc, 3, 22.0f, p * 60.0f + globalTime_ * 22.0f, 2.5f, GREEN);
        }
        DrawCircleGradient(static_cast<int>(bp.x), static_cast<int>(bp.y), 24.0f, WHITE, GREEN);
    } else if (boss_.bossType == 3) {
        // Taken Subjugator: Tall jagged obsidian diamond + orbiting blight orbs
        DrawPoly(bp, 4, 62.0f, 0.0f, { 16, 14, 24, 255 });
        DrawPolyLinesEx(bp, 4, 62.0f, 0.0f, 3.5f, WHITE);
        for (int o = 0; o < 3; ++o) {
            float a = (o * 120.0f - globalTime_ * 95.0f) * DEG2RAD;
            DrawCircleV({ bp.x + std::cos(a) * 58.0f, bp.y + std::sin(a) * 58.0f }, 9.0f, MAGENTA);
        }
        DrawCircleGradient(static_cast<int>(bp.x), static_cast<int>(bp.y), 20.0f, WHITE, MAGENTA);
    } else if (boss_.bossType == 4) {
        // Dul Incaru: Awoken starlight hexagon + gilded crescent wings
        DrawTriangle({ bp.x + 12.0f, bp.y - 68.0f }, { bp.x - 18.0f, bp.y - 16.0f }, { bp.x + 42.0f, bp.y - 16.0f }, { 92, 48, 128, 220 });
        DrawTriangle({ bp.x + 12.0f, bp.y + 68.0f }, { bp.x + 42.0f, bp.y + 16.0f }, { bp.x - 18.0f, bp.y + 16.0f }, { 92, 48, 128, 220 });
        DrawPoly(bp, 6, 42.0f, 30.0f, { 38, 24, 64, 255 });
        DrawPolyLinesEx(bp, 6, 42.0f, 30.0f, 3.0f, GOLD);
        DrawCircleGradient(static_cast<int>(bp.x), static_cast<int>(bp.y), 20.0f, WHITE, bossCol);
    } else if (boss_.bossType == 5) {
        // Atraks-1: Sleek BrayTech white/cobalt Exo frame + twin glowing scanner eyes
        DrawPoly(bp, 6, 52.0f, 0.0f, { 215, 230, 245, 255 });
        DrawPolyLinesEx(bp, 6, 52.0f, 0.0f, 3.5f, SKYBLUE);
        DrawCircle(static_cast<int>(bp.x - 16.0f), static_cast<int>(bp.y - 10.0f), 7.5f, { 25, 185, 255, 255 });
        DrawCircle(static_cast<int>(bp.x - 16.0f), static_cast<int>(bp.y + 10.0f), 7.5f, { 25, 185, 255, 255 });
    } else if (boss_.bossType == 6) {
        // Savathun: Ornate Lucent Moth Wings + Hive Crown
        DrawTriangle({ bp.x + 10.0f, bp.y - 72.0f }, { bp.x - 35.0f, bp.y }, { bp.x + 45.0f, bp.y - 8.0f }, { 75, 235, 195, 165 });
        DrawTriangle({ bp.x + 10.0f, bp.y + 72.0f }, { bp.x + 45.0f, bp.y + 8.0f }, { bp.x - 35.0f, bp.y }, { 75, 235, 195, 165 });
        DrawPoly(bp, 5, 40.0f, 180.0f, { 112, 118, 104, 255 });
        DrawPolyLinesEx(bp, 5, 40.0f, 180.0f, 3.0f, GOLD);
        DrawCircleGradient(static_cast<int>(bp.x - 6.0f), static_cast<int>(bp.y), 18.0f, WHITE, { 75, 235, 195, 255 });
    } else {
        // The Witness: Dark monolith + prismatic resonance rings + multi-layered crown halo
        DrawRectangle(static_cast<int>(bp.x - 28.0f), static_cast<int>(bp.y - 54.0f), 56, 108, { 18, 16, 26, 255 });
        DrawRectangleLinesEx({ bp.x - 28.0f, bp.y - 54.0f, 56.0f, 108.0f }, 3.0f, GOLD);
        DrawRing(bp, 62.0f, 67.0f, globalTime_ * 80.0f, globalTime_ * 80.0f + 260.0f, 24, SKYBLUE);
        DrawRing(bp, 70.0f, 74.0f, -globalTime_ * 60.0f, -globalTime_ * 60.0f + 240.0f, 24, MAGENTA);
        DrawCircleGradient(static_cast<int>(bp.x), static_cast<int>(bp.y - 16.0f), 20.0f, WHITE, GOLD);
    }

    float barW = 540.0f;
    float barX = (VIRTUAL_WIDTH - barW) * 0.5f;
    float barY = 64.0f;
    DrawRectangle(static_cast<int>(barX - 4), static_cast<int>(barY - 26), static_cast<int>(barW + 8), 52, { 10, 12, 20, 210 });
    DrawText(boss_.name.c_str(), static_cast<int>(barX + 6), static_cast<int>(barY - 22), 18, GOLD);

    float hpRatio = std::clamp(boss_.health / boss_.maxHealth, 0.0f, 1.0f);
    DrawRectangle(static_cast<int>(barX), static_cast<int>(barY), static_cast<int>(barW), 16, { 45, 25, 25, 255 });
    DrawRectangle(static_cast<int>(barX), static_cast<int>(barY), static_cast<int>(barW * hpRatio), 16,
                  boss_.shieldImmune ? SKYBLUE : ORANGE);
    DrawRectangleLinesEx({ barX, barY, barW, 16.0f }, 1.5f, WHITE);

    if (boss_.shieldImmune) {
        DrawText("OVERSHIELD ACTIVE // FIRE DRONE [C / X] OR SUPER TO SHATTER", static_cast<int>(barX + 35), static_cast<int>(barY + 20), 14, SKYBLUE);
    } else {
        DrawText("SHIELD BROKEN // FINISH WITH DRONE & LIGHT ABILITIES!", static_cast<int>(barX + 60), static_cast<int>(barY + 20), 14, GOLD);
    }
}

void GhostGame::RenderProjectiles() const {
    for (const auto& pr : projectiles_) {
        if (!pr.active) continue;
        DrawCircleGradient(static_cast<int>(pr.pos.x), static_cast<int>(pr.pos.y), pr.radius * 1.8f, pr.color, BLANK);
        DrawCircleV(pr.pos, pr.radius * 0.7f, WHITE);
    }
}

void GhostGame::RenderParticles() const {
    for (const auto& p : particles_) {
        if (!p.active) continue;
        float ratio = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        Color c = p.color;
        c.a = static_cast<unsigned char>(ratio * 240.0f);
        DrawCircleV(p.pos, p.size * ratio, c);
    }
}

void GhostGame::RenderGhostRig(Vector2 pos, float radius, float rotDeg, float expand, float spinAngle,
                               Vector2 eyeOffset, int shellIdx, float alpha) const {
    const auto& shell = GetShellCatalog()[std::clamp(shellIdx, 0, static_cast<int>(NUM_SHELLS) - 1)];

    auto ApplyAlpha = [alpha](Color c) -> Color {
        c.a = static_cast<unsigned char>(c.a * alpha);
        return c;
    };

    Color primary = ApplyAlpha(shell.primaryColor);
    Color secondary = ApplyAlpha(shell.secondaryColor);
    Color coreCol = ApplyAlpha(shell.coreColor);

    // If a custom shader is equipped, tint primary and secondary colors!
    if (profile_.equippedShader > 0 && profile_.equippedShader < static_cast<int>(NUM_SHADERS)) {
        const auto& shd = GetShaderCatalog()[profile_.equippedShader];
        primary = ApplyAlpha(shd.primary);
        secondary = ApplyAlpha(shd.secondary);
        coreCol = ApplyAlpha(shd.core);
    }

    float rotRad = rotDeg * DEG2RAD;
    float wingDist = radius * (1.18f + expand * 0.42f);

    // Background / Halo accents per shell type
    if (shell.finShape == 1) {
        // Sagira's Shell: Geometric Osiris brass lattice ring
        DrawPolyLinesEx(pos, 8, radius * (1.72f + expand * 0.25f), -spinAngle * 25.0f, 2.0f, primary);
    } else if (shell.finShape == 5) {
        // Flawless Trials Shell: Radiant golden Sun of Osiris disc
        DrawCircleGradient(static_cast<int>(pos.x), static_cast<int>(pos.y), radius * 1.95f,
                           ApplyAlpha({ 255, 215, 65, 110 }), BLANK);
        DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), radius * 1.55f, primary);
    } else if (shell.finShape == 7) {
        // Cosmos Exotic Shell: Nebula glow + orbiting miniature moons
        DrawCircleGradient(static_cast<int>(pos.x), static_cast<int>(pos.y), radius * 2.0f,
                           ApplyAlpha({ 145, 70, 255, 130 }), BLANK);
        for (int m = 0; m < 3; ++m) {
            float ma = spinAngle * 1.8f + m * 2.094f;
            Vector2 mp = { pos.x + std::cos(ma) * radius * 1.75f, pos.y + std::sin(ma) * radius * 1.15f };
            DrawCircleV(mp, radius * 0.18f, secondary);
            DrawCircleLines(static_cast<int>(mp.x), static_cast<int>(mp.y), radius * 0.26f, coreCol);
        }
    } else if (shell.finShape == 9) {
        // Armory Forged Shell: Bergusia radiance phase rings
        DrawPolyLinesEx(pos, 6, radius * (1.70f + expand * 0.25f), spinAngle * 35.0f, 2.2f, coreCol);
        DrawPolyLinesEx(pos, 6, radius * (1.48f + expand * 0.15f), -spinAngle * 35.0f, 1.5f, secondary);
    } else if (shell.finShape == 10) {
        // Taken King Shell: Ascendant Blight shadow-fire aura
        DrawCircleGradient(static_cast<int>(pos.x), static_cast<int>(pos.y), radius * 2.15f,
                           ApplyAlpha({ 180, 245, 255, 135 }), ApplyAlpha({ 10, 18, 28, 0 }));
    } else if (shell.finShape == 11) {
        // Traveler's Chosen Shell: Pure Light golden filigree halo
        DrawCircleGradient(static_cast<int>(pos.x), static_cast<int>(pos.y), radius * 2.1f,
                           ApplyAlpha({ 200, 245, 255, 140 }), BLANK);
        DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), radius * (1.68f + expand * 0.25f), secondary);
    }

    if (shell.finShape == 2) {
        // Tyrant Shell: Rasputin Warmind heavy 4-plate octahedron + 4 IKELOS telemetry nodes
        for (int i = 0; i < 4; ++i) {
            float baseAngle = i * (3.14159265f * 0.5f) + 0.785398f + spinAngle * 0.2f + rotRad;
            Vector2 dir = { std::cos(baseAngle), std::sin(baseAngle) };
            Vector2 perp = { -dir.y, dir.x };
            Vector2 finCenter = Vector2Add(pos, Vector2Scale(dir, wingDist * 1.05f));
            Vector2 pTip = Vector2Add(finCenter, Vector2Scale(dir, radius * 0.85f));
            Vector2 pIn  = Vector2Subtract(finCenter, Vector2Scale(dir, radius * 0.42f));
            Vector2 pL   = Vector2Add(finCenter, Vector2Scale(perp, radius * 0.58f));
            Vector2 pR   = Vector2Subtract(finCenter, Vector2Scale(perp, radius * 0.58f));
            DrawTriangle(pTip, pR, pL, primary);
            DrawTriangle(pIn, pL, pR, primary);
            DrawLineEx(pTip, pL, 2.0f, secondary);
            DrawLineEx(pTip, pR, 2.0f, secondary);
        }
    } else if (shell.finShape == 3) {
        // Harpy Shell: 6 Vex brass curved fins + radiolarian conduits
        for (int i = 0; i < 6; ++i) {
            float baseAngle = i * (3.14159265f / 3.0f) + spinAngle * 0.28f + rotRad;
            Vector2 dir = { std::cos(baseAngle), std::sin(baseAngle) };
            Vector2 perp = { -dir.y, dir.x };
            Vector2 finCenter = Vector2Add(pos, Vector2Scale(dir, wingDist * 1.08f));
            Vector2 pTip = Vector2Add(finCenter, Vector2Scale(dir, radius * 0.82f));
            Vector2 pBaseL = Vector2Add(Vector2Subtract(finCenter, Vector2Scale(dir, radius * 0.30f)), Vector2Scale(perp, radius * 0.44f));
            Vector2 pBaseR = Vector2Subtract(Vector2Subtract(finCenter, Vector2Scale(dir, radius * 0.30f)), Vector2Scale(perp, radius * 0.22f));
            DrawTriangle(pTip, pBaseR, pBaseL, (i % 2 == 0) ? primary : secondary);
            DrawTriangleLines(pTip, pBaseR, pBaseL, coreCol);
        }
    } else if (shell.finShape == 6) {
        // Nine Lives Shell (Cat Ghost): Smooth feline shell + Cat Ears on top + Whiskers!
        for (int s = -1; s <= 1; s += 2) {
            float earAngle = (-1.570796f + s * 0.62f) + rotRad;
            Vector2 dir = { std::cos(earAngle), std::sin(earAngle) };
            Vector2 perp = { -dir.y, dir.x };
            Vector2 earBase = Vector2Add(pos, Vector2Scale(dir, radius * (0.95f + expand * 0.25f)));
            Vector2 earTip  = Vector2Add(earBase, Vector2Scale(dir, radius * 0.92f));
            Vector2 earL    = Vector2Add(earBase, Vector2Scale(perp, radius * 0.48f));
            Vector2 earR    = Vector2Subtract(earBase, Vector2Scale(perp, radius * 0.48f));
            DrawTriangle(earTip, earR, earL, primary);
            // Pink inner ear
            Vector2 inTip = Vector2Add(earBase, Vector2Scale(dir, radius * 0.68f));
            Vector2 inL   = Vector2Add(earBase, Vector2Scale(perp, radius * 0.26f));
            Vector2 inR   = Vector2Subtract(earBase, Vector2Scale(perp, radius * 0.26f));
            DrawTriangle(inTip, inR, inL, ApplyAlpha({ 255, 155, 175, 255 }));

            // Whiskers on cheeks
            for (int w = -1; w <= 1; ++w) {
                float wa = (s > 0 ? 0.0f : 3.14159265f) + w * 0.22f + rotRad;
                Vector2 wd = { std::cos(wa), std::sin(wa) };
                Vector2 wStart = Vector2Add(pos, Vector2Scale(wd, radius * 0.85f));
                Vector2 wEnd   = Vector2Add(pos, Vector2Scale(wd, radius * 1.65f));
                DrawLineEx(wStart, wEnd, 1.8f, secondary);
            }
        }
        // Lower collar bell & cheek plates
        for (int i = 0; i < 4; ++i) {
            float a = (0.4f + i * 0.78f) + rotRad;
            Vector2 p = Vector2Add(pos, { std::cos(a) * wingDist * 0.88f, std::sin(a) * wingDist * 0.88f });
            DrawPoly(p, 4, radius * 0.36f, rotDeg, (i % 2 == 0) ? primary : secondary);
        }
        DrawCircleV({ pos.x, pos.y + radius * 0.95f }, radius * 0.24f, ApplyAlpha(GOLD));
    } else {
        // 8-fin radial shells with custom geometry per finShape
        for (int i = 0; i < 8; ++i) {
            float baseAngle = i * (3.14159265f * 0.25f) + spinAngle * ((i % 2 == 0) ? 0.35f : -0.25f) + rotRad;
            Vector2 dir = { std::cos(baseAngle), std::sin(baseAngle) };
            Vector2 perp = { -dir.y, dir.x };

            float d = (i % 2 == 0) ? wingDist : wingDist * 0.88f;
            Vector2 finCenter = Vector2Add(pos, Vector2Scale(dir, d));

            float tipLen = radius * 0.58f;
            float halfW = radius * 0.38f;
            if (shell.finShape == 1 || shell.finShape == 5) {
                tipLen = radius * 0.88f;
                halfW = radius * 0.30f;
            } else if (shell.finShape == 4 || shell.finShape == 10) {
                tipLen = radius * ((i % 2 == 0) ? 0.95f : 0.52f);
                halfW = radius * 0.28f;
            } else if (shell.finShape == 8) {
                tipLen = radius * 0.75f;
                halfW = radius * 0.42f;
            } else if (shell.finShape == 11) {
                tipLen = radius * 0.46f;
                halfW = radius * 0.48f;
            }

            Vector2 pTip = Vector2Add(finCenter, Vector2Scale(dir, tipLen));
            Vector2 pBaseL = Vector2Add(Vector2Subtract(finCenter, Vector2Scale(dir, radius * 0.28f)), Vector2Scale(perp, halfW));
            Vector2 pBaseR = Vector2Subtract(Vector2Subtract(finCenter, Vector2Scale(dir, radius * 0.28f)), Vector2Scale(perp, halfW));

            Color finCol = (i % 2 == 0) ? primary : secondary;
            DrawTriangle(pTip, pBaseR, pBaseL, finCol);
            Color outlineCol = (shell.finShape == 8) ? secondary :
                               (shell.finShape == 10) ? coreCol : ApplyAlpha({ 20, 28, 38, 220 });
            DrawTriangleLines(pTip, pBaseR, pBaseL, outlineCol);

            if (shell.finShape == 4 && (i % 2 == 0)) {
                // Hive Soulfire rune orbs between bone spikes
                DrawCircleV(Vector2Add(pTip, Vector2Scale(dir, radius * 0.16f)), radius * 0.12f, coreCol);
            } else if (shell.finShape == 8) {
                // SIVA crimson tendril lines from core to each nanite plate
                DrawLineEx(pos, finCenter, 2.0f, secondary);
            }
        }
    }

    // Core Housing Sphere
    Color housingCol = (shell.finShape == 10) ? ApplyAlpha({ 8, 10, 16, 255 }) :
                       (shell.finShape == 11) ? ApplyAlpha({ 240, 244, 255, 255 }) :
                       ApplyAlpha({ 20, 24, 32, 255 });
    DrawCircleV(pos, radius * 0.72f, housingCol);
    DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), radius * 0.74f, secondary);

    // Iris & Pupil
    Vector2 irisPos = Vector2Add(pos, eyeOffset);
    DrawCircleGradient(static_cast<int>(irisPos.x), static_cast<int>(irisPos.y), radius * 0.55f, coreCol, BLANK);

    if (shell.finShape == 6) {
        // Cat vertical slit pupil
        DrawCircleV(irisPos, radius * 0.34f, coreCol);
        DrawRectanglePro({ irisPos.x, irisPos.y, radius * 0.14f, radius * 0.52f },
                         { radius * 0.07f, radius * 0.26f }, rotDeg, ApplyAlpha(BLACK));
    } else if (shell.finShape == 3 || shell.finShape == 1) {
        // Vex / Osiris triangular eye
        DrawPoly(irisPos, 3, radius * 0.38f, rotDeg - 90.0f, coreCol);
        DrawPoly(irisPos, 3, radius * 0.19f, rotDeg - 90.0f, ApplyAlpha(WHITE));
    } else if (shell.finShape == 4) {
        // Hive 3-eye cluster
        DrawCircleV({ irisPos.x - radius * 0.16f, irisPos.y + radius * 0.10f }, radius * 0.14f, coreCol);
        DrawCircleV({ irisPos.x + radius * 0.16f, irisPos.y + radius * 0.10f }, radius * 0.14f, coreCol);
        DrawCircleV({ irisPos.x, irisPos.y - radius * 0.16f }, radius * 0.16f, ApplyAlpha(WHITE));
    } else {
        DrawPoly(irisPos, 4, radius * 0.36f, rotDeg, coreCol);
        DrawPoly(irisPos, 4, radius * 0.19f, rotDeg, ApplyAlpha(WHITE));
    }
}

void GhostGame::RenderHud() const {
    Color subCol = GetSubclassAccentColor();

    DrawRectangle(22, 26, 350, 86, { 10, 14, 24, 200 });
    DrawRectangleLinesEx({ 22.0f, 26.0f, 350.0f, 86.0f }, 1.5f, { 255, 255, 255, 65 });

    Vector2 iconCenter = { 62.0f, 68.0f };
    float superRatio = isNightfallMode_ ? std::clamp(ghost_.superEnergy / 100.0f, 0.0f, 1.0f) : 0.0f;
    DrawPoly(iconCenter, 4, 26.0f, 0.0f, (isNightfallMode_ && superRatio >= 1.0f) ? GOLD : subCol);
    DrawPolyLinesEx(iconCenter, 4, 29.0f, 0.0f, 2.0f, WHITE);

    const char* subName = "VOIDWALKER // BLINK";
    switch (static_cast<LightSubclass>(profile_.selectedSubclass)) {
        case LightSubclass::VOID_WALKER:            subName = "VOIDWALKER // BLINK"; break;
        case LightSubclass::SOLAR_DAWN:             subName = "DAWNBLADE // ICARUS"; break;
        case LightSubclass::ARC_CALLER:             subName = "STORMCALLER // IONIC"; break;
        case LightSubclass::STASIS_SHADEBINDER:     subName = "SHADEBINDER // STASIS"; break;
        case LightSubclass::STRAND_RUNNER:          subName = "THREADRUNNER // GRAPPLE"; break;
        case LightSubclass::PRISMATIC_TRANSCENDENT: subName = "TRANSCENDENT // PRISMATIC"; break;
    }
    DrawText(subName, 102, 34, 16, WHITE);

    bool tacCanCast = (ghost_.tacticalCharges > 0);
    float tacRechargeFrac = 1.0f - std::clamp(ghost_.tacticalCooldown / std::max(0.05f, ghost_.tacticalMaxCooldown), 0.0f, 1.0f);
    float tacReady = (ghost_.tacticalCharges >= ghost_.maxTacticalCharges) ? 1.0f : tacRechargeFrac;
    char tacStatusTxt[32];
    if (ghost_.maxTacticalCharges > 1) {
        if (ghost_.tacticalCharges >= 2) std::snprintf(tacStatusTxt, sizeof(tacStatusTxt), "BLINK x2 READY");
        else if (ghost_.tacticalCharges == 1) std::snprintf(tacStatusTxt, sizeof(tacStatusTxt), "BLINK x1 READY");
        else std::snprintf(tacStatusTxt, sizeof(tacStatusTxt), "CHARGING...");
    } else {
        std::snprintf(tacStatusTxt, sizeof(tacStatusTxt), "%s", tacCanCast ? "BLINK READY" : "CHARGING...");
    }

    if (isNightfallMode_) {
        DrawRectangle(102, 56, 245, 16, { 30, 36, 48, 255 });
        Color superBarCol = (ghost_.superActiveTimer > 0.0f) ? WHITE :
                            (superRatio >= 1.0f) ? GOLD : subCol;
        DrawRectangle(102, 56, static_cast<int>(245.0f * superRatio), 16, superBarCol);
        DrawRectangleLines(102, 56, 245, 16, WHITE);

        if (ghost_.superActiveTimer > 0.0f) {
            DrawText("SUPER ACTIVE!", 160, 57, 14, BLACK);
        } else if (superRatio >= 1.0f) {
            DrawText("SUPER READY", 164, 57, 14, BLACK);
        }

        DrawRectangle(102, 80, 132, 10, { 30, 36, 48, 255 });
        DrawRectangle(102, 80, static_cast<int>(132.0f * tacReady), 10, tacCanCast ? subCol : GRAY);
        DrawText(tacStatusTxt, 242, 78, 12, tacCanCast ? WHITE : LIGHTGRAY);
    } else {
        DrawText("PATROL // TACTICAL FLIGHT MODE", 102, 56, 13, SKYBLUE);
        DrawRectangle(102, 78, 145, 12, { 30, 36, 48, 255 });
        DrawRectangle(102, 78, static_cast<int>(145.0f * tacReady), 12, tacCanCast ? subCol : GRAY);
        DrawRectangleLines(102, 78, 145, 12, { 255, 255, 255, 120 });
        DrawText(tacStatusTxt, 254, 78, 12, tacCanCast ? WHITE : LIGHTGRAY);
    }

    for (int i = 0; i < ghost_.overshieldCharges; ++i) {
        DrawPoly({ 36.0f + i * 24.0f, 126.0f }, 6, 9.0f, 0.0f, SKYBLUE);
    }

    // Top-Right Score, Glimmer, Shards & Streak Tracker
    float hudBoxH = isNightfallMode_ ? 104.0f : 86.0f;
    DrawRectangle(VIRTUAL_WIDTH - 295, 26, 273, static_cast<int>(hudBoxH), { 10, 14, 24, 200 });
    DrawRectangleLinesEx({ VIRTUAL_WIDTH - 295.0f, 26.0f, 273.0f, hudBoxH }, 1.5f, { 255, 255, 255, 65 });

    char scoreBuf[64];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "GATES: %d  (x%d)", scoreGates_, streakGates_);
    DrawText(scoreBuf, VIRTUAL_WIDTH - 278, 33, 23, WHITE);

    char glimBuf[64];
    std::snprintf(glimBuf, sizeof(glimBuf), "GLIMMER: %d (+%d)", profile_.glimmer, runGlimmerEarned_);
    DrawText(glimBuf, VIRTUAL_WIDTH - 278, 62, 15, SKYBLUE);

    if (isNightfallMode_) {
        char shardBuf[64];
        std::snprintf(shardBuf, sizeof(shardBuf), "SHARDS: %d (+%d)", profile_.legendaryShards, runShardsEarned_);
        DrawText(shardBuf, VIRTUAL_WIDTH - 278, 82, 15, Color{ 215, 135, 255, 255 });
        DrawText("MODE: NIGHTFALL STRIKE", VIRTUAL_WIDTH - 278, 103, 13, GOLD);
    } else {
        DrawText("MODE: VANGUARD PATROL", VIRTUAL_WIDTH - 278, 86, 14, LIGHTGRAY);
    }

    // On-Screen Holographic Touch Pause Button (Mobile-Friendly)
    Rectangle pauseBtn = { VIRTUAL_WIDTH - 362.0f, 26.0f, 54.0f, 54.0f };
    bool pausePressed = (touchFeedbackPause_ > 0.0f);
    bool pauseHover = IsRectHovered(pauseBtn) || pausePressed;
    DrawRectangleRec(pauseBtn, pausePressed ? Color{ 55, 80, 120, 255 } : (pauseHover ? Color{ 30, 48, 72, 230 } : Color{ 14, 20, 32, 200 }));
    DrawRectangleLinesEx(pauseBtn, pausePressed ? 2.5f : 1.8f, pausePressed ? GOLD : (pauseHover ? WHITE : SKYBLUE));
    DrawRectangle(static_cast<int>(pauseBtn.x + 17), static_cast<int>(pauseBtn.y + 13), 6, 22, pausePressed ? GOLD : WHITE);
    DrawRectangle(static_cast<int>(pauseBtn.x + 31), static_cast<int>(pauseBtn.y + 13), 6, 22, pausePressed ? GOLD : WHITE);
    DrawText("PAUSE", static_cast<int>(pauseBtn.x + 11), static_cast<int>(pauseBtn.y + 38), 9, pausePressed ? GOLD : LIGHTGRAY);

    // Vanguard Streak Medal Callout Popup
    if (activeMedal_.active) {
        char medTxt[128];
        if (activeMedal_.bonusGlimmer > 0) {
            std::snprintf(medTxt, sizeof(medTxt), "%s  (+%d GLIMMER)", activeMedal_.medalName.c_str(), activeMedal_.bonusGlimmer);
        } else {
            std::snprintf(medTxt, sizeof(medTxt), "%s", activeMedal_.medalName.c_str());
        }
        int mw = MeasureText(medTxt, 20);
        DrawRectangle(VIRTUAL_WIDTH / 2 - mw / 2 - 20, 28, mw + 40, 34, { 14, 20, 34, 235 });
        DrawRectangleLines(VIRTUAL_WIDTH / 2 - mw / 2 - 20, 28, mw + 40, 34, activeMedal_.color);
        DrawText(medTxt, VIRTUAL_WIDTH / 2 - mw / 2, 36, 20, activeMedal_.color);
    }

    // Touch / Mouse On-Screen Action Buttons (Bottom-Right or Bottom-Left if left-handed)
    // Patrol mode has NO weapons or scan; Nightfall mode shows the manual [FIRE DRONE] button!
    float btnStartX = profile_.leftHandedControls ? 20.0f : (VIRTUAL_WIDTH - 378.0f);
    Rectangle drnBtn = { btnStartX, VIRTUAL_HEIGHT - 92.0f, 112.0f, 62.0f };
    Rectangle tacBtn = isNightfallMode_
                     ? Rectangle{ btnStartX + 122.0f, VIRTUAL_HEIGHT - 92.0f, 112.0f, 62.0f }
                     : Rectangle{ profile_.leftHandedControls ? 20.0f : (VIRTUAL_WIDTH - 134.0f), VIRTUAL_HEIGHT - 92.0f, 112.0f, 62.0f };
    Rectangle supBtn = { btnStartX + 244.0f, VIRTUAL_HEIGHT - 92.0f, 112.0f, 62.0f };

    bool hasGamepad = IsGamepadAvailable(0);
#if defined(PLATFORM_ANDROID)
    bool showKeyHints = false;
#else
    bool showKeyHints = (activeInputDevice_ != "TOUCH SCREEN");
#endif
    const char* drnHint = hasGamepad ? "(X) / RT" : "[C]";
    const char* tacHint = hasGamepad ? "(B) / LB" : "[SHIFT]";
    const char* supHint = hasGamepad ? "(Y) / RB" : "[F]";

    auto drawCenteredBtnText = [&](Rectangle r, const char* label, const char* keyHint, Color labelCol) {
        if (showKeyHints) {
            int lw = MeasureText(label, 15);
            DrawText(label, static_cast<int>(r.x + (r.width - lw) * 0.5f), static_cast<int>(r.y + 9), 15, labelCol);
            int kw = MeasureText(keyHint, 12);
            DrawText(keyHint, static_cast<int>(r.x + (r.width - kw) * 0.5f), static_cast<int>(r.y + 29), 12, LIGHTGRAY);
        } else {
            int lw = MeasureText(label, 16);
            DrawText(label, static_cast<int>(r.x + (r.width - lw) * 0.5f), static_cast<int>(r.y + 16), 16, labelCol);
        }
    };

    if (isNightfallMode_ && profile_.equippedDrone >= 0 && profile_.equippedDrone < static_cast<int>(NUM_DRONES)) {
        const auto& droneDef = GetDroneCatalog()[profile_.equippedDrone];
        float drnReady = 1.0f - std::clamp(ghost_.droneCooldown / std::max(0.05f, ghost_.droneMaxCooldown), 0.0f, 1.0f);
        bool drnFeedback = (touchFeedbackDrn_ > 0.0f);
        DrawRectangleRec(drnBtn, drnFeedback ? Color{ 45, 85, 120, 255 } : ((drnReady >= 1.0f) ? Color{ 28, 52, 74, 225 } : Color{ 22, 26, 34, 185 }));
        DrawRectangleLinesEx(drnBtn, drnFeedback ? 2.5f : 2.0f, drnFeedback ? WHITE : ((drnReady >= 1.0f) ? droneDef.color : GRAY));
        drawCenteredBtnText(drnBtn, "DRONE", drnHint, WHITE);
        DrawRectangle(static_cast<int>(drnBtn.x + 10), static_cast<int>(drnBtn.y + 48), 92, 6, { 18, 22, 30, 255 });
        DrawRectangle(static_cast<int>(drnBtn.x + 10), static_cast<int>(drnBtn.y + 48), static_cast<int>(92.0f * drnReady), 6, droneDef.color);
    }

    bool tacFeedback = (touchFeedbackTac_ > 0.0f);
    DrawRectangleRec(tacBtn, tacFeedback ? Color{ 55, 105, 155, 255 } : (tacCanCast ? Color{ 32, 58, 86, 220 } : Color{ 25, 28, 35, 180 }));
    DrawRectangleLinesEx(tacBtn, tacFeedback ? 2.5f : 2.0f, tacFeedback ? WHITE : (tacCanCast ? subCol : GRAY));
    drawCenteredBtnText(tacBtn, (ghost_.tacticalCharges >= 2) ? "BLINK x2" : "BLINK", tacHint, tacCanCast ? WHITE : LIGHTGRAY);
    if (ghost_.maxTacticalCharges == 2) {
        float seg0 = (ghost_.tacticalCharges >= 1) ? 1.0f : tacRechargeFrac;
        float seg1 = (ghost_.tacticalCharges >= 2) ? 1.0f : ((ghost_.tacticalCharges == 1) ? tacRechargeFrac : 0.0f);
        DrawRectangle(static_cast<int>(tacBtn.x + 10), static_cast<int>(tacBtn.y + 48), 44, 6, { 18, 22, 30, 255 });
        DrawRectangle(static_cast<int>(tacBtn.x + 58), static_cast<int>(tacBtn.y + 48), 44, 6, { 18, 22, 30, 255 });
        DrawRectangle(static_cast<int>(tacBtn.x + 10), static_cast<int>(tacBtn.y + 48), static_cast<int>(44.0f * seg0), 6, (seg0 >= 1.0f) ? subCol : GRAY);
        DrawRectangle(static_cast<int>(tacBtn.x + 58), static_cast<int>(tacBtn.y + 48), static_cast<int>(44.0f * seg1), 6, (seg1 >= 1.0f) ? subCol : GRAY);
    } else {
        DrawRectangle(static_cast<int>(tacBtn.x + 10), static_cast<int>(tacBtn.y + 48), 92, 6, { 18, 22, 30, 255 });
        DrawRectangle(static_cast<int>(tacBtn.x + 10), static_cast<int>(tacBtn.y + 48), static_cast<int>(92.0f * tacReady), 6, tacCanCast ? subCol : GRAY);
    }

    if (isNightfallMode_) {
        bool supFeedback = (touchFeedbackSup_ > 0.0f);
        DrawRectangleRec(supBtn, supFeedback ? Color{ 145, 115, 35, 255 } : ((superRatio >= 1.0f) ? Color{ 95, 75, 20, 230 } : Color{ 25, 28, 35, 180 }));
        DrawRectangleLinesEx(supBtn, supFeedback ? 2.5f : 2.0f, supFeedback ? WHITE : ((superRatio >= 1.0f) ? GOLD : GRAY));
        drawCenteredBtnText(supBtn, "SUPER", supHint, (superRatio >= 1.0f || supFeedback) ? GOLD : LIGHTGRAY);
        DrawRectangle(static_cast<int>(supBtn.x + 10), static_cast<int>(supBtn.y + 48), 92, 6, { 18, 22, 30, 255 });
        DrawRectangle(static_cast<int>(supBtn.x + 10), static_cast<int>(supBtn.y + 48), static_cast<int>(92.0f * superRatio), 6, (superRatio >= 1.0f) ? GOLD : subCol);
    }

    // First-Run Control Hint Pill (fades cleanly after 4.5s on a player's very first run)
    if (profile_.totalRunsStarted <= 1 && runTime_ < 4.5f) {
        float fade = (runTime_ > 3.5f) ? std::clamp(1.0f - (runTime_ - 3.5f), 0.0f, 1.0f) : 1.0f;
        unsigned char bgA = static_cast<unsigned char>(fade * 210.0f);
        unsigned char txtA = static_cast<unsigned char>(fade * 245.0f);
#if defined(PLATFORM_ANDROID)
        const char* hint = isNightfallMode_
            ? "CONTROLS: Tap Screen = Flap  |  [BLINK] & [SUPER]: Abilities  |  [DRONE]: Shoot"
            : "CONTROLS: Tap Anywhere = Flap  |  Tap [BLINK] for Subclass Blink";
#else
        const char* hint = hasGamepad
            ? (isNightfallMode_
                ? "CONTROLS: [(A)] Flap  |  [(B)/LB] Blink  |  [(Y)/RB] Super  |  [(X)/RT] Drone"
                : "CONTROLS: [(A)] Flap  |  [(B)/LB] Subclass Blink")
            : (isNightfallMode_
                ? "CONTROLS: [SPACE / Click] Flap  |  [SHIFT / RMB] Blink  |  [F / Q] Super  |  [C / X] Drone"
                : "CONTROLS: [SPACE / Click] Flap  |  [SHIFT / RMB] Subclass Blink");
#endif
        int hw = MeasureText(hint, 14);
        DrawRectangle(VIRTUAL_WIDTH / 2 - hw / 2 - 18, 86, hw + 36, 32, { 10, 14, 24, bgA });
        DrawRectangleLines(VIRTUAL_WIDTH / 2 - hw / 2 - 18, 86, hw + 36, 32, Color{ 100, 195, 255, txtA });
        DrawText(hint, VIRTUAL_WIDTH / 2 - hw / 2, 95, 14, Color{ 230, 245, 255, txtA });
    }

    if (biomeBannerTimer_ > 0.0f) {
        float a = std::min(1.0f, biomeBannerTimer_ * 1.2f);
        unsigned char alphaByte = static_cast<unsigned char>(a * 235.0f);
        const char* bName = GetBiomeName(GetCurrentBiome());
        int tw = MeasureText(bName, 20);
        DrawRectangle(VIRTUAL_WIDTH / 2 - tw / 2 - 24, 132, tw + 48, 38, { 10, 14, 24, static_cast<unsigned char>(a * 190) });
        DrawLine(VIRTUAL_WIDTH / 2 - tw / 2 - 24, 132, VIRTUAL_WIDTH / 2 + tw / 2 + 24, 132, { 255, 255, 255, alphaByte });
        DrawText(bName, VIRTUAL_WIDTH / 2 - tw / 2, 141, 20, { 255, 255, 255, alphaByte });
    }

    if (unpauseRampTimer_ > 0.0f) {
        const char* resMsg = "RESUMING...";
        int rw = MeasureText(resMsg, 15);
        Rectangle pill = { VIRTUAL_WIDTH * 0.5f - (rw + 36.0f) * 0.5f, 44.0f, rw + 36.0f, 30.0f };
        DrawRectangleRec(pill, { 12, 20, 34, 215 });
        DrawRectangleLinesEx(pill, 1.5f, SKYBLUE);
        DrawText(resMsg, static_cast<int>(pill.x + (pill.width - rw) * 0.5f), static_cast<int>(pill.y + 8.0f), 15, SKYBLUE);
    }
}

void GhostGame::RenderSplashIntro() {
    // Fill screen with vibrant Sebio sky-blue background
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Color{ 128, 219, 255, 255 });

    float t = splashTimer_;
    // Total splash duration: 2.4s (fade in 0.5s, flash 0.6s, hold, fade out 0.5s)
    float alpha = 1.0f;
    if (t < 0.5f) {
        alpha = t / 0.5f;
    } else if (t > 1.9f) {
        alpha = std::clamp((2.4f - t) / 0.5f, 0.0f, 1.0f);
    }

    // Flash glow pulse
    float flash = 0.0f;
    if (t >= 0.5f && t <= 1.1f) {
        flash = std::sin((t - 0.5f) / 0.6f * 3.14159265f);
    }

    if (splashLogo_.id != 0) {
        float scale = 0.92f + flash * 0.05f;
        float w = splashLogo_.width * scale;
        float h = splashLogo_.height * scale;
        Rectangle dest = { (VIRTUAL_WIDTH - w) * 0.5f, (VIRTUAL_HEIGHT - h) * 0.5f, w, h };
        Color logoCol = { 255, 255, 255, static_cast<unsigned char>(255 * alpha) };
        DrawTexturePro(splashLogo_, { 0, 0, (float)splashLogo_.width, (float)splashLogo_.height }, dest, { 0, 0 }, 0.0f, logoCol);
    } else {
        const char* l1 = "SEBINY LABS";
        int w1 = MeasureText(l1, 56);
        DrawText(l1, (VIRTUAL_WIDTH - w1) / 2, 310, 56, Color{ 235, 30, 45, static_cast<unsigned char>(255 * alpha) });
        const char* l2 = "Discover new adventures";
        int w2 = MeasureText(l2, 22);
        DrawText(l2, (VIRTUAL_WIDTH - w2) / 2, 385, 22, Color{ 20, 60, 180, static_cast<unsigned char>(255 * alpha) });
    }

    if (flash > 0.0f) {
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, Color{ 255, 255, 255, static_cast<unsigned char>(flash * 140) });
    }

    const char* skipHint = "TAP OR PRESS SPACE TO SKIP";
    int shw = MeasureText(skipHint, 13);
    DrawText(skipHint, (VIRTUAL_WIDTH - shw) / 2, VIRTUAL_HEIGHT - 38, 13, Color{ 40, 80, 140, static_cast<unsigned char>(160 * alpha) });
}

void GhostGame::RenderBottomBrandingFooter() const {
    DrawRectangle(0, VIRTUAL_HEIGHT - 54, VIRTUAL_WIDTH, 54, { 6, 9, 16, 235 });
    DrawLine(0, VIRTUAL_HEIGHT - 54, VIRTUAL_WIDTH, VIRTUAL_HEIGHT - 54, { 120, 180, 230, 100 });

    const char* studioLine = "SEBINY LABS";
    int sw = MeasureText(studioLine, 18);
    DrawText(studioLine, (VIRTUAL_WIDTH - sw) / 2, VIRTUAL_HEIGHT - 46, 18, GOLD);

    const char* ipLine = "Non-commercial fan tribute. Destiny, Ghost, The Traveler, and related IP are trademarks and copyright of Bungie, Inc.";
    int iw = MeasureText(ipLine, 14);
    DrawText(ipLine, (VIRTUAL_WIDTH - iw) / 2, VIRTUAL_HEIGHT - 22, 14, { 175, 190, 210, 230 });
}

void GhostGame::RenderTitleFrontPage() {
    // Destiny 2 Style Front Page (White Traveler Silhouette, Epic Title, Press Any Key prompt)
    Vector2 center = { VIRTUAL_WIDTH * 0.5f, 260.0f };
    float pulse = std::sin(globalTime_ * 1.8f);

    // Traveler Paracausal Halo
    DrawCircleGradient(static_cast<int>(center.x), static_cast<int>(center.y), 240.0f,
                       { 240, 248, 255, static_cast<unsigned char>(50 + pulse * 15) }, BLANK);
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 220.0f, { 255, 255, 255, 30 });
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 160.0f, { 255, 255, 255, 45 });

    // Ghost floating in front of Traveler
    RenderGhostRig(center, 46.0f, std::sin(globalTime_ * 1.2f) * 6.0f,
                   0.35f + pulse * 0.15f, globalTime_ * 1.2f, { 3.0f, pulse * 2.0f },
                   profile_.equippedShell, 1.0f);

    const char* mainDestiny = "DESTINY";
    int mdw = MeasureText(mainDestiny, 72);
    DrawText(mainDestiny, (VIRTUAL_WIDTH - mdw) / 2, 368, 72, WHITE);

    const char* subGameTitle = "THE FLAPPY GHOST";
    int fgw = MeasureText(subGameTitle, 24);
    DrawText(subGameTitle, (VIRTUAL_WIDTH - fgw) / 2, 448, 24, SKYBLUE);

    DrawLine(VIRTUAL_WIDTH / 2 - 320, 494, VIRTUAL_WIDTH / 2 + 320, 494, { 255, 255, 255, 80 });

    // Pulsing "Press Any Key / Tap to Enter" Call to Action
    float blink = std::sin(globalTime_ * 4.0f) * 0.5f + 0.5f;
    Color promptCol = { 255, 255, 255, static_cast<unsigned char>(140 + blink * 115) };
    Rectangle enterBtn = { VIRTUAL_WIDTH / 2.0f - 260.0f, 540.0f, 520.0f, 54.0f };
    bool hovEnter = IsRectHovered(enterBtn);
    DrawRectangleRec(enterBtn, hovEnter ? Color{ 35, 55, 85, 240 } : Color{ 14, 20, 34, 210 });
    DrawRectangleLinesEx(enterBtn, 2.0f, hovEnter ? SKYBLUE : Color{ 120, 160, 210, 150 });
    bool hasGamepad = IsGamepadAvailable(0);
    const char* prompt = hasGamepad
                       ? "PRESS [START] / [(A)] OR CLICK TO ENTER"
                       : "PRESS [SPACE] / [ENTER] OR CLICK TO ENTER";
    int pw = MeasureText(prompt, 18);
    DrawText(prompt, (VIRTUAL_WIDTH - pw) / 2, 558, 18, promptCol);

    const char* ostNotice = "PLAYING: DESTINY 2 ORIGINAL SOUNDTRACK - INNER LIGHT";
    int onw = MeasureText(ostNotice, 14);
    DrawText(ostNotice, (VIRTUAL_WIDTH - onw) / 2, 608, 14, { 140, 185, 225, 200 });

    RenderBottomBrandingFooter();
}

void GhostGame::RenderTitleDirector() {
    const char* mainDestiny = "DESTINY";
    int mdw = MeasureText(mainDestiny, 52);
    DrawText(mainDestiny, (VIRTUAL_WIDTH - mdw) / 2, 34, 52, WHITE);

    const char* subGameTitle = "THE FLAPPY GHOST";
    int fgw = MeasureText(subGameTitle, 18);
    DrawText(subGameTitle, (VIRTUAL_WIDTH - fgw) / 2, 90, 18, SKYBLUE);

    char glimPill[96];
    std::snprintf(glimPill, sizeof(glimPill), "GLIMMER: %d   |   SHARDS: %d", profile_.glimmer, profile_.legendaryShards);
    int gpw = MeasureText(glimPill, 15);
    Rectangle pillRec = { (VIRTUAL_WIDTH - (gpw + 36.0f)) * 0.5f, 120.0f, gpw + 36.0f, 28.0f };
    DrawRectangleRec(pillRec, { 14, 22, 36, 200 });
    DrawRectangleLinesEx(pillRec, 1.2f, { 145, 165, 245, 150 });
    DrawText(glimPill, static_cast<int>(pillRec.x + 18.0f), static_cast<int>(pillRec.y + 7.0f), 15, SKYBLUE);

    Vector2 previewPos = { VIRTUAL_WIDTH * 0.5f, 232.0f + std::sin(globalTime_ * 2.5f) * 8.0f };
    DrawCircleLines(static_cast<int>(previewPos.x), static_cast<int>(previewPos.y), 74.0f, { 255, 255, 255, 40 });
    RenderGhostRig(previewPos, 35.0f, std::sin(globalTime_ * 1.7f) * 8.0f,
                   0.25f + 0.15f * std::sin(globalTime_ * 3.2f),
                   globalTime_ * 1.4f, { std::cos(globalTime_ * 1.5f) * 5.0f, std::sin(globalTime_ * 2.1f) * 3.0f },
                   profile_.equippedShell, 1.0f);

    bool nightfallUnlocked = HasUnlockedAnyDrone();
    float cx = VIRTUAL_WIDTH * 0.5f;

    // 2-Segment Mode Toggle: [ PATROL ] | [ NIGHTFALL ]
    float toggleW = 360.0f;
    Rectangle segPatrol    = { cx - toggleW * 0.5f, 336.0f, toggleW * 0.5f - 3.0f, 38.0f };
    Rectangle segNightfall = { cx + 3.0f,           336.0f, toggleW * 0.5f - 3.0f, 38.0f };

    bool patSel = !menuSelectNightfall_;
    DrawRectangleRec(segPatrol, patSel ? SKYBLUE : Color{ 16, 24, 38, 220 });
    DrawRectangleLinesEx(segPatrol, 1.5f, SKYBLUE);
    int pw = MeasureText("PATROL", 15);
    DrawText("PATROL", static_cast<int>(segPatrol.x + (segPatrol.width - pw) * 0.5f), static_cast<int>(segPatrol.y + 11.0f), 15, patSel ? BLACK : WHITE);
    if (IsButtonClicked(segPatrol)) {
        menuSelectNightfall_ = false;
        audio_.PlayUiSelect();
    }

    Color nfAccent = nightfallUnlocked ? GOLD : RED;
    bool nfSel = menuSelectNightfall_;
    DrawRectangleRec(segNightfall, nfSel ? nfAccent : Color{ 16, 24, 38, 220 });
    DrawRectangleLinesEx(segNightfall, 1.5f, nfAccent);
    const char* nfSegLbl = nightfallUnlocked ? "NIGHTFALL" : "NIGHTFALL [LOCKED]";
    int nfw = MeasureText(nfSegLbl, 14);
    DrawText(nfSegLbl, static_cast<int>(segNightfall.x + (segNightfall.width - nfw) * 0.5f), static_cast<int>(segNightfall.y + 12.0f), 14,
             nfSel ? (nightfallUnlocked ? BLACK : WHITE) : (nightfallUnlocked ? GOLD : LIGHTGRAY));
    if (IsButtonClicked(segNightfall)) {
        menuSelectNightfall_ = true;
        audio_.PlayUiSelect();
    }

    auto DrawCleanHubBtn = [&](Rectangle r, const char* label, int fs, Color accent, bool primary) -> bool {
        bool hov = IsRectHovered(r);
        Color bg = primary
            ? (hov ? Color{ 34, 54, 86, 245 } : Color{ 18, 30, 50, 235 })
            : (hov ? Color{ 28, 42, 66, 235 } : Color{ 14, 20, 34, 215 });
        DrawRectangleRec(r, bg);
        DrawRectangleLinesEx(r, hov ? 2.5f : 1.6f, hov ? accent : Color{ 130, 160, 195, 150 });
        DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), 5, static_cast<int>(r.height), accent);
        int lw = MeasureText(label, fs);
        int ty = static_cast<int>(r.y + (r.height - fs) * 0.5f);
        DrawText(label, static_cast<int>(r.x + (r.width - lw) * 0.5f), ty, fs, WHITE);
        return IsButtonClicked(r);
    };

    // Button 1: PLAY
    Color playAccent = (!menuSelectNightfall_) ? SKYBLUE : (nightfallUnlocked ? GOLD : RED);
    const char* playLbl = (menuSelectNightfall_ && !nightfallUnlocked) ? "UNLOCK DRONE (200G)" : "PLAY";
    Rectangle btnPlay = { cx - 180.0f, 384.0f, 360.0f, 58.0f };
    if (DrawCleanHubBtn(btnPlay, playLbl, (menuSelectNightfall_ && !nightfallUnlocked) ? 18 : 24, playAccent, true)) {
        if (!menuSelectNightfall_) {
            StartNewRun(false);
        } else if (nightfallUnlocked) {
            StartNewRun(true);
        } else {
            towerTab_ = 2;
            activeToast_.active = true;
            activeToast_.title = "NIGHTFALL REQUIRES A COMBAT DRONE";
            activeToast_.subtitle = "Unlock your first Combat Drone here (starting at 200 Glimmer)!";
            activeToast_.timer = 4.0f;
            SetScreenState(ScreenState::SHELL_HANGAR);
            audio_.PlayUiSelect();
        }
    }

    // Row of 3 Clean Single-Word Hub Buttons: LOADOUT | RECORDS | SETTINGS
    float rowW = 636.0f;
    float btnW = 200.0f;
    float gap  = 18.0f;
    float rowX = cx - rowW * 0.5f;
    Rectangle btnLoadout  = { rowX,                    458.0f, btnW, 52.0f };
    Rectangle btnRecords  = { rowX + (btnW + gap),     458.0f, btnW, 52.0f };
    Rectangle btnSettings = { rowX + (btnW + gap) * 2, 458.0f, btnW, 52.0f };

    if (DrawCleanHubBtn(btnLoadout, "LOADOUT", 19, GetSubclassAccentColor(), false)) {
        SetScreenState(ScreenState::SUBCLASS_ARMORY);
        audio_.PlayUiSelect();
    }
    if (HasAnyAffordableUpgrade()) {
        DrawUpgradeBadge(btnLoadout, globalTime_);
    }
    if (DrawCleanHubBtn(btnRecords, "RECORDS", 19, GREEN, false)) {
        SetScreenState(ScreenState::TRIUMPHS_LOG);
        audio_.PlayUiSelect();
    }
    if (DrawCleanHubBtn(btnSettings, "SETTINGS", 19, PURPLE, false)) {
        settingsTab_ = 0;
        settingsPrevState_ = ScreenState::TITLE_DIRECTOR;
        SetScreenState(ScreenState::SETTINGS_VIEW);
        audio_.PlayUiSelect();
    }

    RenderBottomBrandingFooter();
}

void GhostGame::RenderSubclassArmory() {
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 8, 12, 22, 230 });
    float uiX = (VIRTUAL_WIDTH - 1280.0f) * 0.5f;
    DrawText("LOADOUT", static_cast<int>(uiX + 50), 22, 26, WHITE);

    char glimHeader[96];
    std::snprintf(glimHeader, sizeof(glimHeader), "GLIMMER: %d   |   SHARDS: %d", profile_.glimmer, profile_.legendaryShards);
    int ghw = MeasureText(glimHeader, 19);
    DrawText(glimHeader, static_cast<int>(uiX + 1230.0f) - ghw, 26, 19, SKYBLUE);

    const char* loadoutTabs[6] = { "SUBCLASS", "SHELLS I", "SHELLS II", "DRONES", "SHADERS", "CATALYSTS" };
    Color loadoutCols[6] = { GetSubclassAccentColor(), SKYBLUE, ORANGE, GREEN, Color{ 255, 120, 220, 255 }, GOLD };
    for (int t = 0; t < 6; ++t) {
        Rectangle tabR = { uiX + 50.0f + t * 198.0f, 58.0f, 188.0f, 34.0f };
        bool active = (t == 0);
        DrawRectangleRec(tabR, active ? loadoutCols[t] : Color{ 22, 30, 48, 255 });
        DrawRectangleLinesEx(tabR, 1.5f, active ? WHITE : loadoutCols[t]);
        int tw = MeasureText(loadoutTabs[t], 14);
        DrawText(loadoutTabs[t], static_cast<int>(tabR.x + (tabR.width - tw) * 0.5f), static_cast<int>(tabR.y + 10), 14, active ? BLACK : WHITE);
        if (HasAffordableLoadoutTab(t)) {
            DrawUpgradeBadge(tabR, globalTime_);
        }
        if (t > 0 && IsButtonClicked(tabR)) {
            towerTab_ = t - 1;
            SetScreenState(ScreenState::SHELL_HANGAR);
            audio_.PlayUiSelect();
            return;
        }
    }

    struct SubCard {
        int idx;
        const char* name;
        const char* element;
        const char* tacticalDesc;
        const char* superDesc;
        int glimmerCost;
        int shardCost;
        Color col;
    };

    const SubCard cards[NUM_SUBCLASSES] = {
        { 0, "VOIDWALKER", "VOID LIGHT",
          "BLINK [ASTROCYTE WARP]: Instant 145px spatial teleport forward past barriers + Void echo.",
          "NIGHTFALL SUPER [NOVA BOMB]: Hurls a Void singularity clearing bosses, shields & 1 barrier.",
          0, 0, { 175, 85, 255, 255 } },
        { 1, "DAWNBLADE", "SOLAR LIGHT",
          "BLINK [ICARUS HOVER JET]: 1.15s zero-G horizontal Solar cruise + 2.25x Mote vacuum.",
          "NIGHTFALL SUPER [DAYBREAK]: Every flap hurls homing Solar blades & burns enemy fire.",
          0, 0, { 255, 145, 35, 255 } },
        { 2, "STORMCALLER", "ARC LIGHT",
          "BLINK [IONIC SURGE]: Flat lightning bolt dash, chain-jolts nearby threats & refunds CD on gate.",
          "NIGHTFALL SUPER [CHAOS REACH]: Aimable Arc laser melting bosses, enemies & Oracles.",
          0, 0, { 75, 220, 255, 255 } },
        { 3, "SHADEBINDER", "STASIS DARKNESS",
          "BLINK [FROST PULSE]: 3s hazard time-freeze + conjures a bounceable Stasis ice rail below.",
          "NIGHTFALL SUPER [WINTER'S WRATH]: Freezes hazards & shatters all foes/shots into Motes.",
          500, 0, { 70, 130, 255, 255 } },
        { 4, "THREADRUNNER", "STRAND DARKNESS",
          "BLINK [STRAND GRAPPLE]: Fires an upward grapple anchor ahead to swing & phase cleanly.",
          "NIGHTFALL SUPER [NEEDLESTORM]: Unleashes a 12-Threadling swarm hunting foes & Bosses.",
          500, 0, { 65, 245, 130, 255 } },
        { 5, "TRANSCENDENT", "PRISMATIC",
          "BLINK [2 CHARGES]: Dual-charge 90px spatial micro-warp + 1.6s Stasis freeze & seekers!",
          "NIGHTFALL SUPER [TRANSCENDENCE]: 4x Blink recharge + dual Light/Dark homing bursts!",
          1500, 15, { 255, 125, 235, 255 } }
    };

    // Clean, spacious 3x2 grid (Row 0 = 3 Light Subclasses, Row 1 = 3 Darkness/Prismatic Subclasses)
    for (int i = 0; i < static_cast<int>(NUM_SUBCLASSES); ++i) {
        int col = i % 3;
        int row = i / 3;
        Rectangle cardRect = { uiX + 50.0f + col * 398.0f, 102.0f + row * 234.0f, 384.0f, 224.0f };
        int innerW = static_cast<int>(cardRect.width) - 32; // 352px strictly inside cardRect
        bool unlocked = profile_.unlockedSubclasses[i];
        bool selected = (profile_.selectedSubclass == i);
        bool hov = IsRectHovered(cardRect);

        DrawRectangleRec(cardRect, selected ? Color{ 28, 38, 60, 245 } : Color{ 16, 22, 34, 235 });
        DrawRectangleLinesEx(cardRect, selected ? 3.0f : 1.5f, (selected || hov) ? cards[i].col : GRAY);

        Vector2 crestPos = { cardRect.x + 38.0f, cardRect.y + 38.0f };
        DrawPoly(crestPos, 4, 22.0f, 0.0f, unlocked ? cards[i].col : DARKGRAY);
        DrawPolyLinesEx(crestPos, 4, 25.0f, 0.0f, 2.0f, WHITE);
        DrawPoly(crestPos, 4, 9.0f, 0.0f, unlocked ? WHITE : GRAY);

        DrawText(cards[i].element, static_cast<int>(cardRect.x + 74), static_cast<int>(cardRect.y + 14), 13, cards[i].col);
        DrawText(cards[i].name, static_cast<int>(cardRect.x + 74), static_cast<int>(cardRect.y + 32), 22, WHITE);

        DrawRectangle(static_cast<int>(cardRect.x + 16), static_cast<int>(cardRect.y + 66), innerW, 2, cards[i].col);

        DrawTextWrapped(cards[i].tacticalDesc,
                        static_cast<int>(cardRect.x + 16), static_cast<int>(cardRect.y + 76),
                        innerW, 13, 17, LIGHTGRAY);

        DrawTextWrapped(cards[i].superDesc,
                        static_cast<int>(cardRect.x + 16), static_cast<int>(cardRect.y + 124),
                        innerW, 13, 17, LIGHTGRAY);

        Rectangle equipBtn = { cardRect.x + 16.0f, cardRect.y + 176.0f, static_cast<float>(innerW), 38.0f };
        if (selected) {
            DrawRectangleRec(equipBtn, cards[i].col);
            const char* btnLabel = "EQUIPPED";
            int blw = MeasureText(btnLabel, 16);
            DrawText(btnLabel, static_cast<int>(equipBtn.x + (equipBtn.width - blw) * 0.5f),
                     static_cast<int>(equipBtn.y + 11), 16, BLACK);
        } else if (unlocked) {
            DrawRectangleRec(equipBtn, Color{ 35, 45, 65, 255 });
            const char* btnLabel = "ATTUNE SUBCLASS";
            int blw = MeasureText(btnLabel, 16);
            DrawText(btnLabel, static_cast<int>(equipBtn.x + (equipBtn.width - blw) * 0.5f),
                     static_cast<int>(equipBtn.y + 11), 16, WHITE);
        } else {
            bool canAfford = (profile_.glimmer >= cards[i].glimmerCost && profile_.legendaryShards >= cards[i].shardCost);
            DrawRectangleRec(equipBtn, canAfford ? Color{ 28, 82, 58, 255 } : Color{ 48, 30, 36, 255 });
            char costTxt[64];
            if (cards[i].shardCost > 0) {
                std::snprintf(costTxt, sizeof(costTxt), "UNLOCK (%dG, %d SHARDS)", cards[i].glimmerCost, cards[i].shardCost);
            } else {
                std::snprintf(costTxt, sizeof(costTxt), "UNLOCK (%d GLIMMER)", cards[i].glimmerCost);
            }
            int blw = MeasureText(costTxt, 15);
            DrawText(costTxt, static_cast<int>(equipBtn.x + (equipBtn.width - blw) * 0.5f),
                     static_cast<int>(equipBtn.y + 12), 15, canAfford ? GREEN : RED);
        }

        if (IsButtonClicked(cardRect)) {
            if (unlocked) {
                profile_.selectedSubclass = i;
                SaveData();
                audio_.PlayUiSelect();
            } else if (profile_.glimmer >= cards[i].glimmerCost && profile_.legendaryShards >= cards[i].shardCost) {
                profile_.glimmer -= cards[i].glimmerCost;
                profile_.legendaryShards -= cards[i].shardCost;
                profile_.unlockedSubclasses[i] = true;
                profile_.selectedSubclass = i;
                int subUnlocked = 0;
                for (bool u : profile_.unlockedSubclasses) if (u) subUnlocked++;
                if (subUnlocked >= static_cast<int>(NUM_SUBCLASSES)) UnlockTriumph(13);
                SaveData();
                audio_.PlayExoticPickup();
            }
        }
    }

    Rectangle backBtn = { uiX + 50.0f, 585.0f, 235.0f, 48.0f };
    DrawRectangleRec(backBtn, IsRectHovered(backBtn) ? Color{ 45, 65, 95, 255 } : Color{ 25, 35, 52, 255 });
    DrawRectangleLinesEx(backBtn, 2.0f, SKYBLUE);
    DrawText("< RETURN TO DIRECTOR", static_cast<int>(backBtn.x + 18), static_cast<int>(backBtn.y + 16), 16, WHITE);
    if (IsButtonClicked(backBtn)) {
        SetScreenState(ScreenState::TITLE_DIRECTOR);
        audio_.PlayUiSelect();
    }
}

void GhostGame::RenderShellHangar() {
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 8, 12, 22, 230 });
    float uiX = (VIRTUAL_WIDTH - 1280.0f) * 0.5f;
    DrawText("LOADOUT", static_cast<int>(uiX + 50), 22, 26, WHITE);

    char glimHeader[96];
    std::snprintf(glimHeader, sizeof(glimHeader), "GLIMMER: %d   |   SHARDS: %d", profile_.glimmer, profile_.legendaryShards);
    int ghw = MeasureText(glimHeader, 19);
    DrawText(glimHeader, static_cast<int>(uiX + 1230.0f) - ghw, 26, 19, SKYBLUE);

    const char* loadoutTabs[6] = { "SUBCLASS", "SHELLS I", "SHELLS II", "DRONES", "SHADERS", "CATALYSTS" };
    Color loadoutCols[6] = { GetSubclassAccentColor(), SKYBLUE, ORANGE, GREEN, Color{ 255, 120, 220, 255 }, GOLD };
    for (int t = 0; t < 6; ++t) {
        Rectangle tabR = { uiX + 50.0f + t * 198.0f, 58.0f, 188.0f, 34.0f };
        bool active = (t > 0 && towerTab_ == (t - 1));
        DrawRectangleRec(tabR, active ? loadoutCols[t] : Color{ 22, 30, 48, 255 });
        DrawRectangleLinesEx(tabR, 1.5f, active ? WHITE : loadoutCols[t]);
        int tw = MeasureText(loadoutTabs[t], 14);
        DrawText(loadoutTabs[t], static_cast<int>(tabR.x + (tabR.width - tw) * 0.5f), static_cast<int>(tabR.y + 10), 14, active ? BLACK : WHITE);
        if (HasAffordableLoadoutTab(t)) {
            DrawUpgradeBadge(tabR, globalTime_);
        }
        if (IsButtonClicked(tabR)) {
            if (t == 0) {
                SetScreenState(ScreenState::SUBCLASS_ARMORY);
                audio_.PlayUiSelect();
                return;
            }
            towerTab_ = t - 1;
            audio_.PlayUiSelect();
        }
    }

    if (towerTab_ == 0 || towerTab_ == 1) {
        const auto& catalog = GetShellCatalog();
        int startIdx = (towerTab_ == 0) ? 0 : 6;
        for (int localIdx = 0; localIdx < 6; ++localIdx) {
            int i = startIdx + localIdx;
            if (i >= static_cast<int>(NUM_SHELLS)) break;
            int col = localIdx % 2;
            int row = localIdx / 2;
            Rectangle r = { uiX + 60.0f + col * 365.0f, 138.0f + row * 142.0f, 350.0f, 132.0f };
            bool unlocked = profile_.unlockedShells[i];
            bool equipped = (profile_.equippedShell == i);
            bool hov = IsRectHovered(r);

            DrawRectangleRec(r, equipped ? Color{ 28, 44, 68, 245 } : Color{ 16, 22, 34, 230 });
            DrawRectangleLinesEx(r, equipped ? 2.5f : 1.5f, equipped ? GOLD : (hov ? SKYBLUE : GRAY));

            RenderGhostRig({ r.x + 52.0f, r.y + 66.0f }, 22.0f, 0.0f, hov ? 0.35f : 0.1f,
                           globalTime_ * 1.6f, { 3.0f, 0.0f }, i, 1.0f);

            DrawText(catalog[i].name, static_cast<int>(r.x + 106), static_cast<int>(r.y + 12), 16, WHITE);
            DrawTextWrapped(catalog[i].loreSubtitle,
                            static_cast<int>(r.x + 106), static_cast<int>(r.y + 34),
                            230, 13, 16, LIGHTGRAY);

            Rectangle actBtn = { r.x + 106.0f, r.y + 90.0f, 228.0f, 32.0f };
            if (equipped) {
                DrawRectangleRec(actBtn, GOLD);
                DrawText("EQUIPPED", static_cast<int>(actBtn.x + 74), static_cast<int>(actBtn.y + 9), 15, BLACK);
            } else if (unlocked) {
                DrawRectangleRec(actBtn, { 38, 68, 98, 255 });
                DrawText("EQUIP SHELL", static_cast<int>(actBtn.x + 64), static_cast<int>(actBtn.y + 9), 15, WHITE);
            } else {
                bool canAfford = (profile_.glimmer >= catalog[i].glimmerCost && profile_.legendaryShards >= catalog[i].shardCost);
                DrawRectangleRec(actBtn, canAfford ? Color{ 30, 85, 60, 255 } : Color{ 45, 30, 35, 255 });
                char costTxt[64];
                if (catalog[i].shardCost > 0) {
                    std::snprintf(costTxt, sizeof(costTxt), "UNLOCK (%dG, %d SHARDS)", catalog[i].glimmerCost, catalog[i].shardCost);
                } else {
                    std::snprintf(costTxt, sizeof(costTxt), "UNLOCK (%d GLIMMER)", catalog[i].glimmerCost);
                }
                int cw = MeasureText(costTxt, 13);
                DrawText(costTxt, static_cast<int>(actBtn.x + (actBtn.width - cw) * 0.5f), static_cast<int>(actBtn.y + 10), 13, canAfford ? GREEN : RED);
            }

            if (IsButtonClicked(r)) {
                if (unlocked) {
                    profile_.equippedShell = i;
                    SaveData();
                    audio_.PlayUiSelect();
                } else if (profile_.glimmer >= catalog[i].glimmerCost && profile_.legendaryShards >= catalog[i].shardCost) {
                    profile_.glimmer -= catalog[i].glimmerCost;
                    profile_.legendaryShards -= catalog[i].shardCost;
                    profile_.unlockedShells[i] = true;
                    profile_.equippedShell = i;
                    SaveData();
                    audio_.PlayExoticPickup();

                    int countUnlocked = 0;
                    for (bool u : profile_.unlockedShells) if (u) countUnlocked++;
                    if (countUnlocked >= 3) UnlockTriumph(9);
                }
            }
        }

        DrawText("GHOST TELEMETRY MODS", static_cast<int>(uiX + 810), 108, 20, GOLD);

        auto DrawUpgradeRow = [&](int idx, const char* title, const char* desc, int& level, int maxLvl, int baseCost) {
            Rectangle ur = { uiX + 810.0f, 138.0f + idx * 105.0f, 410.0f, 96.0f };
            int cost = baseCost * (level + 1);
            bool maxed = (level >= maxLvl);
            bool canBuy = (!maxed && profile_.glimmer >= cost);

            DrawRectangleRec(ur, { 16, 22, 36, 235 });
            DrawRectangleLinesEx(ur, 1.5f, canBuy ? SKYBLUE : GRAY);
            DrawText(title, static_cast<int>(ur.x + 14), static_cast<int>(ur.y + 10), 16, WHITE);
            DrawTextWrapped(desc, static_cast<int>(ur.x + 14), static_cast<int>(ur.y + 32), 382, 13, 16, LIGHTGRAY);

            for (int p = 0; p < maxLvl; ++p) {
                DrawRectangle(static_cast<int>(ur.x + 14 + p * 28), static_cast<int>(ur.y + 64), 22, 16,
                              (p < level) ? GOLD : Color{ 40, 48, 62, 255 });
            }

            Rectangle buyBtn = { ur.x + 225.0f, ur.y + 54.0f, 170.0f, 34.0f };
            DrawRectangleRec(buyBtn, maxed ? DARKGRAY : (canBuy ? Color{ 35, 85, 65, 255 } : Color{ 45, 35, 40, 255 }));
            char btxt[48];
            if (maxed) std::snprintf(btxt, sizeof(btxt), "MAX MASTERWORK");
            else std::snprintf(btxt, sizeof(btxt), "UPGRADE (%d G)", cost);
            DrawText(btxt, static_cast<int>(buyBtn.x + 14), static_cast<int>(buyBtn.y + 10), 14, maxed ? GOLD : (canBuy ? WHITE : GRAY));

            if (IsButtonClicked(buyBtn) && canBuy) {
                profile_.glimmer -= cost;
                level++;
                SaveData();
                audio_.PlayMotePickup();
            }
        };

        DrawUpgradeRow(0, "MOTE MAGNETISM COIL", "Increases Mote of Light attraction radius.", profile_.upgrades.lightMagnetLevel, 5, 100);
        DrawUpgradeRow(1, "PARACAUSAL CAPACITOR", "Boosts Nightfall Super energy & duration.", profile_.upgrades.superEfficiencyLevel, 5, 120);
        DrawUpgradeRow(2, "PHASE BLINK ACCELERATOR", "Reduces Blink/Dash tactical cooldown.", profile_.upgrades.phaseCooldownLevel, 5, 120);
        DrawUpgradeRow(3, "VANGUARD OVERSHIELD", "Start every run with extra Overshield charges.", profile_.upgrades.overshieldCapLevel, 3, 250);
    } else if (towerTab_ == 2) {
        // Tab 2: Manual-Fire Combat Drones (Unlocks Nightfall Mode!)
        DrawText("VANGUARD COMBAT DRONES // NIGHTFALL STRIKE ARSENAL", static_cast<int>(uiX + 60), 114, 22, GREEN);
        DrawText("Unlocking your first Drone unlocks NIGHTFALL STRIKE — earn Legendary Shards in Nightfall (+1/10 Gates, +2 Engram, +5 Boss)!",
                 static_cast<int>(uiX + 60), 142, 15, LIGHTGRAY);

        const auto& drones = GetDroneCatalog();
        for (int i = 0; i < static_cast<int>(NUM_DRONES); ++i) {
            Rectangle dr = { uiX + 60.0f + (i % 2) * 580.0f, 180.0f + (i / 2) * 182.0f, 560.0f, 164.0f };
            bool unlocked = profile_.unlockedDrones[i];
            bool equipped = (profile_.equippedDrone == i);

            DrawRectangleRec(dr, equipped ? Color{ 28, 44, 64, 245 } : Color{ 16, 22, 34, 235 });
            DrawRectangleLinesEx(dr, equipped ? 3.0f : 1.5f, equipped ? GOLD : drones[i].color);

            Vector2 dp = { dr.x + 52.0f, dr.y + 82.0f };
            int sides = (i == 0) ? 6 : (i == 1) ? 4 : (i == 2) ? 8 : 3;
            DrawCircleGradient(static_cast<int>(dp.x), static_cast<int>(dp.y), 32.0f, drones[i].color, BLANK);
            DrawPoly(dp, sides, 18.0f, globalTime_ * 120.0f, WHITE);
            DrawPolyLinesEx(dp, sides, 24.0f, -globalTime_ * 90.0f, 2.5f, drones[i].color);

            DrawText(drones[i].name, static_cast<int>(dr.x + 104), static_cast<int>(dr.y + 16), 20, drones[i].color);
            DrawTextWrapped(drones[i].description, static_cast<int>(dr.x + 104), static_cast<int>(dr.y + 44), 435, 14, 19, WHITE);

            char cdTxt[64];
            std::snprintf(cdTxt, sizeof(cdTxt), "MANUAL FIRE COOLDOWN: %.2fs", drones[i].fireCooldown);
            DrawText(cdTxt, static_cast<int>(dr.x + 104), static_cast<int>(dr.y + 88), 13, SKYBLUE);

            Rectangle btn = { dr.x + 104.0f, dr.y + 112.0f, 250.0f, 38.0f };
            if (equipped) {
                DrawRectangleRec(btn, GOLD);
                int bw = MeasureText("EQUIPPED DRONE", 15);
                DrawText("EQUIPPED DRONE", static_cast<int>(btn.x + (btn.width - bw) * 0.5f), static_cast<int>(btn.y + 11), 15, BLACK);
            } else if (unlocked) {
                DrawRectangleRec(btn, Color{ 38, 68, 98, 255 });
                int bw = MeasureText("EQUIP DRONE", 15);
                DrawText("EQUIP DRONE", static_cast<int>(btn.x + (btn.width - bw) * 0.5f), static_cast<int>(btn.y + 11), 15, WHITE);
            } else {
                bool canBuy = (profile_.glimmer >= drones[i].glimmerCost && profile_.legendaryShards >= drones[i].shardCost);
                DrawRectangleRec(btn, canBuy ? Color{ 30, 88, 60, 255 } : Color{ 48, 30, 35, 255 });
                char cstr[64];
                if (drones[i].shardCost > 0) {
                    std::snprintf(cstr, sizeof(cstr), "UNLOCK (%dG, %d SHARDS)", drones[i].glimmerCost, drones[i].shardCost);
                } else {
                    std::snprintf(cstr, sizeof(cstr), "UNLOCK (%d GLIMMER)", drones[i].glimmerCost);
                }
                int bw = MeasureText(cstr, 14);
                DrawText(cstr, static_cast<int>(btn.x + (btn.width - bw) * 0.5f), static_cast<int>(btn.y + 12), 14, canBuy ? GREEN : RED);
            }

            if (IsButtonClicked(dr)) {
                if (unlocked) {
                    profile_.equippedDrone = i;
                    SaveData();
                    audio_.PlayUiSelect();
                } else if (profile_.glimmer >= drones[i].glimmerCost && profile_.legendaryShards >= drones[i].shardCost) {
                    bool wasFirstDrone = !HasUnlockedAnyDrone();
                    profile_.glimmer -= drones[i].glimmerCost;
                    profile_.legendaryShards -= drones[i].shardCost;
                    profile_.unlockedDrones[i] = true;
                    profile_.equippedDrone = i;
                    SaveData();
                    audio_.PlayExoticPickup();
                    if (wasFirstDrone) {
                        activeToast_.active = true;
                        activeToast_.title = "NIGHTFALL STRIKE UNLOCKED!";
                        activeToast_.subtitle = "You unlocked your first Combat Drone! Launch Nightfall from the Director.";
                        activeToast_.timer = 4.5f;
                    }
                }
            }
        }
    } else if (towerTab_ == 3) {
        // Tab 3: Eva Levante's Shaders (With smooth vertical scrolling & scrollbar)
        DrawText("EVA LEVANTE // GUARDIAN ARMOR & SHELL SHADERS", static_cast<int>(uiX + 60), 118, 22, Color{ 255, 120, 220, 255 });
        DrawText("Apply legendary & exotic shader palettes to recolor all 12 Ghost shells. (Scroll/Drag scrollbar for more)", static_cast<int>(uiX + 60), 146, 16, LIGHTGRAY);

        constexpr float TOTAL_SHADER_HEIGHT = 6 * 125.0f;
        constexpr float SHADER_VIEWPORT_H = 385.0f;
        float maxShaderScroll = std::max(0.0f, TOTAL_SHADER_HEIGHT - SHADER_VIEWPORT_H + 20.0f);
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            shaderScrollY_ = std::clamp(shaderScrollY_ - wheel * 45.0f, 0.0f, maxShaderScroll);
        }

        float trackX = uiX + 1205.0f;
        float trackY = 185.0f;
        float trackH = SHADER_VIEWPORT_H;
        Rectangle shaderViewport = { uiX + 50.0f, 180.0f, 1150.0f, SHADER_VIEWPORT_H };
        bool ptrDown = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0);
        bool shaderReleaseTap = false;
        Vector2 shaderTapPos{};
        if (ptrDown) {
            Vector2 p = (GetTouchPointCount() > 0) ? ToVirtualCoords(GetTouchPosition(0)) : GetVirtualMousePos();
            Rectangle trackHit = { trackX - 24.0f, trackY - 10.0f, 52.0f, trackH + 20.0f };
            if (maxShaderScroll > 0.0f && CheckCollisionPointRec(p, trackHit)) {
                float frac = std::clamp((p.y - trackY) / trackH, 0.0f, 1.0f);
                shaderScrollY_ = frac * maxShaderScroll;
                listDragging_ = false;
                listDragMoved_ = true;
            } else if (!listDragging_ && CheckCollisionPointRec(p, shaderViewport)) {
                listDragging_ = true;
                listDragLastPos_ = p;
                listDragTotalDy_ = 0.0f;
                listDragMoved_ = false;
            } else if (listDragging_) {
                float dy = p.y - listDragLastPos_.y;
                listDragLastPos_ = p;
                listDragTotalDy_ += std::abs(dy);
                if (listDragTotalDy_ > 10.0f) listDragMoved_ = true;
                if (maxShaderScroll > 0.0f) {
                    shaderScrollY_ = std::clamp(shaderScrollY_ - dy, 0.0f, maxShaderScroll);
                }
            }
        } else if (listDragging_) {
            if (!listDragMoved_ && screenTransitionCooldown_ <= 0.0f) {
                shaderReleaseTap = true;
                shaderTapPos = listDragLastPos_;
            }
            listDragging_ = false;
            listDragMoved_ = false;
        }

        float scX = static_cast<float>(GetScreenWidth()) / std::max(1.0f, static_cast<float>(GetRenderWidth()));
        float scY = static_cast<float>(GetScreenHeight()) / std::max(1.0f, static_cast<float>(GetRenderHeight()));
        BeginScissorMode(static_cast<int>(std::round((uiOffsetX_ + (uiX + 50.0f) * uiScale_) * scX)),
                         static_cast<int>(std::round((uiOffsetY_ + 180.0f * uiScale_) * scY)),
                         static_cast<int>(std::round(1150.0f * uiScale_ * scX)),
                         static_cast<int>(std::round(SHADER_VIEWPORT_H * uiScale_ * scY)));
        const auto& shaders = GetShaderCatalog();
        for (int i = 0; i < static_cast<int>(NUM_SHADERS); ++i) {
            float itemY = 185.0f + (i / 2) * 125.0f - shaderScrollY_;
            Rectangle sr = { uiX + 60.0f + (i % 2) * 565.0f, itemY, 545.0f, 108.0f };
            bool unlocked = profile_.unlockedShaders[i];
            bool equipped = (profile_.equippedShader == i);

            DrawRectangleRec(sr, equipped ? Color{ 34, 42, 62, 245 } : Color{ 16, 22, 34, 235 });
            DrawRectangleLinesEx(sr, equipped ? 3.0f : 1.5f, equipped ? GOLD : Color{ 140, 160, 190, 160 });

            // 4-quadrant shader swatch
            Vector2 sw = { sr.x + 48.0f, sr.y + 54.0f };
            DrawCircleV(sw, 24.0f, shaders[i].primary);
            DrawPolyLinesEx(sw, 4, 28.0f, 0.0f, 2.5f, shaders[i].secondary);
            DrawPoly(sw, 4, 12.0f, 0.0f, shaders[i].core);

            DrawText(shaders[i].name, static_cast<int>(sr.x + 96), static_cast<int>(sr.y + 18), 18, WHITE);

            Rectangle btn = { sr.x + 96.0f, sr.y + 54.0f, 220.0f, 36.0f };
            if (equipped) {
                DrawRectangleRec(btn, GOLD);
                DrawText("EQUIPPED SHADER", static_cast<int>(btn.x + 36), static_cast<int>(btn.y + 10), 14, BLACK);
            } else if (unlocked) {
                DrawRectangleRec(btn, Color{ 38, 68, 98, 255 });
                DrawText("APPLY SHADER", static_cast<int>(btn.x + 52), static_cast<int>(btn.y + 10), 14, WHITE);
            } else {
                bool canBuy = (profile_.glimmer >= shaders[i].glimmerCost);
                DrawRectangleRec(btn, canBuy ? Color{ 30, 88, 60, 255 } : Color{ 48, 30, 35, 255 });
                char cstr[64];
                std::snprintf(cstr, sizeof(cstr), "PURCHASE (%d G)", shaders[i].glimmerCost);
                DrawText(cstr, static_cast<int>(btn.x + 38), static_cast<int>(btn.y + 10), 14, canBuy ? GREEN : RED);
            }

            if (shaderReleaseTap && itemY >= 140.0f && itemY <= 570.0f && CheckCollisionPointRec(shaderTapPos, sr)) {
                if (unlocked) {
                    profile_.equippedShader = i;
                    SaveData();
                    audio_.PlayUiSelect();
                } else if (profile_.glimmer >= shaders[i].glimmerCost) {
                    profile_.glimmer -= shaders[i].glimmerCost;
                    profile_.unlockedShaders[i] = true;
                    profile_.equippedShader = i;
                    int shCount = 0;
                    for (bool b : profile_.unlockedShaders) if (b) shCount++;
                    if (shCount >= 6) UnlockTriumph(15);
                    SaveData();
                    audio_.PlayExoticPickup();
                }
            }
        }
        EndScissorMode();

        // Elegant Destiny vertical scrollbar
        if (maxShaderScroll > 0.0f) {
            DrawRectangle(static_cast<int>(trackX), static_cast<int>(trackY), 6, static_cast<int>(trackH), { 25, 35, 50, 200 });
            float thumbH = std::max(40.0f, trackH * (SHADER_VIEWPORT_H / TOTAL_SHADER_HEIGHT));
            float thumbY = trackY + (shaderScrollY_ / maxShaderScroll) * (trackH - thumbH);
            Rectangle thumbR = { trackX - 2.0f, thumbY, 10.0f, thumbH };
            bool hov = IsRectHovered(thumbR);
            DrawRectangleRec(thumbR, hov ? GOLD : Color{ 80, 140, 200, 220 });
        }
    } else {
        // Tab 4: Xur's Exotic Ghost Catalysts
        DrawText("XUR, AGENT OF THE NINE // EXOTIC GHOST CATALYSTS", static_cast<int>(uiX + 60), 118, 22, GOLD);
        DrawText("Trade Glimmer & Nightfall Legendary Shards with Xur for passive Exotic combat catalysts.", static_cast<int>(uiX + 60), 146, 16, LIGHTGRAY);

        const auto& exotics = GetExoticCatalog();
        for (int i = 0; i < static_cast<int>(NUM_EXOTIC_PERKS); ++i) {
            Rectangle er = { uiX + 60.0f + (i % 2) * 580.0f, 185.0f + (i / 2) * 180.0f, 560.0f, 160.0f };
            bool unlocked = profile_.unlockedExotics[i];
            bool equipped = (profile_.equippedExoticPerk == i);

            DrawRectangleRec(er, equipped ? Color{ 34, 42, 62, 245 } : Color{ 16, 22, 34, 235 });
            DrawRectangleLinesEx(er, equipped ? 3.0f : 1.5f, equipped ? GOLD : exotics[i].accentColor);

            DrawPoly({ er.x + 48.0f, er.y + 80.0f }, 6, 28.0f, globalTime_ * 45.0f, exotics[i].accentColor);
            DrawPolyLinesEx({ er.x + 48.0f, er.y + 80.0f }, 6, 32.0f, -globalTime_ * 45.0f, 2.0f, WHITE);

            DrawText(exotics[i].name, static_cast<int>(er.x + 96), static_cast<int>(er.y + 18), 20, exotics[i].accentColor);
            DrawTextWrapped(exotics[i].description, static_cast<int>(er.x + 96), static_cast<int>(er.y + 48), 440, 15, 20, WHITE);

            Rectangle btn = { er.x + 96.0f, er.y + 106.0f, 250.0f, 38.0f };
            if (equipped) {
                DrawRectangleRec(btn, GOLD);
                int bw = MeasureText("EQUIPPED CATALYST", 15);
                DrawText("EQUIPPED CATALYST", static_cast<int>(btn.x + (btn.width - bw) * 0.5f), static_cast<int>(btn.y + 11), 15, BLACK);
            } else if (unlocked) {
                DrawRectangleRec(btn, { 38, 68, 98, 255 });
                int bw = MeasureText("EQUIP CATALYST", 15);
                DrawText("EQUIP CATALYST", static_cast<int>(btn.x + (btn.width - bw) * 0.5f), static_cast<int>(btn.y + 11), 15, WHITE);
            } else {
                bool canBuy = (profile_.glimmer >= exotics[i].glimmerCost && profile_.legendaryShards >= exotics[i].shardCost);
                DrawRectangleRec(btn, canBuy ? Color{ 30, 88, 60, 255 } : Color{ 48, 30, 35, 255 });
                char cstr[64];
                if (exotics[i].shardCost > 0) {
                    std::snprintf(cstr, sizeof(cstr), "ACQUIRE (%dG, %d SHARDS)", exotics[i].glimmerCost, exotics[i].shardCost);
                } else {
                    std::snprintf(cstr, sizeof(cstr), "ACQUIRE (%d GLIMMER)", exotics[i].glimmerCost);
                }
                int bw = MeasureText(cstr, 14);
                DrawText(cstr, static_cast<int>(btn.x + (btn.width - bw) * 0.5f), static_cast<int>(btn.y + 12), 14, canBuy ? GREEN : RED);
            }

            if (IsButtonClicked(er)) {
                if (unlocked) {
                    profile_.equippedExoticPerk = i;
                    SaveData();
                    audio_.PlayUiSelect();
                } else if (profile_.glimmer >= exotics[i].glimmerCost && profile_.legendaryShards >= exotics[i].shardCost) {
                    profile_.glimmer -= exotics[i].glimmerCost;
                    profile_.legendaryShards -= exotics[i].shardCost;
                    profile_.unlockedExotics[i] = true;
                    profile_.equippedExoticPerk = i;
                    SaveData();
                    audio_.PlayExoticPickup();
                }
            }
        }
    }

    Rectangle backBtn = { uiX + 50.0f, 585.0f, 235.0f, 48.0f };
    DrawRectangleRec(backBtn, IsRectHovered(backBtn) ? Color{ 45, 65, 95, 255 } : Color{ 25, 35, 52, 255 });
    DrawRectangleLinesEx(backBtn, 2.0f, SKYBLUE);
    DrawText("< RETURN TO DIRECTOR", static_cast<int>(backBtn.x + 18), static_cast<int>(backBtn.y + 16), 16, WHITE);
    if (IsButtonClicked(backBtn)) {
        SetScreenState(ScreenState::TITLE_DIRECTOR);
        audio_.PlayUiSelect();
    }
}

void GhostGame::RenderSettingsView() {
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 8, 12, 22, 245 });
    float uiX = (VIRTUAL_WIDTH - 1280.0f) * 0.5f;

    DrawText("SETTINGS", static_cast<int>(uiX + 70), 28, 26, WHITE);
    Rectangle tabSet0 = { uiX + 380.0f, 24.0f, 220.0f, 34.0f };
    Rectangle tabSet1 = { uiX + 612.0f, 24.0f, 220.0f, 34.0f };
    DrawRectangleRec(tabSet0, (settingsTab_ == 0) ? SKYBLUE : Color{ 22, 30, 48, 255 });
    DrawRectangleLinesEx(tabSet0, 1.5f, (settingsTab_ == 0) ? WHITE : SKYBLUE);
    int s0w = MeasureText("OPTIONS", 14);
    DrawText("OPTIONS", static_cast<int>(tabSet0.x + (tabSet0.width - s0w) * 0.5f), 34, 14, (settingsTab_ == 0) ? BLACK : WHITE);
    DrawRectangleRec(tabSet1, (settingsTab_ == 1) ? GOLD : Color{ 22, 30, 48, 255 });
    DrawRectangleLinesEx(tabSet1, 1.5f, (settingsTab_ == 1) ? WHITE : GOLD);
    int s1w = MeasureText("CONTROLS", 14);
    DrawText("CONTROLS", static_cast<int>(tabSet1.x + (tabSet1.width - s1w) * 0.5f), 34, 14, (settingsTab_ == 1) ? BLACK : WHITE);
    if (IsButtonClicked(tabSet0)) { settingsTab_ = 0; audio_.PlayUiSelect(); }
    if (IsButtonClicked(tabSet1)) { settingsTab_ = 1; audio_.PlayUiSelect(); }

    char devBuf[128];
    std::snprintf(devBuf, sizeof(devBuf), "INPUT: %s", activeInputDevice_.c_str());
    int dw = MeasureText(devBuf, 15);
    DrawText(devBuf, static_cast<int>(uiX + 1210.0f) - dw, 34, 15, GOLD);

    DrawLine(static_cast<int>(uiX + 70), 72, static_cast<int>(uiX + 1210), 72, { 255, 255, 255, 55 });

    if (settingsTab_ == 1) {
        auto DrawCtrlCard = [&](float x, const char* hdr, Color col, const std::initializer_list<const char*>& rows) {
            Rectangle cr = { x, 96.0f, 360.0f, 420.0f };
            DrawRectangleRec(cr, { 14, 20, 36, 230 });
            DrawRectangleLinesEx(cr, 2.0f, col);
            DrawRectangle(static_cast<int>(cr.x), static_cast<int>(cr.y), static_cast<int>(cr.width), 36, { 22, 32, 52, 245 });
            DrawText(hdr, static_cast<int>(cr.x + 18), static_cast<int>(cr.y + 10), 16, col);
            float ry = cr.y + 54.0f;
            for (const char* line : rows) {
                DrawText(line, static_cast<int>(cr.x + 18), static_cast<int>(ry), 14, WHITE);
                ry += 38.0f;
            }
        };
        DrawCtrlCard(uiX + 70.0f, "KEYBOARD & MOUSE", SKYBLUE, {
            "SPACE / W / Click : Flap",
            "SHIFT / E / R-Click : Tactical Blink",
            "C / X : Fire Combat Drone",
            "F / Q : Cast Nightfall Super",
            "N : Next Soundtrack",
            "M : Mute Audio",
            "F11 / Alt+Enter : Fullscreen",
            "ESC / P : Pause Flight"
        });
        DrawCtrlCard(uiX + 460.0f, "GAMEPAD CONTROLLER", GOLD, {
            "(A) / Cross : Flap",
            "(B) / LB : Tactical Blink",
            "(X) / RT : Fire Combat Drone",
            "(Y) / RB : Cast Nightfall Super",
            "R-Stick Click : Next Soundtrack",
            "START / Options : Pause Flight"
        });
        DrawCtrlCard(uiX + 850.0f, "MOBILE & TOUCH", GREEN, {
            "Tap Anywhere : Flap",
            "Hold Tap (Dawnblade) : Solar Glide",
            "[BLINK / DASH] : Tactical Ability",
            "[FIRE DRONE] : Combat Drone (Nightfall)",
            "[CAST SUPER] : Super Ability (Nightfall)",
            "Top-Right [II] : Pause Flight"
        });
        DrawLine(static_cast<int>(uiX + 70), 545, static_cast<int>(uiX + 1210), 545, { 255, 255, 255, 45 });
        Rectangle backBtn = { uiX + 70.0f, 580.0f, 260.0f, 48.0f };
        DrawRectangleRec(backBtn, IsRectHovered(backBtn) ? Color{ 45, 65, 95, 255 } : Color{ 25, 35, 52, 255 });
        DrawRectangleLinesEx(backBtn, 2.0f, SKYBLUE);
        DrawText("< BACK", static_cast<int>(backBtn.x + 98), static_cast<int>(backBtn.y + 16), 16, WHITE);
        if (IsButtonClicked(backBtn) || IsKeyPressed(KEY_ESCAPE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
            SaveData();
            SetScreenState(settingsPrevState_);
            audio_.PlayUiSelect();
        }
        return;
    }

    // --- LEFT COLUMN: AUDIO CONTROLS ---
    DrawText("AUDIO BALANCING", static_cast<int>(uiX + 70), 92, 18, SKYBLUE);

    auto DrawSlider = [&](float y, const char* label, float& val, Color barCol) {
        DrawText(label, static_cast<int>(uiX + 70), static_cast<int>(y), 16, WHITE);
        Rectangle track = { uiX + 70.0f, y + 26.0f, 480.0f, 22.0f };
        DrawRectangleRec(track, { 18, 24, 38, 255 });
        DrawRectangleLinesEx(track, 1.5f, { 80, 110, 150, 200 });

        float fillW = std::clamp(val, 0.0f, 1.0f) * track.width;
        DrawRectangle(static_cast<int>(track.x), static_cast<int>(track.y), static_cast<int>(fillW), static_cast<int>(track.height), barCol);

        char pct[16];
        std::snprintf(pct, sizeof(pct), "%d%%", static_cast<int>(val * 100.0f));
        DrawText(pct, static_cast<int>(track.x + track.width + 16.0f), static_cast<int>(track.y + 2.0f), 16, WHITE);

        // Interaction
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0) {
            Vector2 p = (GetTouchPointCount() > 0) ? ToVirtualCoords(GetTouchPosition(0)) : GetVirtualMousePos();
            Rectangle hit = { track.x - 10.0f, track.y - 10.0f, track.width + 20.0f, track.height + 20.0f };
            if (CheckCollisionPointRec(p, hit)) {
                val = std::clamp((p.x - track.x) / track.width, 0.0f, 1.0f);
                audio_.SetMasterVolume(profile_.masterVolume);
                audio_.SetMusicVolume(profile_.musicVolume);
                audio_.SetSfxVolume(profile_.sfxVolume);
            }
        }
    };

    DrawSlider(124.0f, "MASTER VOLUME", profile_.masterVolume, GOLD);
    DrawSlider(188.0f, "MUSIC VOLUME (BGM)", profile_.musicVolume, SKYBLUE);
    DrawSlider(252.0f, "SOUND EFFECTS (SFX)", profile_.sfxVolume, ORANGE);

    // Mute Toggle Button
    Rectangle muteBtn = { uiX + 70.0f, 324.0f, 240.0f, 44.0f };
    bool isMuted = audio_.IsMuted();
    DrawRectangleRec(muteBtn, isMuted ? Color{ 75, 25, 30, 255 } : Color{ 22, 45, 38, 255 });
    DrawRectangleLinesEx(muteBtn, 1.8f, isMuted ? RED : GREEN);
    DrawText(isMuted ? "MUTED [M]" : "AUDIO ACTIVE [M]", static_cast<int>(muteBtn.x + 38), static_cast<int>(muteBtn.y + 13), 16, WHITE);
    if (IsButtonClicked(muteBtn)) {
        audio_.ToggleMute();
        audio_.ApplyVolumes();
    }

    // --- GAMEPLAY SOUNDTRACK JUKEBOX SELECTOR ---
    DrawText("GAMEPLAY SOUNDTRACK JUKEBOX [N]:", static_cast<int>(uiX + 70), 390, 15, WHITE);
    Rectangle prevTrackBtn = { uiX + 70.0f, 412.0f, 42.0f, 38.0f };
    Rectangle nextTrackBtn = { uiX + 508.0f, 412.0f, 42.0f, 38.0f };
    Rectangle trackCardRec = { uiX + 118.0f, 412.0f, 384.0f, 38.0f };

    DrawRectangleRec(prevTrackBtn, IsRectHovered(prevTrackBtn) ? Color{ 40, 65, 100, 255 } : Color{ 20, 28, 44, 220 });
    DrawRectangleLinesEx(prevTrackBtn, 1.5f, SKYBLUE);
    DrawText("<", static_cast<int>(prevTrackBtn.x + 15), static_cast<int>(prevTrackBtn.y + 9), 18, WHITE);
    if (IsButtonClicked(prevTrackBtn)) {
        profile_.musicTrackChoice = (profile_.musicTrackChoice + 4) % 5;
        audio_.SetMusicTrackPreference(profile_.musicTrackChoice);
        audio_.PlayUiSelect();
    }

    DrawRectangleRec(nextTrackBtn, IsRectHovered(nextTrackBtn) ? Color{ 40, 65, 100, 255 } : Color{ 20, 28, 44, 220 });
    DrawRectangleLinesEx(nextTrackBtn, 1.5f, SKYBLUE);
    DrawText(">", static_cast<int>(nextTrackBtn.x + 16), static_cast<int>(nextTrackBtn.y + 9), 18, WHITE);
    if (IsButtonClicked(nextTrackBtn)) {
        profile_.musicTrackChoice = (profile_.musicTrackChoice + 1) % 5;
        audio_.SetMusicTrackPreference(profile_.musicTrackChoice);
        audio_.PlayUiSelect();
    }

    DrawRectangleRec(trackCardRec, { 16, 24, 38, 240 });
    DrawRectangleLinesEx(trackCardRec, 1.5f, profile_.musicTrackChoice == 0 ? GOLD : SKYBLUE);
    const char* tLabel = (profile_.musicTrackChoice == 0)
        ? "AUTO-SHUFFLE ALL 4 TRACKS [DEFAULT]"
        : audio_.GetActionTrackName(profile_.musicTrackChoice - 1);
    int tlw = MeasureText(tLabel, 12);
    DrawText(tLabel, static_cast<int>(trackCardRec.x + (trackCardRec.width - tlw) / 2.0f), static_cast<int>(trackCardRec.y + 12), 12, profile_.musicTrackChoice == 0 ? GOLD : WHITE);

    DrawText("* Press [N] during flights to skip track instantly", static_cast<int>(uiX + 70), 458, 12, { 140, 180, 220, 200 });

    // --- RIGHT COLUMN: VISUALS, TOUCH & ACCESSIBILITY ---
    DrawText("VISUALS & ACCESSIBILITY", static_cast<int>(uiX + 660), 92, 18, SKYBLUE);

    auto drawCenteredOptBtn = [&](Rectangle r, const char* lbl, int fs, bool sel) {
        DrawRectangleRec(r, sel ? Color{ 35, 65, 105, 255 } : Color{ 18, 24, 38, 220 });
        DrawRectangleLinesEx(r, sel ? 2.0f : 1.2f, sel ? GOLD : GRAY);
        int lw = MeasureText(lbl, fs);
        DrawText(lbl, static_cast<int>(r.x + (r.width - lw) * 0.5f), static_cast<int>(r.y + 10), fs, sel ? GOLD : LIGHTGRAY);
    };

    // 1. UI Text Size (Accessibility)
    DrawText("UI TEXT SIZE (ACCESSIBILITY):", static_cast<int>(uiX + 660), 118, 15, WHITE);
    const char* txtSizeLabels[3] = { "NORMAL (100%)", "LARGE (115%)", "EXTRA LARGE" };
    for (int ts = 0; ts < 3; ++ts) {
        Rectangle tb = { uiX + 660.0f + ts * 168.0f, 141.0f, 156.0f, 36.0f };
        bool sel = (profile_.textSize == ts);
        drawCenteredOptBtn(tb, txtSizeLabels[ts], 12, sel);
        if (IsButtonClicked(tb)) {
            profile_.textSize = ts;
            RefreshUiFonts(uiScale_);
            audio_.PlayUiSelect();
        }
    }

    // 2. Screen Shake Scale
    DrawText("SCREEN SHAKE INTENSITY:", static_cast<int>(uiX + 660), 190, 15, WHITE);
    const char* shakeLabels[3] = { "OFF (0%)", "NORMAL (50%)", "INTENSE (100%)" };
    float shakeVals[3] = { 0.0f, 0.5f, 1.0f };
    for (int i = 0; i < 3; ++i) {
        Rectangle sb = { uiX + 660.0f + i * 168.0f, 213.0f, 156.0f, 36.0f };
        bool sel = (std::abs(profile_.screenShakeScale - shakeVals[i]) < 0.15f);
        drawCenteredOptBtn(sb, shakeLabels[i], 12, sel);
        if (IsButtonClicked(sb)) {
            profile_.screenShakeScale = shakeVals[i];
            audio_.PlayUiSelect();
        }
    }

    // 3. Screen Flash Pulses Toggle
    DrawText("SOLAR / EXPLOSION SCREEN FLASHES:", static_cast<int>(uiX + 660), 262, 15, WHITE);
    Rectangle flashOnBtn  = { uiX + 660.0f, 285.0f, 160.0f, 36.0f };
    Rectangle flashOffBtn = { uiX + 832.0f, 285.0f, 160.0f, 36.0f };
    drawCenteredOptBtn(flashOnBtn, "ENABLED", 13, profile_.screenFlashes);
    if (IsButtonClicked(flashOnBtn)) { profile_.screenFlashes = true; audio_.PlayUiSelect(); }
    drawCenteredOptBtn(flashOffBtn, "DISABLED", 13, !profile_.screenFlashes);
    if (IsButtonClicked(flashOffBtn)) { profile_.screenFlashes = false; audio_.PlayUiSelect(); }

    // 4. Target Framerate Mode
    DrawText("PERFORMANCE / TARGET FPS:", static_cast<int>(uiX + 660), 334, 15, WHITE);
    int fpsVals[3] = { 60, 120, 30 };
    const char* fpsLabels[3] = { "60 FPS", "120 FPS", "30 FPS" };
    for (int f = 0; f < 3; ++f) {
        Rectangle fb = { uiX + 660.0f + f * 168.0f, 357.0f, 156.0f, 36.0f };
        bool sel = (profile_.targetFps == fpsVals[f]);
        drawCenteredOptBtn(fb, fpsLabels[f], 13, sel);
        if (IsButtonClicked(fb)) {
            profile_.targetFps = fpsVals[f];
            SetTargetFPS(profile_.targetFps);
            audio_.PlayUiSelect();
        }
    }

    // 5. Touch Button Handedness Mode
    DrawText("MOBILE TOUCH CONTROLS HANDEDNESS:", static_cast<int>(uiX + 660), 406, 15, WHITE);
    Rectangle handRight = { uiX + 660.0f, 429.0f, 210.0f, 36.0f };
    Rectangle handLeft  = { uiX + 882.0f, 429.0f, 210.0f, 36.0f };
    drawCenteredOptBtn(handRight, "RIGHT-HANDED", 13, !profile_.leftHandedControls);
    if (IsButtonClicked(handRight)) { profile_.leftHandedControls = false; audio_.PlayUiSelect(); }
    drawCenteredOptBtn(handLeft, "LEFT-HANDED", 13, profile_.leftHandedControls);
    if (IsButtonClicked(handLeft)) { profile_.leftHandedControls = true; audio_.PlayUiSelect(); }

    // Clear Save Data Button (Right Column)
    Rectangle resetBtn = { uiX + 660.0f, 482.0f, 310.0f, 40.0f };
    DrawRectangleRec(resetBtn, showResetConfirm_ ? Color{ 110, 30, 35, 255 } : Color{ 48, 20, 26, 230 });
    DrawRectangleLinesEx(resetBtn, 2.0f, RED);
    const char* rstLbl = showResetConfirm_ ? "CONFIRM RESET PROGRESS?" : "CLEAR SAVE DATA (RESET)";
    int rlw = MeasureText(rstLbl, 13);
    DrawText(rstLbl, static_cast<int>(resetBtn.x + (resetBtn.width - rlw) * 0.5f), static_cast<int>(resetBtn.y + 12), 13, WHITE);
    if (IsButtonClicked(resetBtn)) {
        if (!showResetConfirm_) {
            showResetConfirm_ = true;
            audio_.PlayUiSelect();
        } else {
            // Reset profile back to clean factory state while preserving user settings
            SaveProfile fresh{};
            fresh.masterVolume = profile_.masterVolume;
            fresh.musicVolume = profile_.musicVolume;
            fresh.sfxVolume = profile_.sfxVolume;
            fresh.screenShakeScale = profile_.screenShakeScale;
            fresh.screenFlashes = profile_.screenFlashes;
            fresh.targetFps = profile_.targetFps;
            fresh.leftHandedControls = profile_.leftHandedControls;
            fresh.musicTrackChoice = profile_.musicTrackChoice;
            fresh.textSize = profile_.textSize;
            profile_ = fresh;
            SaveData();
            showResetConfirm_ = false;
            audio_.PlayDeath();
        }
    }
    if (showResetConfirm_) {
        Rectangle cancelReset = { uiX + 985.0f, 482.0f, 110.0f, 40.0f };
        DrawRectangleRec(cancelReset, { 28, 38, 52, 240 });
        DrawRectangleLinesEx(cancelReset, 1.5f, SKYBLUE);
        int clw = MeasureText("CANCEL", 13);
        DrawText("CANCEL", static_cast<int>(cancelReset.x + (cancelReset.width - clw) * 0.5f), static_cast<int>(cancelReset.y + 12), 13, WHITE);
        if (IsButtonClicked(cancelReset)) { showResetConfirm_ = false; audio_.PlayUiSelect(); }
    }

    // --- BOTTOM ROW: FOOTER & BACK ---
    DrawLine(static_cast<int>(uiX + 70), 545, static_cast<int>(uiX + 1210), 545, { 255, 255, 255, 45 });

    // Engine & Build Info
    const char* bInfo = "DESTINY: THE FLAPPY GHOST  |  v1.0  |  SEBINY LABS";
    int biw = MeasureText(bInfo, 13);
    DrawText(bInfo, (VIRTUAL_WIDTH - biw) / 2, 555, 13, { 140, 175, 215, 180 });

    // Back to Previous Screen Button
    Rectangle backBtn = { uiX + 70.0f, 580.0f, 260.0f, 48.0f };
    DrawRectangleRec(backBtn, IsRectHovered(backBtn) ? Color{ 45, 65, 95, 255 } : Color{ 25, 35, 52, 255 });
    DrawRectangleLinesEx(backBtn, 2.0f, SKYBLUE);
    DrawText("< BACK", static_cast<int>(backBtn.x + 98), static_cast<int>(backBtn.y + 16), 16, WHITE);
    if (IsButtonClicked(backBtn) || IsKeyPressed(KEY_ESCAPE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
        SaveData();
        SetScreenState(settingsPrevState_);
        audio_.PlayUiSelect();
    }
}

void GhostGame::RenderCareerStats() {
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 8, 12, 24, 245 });
    float uiX = (VIRTUAL_WIDTH - 1280.0f) * 0.5f;

    DrawText("RECORDS", static_cast<int>(uiX + 70), 28, 26, WHITE);
    Rectangle tabTri = { uiX + 430.0f, 24.0f, 200.0f, 34.0f };
    Rectangle tabCar = { uiX + 642.0f, 24.0f, 200.0f, 34.0f };
    DrawRectangleRec(tabTri, Color{ 22, 30, 48, 255 });
    DrawRectangleLinesEx(tabTri, 1.5f, GREEN);
    int ttw = MeasureText("TRIUMPHS", 14);
    DrawText("TRIUMPHS", static_cast<int>(tabTri.x + (tabTri.width - ttw) * 0.5f), 34, 14, WHITE);
    DrawRectangleRec(tabCar, GOLD);
    DrawRectangleLinesEx(tabCar, 1.5f, WHITE);
    int tcw = MeasureText("CAREER", 14);
    DrawText("CAREER", static_cast<int>(tabCar.x + (tabCar.width - tcw) * 0.5f), 34, 14, BLACK);
    if (IsButtonClicked(tabTri)) { SetScreenState(ScreenState::TRIUMPHS_LOG); audio_.PlayUiSelect(); return; }

    char glimBuf[128];
    std::snprintf(glimBuf, sizeof(glimBuf), "GLIMMER: %d   |   SHARDS: %d", profile_.glimmer, profile_.legendaryShards);
    int gbw = MeasureText(glimBuf, 18);
    DrawText(glimBuf, static_cast<int>(uiX + 1210.0f) - gbw, 32, 18, SKYBLUE);

    DrawLine(static_cast<int>(uiX + 70), 68, static_cast<int>(uiX + 1210), 68, { 255, 255, 255, 55 });

    auto DrawStatRow = [](float x, float y, float w, const char* label, const std::string& val, Color valCol) {
        DrawText(label, static_cast<int>(x), static_cast<int>(y), 15, WHITE);
        int vw = MeasureText(val.c_str(), 15);
        DrawText(val.c_str(), static_cast<int>(x + w - vw), static_cast<int>(y), 15, valCol);
        DrawLine(static_cast<int>(x), static_cast<int>(y + 24), static_cast<int>(x + w), static_cast<int>(y + 24), { 255, 255, 255, 20 });
    };

    // LEFT CARD: FLIGHT & NIGHTFALL COMBAT METRICS
    Rectangle leftCard = { uiX + 70.0f, 84.0f, 550.0f, 475.0f };
    DrawRectangleRec(leftCard, { 14, 20, 36, 230 });
    DrawRectangleLinesEx(leftCard, 2.0f, ORANGE);
    DrawRectangle(static_cast<int>(leftCard.x), static_cast<int>(leftCard.y), static_cast<int>(leftCard.width), 34, { 38, 22, 16, 240 });
    DrawText("VANGUARD FLIGHT & NIGHTFALL COMBAT RECORDS", static_cast<int>(leftCard.x + 18), static_cast<int>(leftCard.y + 9), 15, GOLD);

    float sy = leftCard.y + 48.0f;
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "Total Distance Flown (Lifetime Gates):", std::to_string(profile_.totalDistanceFlown) + " Gates", SKYBLUE);
    sy += 38.0f;
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "Best Vanguard Patrol Score:", std::to_string(profile_.bestScoreStandard) + " Gates", GOLD);
    sy += 38.0f;
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "Best Nightfall Strike Score:", std::to_string(profile_.bestScoreNightfall) + " Gates", GOLD);
    sy += 38.0f;
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "Total Flight Sorties Launched:", std::to_string(profile_.totalRunsStarted), WHITE);
    sy += 38.0f;
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "Paracausal Super Abilities Unleashed:", std::to_string(profile_.totalSupersActivated), ORANGE);
    sy += 38.0f;
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "Quantum Blink Teleports Executed:", std::to_string(profile_.totalBlinkTeleports), PURPLE);
    sy += 38.0f;
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "High-Value Boss Entities Slain:", std::to_string(profile_.totalBossesDefeated), RED);
    sy += 38.0f;
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "Light & Dark Motes Synthesized:", std::to_string(profile_.totalMotesCollected), GREEN);
    sy += 38.0f;
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "Daily Vanguard Bounties Completed:", std::to_string(profile_.totalBountiesCompleted), GOLD);
    sy += 38.0f;
    const char* subName = "Voidwalker";
    switch (static_cast<LightSubclass>(profile_.selectedSubclass)) {
        case LightSubclass::VOID_WALKER: subName = "Voidwalker (Void)"; break;
        case LightSubclass::SOLAR_DAWN: subName = "Dawnblade (Solar)"; break;
        case LightSubclass::ARC_CALLER: subName = "Stormcaller (Arc)"; break;
        case LightSubclass::STASIS_SHADEBINDER: subName = "Shadebinder (Stasis)"; break;
        case LightSubclass::STRAND_RUNNER: subName = "Threadrunner (Strand)"; break;
        case LightSubclass::PRISMATIC_TRANSCENDENT: subName = "Transcendent (Prismatic)"; break;
    }
    DrawStatRow(leftCard.x + 20, sy, leftCard.width - 40, "Active Combat Subclass:", subName, SKYBLUE);

    // RIGHT CARD: GUARDIAN INVENTORY & SYSTEM ACCREDITATION
    Rectangle rightCard = { uiX + 660.0f, 84.0f, 550.0f, 475.0f };
    DrawRectangleRec(rightCard, { 14, 20, 36, 230 });
    DrawRectangleLinesEx(rightCard, 2.0f, SKYBLUE);
    DrawRectangle(static_cast<int>(rightCard.x), static_cast<int>(rightCard.y), static_cast<int>(rightCard.width), 34, { 18, 28, 48, 240 });
    DrawText("GUARDIAN ARSENAL & SYSTEM ACCREDITATION", static_cast<int>(rightCard.x + 18), static_cast<int>(rightCard.y + 9), 15, SKYBLUE);

    int shellCount = 0;
    for (bool b : profile_.unlockedShells) if (b) shellCount++;
    int subCount = 0;
    for (bool b : profile_.unlockedSubclasses) if (b) subCount++;
    int droneCount = 0;
    for (bool b : profile_.unlockedDrones) if (b) droneCount++;
    int shaderCount = 0;
    for (bool b : profile_.unlockedShaders) if (b) shaderCount++;
    int triCount = 0;
    for (bool b : profile_.unlockedTriumphs) if (b) triCount++;

    sy = rightCard.y + 48.0f;
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "All-Time Glimmer & Shards Earned:",
                std::to_string(profile_.totalGlimmerEarnedLifetime) + " G  |  " + std::to_string(profile_.totalShardsEarnedLifetime) + " S", GOLD);
    sy += 38.0f;
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "Exotic Ghost Shells Acquired:", std::to_string(shellCount) + " / " + std::to_string(NUM_SHELLS), SKYBLUE);
    sy += 38.0f;
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "Paracausal Subclasses Mastered:", std::to_string(subCount) + " / " + std::to_string(NUM_SUBCLASSES), PURPLE);
    sy += 38.0f;
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "Tactical Combat Drones Requisitioned:", std::to_string(droneCount) + " / " + std::to_string(NUM_DRONES), ORANGE);
    sy += 38.0f;
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "Cosmetic Armor Shaders Collected:", std::to_string(shaderCount) + " / " + std::to_string(NUM_SHADERS), WHITE);
    sy += 38.0f;
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "Vanguard Triumphs Sealed:", std::to_string(triCount) + " / " + std::to_string(NUM_TRIUMPHS), GREEN);
    sy += 38.0f;
    std::string droneName = (profile_.equippedDrone >= 0) ? GetDroneCatalog()[profile_.equippedDrone].name : "NONE (LOCKED)";
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "Equipped Combat Drone:", droneName, (profile_.equippedDrone >= 0) ? GOLD : RED);
    sy += 38.0f;
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "Active Ghost Shell Chassis:", GetShellCatalog()[profile_.equippedShell].name, GOLD);
    sy += 38.0f;
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "Current Soundtrack:", audio_.GetCurrentActionTrackName(), ORANGE);

    // Bottom Navigation Bar
    Rectangle statsBackBtn = { uiX + 70.0f, 585.0f, 235.0f, 48.0f };
    DrawRectangleRec(statsBackBtn, IsRectHovered(statsBackBtn) ? Color{ 45, 65, 95, 255 } : Color{ 25, 35, 52, 255 });
    DrawRectangleLinesEx(statsBackBtn, 2.0f, SKYBLUE);
    DrawText("< RETURN TO DIRECTOR", static_cast<int>(statsBackBtn.x + 18), static_cast<int>(statsBackBtn.y + 16), 16, WHITE);
    if (IsButtonClicked(statsBackBtn) || IsKeyPressed(KEY_ESCAPE) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
        SetScreenState(ScreenState::TITLE_DIRECTOR);
        audio_.PlayUiSelect();
    }
}

void GhostGame::RenderTriumphsLog() {
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 8, 12, 22, 230 });
    float uiX = (VIRTUAL_WIDTH - 1280.0f) * 0.5f;
    DrawText("RECORDS", static_cast<int>(uiX + 70), 28, 26, WHITE);
    Rectangle tabTri = { uiX + 430.0f, 24.0f, 200.0f, 34.0f };
    Rectangle tabCar = { uiX + 642.0f, 24.0f, 200.0f, 34.0f };
    DrawRectangleRec(tabTri, GREEN);
    DrawRectangleLinesEx(tabTri, 1.5f, WHITE);
    int ttw = MeasureText("TRIUMPHS", 14);
    DrawText("TRIUMPHS", static_cast<int>(tabTri.x + (tabTri.width - ttw) * 0.5f), 34, 14, BLACK);
    DrawRectangleRec(tabCar, Color{ 22, 30, 48, 255 });
    DrawRectangleLinesEx(tabCar, 1.5f, GOLD);
    int tcw = MeasureText("CAREER", 14);
    DrawText("CAREER", static_cast<int>(tabCar.x + (tabCar.width - tcw) * 0.5f), 34, 14, WHITE);
    if (IsButtonClicked(tabCar)) { SetScreenState(ScreenState::CAREER_STATS); audio_.PlayUiSelect(); return; }

    char glimHeader[96];
    std::snprintf(glimHeader, sizeof(glimHeader), "GLIMMER: %d   |   SHARDS: %d", profile_.glimmer, profile_.legendaryShards);
    int ghw = MeasureText(glimHeader, 18);
    DrawText(glimHeader, static_cast<int>(uiX + 1210.0f) - ghw, 32, 18, SKYBLUE);
    DrawLine(static_cast<int>(uiX + 70), 68, static_cast<int>(uiX + 1210), 68, { 255, 255, 255, 55 });

    // --- SECTION 1: ROTATING DAILY VANGUARD BOUNTIES (3 ACTIVE) ---
    DrawText("ACTIVE DAILY BOUNTIES (RESETS EVERY 24H)", static_cast<int>(uiX + 70), 72, 16, GOLD);
    for (size_t b = 0; b < NUM_DAILY_BOUNTIES; ++b) {
        const auto& bounty = dailyBounties_[b];
        Rectangle br = { uiX + 70.0f + b * 380.0f, 96.0f, 365.0f, 95.0f };
        bool done = bounty.completed;
        DrawRectangleRec(br, done ? Color{ 22, 46, 38, 235 } : Color{ 14, 20, 32, 220 });
        DrawRectangleLinesEx(br, 1.5f, done ? GREEN : (bounty.currentProgress > 0 ? SKYBLUE : Color{ 60, 75, 95, 200 }));

        DrawText(bounty.title, static_cast<int>(br.x + 14), static_cast<int>(br.y + 10), 16, done ? GREEN : GOLD);
        DrawTextWrapped(bounty.description, static_cast<int>(br.x + 14), static_cast<int>(br.y + 30), 335, 12, 16, LIGHTGRAY);

        // Progress bar
        float ratio = std::clamp(static_cast<float>(bounty.currentProgress) / std::max(1, bounty.targetCount), 0.0f, 1.0f);
        DrawRectangle(static_cast<int>(br.x + 14), static_cast<int>(br.y + 68), 220, 14, { 25, 32, 45, 255 });
        DrawRectangle(static_cast<int>(br.x + 14), static_cast<int>(br.y + 68), static_cast<int>(220 * ratio), 14, done ? GREEN : SKYBLUE);

        char pStr[48];
        std::snprintf(pStr, sizeof(pStr), "%d / %d", bounty.currentProgress, bounty.targetCount);
        DrawText(pStr, static_cast<int>(br.x + 242), static_cast<int>(br.y + 68), 13, WHITE);

        char rStr[32];
        std::snprintf(rStr, sizeof(rStr), done ? "CLAIMED" : "+%d G", bounty.rewardGlimmer);
        DrawText(rStr, static_cast<int>(br.x + 300), static_cast<int>(br.y + 68), 13, done ? GREEN : GOLD);
    }

    // --- SECTION 2: PERMANENT VANGUARD TRIUMPHS (With smooth scrolling & scrollbar) ---
    DrawText("LIFETIME TRIUMPHS & RECORDS (Scroll/Drag scrollbar for all 16 Triumphs)", static_cast<int>(uiX + 70), 204, 16, SKYBLUE);

    constexpr float TOTAL_TRIUMPH_HEIGHT = 8 * 66.0f;
    constexpr float TRIUMPH_VIEWPORT_H = 345.0f;
    float maxTriumphScroll = std::max(0.0f, TOTAL_TRIUMPH_HEIGHT - TRIUMPH_VIEWPORT_H + 15.0f);
    float tWheel = GetMouseWheelMove();
    if (tWheel != 0.0f) {
        triumphScrollY_ = std::clamp(triumphScrollY_ - tWheel * 45.0f, 0.0f, maxTriumphScroll);
    }

    float tTrackX = uiX + 1205.0f;
    float tTrackY = 228.0f;
    float tTrackH = TRIUMPH_VIEWPORT_H;
    Rectangle triumphViewport = { uiX + 60.0f, 225.0f, 1140.0f, TRIUMPH_VIEWPORT_H };
    bool tPtrDown = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0);
    if (maxTriumphScroll > 0.0f && tPtrDown) {
        Vector2 p = (GetTouchPointCount() > 0) ? ToVirtualCoords(GetTouchPosition(0)) : GetVirtualMousePos();
        Rectangle trackHit = { tTrackX - 24.0f, tTrackY - 10.0f, 52.0f, tTrackH + 20.0f };
        if (CheckCollisionPointRec(p, trackHit)) {
            float frac = std::clamp((p.y - tTrackY) / tTrackH, 0.0f, 1.0f);
            triumphScrollY_ = frac * maxTriumphScroll;
            listDragging_ = false;
        } else if (!listDragging_ && CheckCollisionPointRec(p, triumphViewport)) {
            listDragging_ = true;
            listDragLastPos_ = p;
        } else if (listDragging_) {
            float dy = p.y - listDragLastPos_.y;
            listDragLastPos_ = p;
            triumphScrollY_ = std::clamp(triumphScrollY_ - dy, 0.0f, maxTriumphScroll);
        }
    } else {
        listDragging_ = false;
    }

    float scX = static_cast<float>(GetScreenWidth()) / std::max(1.0f, static_cast<float>(GetRenderWidth()));
    float scY = static_cast<float>(GetScreenHeight()) / std::max(1.0f, static_cast<float>(GetRenderHeight()));
    BeginScissorMode(static_cast<int>(std::round((uiOffsetX_ + (uiX + 60.0f) * uiScale_) * scX)),
                     static_cast<int>(std::round((uiOffsetY_ + 225.0f * uiScale_) * scY)),
                     static_cast<int>(std::round(1140.0f * uiScale_ * scX)),
                     static_cast<int>(std::round(TRIUMPH_VIEWPORT_H * uiScale_ * scY)));
    const auto& cat = GetTriumphCatalog();
    for (int i = 0; i < static_cast<int>(NUM_TRIUMPHS); ++i) {
        int col = i % 2;
        int row = i / 2;
        float itemY = 228.0f + row * 66.0f - triumphScrollY_;
        Rectangle tr = { uiX + 70.0f + col * 565.0f, itemY, 545.0f, 60.0f };
        bool done = profile_.unlockedTriumphs[i];

        DrawRectangleRec(tr, done ? Color{ 24, 42, 54, 240 } : Color{ 14, 18, 28, 220 });
        DrawRectangleLinesEx(tr, 1.2f, done ? GOLD : Color{ 60, 70, 85, 180 });

        DrawPoly({ tr.x + 24.0f, tr.y + 30.0f }, 4, 12.0f, 0.0f, done ? GOLD : DARKGRAY);
        DrawText(cat[i].title, static_cast<int>(tr.x + 46), static_cast<int>(tr.y + 8), 15, done ? GOLD : LIGHTGRAY);
        DrawTextWrapped(cat[i].description, static_cast<int>(tr.x + 46), static_cast<int>(tr.y + 28), 380, 12, 15, WHITE);

        char rew[48];
        std::snprintf(rew, sizeof(rew), done ? "CLAIMED" : "+%d G", cat[i].rewardGlimmer);
        DrawText(rew, static_cast<int>(tr.x + 445), static_cast<int>(tr.y + 20), 14, done ? GREEN : SKYBLUE);
    }
    EndScissorMode();

    if (maxTriumphScroll > 0.0f) {
        DrawRectangle(static_cast<int>(tTrackX), static_cast<int>(tTrackY), 6, static_cast<int>(tTrackH), { 25, 35, 50, 200 });
        float thumbH = std::max(40.0f, tTrackH * (TRIUMPH_VIEWPORT_H / TOTAL_TRIUMPH_HEIGHT));
        float thumbY = tTrackY + (triumphScrollY_ / maxTriumphScroll) * (tTrackH - thumbH);
        Rectangle thumbR = { tTrackX - 2.0f, thumbY, 10.0f, thumbH };
        bool hov = IsRectHovered(thumbR);
        DrawRectangleRec(thumbR, hov ? GOLD : Color{ 80, 140, 200, 220 });
    }

    Rectangle backBtn = { uiX + 70.0f, 585.0f, 235.0f, 48.0f };
    DrawRectangleRec(backBtn, IsRectHovered(backBtn) ? Color{ 45, 65, 95, 255 } : Color{ 25, 35, 52, 255 });
    DrawRectangleLinesEx(backBtn, 2.0f, SKYBLUE);
    DrawText("< RETURN TO DIRECTOR", static_cast<int>(backBtn.x + 18), static_cast<int>(backBtn.y + 16), 16, WHITE);
    if (IsButtonClicked(backBtn)) {
        SetScreenState(ScreenState::TITLE_DIRECTOR);
        audio_.PlayUiSelect();
    }
}

void GhostGame::RenderPauseOverlay() {
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 6, 10, 18, 195 });
    const char* pTitle = "GHOST TELEMETRY PAUSED";
    int pw = MeasureText(pTitle, 38);
    DrawText(pTitle, (VIRTUAL_WIDTH - pw) / 2, 196, 38, WHITE);

#if defined(PLATFORM_ANDROID)
    bool touchClean = true;
#else
    bool touchClean = (activeInputDevice_ == "TOUCH SCREEN");
#endif
    bool hasPad = IsGamepadAvailable(0);

    const char* resLbl = touchClean ? "RESUME"   : (hasPad ? "RESUME (A)"    : "RESUME [SPACE]");
    const char* rstLbl = touchClean ? "RESTART"  : "RESTART [R]";
    const char* setLbl = "SETTINGS";
    const char* dirLbl = touchClean ? "DIRECTOR" : (hasPad ? "DIRECTOR (B)"  : "DIRECTOR [ESC]");

    auto DrawPauseBtn = [&](Rectangle r, const char* lbl, Color borderCol, Color hovBg, Color idleBg) -> bool {
        bool hov = IsRectHovered(r);
        DrawRectangleRec(r, hov ? hovBg : idleBg);
        DrawRectangleLinesEx(r, 2.0f, borderCol);
        int lw = MeasureText(lbl, 17);
        DrawText(lbl, static_cast<int>(r.x + (r.width - lw) * 0.5f), static_cast<int>(r.y + (r.height - 17.0f) * 0.5f), 17, WHITE);
        return IsButtonClicked(r);
    };

    Rectangle resumeBtn   = { VIRTUAL_WIDTH / 2.0f - 160.0f, 258.0f, 320.0f, 50.0f };
    Rectangle restartBtn  = { VIRTUAL_WIDTH / 2.0f - 160.0f, 320.0f, 320.0f, 50.0f };
    Rectangle settingsBtn = { VIRTUAL_WIDTH / 2.0f - 160.0f, 382.0f, 320.0f, 50.0f };
    Rectangle dirBtn      = { VIRTUAL_WIDTH / 2.0f - 160.0f, 444.0f, 320.0f, 50.0f };

    if (DrawPauseBtn(resumeBtn, resLbl, SKYBLUE, Color{ 40, 75, 110, 255 }, Color{ 22, 36, 56, 255 })) {
        unpauseRampTimer_ = 0.75f;
        SetScreenState(ScreenState::PLAYING);
        audio_.PlayUiSelect();
    }
    if (DrawPauseBtn(restartBtn, rstLbl, GREEN, Color{ 38, 86, 66, 255 }, Color{ 20, 48, 38, 255 })) {
        StartNewRun(isNightfallMode_);
    }
    if (DrawPauseBtn(settingsBtn, setLbl, PURPLE, Color{ 55, 45, 80, 255 }, Color{ 28, 22, 42, 255 })) {
        settingsTab_ = 0;
        settingsPrevState_ = ScreenState::PAUSED;
        SetScreenState(ScreenState::SETTINGS_VIEW);
        audio_.PlayUiSelect();
    }
    if (DrawPauseBtn(dirBtn, dirLbl, ORANGE, Color{ 75, 35, 45, 255 }, Color{ 38, 22, 30, 255 })) {
        SaveData();
        SetScreenState(ScreenState::TITLE_DIRECTOR);
        audio_.PlayUiSelect();
    }
}

void GhostGame::RenderLightFadedOverlay() {
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 12, 6, 10, 215 });
    DrawRectangle(0, 175, VIRTUAL_WIDTH, 115, { 45, 12, 16, 230 });
    DrawLine(0, 175, VIRTUAL_WIDTH, 175, { 220, 55, 55, 255 });
    DrawLine(0, 290, VIRTUAL_WIDTH, 290, { 220, 55, 55, 255 });

    const char* skullHeader = "DARKNESS ZONE // GUARDIAN DOWN";
    int shw = MeasureText(skullHeader, 18);
    DrawText(skullHeader, (VIRTUAL_WIDTH - shw) / 2, 192, 18, { 240, 110, 110, 255 });

    const char* fadedText = "YOUR LIGHT FADES AWAY...";
    int ftw = MeasureText(fadedText, 44);
    DrawText(fadedText, (VIRTUAL_WIDTH - ftw) / 2, 224, 44, WHITE);

    DrawRectangle(VIRTUAL_WIDTH / 2 - 230, 318, 460, 140, { 16, 20, 32, 235 });
    DrawRectangleLinesEx({ VIRTUAL_WIDTH / 2.0f - 230.0f, 318.0f, 460.0f, 140.0f }, 1.5f, GRAY);

    char line1[96], line2[96], line3[96];
    std::snprintf(line1, sizeof(line1), "BARRIERS CLEARED: %d   (BEST: %d)",
                  scoreGates_, isNightfallMode_ ? profile_.bestScoreNightfall : profile_.bestScoreStandard);
    std::snprintf(line2, sizeof(line2), "MOTES OF LIGHT HARVESTED: %d", runMotes_);
    if (isNightfallMode_ || runShardsEarned_ > 0) {
        std::snprintf(line3, sizeof(line3), "GLIMMER: +%d   |   LEGENDARY SHARDS: +%d", runGlimmerEarned_, runShardsEarned_);
    } else {
        std::snprintf(line3, sizeof(line3), "GLIMMER EARNED THIS RUN: +%d", runGlimmerEarned_);
    }

    DrawText(line1, VIRTUAL_WIDTH / 2 - 190, 338, 20, GOLD);
    DrawText(line2, VIRTUAL_WIDTH / 2 - 190, 374, 18, SKYBLUE);
    DrawText(line3, VIRTUAL_WIDTH / 2 - 190, 408, 18, WHITE);

#if defined(PLATFORM_ANDROID)
    bool touchClean = true;
#else
    bool touchClean = (activeInputDevice_ == "TOUCH SCREEN");
#endif
    bool hasPad = IsGamepadAvailable(0);

    const char* revLbl = touchClean ? "REVIVE"   : (hasPad ? "REVIVE (A)"   : "REVIVE [SPACE]");
    const char* ldtLbl = touchClean ? "LOADOUT"  : (hasPad ? "LOADOUT (Y)"  : "LOADOUT [L]");
    const char* dirLbl = touchClean ? "DIRECTOR" : (hasPad ? "DIRECTOR (B)" : "DIRECTOR [ESC]");

    float btnW = 192.0f;
    float gap = 14.0f;
    float rowX = VIRTUAL_WIDTH * 0.5f - (btnW * 3.0f + gap * 2.0f) * 0.5f;
    Rectangle revBtn = { rowX,                     482.0f, btnW, 56.0f };
    Rectangle ldtBtn = { rowX + (btnW + gap),      482.0f, btnW, 56.0f };
    Rectangle dirBtn = { rowX + (btnW + gap) * 2,  482.0f, btnW, 56.0f };

    auto DrawOverBtn = [&](Rectangle r, const char* lbl, Color borderCol, Color hovBg, Color idleBg) -> bool {
        bool hov = IsRectHovered(r);
        DrawRectangleRec(r, hov ? hovBg : idleBg);
        DrawRectangleLinesEx(r, 2.0f, borderCol);
        int lw = MeasureText(lbl, 17);
        DrawText(lbl, static_cast<int>(r.x + (r.width - lw) * 0.5f), static_cast<int>(r.y + (r.height - 17.0f) * 0.5f), 17, WHITE);
        return IsButtonClicked(r);
    };

    if (DrawOverBtn(revBtn, revLbl, GREEN, Color{ 45, 95, 75, 255 }, Color{ 25, 62, 48, 255 })) {
        StartNewRun(isNightfallMode_);
    }
    if (DrawOverBtn(ldtBtn, ldtLbl, GetSubclassAccentColor(), Color{ 48, 42, 82, 255 }, Color{ 26, 24, 48, 255 })) {
        SetScreenState(ScreenState::SUBCLASS_ARMORY);
        audio_.PlayUiSelect();
    }
    if (HasAnyAffordableUpgrade()) {
        DrawUpgradeBadge(ldtBtn, globalTime_);
    }
    if (DrawOverBtn(dirBtn, dirLbl, SKYBLUE, Color{ 45, 65, 95, 255 }, Color{ 25, 38, 58, 255 })) {
        SetScreenState(ScreenState::TITLE_DIRECTOR);
        audio_.PlayUiSelect();
    }
}

void GhostGame::RenderTriumphToast() const {
    if (!activeToast_.active) return;

    float w = 620.0f;
    float x = (VIRTUAL_WIDTH - w) * 0.5f;
    float y = VIRTUAL_HEIGHT - 125.0f;

    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), 64, { 14, 22, 34, 245 });
    DrawRectangleLinesEx({ x, y, w, 64.0f }, 2.5f, GOLD);
    DrawPoly({ x + 34.0f, y + 32.0f }, 4, 18.0f, 0.0f, GOLD);

    DrawText(activeToast_.title.c_str(), static_cast<int>(x + 66), static_cast<int>(y + 12), 18, GOLD);
    DrawText(activeToast_.subtitle.c_str(), static_cast<int>(x + 66), static_cast<int>(y + 36), 14, WHITE);
}

} // namespace FlappyGhost
