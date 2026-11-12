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
void DrawTextWrapped(const char* text, int x, int y, int maxWidth, int fontSize, int lineHeight, Color color) {
    if (!text || !text[0]) return;
    std::string input(text);
    std::string currentLine;
    std::string word;
    int curY = y;

    auto flushLine = [&]() {
        if (!currentLine.empty()) {
            DrawText(currentLine.c_str(), x, curY, fontSize, color);
            curY += lineHeight;
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
    BeginTextureMode(targetRenderTex_);
    ClearBackground(BLACK);

    Vector2 shakeOffset = { 0.0f, 0.0f };
    if (screenShakeTrauma_ > 0.0f && profile_.screenShakeScale > 0.0f) {
        float mag = screenShakeTrauma_ * screenShakeTrauma_ * 18.0f * profile_.screenShakeScale;
        shakeOffset.x = std::sin(globalTime_ * 67.0f) * mag;
        shakeOffset.y = std::cos(globalTime_ * 53.0f) * mag;
    }

    Camera2D cam{};
    cam.offset = shakeOffset;
    cam.target = { 0.0f, 0.0f };
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;

    BeginMode2D(cam);

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

        // Strand Grapple Tether Line
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

    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle src = { 0.0f, 0.0f, static_cast<float>(targetRenderTex_.texture.width),
                      -static_cast<float>(targetRenderTex_.texture.height) };
    Rectangle dst = { 0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()) };
    DrawTexturePro(targetRenderTex_.texture, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
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
    Color bossCol = (boss_.bossType == 0) ? PURPLE :
                    (boss_.bossType == 1) ? GOLD :
                    (boss_.bossType == 2) ? GREEN : MAGENTA;

    if (boss_.shieldImmune) {
        DrawCircleLines(static_cast<int>(bp.x), static_cast<int>(bp.y), 74.0f + std::sin(globalTime_ * 8.0f) * 4.0f, SKYBLUE);
        DrawCircle(static_cast<int>(bp.x), static_cast<int>(bp.y), 72.0f, { 100, 210, 255, 45 });
    }

    DrawPoly(bp, 8, 56.0f, globalTime_ * 35.0f, { 28, 30, 42, 255 });
    DrawPolyLinesEx(bp, 8, 56.0f, globalTime_ * 35.0f, 3.5f, bossCol);
    DrawCircleGradient(static_cast<int>(bp.x), static_cast<int>(bp.y), 28.0f, WHITE, bossCol);

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

    float tacReady = 1.0f - std::clamp(ghost_.tacticalCooldown / ghost_.tacticalMaxCooldown, 0.0f, 1.0f);
    if (isNightfallMode_) {
        DrawRectangle(102, 56, 245, 16, { 30, 36, 48, 255 });
        Color superBarCol = (ghost_.superActiveTimer > 0.0f) ? WHITE :
                            (superRatio >= 1.0f) ? GOLD : subCol;
        DrawRectangle(102, 56, static_cast<int>(245.0f * superRatio), 16, superBarCol);
        DrawRectangleLines(102, 56, 245, 16, WHITE);

        if (ghost_.superActiveTimer > 0.0f) {
            DrawText("SUPER ACTIVE!", 160, 57, 14, BLACK);
        } else if (superRatio >= 1.0f) {
            DrawText("SUPER READY [F / Q]", 145, 57, 14, BLACK);
        }

        DrawRectangle(102, 80, 140, 10, { 30, 36, 48, 255 });
        DrawRectangle(102, 80, static_cast<int>(140.0f * tacReady), 10, (tacReady >= 1.0f) ? SKYBLUE : GRAY);
        DrawText((tacReady >= 1.0f) ? "TACTICAL: READY" : "RECHARGING...", 250, 78, 12, LIGHTGRAY);
    } else {
        DrawText("PATROL // TACTICAL FLIGHT MODE", 102, 56, 13, SKYBLUE);
        DrawRectangle(102, 78, 155, 12, { 30, 36, 48, 255 });
        DrawRectangle(102, 78, static_cast<int>(155.0f * tacReady), 12, (tacReady >= 1.0f) ? SKYBLUE : GRAY);
        DrawRectangleLines(102, 78, 155, 12, { 255, 255, 255, 120 });
        DrawText((tacReady >= 1.0f) ? "BLINK READY" : "CHARGING...", 264, 78, 12, (tacReady >= 1.0f) ? WHITE : LIGHTGRAY);
    }

    for (int i = 0; i < ghost_.overshieldCharges; ++i) {
        DrawPoly({ 36.0f + i * 24.0f, 126.0f }, 6, 9.0f, 0.0f, SKYBLUE);
    }

    // Top-Right Score, Glimmer & Streak Tracker
    DrawRectangle(VIRTUAL_WIDTH - 295, 26, 273, 86, { 10, 14, 24, 200 });
    DrawRectangleLinesEx({ VIRTUAL_WIDTH - 295.0f, 26.0f, 273.0f, 86.0f }, 1.5f, { 255, 255, 255, 65 });

    char scoreBuf[64];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "GATES: %d  (x%d)", scoreGates_, streakGates_);
    DrawText(scoreBuf, VIRTUAL_WIDTH - 278, 34, 24, WHITE);

    char glimBuf[64];
    std::snprintf(glimBuf, sizeof(glimBuf), "GLIMMER: %d (+%d)", profile_.glimmer, runGlimmerEarned_);
    DrawText(glimBuf, VIRTUAL_WIDTH - 278, 66, 16, SKYBLUE);

    DrawText(isNightfallMode_ ? "MODE: NIGHTFALL STRIKE" : "MODE: VANGUARD PATROL",
             VIRTUAL_WIDTH - 278, 88, 14, isNightfallMode_ ? GOLD : LIGHTGRAY);

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
        std::snprintf(medTxt, sizeof(medTxt), "%s  (+%d GLIMMER)", activeMedal_.medalName.c_str(), activeMedal_.bonusGlimmer);
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
    const char* drnHint = hasGamepad ? "[(B) / RT]" : "[C / X]";
    const char* tacHint = hasGamepad ? "[(X) / LB]" : "[SHIFT / RMB]";
    const char* supHint = hasGamepad ? "[(Y) / RB]" : "[F / Q]";

    if (isNightfallMode_ && profile_.equippedDrone >= 0 && profile_.equippedDrone < static_cast<int>(NUM_DRONES)) {
        const auto& droneDef = GetDroneCatalog()[profile_.equippedDrone];
        float drnReady = 1.0f - std::clamp(ghost_.droneCooldown / std::max(0.05f, ghost_.droneMaxCooldown), 0.0f, 1.0f);
        bool drnFeedback = (touchFeedbackDrn_ > 0.0f);
        DrawRectangleRec(drnBtn, drnFeedback ? Color{ 45, 85, 120, 255 } : ((drnReady >= 1.0f) ? Color{ 28, 52, 74, 225 } : Color{ 22, 26, 34, 185 }));
        DrawRectangleLinesEx(drnBtn, drnFeedback ? 2.5f : 2.0f, drnFeedback ? WHITE : ((drnReady >= 1.0f) ? droneDef.color : GRAY));
        DrawText("FIRE DRONE", static_cast<int>(drnBtn.x + 12), static_cast<int>(drnBtn.y + 12), 14, WHITE);
        DrawText(drnHint, static_cast<int>(drnBtn.x + (hasGamepad ? 22 : 30)), static_cast<int>(drnBtn.y + 30), 12, LIGHTGRAY);
        DrawRectangle(static_cast<int>(drnBtn.x + 10), static_cast<int>(drnBtn.y + 48), 92, 6, { 18, 22, 30, 255 });
        DrawRectangle(static_cast<int>(drnBtn.x + 10), static_cast<int>(drnBtn.y + 48), static_cast<int>(92.0f * drnReady), 6, droneDef.color);
    }

    bool tacFeedback = (touchFeedbackTac_ > 0.0f);
    DrawRectangleRec(tacBtn, tacFeedback ? Color{ 55, 105, 155, 255 } : ((tacReady >= 1.0f) ? Color{ 35, 65, 95, 210 } : Color{ 25, 28, 35, 180 }));
    DrawRectangleLinesEx(tacBtn, tacFeedback ? 2.5f : 2.0f, tacFeedback ? WHITE : ((tacReady >= 1.0f) ? SKYBLUE : GRAY));
    DrawText("BLINK / DASH", static_cast<int>(tacBtn.x + 10), static_cast<int>(tacBtn.y + 14), 14, WHITE);
    DrawText(tacHint, static_cast<int>(tacBtn.x + 10), static_cast<int>(tacBtn.y + 34), 12, LIGHTGRAY);

    if (isNightfallMode_) {
        bool supFeedback = (touchFeedbackSup_ > 0.0f);
        DrawRectangleRec(supBtn, supFeedback ? Color{ 145, 115, 35, 255 } : ((superRatio >= 1.0f) ? Color{ 95, 75, 20, 230 } : Color{ 25, 28, 35, 180 }));
        DrawRectangleLinesEx(supBtn, supFeedback ? 2.5f : 2.0f, supFeedback ? WHITE : ((superRatio >= 1.0f) ? GOLD : GRAY));
        DrawText("CAST SUPER", static_cast<int>(supBtn.x + 12), static_cast<int>(supBtn.y + 14), 14, (superRatio >= 1.0f || supFeedback) ? GOLD : GRAY);
        DrawText(supHint, static_cast<int>(supBtn.x + (hasGamepad ? 24 : 32)), static_cast<int>(supBtn.y + 34), 12, LIGHTGRAY);
    }

    // First-Run Control Hint Pill (fades cleanly after 4.5s on a player's very first run)
    if (profile_.totalRunsStarted <= 1 && runTime_ < 4.5f) {
        float fade = (runTime_ > 3.5f) ? std::clamp(1.0f - (runTime_ - 3.5f), 0.0f, 1.0f) : 1.0f;
        unsigned char bgA = static_cast<unsigned char>(fade * 210.0f);
        unsigned char txtA = static_cast<unsigned char>(fade * 245.0f);
#if defined(PLATFORM_ANDROID)
        const char* hint = isNightfallMode_
            ? "CONTROLS: Tap Screen = Flap  |  [BLINK/DASH] & [SUPER]: Combat  |  [FIRE DRONE]: Shoot"
            : "CONTROLS: Tap Anywhere = Flap  |  Tap [BLINK / DASH] to Phase Through Hazards";
#else
        const char* hint = hasGamepad
            ? (isNightfallMode_
                ? "CONTROLS: [(A)] Flap  |  [(B)/LB] Blink  |  [(Y)/RB] Super  |  [(X)/RT] Drone"
                : "CONTROLS: [(A)] Flap  |  [(B)/LB] Tactical Blink")
            : (isNightfallMode_
                ? "CONTROLS: [SPACE / Click] Flap  |  [SHIFT / RMB] Blink  |  [F / Q] Super  |  [C / X] Drone"
                : "CONTROLS: [SPACE / Click] Flap  |  [SHIFT / RMB] Tactical Blink");
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
    const char* preTitle = "DIRECTOR // VANGUARD FLIGHT TELEMETRY";
    int ptw = MeasureText(preTitle, 15);
    DrawText(preTitle, (VIRTUAL_WIDTH - ptw) / 2, 28, 15, { 170, 210, 240, 220 });

    const char* mainDestiny = "DESTINY";
    int mdw = MeasureText(mainDestiny, 52);
    DrawText(mainDestiny, (VIRTUAL_WIDTH - mdw) / 2, 48, 52, WHITE);

    const char* subGameTitle = "THE FLAPPY GHOST";
    int fgw = MeasureText(subGameTitle, 20);
    DrawText(subGameTitle, (VIRTUAL_WIDTH - fgw) / 2, 104, 20, SKYBLUE);

    DrawLine(VIRTUAL_WIDTH / 2 - 280, 134, VIRTUAL_WIDTH / 2 + 280, 134, { 255, 255, 255, 90 });

    Vector2 previewPos = { VIRTUAL_WIDTH * 0.5f, 238.0f + std::sin(globalTime_ * 2.5f) * 8.0f };
    DrawCircleLines(static_cast<int>(previewPos.x), static_cast<int>(previewPos.y), 78.0f, { 255, 255, 255, 45 });
    RenderGhostRig(previewPos, 36.0f, std::sin(globalTime_ * 1.7f) * 8.0f,
                   0.25f + 0.15f * std::sin(globalTime_ * 3.2f),
                   globalTime_ * 1.4f, { std::cos(globalTime_ * 1.5f) * 5.0f, std::sin(globalTime_ * 2.1f) * 3.0f },
                   profile_.equippedShell, 1.0f);

    const auto& equippedShell = GetShellCatalog()[profile_.equippedShell];
    int snw = MeasureText(equippedShell.name, 16);
    DrawText(equippedShell.name, (VIRTUAL_WIDTH - snw) / 2, 326, 16, SKYBLUE);

    char statLine[160];
    std::snprintf(statLine, sizeof(statLine),
                  "GLIMMER: %d   |   BEST PATROL: %d GATES   |   BEST NIGHTFALL: %d GATES   |   BOSSES SLAIN: %d",
                  profile_.glimmer, profile_.bestScoreStandard, profile_.bestScoreNightfall, profile_.totalBossesDefeated);
    int slw = MeasureText(statLine, 16);
    DrawText(statLine, (VIRTUAL_WIDTH - slw) / 2, 355, 16, LIGHTGRAY);

    auto DrawDirectorButton = [&](Rectangle r, const char* title, const char* sub, Color accent) -> bool {
        bool hov = IsRectHovered(r);
        DrawRectangleRec(r, hov ? Color{ 28, 42, 66, 235 } : Color{ 14, 20, 34, 215 });
        DrawRectangleLinesEx(r, hov ? 2.5f : 1.5f, hov ? accent : Color{ 140, 165, 195, 140 });
        DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), 6, static_cast<int>(r.height), accent);

        int maxW = static_cast<int>(r.width) - 28;
        int titleFs = 20;
        while (titleFs > 12 && MeasureText(title, titleFs) > maxW) titleFs--;
        int subFs = 14;
        while (subFs > 10 && MeasureText(sub, subFs) > maxW) subFs--;

        int textBlockH = titleFs + 5 + subFs;
        int topPad = std::max(8, (static_cast<int>(r.height) - textBlockH) / 2);
        DrawText(title, static_cast<int>(r.x + 18), static_cast<int>(r.y) + topPad, titleFs, WHITE);
        DrawText(sub, static_cast<int>(r.x + 18), static_cast<int>(r.y) + topPad + titleFs + 5, subFs, LIGHTGRAY);
        return IsButtonClicked(r);
    };

    bool hasGamepad = IsGamepadAvailable(0);

    float uiX = (VIRTUAL_WIDTH - 1280.0f) * 0.5f;
    Rectangle btnPatrol   = { uiX + 180.0f, 392.0f, 440.0f, 62.0f };
    Rectangle btnNightfall= { uiX + 660.0f, 392.0f, 440.0f, 62.0f };
    Rectangle btnSubclass = { uiX + 72.0f, 464.0f, 216.0f, 58.0f };
    Rectangle btnHangar   = { uiX + 304.0f, 464.0f, 216.0f, 58.0f };
    Rectangle btnTriumphs = { uiX + 536.0f, 464.0f, 216.0f, 58.0f };
    Rectangle btnCareer   = { uiX + 768.0f, 464.0f, 216.0f, 58.0f };
    Rectangle btnSettings = { uiX + 1000.0f, 464.0f, 216.0f, 58.0f };

    const char* patrolLabel = hasGamepad ? "LAUNCH VANGUARD PATROL [(A)]" : "LAUNCH VANGUARD PATROL [SPACE]";
    if (DrawDirectorButton(btnPatrol, patrolLabel, "Pure flight & tactical blinks (no Supers/Drones)", SKYBLUE)) {
        StartNewRun(false);
    }

    bool nightfallUnlocked = HasUnlockedAnyDrone();
    const char* nfLabel = hasGamepad ? "LAUNCH NIGHTFALL STRIKE [(X)]" : "LAUNCH NIGHTFALL STRIKE [N]";
    if (nightfallUnlocked) {
        int dIdx = std::clamp(profile_.equippedDrone, 0, static_cast<int>(NUM_DRONES) - 1);
        std::string nfSub = std::string("Drone: ") + GetDroneCatalog()[dIdx].name + " + Supers | +1.5x Glimmer";
        if (DrawDirectorButton(btnNightfall, nfLabel, nfSub.c_str(), GOLD)) {
            StartNewRun(true);
        }
    } else {
        if (DrawDirectorButton(btnNightfall, "NIGHTFALL STRIKE [LOCKED]", "Unlock your first Combat Drone in The Tower (200G)!", RED)) {
            towerTab_ = 2; // Open directly to Combat Drones tab in The Tower!
            activeToast_.active = true;
            activeToast_.title = "NIGHTFALL REQUIRES A COMBAT DRONE";
            activeToast_.subtitle = "Unlock your first manual-fire Combat Drone here (starting at 200 Glimmer)!";
            activeToast_.timer = 4.0f;
            SetScreenState(ScreenState::SHELL_HANGAR);
            audio_.PlayUiSelect();
        }
    }

    const char* curSubLabel = "Equipped: Voidwalker";
    switch (static_cast<LightSubclass>(profile_.selectedSubclass)) {
        case LightSubclass::VOID_WALKER:            curSubLabel = "Equipped: Voidwalker"; break;
        case LightSubclass::SOLAR_DAWN:             curSubLabel = "Equipped: Dawnblade"; break;
        case LightSubclass::ARC_CALLER:             curSubLabel = "Equipped: Stormcaller"; break;
        case LightSubclass::STASIS_SHADEBINDER:     curSubLabel = "Equipped: Shadebinder"; break;
        case LightSubclass::STRAND_RUNNER:          curSubLabel = "Equipped: Threadrunner"; break;
        case LightSubclass::PRISMATIC_TRANSCENDENT: curSubLabel = "Equipped: Transcendent"; break;
    }
    const char* subBtnTitle = hasGamepad ? "SUBCLASS [(LB)]" : "SUBCLASS [U]";
    if (DrawDirectorButton(btnSubclass, subBtnTitle, curSubLabel, GetSubclassAccentColor())) {
        SetScreenState(ScreenState::SUBCLASS_ARMORY);
        audio_.PlayUiSelect();
    }

    const char* towerTitle = hasGamepad ? "TOWER [(Y)]" : "TOWER [T]";
    if (DrawDirectorButton(btnHangar, towerTitle, "Shells & Drones", ORANGE)) {
        SetScreenState(ScreenState::SHELL_HANGAR);
        audio_.PlayUiSelect();
    }

    int unlockedCount = 0;
    for (bool b : profile_.unlockedTriumphs) if (b) unlockedCount++;
    char triSub[64];
    std::snprintf(triSub, sizeof(triSub), "%d / %d Triumphs", unlockedCount, static_cast<int>(NUM_TRIUMPHS));
    const char* triTitle = hasGamepad ? "TRIUMPHS [(RB)]" : "TRIUMPHS [R]";
    if (DrawDirectorButton(btnTriumphs, triTitle, triSub, GREEN)) {
        SetScreenState(ScreenState::TRIUMPHS_LOG);
        audio_.PlayUiSelect();
    }

    char carSub[64];
    std::snprintf(carSub, sizeof(carSub), "Runs: %d | Gates: %d", profile_.totalRunsStarted, profile_.totalDistanceFlown);
    if (DrawDirectorButton(btnCareer, "CAREER [C]", carSub, GOLD)) {
        SetScreenState(ScreenState::CAREER_STATS);
        audio_.PlayUiSelect();
    }

    const char* setBtnTitle = hasGamepad ? "SETTINGS [BACK]" : "SETTINGS [S]";
    if (DrawDirectorButton(btnSettings, setBtnTitle, "Audio & Controls", PURPLE)) {
        settingsPrevState_ = ScreenState::TITLE_DIRECTOR;
        SetScreenState(ScreenState::SETTINGS_VIEW);
        audio_.PlayUiSelect();
    }

    const char* ctrlHint = hasGamepad
                         ? "CONTROLS: [(A) / START] Launch Patrol  |  [(X)] Nightfall  |  [(Y)] Tower  |  [(B)] Back"
                         : "CONTROLS: [SPACE] Patrol  |  [N] Nightfall  |  [T] Tower  |  [C] Career  |  [N-Key in run] Next OST Track";
    int chw = MeasureText(ctrlHint, 14);
    DrawText(ctrlHint, (VIRTUAL_WIDTH - chw) / 2, 545, 14, { 180, 210, 235, 215 });

    RenderBottomBrandingFooter();
}

void GhostGame::RenderSubclassArmory() {
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 8, 12, 22, 230 });
    float uiX = (VIRTUAL_WIDTH - 1280.0f) * 0.5f;
    DrawText("GUARDIAN SUBCLASS ARMORY // LIGHT & DARKNESS", static_cast<int>(uiX + 50), 24, 26, WHITE);
    DrawText("Tacticals work everywhere. Super abilities charge & unleash exclusively in Nightfall Strike!", static_cast<int>(uiX + 50), 56, 15, LIGHTGRAY);

    char glimHeader[64];
    std::snprintf(glimHeader, sizeof(glimHeader), "AVAILABLE GLIMMER: %d", profile_.glimmer);
    int ghw = MeasureText(glimHeader, 20);
    DrawText(glimHeader, static_cast<int>(uiX + 1230.0f) - ghw, 28, 20, SKYBLUE);

    struct SubCard {
        int idx;
        const char* name;
        const char* element;
        const char* tacticalDesc;
        const char* superDesc;
        int glimmerCost;
        Color col;
    };

    const SubCard cards[NUM_SUBCLASSES] = {
        { 0, "VOIDWALKER", "VOID LIGHT",
          "TACTICAL [BLINK]: Short phase teleport (0.32s) through an immediate barrier ahead.",
          "NIGHTFALL SUPER [NOVA BOMB]: Hurls a Void singularity clearing bosses, shields & 1 barrier.",
          0, { 175, 85, 255, 255 } },
        { 1, "DAWNBLADE", "SOLAR LIGHT",
          "TACTICAL [ICARUS DASH]: Stabilize momentum, hover glide & phase through hazards.",
          "NIGHTFALL SUPER [DAYBREAK]: Every flap hurls homing Solar blades & burns enemy fire.",
          0, { 255, 145, 35, 255 } },
        { 2, "STORMCALLER", "ARC LIGHT",
          "TACTICAL [IONIC BLINK]: High-voltage phase dash with rapid vertical recovery.",
          "NIGHTFALL SUPER [CHAOS REACH]: Aimable Arc laser melting bosses, enemies & Oracles.",
          0, { 75, 220, 255, 255 } },
        { 3, "SHADEBINDER", "STASIS DARKNESS",
          "TACTICAL [FROST PULSE]: Freezes moving Vex lasers, Oracles & Boss shots for 2.5s.",
          "NIGHTFALL SUPER [WINTER'S WRATH]: Freezes hazards & shatters all foes/shots into Motes.",
          500, { 70, 130, 255, 255 } },
        { 4, "THREADRUNNER", "STRAND DARKNESS",
          "TACTICAL [GRAPPLE]: Launches a Strand grapple line ahead to swing & phase upward.",
          "NIGHTFALL SUPER [NEEDLESTORM]: Unleashes a 12-Threadling swarm hunting foes & Bosses.",
          500, { 65, 245, 130, 255 } },
        { 5, "TRANSCENDENT", "PRISMATIC",
          "TACTICAL [PRISMATIC SURGE]: Freezes obstacles & surges with Light + Dark phase.",
          "NIGHTFALL SUPER [TRANSCENDENCE]: 4x Tactical recharge + dual Light/Dark homing bursts!",
          1500, { 255, 125, 235, 255 } }
    };

    // Clean, spacious 3x2 grid (Row 0 = 3 Light Subclasses, Row 1 = 3 Darkness/Prismatic Subclasses)
    for (int i = 0; i < static_cast<int>(NUM_SUBCLASSES); ++i) {
        int col = i % 3;
        int row = i / 3;
        Rectangle cardRect = { uiX + 50.0f + col * 398.0f, 86.0f + row * 242.0f, 384.0f, 228.0f };
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
            bool canAfford = (profile_.glimmer >= cards[i].glimmerCost);
            DrawRectangleRec(equipBtn, canAfford ? Color{ 28, 82, 58, 255 } : Color{ 48, 30, 36, 255 });
            char costTxt[48];
            std::snprintf(costTxt, sizeof(costTxt), "UNLOCK (%d GLIMMER)", cards[i].glimmerCost);
            int blw = MeasureText(costTxt, 15);
            DrawText(costTxt, static_cast<int>(equipBtn.x + (equipBtn.width - blw) * 0.5f),
                     static_cast<int>(equipBtn.y + 12), 15, canAfford ? GREEN : RED);
        }

        if (IsButtonClicked(cardRect)) {
            if (unlocked) {
                profile_.selectedSubclass = i;
                SaveData();
                audio_.PlayUiSelect();
            } else if (profile_.glimmer >= cards[i].glimmerCost) {
                profile_.glimmer -= cards[i].glimmerCost;
                profile_.unlockedSubclasses[i] = true;
                profile_.selectedSubclass = i;
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
    DrawText("THE TOWER // SHELLS, DRONES, SHADERS & EXOTIC CURIOS", static_cast<int>(uiX + 60), 24, 25, WHITE);

    char glimHeader[64];
    std::snprintf(glimHeader, sizeof(glimHeader), "AVAILABLE GLIMMER: %d", profile_.glimmer);
    int ghw = MeasureText(glimHeader, 20);
    DrawText(glimHeader, static_cast<int>(uiX + 1220.0f) - ghw, 28, 20, SKYBLUE);

    // 5 Sub-tabs inside The Tower:
    // [0] Shells I | [1] Shells II | [2] Combat Drones | [3] Eva's Shaders | [4] Xur's Catalysts
    const char* tabLabels[5] = {
        "SHELLS I: VANGUARD",
        "SHELLS II: EXOTIC",
        "COMBAT DRONES",
        "EVA'S SHADERS",
        "XUR'S CATALYSTS"
    };
    Color tabColors[5] = {
        SKYBLUE,
        ORANGE,
        GREEN,
        Color{ 255, 120, 220, 255 },
        GOLD
    };
    for (int t = 0; t < 5; ++t) {
        Rectangle tabR = { uiX + 60.0f + t * 234.0f, 60.0f, 224.0f, 34.0f };
        bool active = (towerTab_ == t);
        DrawRectangleRec(tabR, active ? tabColors[t] : Color{ 22, 30, 48, 255 });
        DrawRectangleLinesEx(tabR, 1.5f, active ? WHITE : tabColors[t]);
        int tw = MeasureText(tabLabels[t], 14);
        DrawText(tabLabels[t], static_cast<int>(tabR.x + (tabR.width - tw) * 0.5f),
                 static_cast<int>(tabR.y + 10), 14, active ? BLACK : WHITE);
        if (IsButtonClicked(tabR)) {
            towerTab_ = t;
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
                bool canAfford = (profile_.glimmer >= catalog[i].glimmerCost);
                DrawRectangleRec(actBtn, canAfford ? Color{ 30, 85, 60, 255 } : Color{ 45, 30, 35, 255 });
                char costTxt[64];
                std::snprintf(costTxt, sizeof(costTxt), "UNLOCK (%d GLIMMER)", catalog[i].glimmerCost);
                DrawText(costTxt, static_cast<int>(actBtn.x + 26), static_cast<int>(actBtn.y + 9), 14, canAfford ? GREEN : RED);
            }

            if (IsButtonClicked(r)) {
                if (unlocked) {
                    profile_.equippedShell = i;
                    SaveData();
                    audio_.PlayUiSelect();
                } else if (profile_.glimmer >= catalog[i].glimmerCost) {
                    profile_.glimmer -= catalog[i].glimmerCost;
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
        DrawText("Unlocking your first Combat Drone unlocks NIGHTFALL STRIKE mode! Press [C / X] in Nightfall to fire your Drone.",
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
                DrawText("EQUIPPED DRONE", static_cast<int>(btn.x + 54), static_cast<int>(btn.y + 11), 15, BLACK);
            } else if (unlocked) {
                DrawRectangleRec(btn, Color{ 38, 68, 98, 255 });
                DrawText("EQUIP DRONE", static_cast<int>(btn.x + 68), static_cast<int>(btn.y + 11), 15, WHITE);
            } else {
                bool canBuy = (profile_.glimmer >= drones[i].glimmerCost);
                DrawRectangleRec(btn, canBuy ? Color{ 30, 88, 60, 255 } : Color{ 48, 30, 35, 255 });
                char cstr[64];
                std::snprintf(cstr, sizeof(cstr), "UNLOCK (%d GLIMMER)", drones[i].glimmerCost);
                DrawText(cstr, static_cast<int>(btn.x + 34), static_cast<int>(btn.y + 11), 15, canBuy ? GREEN : RED);
            }

            if (IsButtonClicked(dr)) {
                if (unlocked) {
                    profile_.equippedDrone = i;
                    SaveData();
                    audio_.PlayUiSelect();
                } else if (profile_.glimmer >= drones[i].glimmerCost) {
                    bool wasFirstDrone = !HasUnlockedAnyDrone();
                    profile_.glimmer -= drones[i].glimmerCost;
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

        BeginScissorMode(static_cast<int>(uiX + 50), 180, 1150, static_cast<int>(SHADER_VIEWPORT_H));
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
        DrawText("Equip an Exotic Catalyst into your Ghost core for passive paracausal combat effects.", static_cast<int>(uiX + 60), 146, 16, LIGHTGRAY);

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

            Rectangle btn = { er.x + 96.0f, er.y + 106.0f, 240.0f, 38.0f };
            if (equipped) {
                DrawRectangleRec(btn, GOLD);
                DrawText("EQUIPPED CATALYST", static_cast<int>(btn.x + 38), static_cast<int>(btn.y + 11), 15, BLACK);
            } else if (unlocked) {
                DrawRectangleRec(btn, { 38, 68, 98, 255 });
                DrawText("EQUIP CATALYST", static_cast<int>(btn.x + 52), static_cast<int>(btn.y + 11), 15, WHITE);
            } else {
                bool canBuy = (profile_.glimmer >= exotics[i].glimmerCost);
                DrawRectangleRec(btn, canBuy ? Color{ 30, 88, 60, 255 } : Color{ 48, 30, 35, 255 });
                char cstr[64];
                std::snprintf(cstr, sizeof(cstr), "ACQUIRE (%d GLIMMER)", exotics[i].glimmerCost);
                DrawText(cstr, static_cast<int>(btn.x + 26), static_cast<int>(btn.y + 11), 15, canBuy ? GREEN : RED);
            }

            if (IsButtonClicked(er)) {
                if (unlocked) {
                    profile_.equippedExoticPerk = i;
                    SaveData();
                    audio_.PlayUiSelect();
                } else if (profile_.glimmer >= exotics[i].glimmerCost) {
                    profile_.glimmer -= exotics[i].glimmerCost;
                    profile_.unlockedExotics[i] = true;
                    profile_.equippedExoticPerk = i;
                    SaveData();
                    audio_.PlayExoticPickup();
                }
            }
        }
    }

    Rectangle backBtn = { uiX + 60.0f, 585.0f, 235.0f, 48.0f };
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

    const char* sTitle = "SYSTEM SETTINGS // AUDIO, VISUALS & CONTROLS";
    DrawText(sTitle, static_cast<int>(uiX + 70), 32, 26, WHITE);

    // Active Input Device Readout
    char devBuf[128];
    std::snprintf(devBuf, sizeof(devBuf), "ACTIVE INPUT: %s", activeInputDevice_.c_str());
    int dw = MeasureText(devBuf, 16);
    DrawText(devBuf, static_cast<int>(uiX + 1210.0f) - dw, 38, 16, GOLD);

    DrawLine(static_cast<int>(uiX + 70), 72, static_cast<int>(uiX + 1210), 72, { 255, 255, 255, 55 });

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

    // Screen Shake Scale
    DrawText("SCREEN SHAKE INTENSITY:", static_cast<int>(uiX + 660), 124, 16, WHITE);
    const char* shakeLabels[3] = { "OFF (0%)", "NORMAL (50%)", "INTENSE (100%)" };
    float shakeVals[3] = { 0.0f, 0.5f, 1.0f };
    for (int i = 0; i < 3; ++i) {
        Rectangle sb = { uiX + 660.0f + i * 168.0f, 150.0f, 156.0f, 38.0f };
        bool sel = (std::abs(profile_.screenShakeScale - shakeVals[i]) < 0.15f);
        DrawRectangleRec(sb, sel ? Color{ 35, 65, 105, 255 } : Color{ 18, 24, 38, 220 });
        DrawRectangleLinesEx(sb, sel ? 2.0f : 1.2f, sel ? GOLD : GRAY);
        DrawText(shakeLabels[i], static_cast<int>(sb.x + 16), static_cast<int>(sb.y + 11), 14, sel ? GOLD : LIGHTGRAY);
        if (IsButtonClicked(sb)) {
            profile_.screenShakeScale = shakeVals[i];
            audio_.PlayUiSelect();
        }
    }

    // Screen Flash Pulses Toggle
    DrawText("SOLAR / EXPLOSION SCREEN FLASHES:", static_cast<int>(uiX + 660), 206, 16, WHITE);
    Rectangle flashOnBtn  = { uiX + 660.0f, 232.0f, 160.0f, 38.0f };
    Rectangle flashOffBtn = { uiX + 832.0f, 232.0f, 160.0f, 38.0f };
    DrawRectangleRec(flashOnBtn, profile_.screenFlashes ? Color{ 35, 65, 105, 255 } : Color{ 18, 24, 38, 220 });
    DrawRectangleLinesEx(flashOnBtn, profile_.screenFlashes ? 2.0f : 1.2f, profile_.screenFlashes ? GOLD : GRAY);
    DrawText("ENABLED", static_cast<int>(flashOnBtn.x + 44), static_cast<int>(flashOnBtn.y + 11), 14, profile_.screenFlashes ? GOLD : LIGHTGRAY);
    if (IsButtonClicked(flashOnBtn)) { profile_.screenFlashes = true; audio_.PlayUiSelect(); }

    DrawRectangleRec(flashOffBtn, !profile_.screenFlashes ? Color{ 35, 65, 105, 255 } : Color{ 18, 24, 38, 220 });
    DrawRectangleLinesEx(flashOffBtn, !profile_.screenFlashes ? 2.0f : 1.2f, !profile_.screenFlashes ? GOLD : GRAY);
    DrawText("DISABLED", static_cast<int>(flashOffBtn.x + 40), static_cast<int>(flashOffBtn.y + 11), 14, !profile_.screenFlashes ? GOLD : LIGHTGRAY);
    if (IsButtonClicked(flashOffBtn)) { profile_.screenFlashes = false; audio_.PlayUiSelect(); }

    // Target Framerate Mode
    DrawText("PERFORMANCE / TARGET FPS:", static_cast<int>(uiX + 660), 288, 16, WHITE);
    int fpsVals[3] = { 60, 120, 30 };
    const char* fpsLabels[3] = { "60 FPS (BALANCED)", "120 FPS (ULTRA)", "30 FPS (SAVER)" };
    for (int f = 0; f < 3; ++f) {
        Rectangle fb = { uiX + 660.0f + f * 168.0f, 314.0f, 156.0f, 38.0f };
        bool sel = (profile_.targetFps == fpsVals[f]);
        DrawRectangleRec(fb, sel ? Color{ 35, 65, 105, 255 } : Color{ 18, 24, 38, 220 });
        DrawRectangleLinesEx(fb, sel ? 2.0f : 1.2f, sel ? GOLD : GRAY);
        DrawText(fpsLabels[f], static_cast<int>(fb.x + 8), static_cast<int>(fb.y + 11), 12, sel ? GOLD : LIGHTGRAY);
        if (IsButtonClicked(fb)) {
            profile_.targetFps = fpsVals[f];
            SetTargetFPS(profile_.targetFps);
            audio_.PlayUiSelect();
        }
    }

    // Touch Button Handedness Mode
    DrawText("MOBILE TOUCH CONTROLS HANDEDNESS:", static_cast<int>(uiX + 660), 370, 16, WHITE);
    Rectangle handRight = { uiX + 660.0f, 396.0f, 210.0f, 38.0f };
    Rectangle handLeft  = { uiX + 882.0f, 396.0f, 210.0f, 38.0f };
    DrawRectangleRec(handRight, !profile_.leftHandedControls ? Color{ 35, 65, 105, 255 } : Color{ 18, 24, 38, 220 });
    DrawRectangleLinesEx(handRight, !profile_.leftHandedControls ? 2.0f : 1.2f, !profile_.leftHandedControls ? GOLD : GRAY);
    DrawText("RIGHT-HANDED (DEFAULT)", static_cast<int>(handRight.x + 12), static_cast<int>(handRight.y + 11), 13, !profile_.leftHandedControls ? GOLD : LIGHTGRAY);
    if (IsButtonClicked(handRight)) { profile_.leftHandedControls = false; audio_.PlayUiSelect(); }

    DrawRectangleRec(handLeft, profile_.leftHandedControls ? Color{ 35, 65, 105, 255 } : Color{ 18, 24, 38, 220 });
    DrawRectangleLinesEx(handLeft, profile_.leftHandedControls ? 2.0f : 1.2f, profile_.leftHandedControls ? GOLD : GRAY);
    DrawText("LEFT-HANDED (MIRRORED)", static_cast<int>(handLeft.x + 12), static_cast<int>(handLeft.y + 11), 13, profile_.leftHandedControls ? GOLD : LIGHTGRAY);
    if (IsButtonClicked(handLeft)) { profile_.leftHandedControls = true; audio_.PlayUiSelect(); }

    // Clear Save Data Button (Right Column)
    Rectangle resetBtn = { uiX + 660.0f, 468.0f, 310.0f, 42.0f };
    DrawRectangleRec(resetBtn, showResetConfirm_ ? Color{ 110, 30, 35, 255 } : Color{ 48, 20, 26, 230 });
    DrawRectangleLinesEx(resetBtn, 2.0f, RED);
    DrawText(showResetConfirm_ ? "CONFIRM RESET PROGRESS?" : "CLEAR SAVE DATA (RESET)", static_cast<int>(resetBtn.x + 18), static_cast<int>(resetBtn.y + 13), 14, WHITE);
    if (IsButtonClicked(resetBtn)) {
        if (!showResetConfirm_) {
            showResetConfirm_ = true;
            audio_.PlayUiSelect();
        } else {
            // Reset profile back to clean factory state
            SaveProfile fresh{};
            fresh.masterVolume = profile_.masterVolume;
            fresh.musicVolume = profile_.musicVolume;
            fresh.sfxVolume = profile_.sfxVolume;
            fresh.screenShakeScale = profile_.screenShakeScale;
            fresh.screenFlashes = profile_.screenFlashes;
            fresh.targetFps = profile_.targetFps;
            fresh.leftHandedControls = profile_.leftHandedControls;
            fresh.musicTrackChoice = profile_.musicTrackChoice;
            profile_ = fresh;
            SaveData();
            showResetConfirm_ = false;
            audio_.PlayDeath();
        }
    }
    if (showResetConfirm_) {
        Rectangle cancelReset = { uiX + 985.0f, 468.0f, 110.0f, 42.0f };
        DrawRectangleRec(cancelReset, { 28, 38, 52, 240 });
        DrawRectangleLinesEx(cancelReset, 1.5f, SKYBLUE);
        DrawText("CANCEL", static_cast<int>(cancelReset.x + 20), static_cast<int>(cancelReset.y + 13), 14, WHITE);
        if (IsButtonClicked(cancelReset)) { showResetConfirm_ = false; audio_.PlayUiSelect(); }
    }

    // --- BOTTOM ROW: FOOTER & BACK ---
    DrawLine(static_cast<int>(uiX + 70), 545, static_cast<int>(uiX + 1210), 545, { 255, 255, 255, 45 });

    // Engine & Build Info
    const char* bInfo = "DESTINY: THE FLAPPY GHOST  |  v1.3.0 EXPANSION  |  SEBINY LABS ENGINE";
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

    const char* sTitle = "VANGUARD CAREER DOSSIER";
    DrawText(sTitle, static_cast<int>(uiX + 70), 32, 22, GOLD);

    char glimBuf[128];
    std::snprintf(glimBuf, sizeof(glimBuf), "CURRENT GLIMMER IN VAULT: %d G", profile_.glimmer);
    int gbw = MeasureText(glimBuf, 15);
    DrawText(glimBuf, static_cast<int>(uiX + 1210.0f) - gbw, 36, 15, SKYBLUE);

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
    DrawStatRow(rightCard.x + 20, sy, rightCard.width - 40, "All-Time Glimmer Synthesized:", std::to_string(profile_.totalGlimmerEarnedLifetime) + " G", GOLD);
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
    DrawLine(static_cast<int>(uiX + 70), 580, static_cast<int>(uiX + 1210), 580, { 255, 255, 255, 45 });
    Rectangle statsBackBtn = { uiX + 70.0f, 600.0f, 260.0f, 48.0f };
    DrawRectangleRec(statsBackBtn, IsRectHovered(statsBackBtn) ? Color{ 45, 65, 95, 255 } : Color{ 25, 35, 52, 255 });
    DrawRectangleLinesEx(statsBackBtn, 2.0f, ORANGE);
    DrawText("< BACK TO DIRECTOR [ESC]", static_cast<int>(statsBackBtn.x + 26), static_cast<int>(statsBackBtn.y + 16), 16, WHITE);
    if (IsButtonClicked(statsBackBtn) || IsKeyPressed(KEY_ESCAPE) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
        SetScreenState(ScreenState::TITLE_DIRECTOR);
        audio_.PlayUiSelect();
    }
}

void GhostGame::RenderTriumphsLog() {
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 8, 12, 22, 230 });
    float uiX = (VIRTUAL_WIDTH - 1280.0f) * 0.5f;
    DrawText("VANGUARD RECORD // DAILY BOUNTIES & TRIUMPHS", static_cast<int>(uiX + 70), 32, 26, WHITE);

    char glimHeader[64];
    std::snprintf(glimHeader, sizeof(glimHeader), "AVAILABLE GLIMMER: %d", profile_.glimmer);
    int ghw = MeasureText(glimHeader, 18);
    DrawText(glimHeader, static_cast<int>(uiX + 1210.0f) - ghw, 36, 18, SKYBLUE);

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

    BeginScissorMode(static_cast<int>(uiX + 60), 225, 1140, static_cast<int>(TRIUMPH_VIEWPORT_H));
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

    Rectangle backBtn = { uiX + 70.0f, 580.0f, 235.0f, 48.0f };
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
    DrawText(pTitle, (VIRTUAL_WIDTH - pw) / 2, 230, 38, WHITE);

    Rectangle resumeBtn   = { VIRTUAL_WIDTH / 2.0f - 160.0f, 290.0f, 320.0f, 52.0f };
    Rectangle settingsBtn = { VIRTUAL_WIDTH / 2.0f - 160.0f, 356.0f, 320.0f, 52.0f };
    Rectangle dirBtn      = { VIRTUAL_WIDTH / 2.0f - 160.0f, 422.0f, 320.0f, 52.0f };

    DrawRectangleRec(resumeBtn, IsRectHovered(resumeBtn) ? Color{ 40, 75, 110, 255 } : Color{ 22, 36, 56, 255 });
    DrawRectangleLinesEx(resumeBtn, 2.0f, SKYBLUE);
    DrawText("RESUME PATROL [SPACE / (A)]", static_cast<int>(resumeBtn.x + 36), static_cast<int>(resumeBtn.y + 17), 17, WHITE);

    DrawRectangleRec(settingsBtn, IsRectHovered(settingsBtn) ? Color{ 55, 45, 80, 255 } : Color{ 28, 22, 42, 255 });
    DrawRectangleLinesEx(settingsBtn, 2.0f, PURPLE);
    DrawText("SYSTEM SETTINGS", static_cast<int>(settingsBtn.x + 85), static_cast<int>(settingsBtn.y + 17), 17, WHITE);

    DrawRectangleRec(dirBtn, IsRectHovered(dirBtn) ? Color{ 75, 35, 45, 255 } : Color{ 38, 22, 30, 255 });
    DrawRectangleLinesEx(dirBtn, 2.0f, ORANGE);
    DrawText("RETURN TO DIRECTOR", static_cast<int>(dirBtn.x + 62), static_cast<int>(dirBtn.y + 17), 17, WHITE);

    if (IsButtonClicked(resumeBtn)) {
        SetScreenState(ScreenState::PLAYING);
        audio_.PlayUiSelect();
    }
    if (IsButtonClicked(settingsBtn)) {
        settingsPrevState_ = ScreenState::PAUSED;
        SetScreenState(ScreenState::SETTINGS_VIEW);
        audio_.PlayUiSelect();
    }
    if (IsButtonClicked(dirBtn)) {
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
    std::snprintf(line3, sizeof(line3), "GLIMMER EARNED THIS RUN: +%d", runGlimmerEarned_);

    DrawText(line1, VIRTUAL_WIDTH / 2 - 190, 338, 20, GOLD);
    DrawText(line2, VIRTUAL_WIDTH / 2 - 190, 374, 18, SKYBLUE);
    DrawText(line3, VIRTUAL_WIDTH / 2 - 190, 408, 18, WHITE);

    Rectangle revBtn = { VIRTUAL_WIDTH / 2.0f - 230.0f, 482.0f, 220.0f, 56.0f };
    Rectangle dirBtn = { VIRTUAL_WIDTH / 2.0f + 10.0f,  482.0f, 220.0f, 56.0f };

    DrawRectangleRec(revBtn, IsRectHovered(revBtn) ? Color{ 45, 95, 75, 255 } : Color{ 25, 62, 48, 255 });
    DrawRectangleLinesEx(revBtn, 2.0f, GREEN);
    DrawText("REVIVE [SPACE]", static_cast<int>(revBtn.x + 38), static_cast<int>(revBtn.y + 19), 18, WHITE);

    DrawRectangleRec(dirBtn, IsRectHovered(dirBtn) ? Color{ 45, 65, 95, 255 } : Color{ 25, 38, 58, 255 });
    DrawRectangleLinesEx(dirBtn, 2.0f, SKYBLUE);
    DrawText("DIRECTOR [ESC]", static_cast<int>(dirBtn.x + 38), static_cast<int>(dirBtn.y + 19), 18, WHITE);

    if (IsButtonClicked(revBtn)) {
        StartNewRun(isNightfallMode_);
    }
    if (IsButtonClicked(dirBtn)) {
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
