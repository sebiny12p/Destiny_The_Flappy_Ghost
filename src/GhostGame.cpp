#include "GhostGame.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>

namespace FlappyGhost {

namespace {
#ifdef __ANDROID__
constexpr const char* SAVE_FILE_PATH = "/data/data/com.sebinylabs.flappyghost/files/flappy_ghost_save.dat";
#else
constexpr const char* SAVE_FILE_PATH = "flappy_ghost_save.dat";
#endif

const std::array<GhostShellDef, NUM_SHELLS> SHELL_CATALOG = {{
    { 0, "GENERALIST SHELL", "Standard Vanguard-issue Ghost shell forged from Traveler plating.", 0,
      { 235, 238, 245, 255 }, { 215, 95, 35, 255 }, { 120, 225, 255, 255 }, { 110, 210, 255, 200 }, 0 },
    { 1, "SAGIRA'S SHELL", "Geometric brass lattice crafted for Disciples of Osiris on Mercury.", 250,
      { 218, 175, 72, 255 }, { 70, 55, 35, 255 }, { 255, 230, 120, 255 }, { 255, 200, 75, 200 }, 1 },
    { 2, "TYRANT SHELL", "Submind Rasputin octahedron housing with IKELOS Seraph telemetry.", 400,
      { 38, 42, 48, 255 }, { 200, 40, 35, 255 }, { 255, 65, 45, 255 }, { 255, 50, 30, 200 }, 2 },
    { 3, "HARPY SHELL", "Simulated Vex brass chassis resonating with radiolarian fluid.", 550,
      { 165, 130, 78, 255 }, { 40, 90, 105, 255 }, { 255, 55, 55, 255 }, { 220, 245, 255, 200 }, 3 },
    { 4, "GHOST OF THE DEEP", "Calcified Hive chitin bound with iridescent Soulfire runes.", 700,
      { 68, 82, 72, 255 }, { 32, 40, 36, 255 }, { 95, 255, 145, 255 }, { 75, 245, 130, 200 }, 4 },
    { 5, "FLAWLESS TRIALS SHELL", "Radiant auric hawk wings awarded to Guardians at the Lighthouse.", 850,
      { 252, 210, 55, 255 }, { 245, 245, 250, 255 }, { 255, 255, 190, 255 }, { 255, 215, 60, 220 }, 5 },
    { 6, "NINE LIVES SHELL", "Festival of the Lost feline shell with cat ears, whiskers & slit iris.", 1000,
      { 230, 125, 45, 255 }, { 245, 235, 220, 255 }, { 95, 255, 85, 255 }, { 255, 165, 80, 210 }, 6 },
    { 7, "COSMOS EXOTIC SHELL", "Nebula starlight housing orbited by miniature celestial moons.", 1200,
      { 125, 65, 235, 255 }, { 55, 195, 255, 255 }, { 245, 180, 255, 255 }, { 165, 110, 255, 220 }, 7 },
    { 8, "SIVA INFECTED SHELL", "Plaguelands carbon plating entangled in crimson SIVA nanite swarms.", 1400,
      { 24, 24, 28, 255 }, { 235, 25, 35, 255 }, { 255, 40, 40, 255 }, { 245, 30, 40, 220 }, 8 },
    { 9, "ARMORY FORGED SHELL", "Black Armory Obsidian accelerator with Bergusia radiance rings.", 1650,
      { 30, 32, 38, 255 }, { 195, 28, 48, 255 }, { 110, 245, 255, 255 }, { 220, 40, 65, 220 }, 9 },
    { 10, "TAKEN KING SHELL", "Ascendant Blight vessel wreathed in shadowy Taken fire and sterile neutrinos.", 1900,
      { 16, 20, 28, 255 }, { 190, 245, 255, 255 }, { 240, 255, 255, 255 }, { 170, 240, 255, 225 }, 10 },
    { 11, "TRAVELER'S CHOSEN", "Fractured Shard of the Traveler radiating pure paracausal Light arcs.", 2500,
      { 248, 248, 255, 255 }, { 235, 195, 80, 255 }, { 150, 245, 255, 255 }, { 230, 245, 255, 235 }, 11 }
}};

const std::array<ExoticPerkDef, NUM_EXOTIC_PERKS> EXOTIC_CATALOG = {{
    { 0, "STANDARD VANGUARD CORE", "No Exotic Catalyst equipped. Standard Ghost flight telemetry.", 0, SKYBLUE },
    { 1, "GJALLARHORN WOLFPACK", "Activating Tactical or Super abilities launches homing Wolfpack micro-rockets!", 400, GOLD },
    { 2, "CRIMSON REDRIX CORE", "Every 8 Motes of Light harvested automatically restores +1 Overshield!", 550, { 255, 65, 75, 255 } },
    { 3, "RISKRUNNER CONDUCTOR", "Flying close to pillar edges triggers Arc Conductor (+18% Super & +10 Glimmer)!", 650, { 80, 235, 255, 255 } }
}};

const std::array<TriumphDef, NUM_TRIUMPHS> TRIUMPH_CATALOG = {{
    { 0, "EYES UP, GUARDIAN", "Clear your first 10 barriers in a single patrol run.", 50 },
    { 1, "VAULT BREAKER", "Reach Sector 2: The Vault of Glass (15+ Gates).", 100 },
    { 2, "INTO THE HELLMOUTH", "Reach Sector 3: The Hellmouth Fortress (30+ Gates).", 150 },
    { 3, "ASCENDANT WALKER", "Reach Sector 4: The Ascendant Plane (45+ Gates).", 250 },
    { 4, "PARACAUSAL SURGE", "Unleash your Light or Darkness Super ability for the first time.", 75 },
    { 5, "BLINK MASTER", "Phase directly through a barrier or laser using Blink/Dash.", 100 },
    { 6, "GODSLAYER PROTOCOL", "Defeat an Ultra Boss Encounter by scanning 3 Vex Confluxes.", 200 },
    { 7, "NIGHTFALL VANGUARD", "Score 20 or higher in the Daily Nightfall Seed mode.", 200 },
    { 8, "EXOTIC ARCHIVIST", "Collect a golden Exotic Engram in mid-flight.", 100 },
    { 9, "FASHION ENDGAME", "Unlock at least 3 custom Ghost Shells in The Tower.", 250 }
}};
const std::array<ShaderDef, NUM_SHADERS> SHADER_CATALOG = {{
    { 0, "DEFAULT TRAWLER", 0, { 235, 238, 245, 255 }, { 215, 95, 35, 255 }, { 120, 225, 255, 255 } },
    { 1, "CALUS'S SELECTED", 300, { 245, 245, 250, 255 }, { 215, 175, 55, 255 }, { 165, 45, 145, 255 } },
    { 2, "SUPERBLACK", 500, { 18, 18, 22, 255 }, { 32, 34, 40, 255 }, { 255, 255, 255, 255 } },
    { 3, "GAMBIT JADESTONE", 450, { 35, 75, 50, 255 }, { 65, 255, 120, 255 }, { 95, 255, 160, 255 } },
    { 4, "PHOTO FINISH RGB", 650, { 45, 45, 55, 255 }, { 255, 75, 180, 255 }, { 65, 230, 255, 255 } },
    { 5, "NEOPOP WAVE", 400, { 40, 50, 75, 255 }, { 255, 120, 220, 255 }, { 80, 255, 240, 255 } }
}};
} // namespace

GhostGame::GhostGame() = default;
GhostGame::~GhostGame() = default;

const std::array<GhostShellDef, NUM_SHELLS>& GhostGame::GetShellCatalog() {
    return SHELL_CATALOG;
}

const std::array<ExoticPerkDef, NUM_EXOTIC_PERKS>& GhostGame::GetExoticCatalog() {
    return EXOTIC_CATALOG;
}

const std::array<TriumphDef, NUM_TRIUMPHS>& GhostGame::GetTriumphCatalog() {
    return TRIUMPH_CATALOG;
}

const std::array<ShaderDef, NUM_SHADERS>& GhostGame::GetShaderCatalog() {
    return SHADER_CATALOG;
}

void GhostGame::Init() {
    targetRenderTex_ = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(targetRenderTex_.texture, TEXTURE_FILTER_BILINEAR);

    audio_.Init();
    LoadSaveData();

    PCG32 starRng(0xCAFEBABE1234ULL);
    for (auto& star : stars_) {
        star.pos = { starRng.RangeFloat(0.0f, static_cast<float>(VIRTUAL_WIDTH)),
                     starRng.RangeFloat(0.0f, static_cast<float>(VIRTUAL_HEIGHT)) };
        star.depth = starRng.RangeFloat(0.15f, 1.0f);
        star.size = 1.0f + star.depth * 2.2f;
        unsigned char b = static_cast<unsigned char>(150 + star.depth * 105);
        star.color = { static_cast<unsigned char>(b - starRng.RangeInt(0, 30)),
                       b,
                       255,
                       static_cast<unsigned char>(110 + starRng.RangeInt(0, 140)) };
    }
}

void GhostGame::Shutdown() {
    SaveData();
    audio_.Shutdown();
    if (targetRenderTex_.id != 0) {
        UnloadRenderTexture(targetRenderTex_);
    }
}

void GhostGame::LoadSaveData() {
    std::ifstream in(SAVE_FILE_PATH, std::ios::binary);
    if (in.is_open()) {
        SaveProfile loaded{};
        in.read(reinterpret_cast<char*>(&loaded), sizeof(SaveProfile));
        if (in && loaded.magic == 0x47485357) {
            profile_ = loaded;
            profile_.equippedShell = std::clamp(profile_.equippedShell, 0, static_cast<int>(NUM_SHELLS) - 1);
            profile_.selectedSubclass = std::clamp(profile_.selectedSubclass, 0, static_cast<int>(NUM_SUBCLASSES) - 1);
            if (!profile_.unlockedSubclasses[profile_.selectedSubclass]) {
                profile_.selectedSubclass = 0;
            }
            profile_.equippedExoticPerk = std::clamp(profile_.equippedExoticPerk, 0, static_cast<int>(NUM_EXOTIC_PERKS) - 1);
            profile_.equippedShader = std::clamp(profile_.equippedShader, 0, static_cast<int>(NUM_SHADERS) - 1);
            return;
        }
    }
    profile_ = SaveProfile{};
}

void GhostGame::SaveData() const {
    std::ofstream out(SAVE_FILE_PATH, std::ios::binary | std::ios::trunc);
    if (out.is_open()) {
        out.write(reinterpret_cast<const char*>(&profile_), sizeof(SaveProfile));
    }
}

void GhostGame::UnlockTriumph(int triumphIdx) {
    if (triumphIdx < 0 || triumphIdx >= static_cast<int>(NUM_TRIUMPHS)) return;
    if (profile_.unlockedTriumphs[triumphIdx]) return;

    profile_.unlockedTriumphs[triumphIdx] = true;
    const auto& def = TRIUMPH_CATALOG[triumphIdx];
    profile_.glimmer += def.rewardGlimmer;
    SaveData();

    activeToast_.active = true;
    activeToast_.title = std::string("TRIUMPH COMPLETED: ") + def.title;
    activeToast_.subtitle = std::string(def.description) + " (+" + std::to_string(def.rewardGlimmer) + " Glimmer)";
    activeToast_.timer = 4.2f;
    audio_.PlayExoticPickup();
}

void GhostGame::TriggerVanguardMedal(const char* medalName, int glimmerBonus, Color col) {
    activeMedal_.active = true;
    activeMedal_.medalName = medalName;
    activeMedal_.bonusGlimmer = glimmerBonus;
    activeMedal_.timer = 2.6f;
    activeMedal_.color = col;
    profile_.glimmer += glimmerBonus;
    runGlimmerEarned_ += glimmerBonus;
}

void GhostGame::StartNewRun(bool dailyNightfall) {
    isNightfallMode_ = dailyNightfall;
    if (dailyNightfall) {
        activeSeed_ = PCG32::GetDailyNightfallSeed();
    } else {
        activeSeed_ = static_cast<uint64_t>(std::time(nullptr)) ^
                      static_cast<uint64_t>(GetTime() * 100000.0);
    }
    rng_.Seed(activeSeed_, isNightfallMode_ ? 777ULL : 101ULL);

    state_ = ScreenState::PLAYING;
    runTime_ = 0.0f;
    distanceMeters_ = 0.0f;
    scrollSpeed_ = isNightfallMode_ ? 345.0f : 310.0f;
    scoreGates_ = 0;
    streakGates_ = 0;
    runMotes_ = 0;
    runGlimmerEarned_ = 0;
    gatesSinceLastBoss_ = 0;
    lastBiome_ = BiomeSector::COSMODROME;
    biomeBannerTimer_ = 3.0f;
    activeMedal_.active = false;

    ghost_ = GhostPlayer{};
    ghost_.pos = { 280.0f, VIRTUAL_HEIGHT * 0.5f };
    ghost_.vel = { 0.0f, -180.0f };
    ghost_.tacticalMaxCooldown = std::max(1.4f, 3.2f - profile_.upgrades.phaseCooldownLevel * 0.32f);
    ghost_.overshieldCharges = profile_.upgrades.overshieldCapLevel;
    ghost_.superEnergy = 20.0f;

    boss_ = BossState{};
    for (auto& o : obstacles_) o.active = false;
    for (auto& c : collectibles_) c.active = false;
    for (auto& p : particles_) p.active = false;
    for (auto& pr : projectiles_) pr.active = false;

    nextObstacleSpawnX_ = 760.0f;
    for (int i = 0; i < 4; ++i) {
        SpawnNextObstacle();
    }

    audio_.PlayFlap();
}

BiomeSector GhostGame::GetCurrentBiome() const {
    int stage = (scoreGates_ / 15) % 4;
    return static_cast<BiomeSector>(stage);
}

const char* GhostGame::GetBiomeName(BiomeSector b) const {
    switch (b) {
        case BiomeSector::COSMODROME:      return "SECTOR 01 // COSMODROME RUINS (FALLEN)";
        case BiomeSector::VAULT_OF_GLASS:  return "SECTOR 02 // THE VAULT OF GLASS (VEX NETWORK)";
        case BiomeSector::HELLMOUTH:       return "SECTOR 03 // THE HELLMOUTH CATACOMBS (HIVE)";
        case BiomeSector::ASCENDANT_PLANE: return "SECTOR 04 // THE ASCENDANT REALM (TAKEN)";
    }
    return "UNKNOWN SECTOR";
}

Color GhostGame::GetSubclassAccentColor() const {
    switch (static_cast<LightSubclass>(profile_.selectedSubclass)) {
        case LightSubclass::VOID_WALKER:            return { 175, 85, 255, 255 };
        case LightSubclass::SOLAR_DAWN:             return { 255, 145, 35, 255 };
        case LightSubclass::ARC_CALLER:             return { 75, 220, 255, 255 };
        case LightSubclass::STASIS_SHADEBINDER:     return { 70, 130, 255, 255 };
        case LightSubclass::STRAND_RUNNER:          return { 65, 245, 130, 255 };
        case LightSubclass::PRISMATIC_TRANSCENDENT: return { 255, 125, 235, 255 }; // Transcendent Pink
    }
    return SKYBLUE;
}

Vector2 GhostGame::GetVirtualMousePos() const {
    Vector2 mouse = GetMousePosition();
    float sw = static_cast<float>(GetScreenWidth());
    float sh = static_cast<float>(GetScreenHeight());
    if (sw <= 0.0f || sh <= 0.0f) return { 0.0f, 0.0f };
    return {
        mouse.x * (static_cast<float>(VIRTUAL_WIDTH) / sw),
        mouse.y * (static_cast<float>(VIRTUAL_HEIGHT) / sh)
    };
}

bool GhostGame::IsRectHovered(Rectangle rect) const {
    return CheckCollisionPointRec(GetVirtualMousePos(), rect);
}

bool GhostGame::IsButtonClicked(Rectangle rect) const {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetVirtualMousePos(), rect)) {
        return true;
    }
    return false;
}

void GhostGame::TriggerBlasterShot() {
    SpawnProjectile({ ghost_.pos.x + 22.0f, ghost_.pos.y }, { 820.0f, 0.0f }, 9.0f, GetSubclassAccentColor(), true, false);
    audio_.PlayLaserShot();
    SpawnParticleBurst({ ghost_.pos.x + 18.0f, ghost_.pos.y }, 6, GetSubclassAccentColor(), 40.0f, 180.0f, 3.5f, 0.22f);
}

void GhostGame::TriggerFlap() {
    ghost_.vel.y = -435.0f;
    ghost_.shellExpand = 1.0f;
    audio_.PlayFlap();

    // Every flap also fires a light telemetry dart forward so enemies are naturally killable by ALL players!
    TriggerBlasterShot();

    const auto& shell = SHELL_CATALOG[profile_.equippedShell];
    SpawnParticleBurst({ ghost_.pos.x - 14.0f, ghost_.pos.y + 6.0f }, 9, shell.trailColor, 40.0f, 160.0f, 4.5f, 0.38f);
}

void GhostGame::TriggerTacticalAbility() {
    if (ghost_.tacticalCooldown > 0.0f) return;

    ghost_.tacticalCooldown = ghost_.tacticalMaxCooldown;
    ghost_.shellExpand = 1.6f;
    audio_.PlayBlink();

    LightSubclass sub = static_cast<LightSubclass>(profile_.selectedSubclass);
    Color accent = GetSubclassAccentColor();

    // Exotic Perk: Gjallarhorn Wolfpack spawns 3 homing micro-rockets on every tactical ability!
    if (static_cast<ExoticPerk>(profile_.equippedExoticPerk) == ExoticPerk::GJALLARHORN_WOLFPACK) {
        for (int w = -1; w <= 1; ++w) {
            SpawnProjectile({ ghost_.pos.x + 18.0f, ghost_.pos.y },
                            { 620.0f, w * 140.0f }, 9.0f, GOLD, true, true);
        }
    }

    if (sub == LightSubclass::VOID_WALKER) {
        ghost_.phaseTimer = 0.55f;
        ghost_.vel.y = -110.0f;
        SpawnParticleBurst(ghost_.pos, 28, accent, 80.0f, 320.0f, 6.0f, 0.45f);
        for (auto& o : obstacles_) if (o.active) o.x -= 95.0f;
        for (auto& c : collectibles_) if (c.active) c.pos.x -= 95.0f;
        nextObstacleSpawnX_ -= 95.0f;
        flashColor_ = accent;
        flashAlpha_ = 0.35f;
    } else if (sub == LightSubclass::SOLAR_DAWN) {
        ghost_.phaseTimer = 0.65f;
        ghost_.vel.y = 0.0f;
        SpawnParticleBurst(ghost_.pos, 32, accent, 100.0f, 360.0f, 6.5f, 0.50f);
        for (int i = -1; i <= 1; ++i) {
            SpawnProjectile({ ghost_.pos.x + 20.0f, ghost_.pos.y },
                            { 650.0f, i * 110.0f }, 12.0f, accent, true, false);
        }
    } else if (sub == LightSubclass::ARC_CALLER) {
        ghost_.phaseTimer = 0.50f;
        ghost_.vel.y = -150.0f;
        SpawnParticleBurst(ghost_.pos, 26, accent, 120.0f, 380.0f, 5.5f, 0.40f);
        SpawnProjectile({ ghost_.pos.x + 24.0f, ghost_.pos.y }, { 850.0f, 0.0f }, 15.0f, accent, true, false);
    } else if (sub == LightSubclass::STASIS_SHADEBINDER) {
        // Stasis Frost Pulse: Freezes all moving obstacles, Oracles, and enemy projectiles for 2.5s!
        ghost_.phaseTimer = 0.45f;
        ghost_.stasisFreezeTimer = 2.5f;
        ghost_.vel.y = -80.0f;
        SpawnParticleBurst(ghost_.pos, 38, accent, 90.0f, 360.0f, 6.5f, 0.55f);
        for (auto& o : obstacles_) {
            if (o.active) o.frozenByStasis = true;
        }
        for (auto& pr : projectiles_) {
            if (pr.active && !pr.friendly) pr.active = false;
        }
        flashColor_ = accent;
        flashAlpha_ = 0.30f;
    } else if (sub == LightSubclass::STRAND_RUNNER) {
        // Strand Grapple: Launches a green grapple tether ahead-upward, swinging smoothly + phasing!
        ghost_.phaseTimer = 0.65f;
        ghost_.grappleTimer = 0.45f;
        ghost_.grappleAnchor = { ghost_.pos.x + 175.0f, std::max(70.0f, ghost_.pos.y - 140.0f) };
        ghost_.vel.y = -320.0f;
        SpawnParticleBurst(ghost_.pos, 30, accent, 90.0f, 340.0f, 5.5f, 0.45f);
        SpawnProjectile({ ghost_.pos.x + 20.0f, ghost_.pos.y }, { 700.0f, 0.0f }, 11.0f, accent, true, true);
    } else if (sub == LightSubclass::PRISMATIC_TRANSCENDENT) {
        // Prismatic Transcendence Surge: Freezes laser obstacles + launches twin Light & Dark piercing bolts!
        ghost_.phaseTimer = 0.70f;
        ghost_.stasisFreezeTimer = 2.0f;
        ghost_.vel.y = -220.0f;
        SpawnParticleBurst(ghost_.pos, 42, accent, 120.0f, 420.0f, 7.0f, 0.60f);
        SpawnProjectile({ ghost_.pos.x + 22.0f, ghost_.pos.y - 14.0f }, { 880.0f, -40.0f }, 14.0f, SKYBLUE, true, true);
        SpawnProjectile({ ghost_.pos.x + 22.0f, ghost_.pos.y + 14.0f }, { 880.0f,  40.0f }, 14.0f, { 255, 140, 40, 255 }, true, true);
        flashColor_ = accent;
        flashAlpha_ = 0.40f;
    }
}

void GhostGame::TriggerSuperAbility() {
    if (ghost_.superEnergy < 100.0f || ghost_.superActiveTimer > 0.0f) return;

    ghost_.superEnergy = 0.0f;
    float durationBonus = profile_.upgrades.superEfficiencyLevel * 0.45f;
    ghost_.superActiveTimer = 4.8f + durationBonus;
    ghost_.invulnerableTimer = std::max(ghost_.invulnerableTimer, ghost_.superActiveTimer + 0.5f);
    ghost_.shellExpand = 2.0f;
    screenShakeTrauma_ = 0.65f;
    flashColor_ = GetSubclassAccentColor();
    flashAlpha_ = 0.55f;

    audio_.PlaySuperActivate();
    UnlockTriumph(4);

    LightSubclass sub = static_cast<LightSubclass>(profile_.selectedSubclass);
    SpawnParticleBurst(ghost_.pos, 60, GetSubclassAccentColor(), 120.0f, 520.0f, 8.0f, 0.8f);

    if (sub == LightSubclass::VOID_WALKER || sub == LightSubclass::STASIS_SHADEBINDER) {
        // Nova Pulse / Winter's Wrath Shatter vaporizes active barriers into Motes of Light!
        for (auto& o : obstacles_) {
            if (o.active && o.x > 0.0f && o.x < VIRTUAL_WIDTH + 100.0f) {
                SpawnParticleBurst({ o.x + o.width * 0.5f, o.gapCenterY }, 35, GetSubclassAccentColor(), 90.0f, 350.0f, 7.0f, 0.6f);
                SpawnCollectible({ o.x + o.width * 0.5f, o.gapCenterY }, CollectibleKind::MOTE_OF_LIGHT);
                o.active = false;
                scoreGates_++;
                profile_.totalGatesCleared++;
            }
        }
        if (boss_.active && !boss_.shieldImmune) {
            boss_.health -= 45.0f;
        }
    } else if (sub == LightSubclass::STRAND_RUNNER) {
        // Needlestorm: Unleashes 9 homing Strand Threadlings!
        for (int i = 0; i < 9; ++i) {
            float vy = (i - 4) * 65.0f;
            SpawnProjectile({ ghost_.pos.x + 20.0f, ghost_.pos.y },
                            { 640.0f, vy }, 10.0f, GetSubclassAccentColor(), true, true);
        }
    } else if (sub == LightSubclass::PRISMATIC_TRANSCENDENT) {
        // Transcendence: Full screen purge of barriers & vaporizes all patrol enemies into Glimmer
        for (auto& o : obstacles_) {
            if (o.active && o.x > 0.0f && o.x < VIRTUAL_WIDTH + 100.0f) {
                SpawnParticleBurst({ o.x + o.width * 0.5f, o.gapCenterY }, 40, { 255, 125, 235, 255 }, 120.0f, 400.0f, 7.5f, 0.7f);
                SpawnCollectible({ o.x + o.width * 0.5f, o.gapCenterY }, CollectibleKind::MOTE_OF_LIGHT);
                o.active = false;
                scoreGates_++;
                profile_.totalGatesCleared++;
            }
        }
        for (auto& e : enemies_) {
            if (e.active) {
                e.active = false;
                SpawnParticleBurst(e.pos, 35, GOLD, 100.0f, 350.0f, 6.0f, 0.6f);
                SpawnCollectible(e.pos, CollectibleKind::MOTE_OF_LIGHT);
                runGlimmerEarned_ += 25;
                profile_.glimmer += 25;
            }
        }
        for (int i = 0; i < 12; ++i) {
            float vy = (i - 6) * 55.0f;
            SpawnProjectile({ ghost_.pos.x + 20.0f, ghost_.pos.y },
                            { 720.0f, vy }, 12.0f, (i % 2 == 0) ? SKYBLUE : Color{ 255, 120, 220, 255 }, true, true);
        }
        if (boss_.active && !boss_.shieldImmune) {
            boss_.health -= 55.0f;
        }
    }
}

void GhostGame::TriggerScanPulse() {
    ghost_.scanPulseTimer = 0.45f;
    ghost_.shellExpand = 1.3f;
    audio_.PlayUiSelect();
}

void GhostGame::HandleFatalImpact() {
    if (ghost_.phaseTimer > 0.0f || ghost_.invulnerableTimer > 0.0f || ghost_.superActiveTimer > 0.0f) {
        return;
    }

    if (ghost_.overshieldCharges > 0) {
        ghost_.overshieldCharges--;
        ghost_.invulnerableTimer = 1.6f;
        ghost_.vel.y = -240.0f;
        streakGates_ = 0;
        screenShakeTrauma_ = 0.55f;
        flashColor_ = { 100, 220, 255, 255 };
        flashAlpha_ = 0.45f;
        audio_.PlayShieldBreak();
        SpawnParticleBurst(ghost_.pos, 40, SKYBLUE, 100.0f, 380.0f, 6.0f, 0.55f);
        return;
    }

    state_ = ScreenState::LIGHT_FADED;
    screenShakeTrauma_ = 0.85f;
    flashColor_ = { 220, 40, 40, 255 };
    flashAlpha_ = 0.65f;
    audio_.PlayDeath();
    SpawnParticleBurst(ghost_.pos, 75, WHITE, 60.0f, 420.0f, 7.0f, 1.1f);

    if (isNightfallMode_) {
        if (scoreGates_ > profile_.bestScoreNightfall) profile_.bestScoreNightfall = scoreGates_;
        if (scoreGates_ >= 20) UnlockTriumph(7);
    } else {
        if (scoreGates_ > profile_.bestScoreStandard) profile_.bestScoreStandard = scoreGates_;
    }
    SaveData();
}

void GhostGame::SpawnNextObstacle() {
    Obstacle* slot = nullptr;
    for (auto& o : obstacles_) {
        if (!o.active) {
            slot = &o;
            break;
        }
    }
    if (!slot) return;

    BiomeSector biome = GetCurrentBiome();
    slot->active = true;
    slot->scored = false;
    slot->nearMissAwarded = false;
    slot->frozenByStasis = (ghost_.stasisFreezeTimer > 0.0f);
    slot->biome = biome;
    slot->x = nextObstacleSpawnX_;
    slot->width = 78.0f;
    slot->oracleDestroyed = false;
    slot->oracleHealth = 1.0f;

    float difficulty = std::min(1.0f, static_cast<float>(scoreGates_) / 70.0f);
    float minGap = isNightfallMode_ ? 148.0f : 156.0f;
    slot->gapHeight = std::max(minGap, 210.0f - difficulty * 50.0f);
    if (boss_.active) {
        slot->gapHeight += 28.0f;
    }

    float margin = 125.0f + slot->gapHeight * 0.5f;
    slot->baseGapY = rng_.RangeFloat(margin, VIRTUAL_HEIGHT - margin);
    slot->gapCenterY = slot->baseGapY;
    slot->movePhase = rng_.RangeFloat(0.0f, 6.28f);

    float roll = rng_.NextFloat();
    if (scoreGates_ < 3) {
        slot->kind = ObstacleKind::STANDARD_GATE;
        slot->moveSpeedY = 0.0f;
    } else if (biome == BiomeSector::VAULT_OF_GLASS && roll < 0.45f) {
        slot->kind = ObstacleKind::MOVING_VEX_LASER;
        slot->moveSpeedY = rng_.RangeFloat(1.6f, 2.8f);
    } else if ((biome == BiomeSector::VAULT_OF_GLASS || biome == BiomeSector::HELLMOUTH) && roll < 0.72f) {
        slot->kind = ObstacleKind::ROTATING_ORACLE;
        slot->gapHeight += 36.0f;
        slot->moveSpeedY = rng_.RangeFloat(1.0f, 2.0f);
        slot->hazardRadius = 26.0f;
    } else if (biome == BiomeSector::ASCENDANT_PLANE && roll < 0.55f) {
        slot->kind = ObstacleKind::TAKEN_BLIGHT_WELL;
        slot->moveSpeedY = rng_.RangeFloat(1.4f, 2.4f);
        slot->hazardRadius = 30.0f;
    } else {
        slot->kind = ObstacleKind::STANDARD_GATE;
        slot->moveSpeedY = (roll < 0.30f + difficulty * 0.30f) ? rng_.RangeFloat(1.2f, 2.2f) : 0.0f;
    }

    float spacing = std::max(300.0f, 385.0f - difficulty * 65.0f);
    float midX = slot->x + spacing * 0.52f;
    float midY = rng_.RangeFloat(150.0f, VIRTUAL_HEIGHT - 150.0f);

    if (boss_.active && boss_.shieldImmune && rng_.Chance(0.55f)) {
        SpawnCollectible({ slot->x + slot->width * 0.5f, slot->baseGapY }, CollectibleKind::VEX_CONFLUX);
    } else if (rng_.Chance(0.08f)) {
        SpawnCollectible({ midX, midY }, CollectibleKind::EXOTIC_ENGRAM);
    } else if (rng_.Chance(0.78f)) {
        SpawnCollectible({ midX, midY }, CollectibleKind::MOTE_OF_LIGHT);
        if (rng_.Chance(0.35f)) {
            SpawnCollectible({ slot->x + slot->width * 0.5f, slot->baseGapY }, CollectibleKind::MOTE_OF_LIGHT);
        }
    }

    // Spawn Killable Patrol Enemy between gates (Fallen Shanks, Vex Harpies, Hive Thralls, Taken Psions)
    if (scoreGates_ >= 2 && rng_.Chance(0.65f)) {
        EnemyType et = static_cast<EnemyType>(static_cast<int>(biome) % 4);
        float ey = rng_.RangeFloat(140.0f, VIRTUAL_HEIGHT - 140.0f);
        SpawnPatrolEnemy({ midX + 35.0f, ey }, et);
    }

    nextObstacleSpawnX_ += spacing;
}

void GhostGame::SpawnPatrolEnemy(Vector2 pos, EnemyType type) {
    for (auto& e : enemies_) {
        if (!e.active) {
            e.active = true;
            e.type = type;
            e.pos = pos;
            e.vel = { 0.0f, 0.0f };
            e.maxHealth = 1.0f; // Dies in 1 blaster or ability hit
            e.health = e.maxHealth;
            e.bobTimer = rng_.RangeFloat(0.0f, 6.28f);
            switch (type) {
                case EnemyType::FALLEN_SHANK:   e.radius = 18.0f; e.color = ORANGE; break;
                case EnemyType::VEX_SUPPLICANT: e.radius = 19.0f; e.color = { 70, 220, 255, 255 }; break;
                case EnemyType::HIVE_THRALL:    e.radius = 16.0f; e.color = { 120, 245, 120, 255 }; break;
                case EnemyType::TAKEN_PSION:    e.radius = 17.0f; e.color = { 210, 100, 255, 255 }; break;
            }
            return;
        }
    }
}

void GhostGame::SpawnCollectible(Vector2 pos, CollectibleKind kind) {
    for (auto& c : collectibles_) {
        if (!c.active) {
            c.active = true;
            c.kind = kind;
            c.pos = pos;
            c.vel = { 0.0f, 0.0f };
            c.radius = (kind == CollectibleKind::VEX_CONFLUX) ? 22.0f :
                       (kind == CollectibleKind::EXOTIC_ENGRAM ? 18.0f : 14.0f);
            c.bobPhase = rng_.RangeFloat(0.0f, 6.28f);
            return;
        }
    }
}

void GhostGame::SpawnBossEncounter() {
    boss_.active = true;
    boss_.bossType = static_cast<int>(GetCurrentBiome());
    switch (boss_.bossType) {
        case 0: boss_.name = "SEPIKS PRIME // EXALTED SERVITOR"; break;
        case 1: boss_.name = "THE TEMPLAR // VEX ORACLE MIND"; break;
        case 2: boss_.name = "SAVATHUN'S SONG // GREAT SHRIEKER"; break;
        default: boss_.name = "TAKEN SUBJUGATOR // WITNESS HERALD"; break;
    }
    boss_.pos = { VIRTUAL_WIDTH + 140.0f, VIRTUAL_HEIGHT * 0.5f };
    boss_.targetY = VIRTUAL_HEIGHT * 0.5f;
    boss_.maxHealth = 100.0f + (scoreGates_ / 20) * 25.0f;
    boss_.health = boss_.maxHealth;
    boss_.attackTimer = 2.0f;
    boss_.phaseAngle = 0.0f;
    boss_.confluxesScanned = 0;
    boss_.shieldImmune = true;

    audio_.PlayBossAlert();
    screenShakeTrauma_ = 0.5f;
}

void GhostGame::SpawnParticleBurst(Vector2 pos, int count, Color color, float speedMin, float speedMax, float size, float life) {
    int spawned = 0;
    for (auto& p : particles_) {
        if (!p.active) {
            p.active = true;
            p.pos = pos;
            float angle = rng_.RangeFloat(0.0f, 6.283185f);
            float spd = rng_.RangeFloat(speedMin, speedMax);
            p.vel = { std::cos(angle) * spd, std::sin(angle) * spd };
            p.size = size * rng_.RangeFloat(0.65f, 1.35f);
            p.maxLife = life * rng_.RangeFloat(0.7f, 1.25f);
            p.life = p.maxLife;
            p.color = color;
            p.additive = true;
            if (++spawned >= count) break;
        }
    }
}

void GhostGame::SpawnProjectile(Vector2 pos, Vector2 vel, float radius, Color color, bool friendly, bool homing) {
    for (auto& pr : projectiles_) {
        if (!pr.active) {
            pr.active = true;
            pr.friendly = friendly;
            pr.homing = homing;
            pr.pos = pos;
            pr.vel = vel;
            pr.radius = radius;
            pr.life = 3.5f;
            pr.color = color;
            return;
        }
    }
}

void GhostGame::UpdateFrameInput() {
    if (IsKeyPressed(KEY_M)) {
        audio_.ToggleMute();
    }

    if (state_ == ScreenState::PLAYING) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
            state_ = ScreenState::PAUSED;
            audio_.PlayUiSelect();
            return;
        }

        bool tapInput = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 vm = GetVirtualMousePos();
            Rectangle tacBtn = { VIRTUAL_WIDTH - 256.0f, VIRTUAL_HEIGHT - 92.0f, 112.0f, 62.0f };
            Rectangle supBtn = { VIRTUAL_WIDTH - 134.0f, VIRTUAL_HEIGHT - 92.0f, 112.0f, 62.0f };
            if (CheckCollisionPointRec(vm, tacBtn)) {
                TriggerTacticalAbility();
                return;
            } else if (CheckCollisionPointRec(vm, supBtn)) {
                TriggerSuperAbility();
                return;
            }
            tapInput = true;
            lastTapTime_ = globalTime_;
        }

        if (tapInput) {
            TriggerFlap();
        }

        if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT) ||
            IsKeyPressed(KEY_E) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            TriggerTacticalAbility();
        }

        if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_Q)) {
            TriggerSuperAbility();
        }

        if (IsKeyPressed(KEY_C)) {
            TriggerScanPulse();
        }
    } else if (state_ == ScreenState::TITLE_FRONT_PAGE) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            GetKeyPressed() > 0) {
            state_ = ScreenState::TITLE_DIRECTOR;
            audio_.PlayUiSelect();
        }
    } else if (state_ == ScreenState::PAUSED) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE)) {
            state_ = ScreenState::PLAYING;
            audio_.PlayUiSelect();
        }
    } else if (state_ == ScreenState::LIGHT_FADED) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) {
            StartNewRun(isNightfallMode_);
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            state_ = ScreenState::TITLE_DIRECTOR;
            audio_.PlayUiSelect();
        }
    } else if (state_ != ScreenState::TITLE_DIRECTOR && state_ != ScreenState::TITLE_FRONT_PAGE) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            state_ = ScreenState::TITLE_DIRECTOR;
            audio_.PlayUiSelect();
        }
    }
}

void GhostGame::UpdateFixed(float dt) {
    globalTime_ += dt;

    float starSpeedScale = (state_ == ScreenState::PLAYING) ? (scrollSpeed_ / 310.0f) : 0.35f;
    for (auto& star : stars_) {
        star.pos.x -= (28.0f + star.depth * 95.0f) * starSpeedScale * dt;
        if (star.pos.x < -10.0f) {
            star.pos.x = VIRTUAL_WIDTH + 10.0f;
            star.pos.y = rng_.RangeFloat(0.0f, static_cast<float>(VIRTUAL_HEIGHT));
        }
    }

    if (screenShakeTrauma_ > 0.0f) screenShakeTrauma_ = std::max(0.0f, screenShakeTrauma_ - 1.8f * dt);
    if (flashAlpha_ > 0.0f) flashAlpha_ = std::max(0.0f, flashAlpha_ - 1.6f * dt);
    if (activeToast_.active) {
        activeToast_.timer -= dt;
        if (activeToast_.timer <= 0.0f) activeToast_.active = false;
    }
    if (activeMedal_.active) {
        activeMedal_.timer -= dt;
        if (activeMedal_.timer <= 0.0f) activeMedal_.active = false;
    }

    UpdateParticlesFixed(dt);

    if (state_ == ScreenState::PLAYING) {
        UpdatePlayingFixed(dt);
    }
}

void GhostGame::UpdatePlayingFixed(float dt) {
    runTime_ += dt;
    distanceMeters_ += (scrollSpeed_ * 0.025f) * dt;

    float targetSpeed = (isNightfallMode_ ? 345.0f : 310.0f) + std::min(190.0f, scoreGates_ * 2.6f);
    if (ghost_.stasisFreezeTimer > 0.0f) {
        targetSpeed *= 0.72f; // Stasis slows incoming barrier velocity!
    }
    scrollSpeed_ = Lerp(scrollSpeed_, targetSpeed, 2.5f * dt);

    BiomeSector currentBiome = GetCurrentBiome();
    if (currentBiome != lastBiome_) {
        lastBiome_ = currentBiome;
        biomeBannerTimer_ = 3.5f;
        flashColor_ = WHITE;
        flashAlpha_ = 0.25f;
        if (currentBiome == BiomeSector::VAULT_OF_GLASS) UnlockTriumph(1);
        if (currentBiome == BiomeSector::HELLMOUTH)      UnlockTriumph(2);
        if (currentBiome == BiomeSector::ASCENDANT_PLANE) UnlockTriumph(3);
    }
    if (biomeBannerTimer_ > 0.0f) biomeBannerTimer_ -= dt;

    if (ghost_.tacticalCooldown > 0.0f) ghost_.tacticalCooldown = std::max(0.0f, ghost_.tacticalCooldown - dt);
    if (ghost_.phaseTimer > 0.0f) ghost_.phaseTimer = std::max(0.0f, ghost_.phaseTimer - dt);
    if (ghost_.grappleTimer > 0.0f) {
        ghost_.grappleTimer = std::max(0.0f, ghost_.grappleTimer - dt);
        ghost_.grappleAnchor.x -= scrollSpeed_ * dt;
    }
    if (ghost_.stasisFreezeTimer > 0.0f) ghost_.stasisFreezeTimer = std::max(0.0f, ghost_.stasisFreezeTimer - dt);
    if (ghost_.invulnerableTimer > 0.0f) ghost_.invulnerableTimer = std::max(0.0f, ghost_.invulnerableTimer - dt);
    if (ghost_.scanPulseTimer > 0.0f) ghost_.scanPulseTimer = std::max(0.0f, ghost_.scanPulseTimer - dt);

    if (ghost_.superActiveTimer > 0.0f) {
        ghost_.superActiveTimer = std::max(0.0f, ghost_.superActiveTimer - dt);
        LightSubclass sub = static_cast<LightSubclass>(profile_.selectedSubclass);
        if (sub == LightSubclass::ARC_CALLER && std::fmod(runTime_, 0.06f) < dt) {
            for (auto& o : obstacles_) {
                if (o.active && o.x > ghost_.pos.x && o.x < VIRTUAL_WIDTH) {
                    o.gapHeight = std::min(440.0f, o.gapHeight + 240.0f * dt);
                    o.oracleDestroyed = true;
                }
            }
            if (boss_.active && !boss_.shieldImmune) {
                boss_.health -= 26.0f * dt;
            }
        } else if ((sub == LightSubclass::SOLAR_DAWN || sub == LightSubclass::STRAND_RUNNER) && std::fmod(runTime_, 0.22f) < dt) {
            SpawnProjectile({ ghost_.pos.x + 22.0f, ghost_.pos.y },
                            { 760.0f, rng_.RangeFloat(-80.0f, 80.0f) }, 11.0f, GetSubclassAccentColor(), true, true);
        }
    } else {
        float trickle = 1.4f + profile_.upgrades.superEfficiencyLevel * 0.25f;
        ghost_.superEnergy = std::min(100.0f, ghost_.superEnergy + trickle * dt);
    }

    // Ghost Physics
    float gravity = 1180.0f;
    if (static_cast<LightSubclass>(profile_.selectedSubclass) == LightSubclass::SOLAR_DAWN &&
        ghost_.vel.y > 40.0f && (IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT))) {
        gravity = 420.0f;
    }
    if (ghost_.phaseTimer > 0.0f) {
        gravity *= 0.35f;
    }

    ghost_.vel.y += gravity * dt;
    ghost_.vel.y = std::clamp(ghost_.vel.y, -620.0f, 680.0f);
    ghost_.pos.y += ghost_.vel.y * dt;

    ghost_.rotation = Lerp(ghost_.rotation, std::clamp(ghost_.vel.y * 0.065f, -32.0f, 45.0f), 12.0f * dt);
    ghost_.shellExpand = Lerp(ghost_.shellExpand, (ghost_.superActiveTimer > 0.0f ? 0.65f : 0.0f), 6.5f * dt);
    ghost_.shellSpinAngle += (1.8f + ghost_.shellExpand * 5.0f) * dt;
    ghost_.eyeLookOffset = { 4.5f, std::clamp(ghost_.vel.y * 0.012f, -4.5f, 4.5f) };

    if (rng_.Chance(0.55f)) {
        const auto& shell = SHELL_CATALOG[profile_.equippedShell];
        Color tc = (ghost_.superActiveTimer > 0.0f) ? GetSubclassAccentColor() : shell.trailColor;
        SpawnParticleBurst({ ghost_.pos.x - 16.0f, ghost_.pos.y + rng_.RangeFloat(-6.0f, 6.0f) },
                           1, tc, 20.0f, 60.0f, 3.8f, 0.28f);
    }

    if (ghost_.pos.y < 36.0f) {
        ghost_.pos.y = 36.0f;
        ghost_.vel.y = 60.0f;
    }
    if (ghost_.pos.y > VIRTUAL_HEIGHT - 44.0f) {
        ghost_.pos.y = VIRTUAL_HEIGHT - 44.0f;
        HandleFatalImpact();
        if (state_ != ScreenState::PLAYING) return;
    }

    nextObstacleSpawnX_ -= scrollSpeed_ * dt;
    if (nextObstacleSpawnX_ < VIRTUAL_WIDTH + 280.0f) {
        SpawnNextObstacle();
    }

    for (auto& o : obstacles_) {
        if (!o.active) continue;
        o.x -= scrollSpeed_ * dt;

        if (o.moveSpeedY > 0.0f && !o.frozenByStasis && ghost_.stasisFreezeTimer <= 0.0f) {
            o.gapCenterY = o.baseGapY + std::sin(runTime_ * o.moveSpeedY + o.movePhase) * 68.0f;
        }

        if (o.kind == ObstacleKind::TAKEN_BLIGHT_WELL && !o.oracleDestroyed && !o.frozenByStasis) {
            Vector2 blightPos = { o.x + o.width * 0.5f, o.gapCenterY };
            float dist = Vector2Distance(ghost_.pos, blightPos);
            if (dist < 240.0f && dist > 10.0f) {
                Vector2 pullDir = Vector2Normalize(Vector2Subtract(blightPos, ghost_.pos));
                ghost_.vel.y += pullDir.y * 520.0f * dt;
            }
        }

        float gapTop = o.gapCenterY - o.gapHeight * 0.5f;
        float gapBottom = o.gapCenterY + o.gapHeight * 0.5f;

        // Score gate check + Vanguard Streak Medals + Riskrunner Perk
        if (!o.scored && (o.x + o.width) < ghost_.pos.x) {
            o.scored = true;
            scoreGates_++;
            streakGates_++;
            gatesSinceLastBoss_++;
            profile_.totalGatesCleared++;
            runGlimmerEarned_ += isNightfallMode_ ? 3 : 2;
            profile_.glimmer += isNightfallMode_ ? 3 : 2;

            if (scoreGates_ >= 10) UnlockTriumph(0);

            // Riskrunner Perk: Close call near barrier edge grants +18% Super & +10 Glimmer
            if (static_cast<ExoticPerk>(profile_.equippedExoticPerk) == ExoticPerk::RISKRUNNER_CONDUCTOR) {
                if (std::abs(ghost_.pos.y - gapTop) < 38.0f || std::abs(ghost_.pos.y - gapBottom) < 38.0f) {
                    ghost_.superEnergy = std::min(100.0f, ghost_.superEnergy + 18.0f);
                    TriggerVanguardMedal("ARC CONDUCTOR // CLOSE CALL!", 10, SKYBLUE);
                    SpawnParticleBurst(ghost_.pos, 20, SKYBLUE, 60.0f, 220.0f, 5.0f, 0.35f);
                }
            }

            // Vanguard Streak Medals!
            if (streakGates_ == 5)       TriggerVanguardMedal("LIGHTBEARER STREAK (5x)", 15, SKYBLUE);
            else if (streakGates_ == 10) TriggerVanguardMedal("UNBROKEN VANGUARD (10x)", 30, GOLD);
            else if (streakGates_ == 20) TriggerVanguardMedal("WE RAN OUT OF MEDALS! (20x)", 60, ORANGE);
            else if (streakGates_ > 20 && streakGates_ % 10 == 0) {
                TriggerVanguardMedal("SEVENTH COLUMN!", 75, GOLD);
            }

            if (!boss_.active && gatesSinceLastBoss_ >= 20) {
                SpawnBossEncounter();
            }
        }

        Rectangle topRect = { o.x, 0.0f, o.width, gapTop };
        Rectangle botRect = { o.x, gapBottom, o.width, VIRTUAL_HEIGHT - gapBottom };

        bool hitObstacle = CheckCollisionCircleRec(ghost_.pos, ghost_.radius * 0.82f, topRect) ||
                           CheckCollisionCircleRec(ghost_.pos, ghost_.radius * 0.82f, botRect);

        if ((o.kind == ObstacleKind::ROTATING_ORACLE || o.kind == ObstacleKind::TAKEN_BLIGHT_WELL) && !o.oracleDestroyed) {
            Vector2 nodePos = { o.x + o.width * 0.5f, o.gapCenterY };
            if (CheckCollisionCircles(ghost_.pos, ghost_.radius * 0.82f, nodePos, o.hazardRadius * 0.85f)) {
                hitObstacle = true;
            }
        }

        if (hitObstacle) {
            if (ghost_.phaseTimer > 0.0f) {
                UnlockTriumph(5);
            } else if (ghost_.superActiveTimer > 0.0f) {
                SpawnParticleBurst({ o.x + o.width * 0.5f, ghost_.pos.y }, 25, GetSubclassAccentColor(), 90.0f, 340.0f, 6.0f, 0.45f);
                o.active = false;
            } else {
                HandleFatalImpact();
                if (state_ != ScreenState::PLAYING) return;
            }
        }

        if (o.x + o.width < -120.0f) {
            o.active = false;
        }
    }

    float magnetRadius = 95.0f + profile_.upgrades.lightMagnetLevel * 32.0f;
    if (ghost_.superActiveTimer > 0.0f) magnetRadius *= 1.85f;

    for (auto& c : collectibles_) {
        if (!c.active) continue;
        c.pos.x -= scrollSpeed_ * dt;
        c.bobPhase += 4.2f * dt;
        c.pos.y += std::sin(c.bobPhase) * 28.0f * dt;

        float dist = Vector2Distance(ghost_.pos, c.pos);
        if (dist < magnetRadius && dist > 1.0f) {
            Vector2 pull = Vector2Normalize(Vector2Subtract(ghost_.pos, c.pos));
            c.pos = Vector2Add(c.pos, Vector2Scale(pull, 440.0f * dt));
        }

        if (CheckCollisionCircles(ghost_.pos, ghost_.radius + 6.0f, c.pos, c.radius)) {
            c.active = false;
            if (c.kind == CollectibleKind::MOTE_OF_LIGHT) {
                runMotes_++;
                profile_.totalMotesCollected++;
                int glim = isNightfallMode_ ? 6 : 4;
                runGlimmerEarned_ += glim;
                profile_.glimmer += glim;
                float superGain = 14.0f + profile_.upgrades.superEfficiencyLevel * 2.2f;
                ghost_.superEnergy = std::min(100.0f, ghost_.superEnergy + superGain);
                audio_.PlayMotePickup();
                SpawnParticleBurst(c.pos, 16, SKYBLUE, 60.0f, 220.0f, 4.5f, 0.38f);

                // Exotic Perk: Crimson Redrix Core restores +1 Overshield every 8 Motes!
                if (static_cast<ExoticPerk>(profile_.equippedExoticPerk) == ExoticPerk::CRIMSON_REDRIX &&
                    (runMotes_ % 8 == 0)) {
                    ghost_.overshieldCharges = std::min(3, ghost_.overshieldCharges + 1);
                    TriggerVanguardMedal("CRIMSON SIPHON // +1 OVERSHIELD", 15, { 255, 70, 85, 255 });
                }
            } else if (c.kind == CollectibleKind::EXOTIC_ENGRAM) {
                runGlimmerEarned_ += 50;
                profile_.glimmer += 50;
                ghost_.superEnergy = std::min(100.0f, ghost_.superEnergy + 45.0f);
                ghost_.overshieldCharges = std::min(3, ghost_.overshieldCharges + 1);
                audio_.PlayExoticPickup();
                SpawnParticleBurst(c.pos, 35, GOLD, 80.0f, 320.0f, 6.0f, 0.55f);
                UnlockTriumph(8);
            } else if (c.kind == CollectibleKind::VEX_CONFLUX) {
                TriggerScanPulse();
                audio_.PlayExoticPickup();
                SpawnParticleBurst(c.pos, 30, { 120, 255, 220, 255 }, 90.0f, 300.0f, 5.5f, 0.50f);
                if (boss_.active) {
                    boss_.confluxesScanned++;
                    if (boss_.confluxesScanned >= 3) {
                        boss_.shieldImmune = false;
                        boss_.health -= boss_.maxHealth * 0.35f;
                        audio_.PlayShieldBreak();
                        screenShakeTrauma_ = 0.5f;
                    }
                }
            }
        }

        if (c.pos.x < -80.0f) {
            c.active = false;
        }
    }

    for (auto& pr : projectiles_) {
        if (!pr.active) continue;

        // Homing Wolfpack / Strand Threadling steering
        if (pr.friendly && pr.homing) {
            Vector2 bestTarget = { pr.pos.x + 300.0f, pr.pos.y };
            float bestDist = 99999.0f;
            for (const auto& o : obstacles_) {
                if (o.active && !o.oracleDestroyed &&
                    (o.kind == ObstacleKind::ROTATING_ORACLE || o.kind == ObstacleKind::TAKEN_BLIGHT_WELL)) {
                    Vector2 np = { o.x + o.width * 0.5f, o.gapCenterY };
                    float d = Vector2Distance(pr.pos, np);
                    if (d < bestDist) { bestDist = d; bestTarget = np; }
                }
            }
            if (boss_.active) {
                float d = Vector2Distance(pr.pos, boss_.pos);
                if (d < bestDist) { bestDist = d; bestTarget = boss_.pos; }
            }
            if (bestDist < 900.0f) {
                Vector2 desired = Vector2Scale(Vector2Normalize(Vector2Subtract(bestTarget, pr.pos)), 720.0f);
                pr.vel = Vector2Lerp(pr.vel, desired, 8.0f * dt);
            }
        }

        pr.pos = Vector2Add(pr.pos, Vector2Scale(pr.vel, dt));
        pr.life -= dt;
        if (pr.life <= 0.0f || pr.pos.x < -60.0f || pr.pos.x > VIRTUAL_WIDTH + 60.0f ||
            pr.pos.y < -60.0f || pr.pos.y > VIRTUAL_HEIGHT + 60.0f) {
            pr.active = false;
            continue;
        }

        if (pr.friendly) {
            for (auto& o : obstacles_) {
                if (o.active && !o.oracleDestroyed &&
                    (o.kind == ObstacleKind::ROTATING_ORACLE || o.kind == ObstacleKind::TAKEN_BLIGHT_WELL)) {
                    Vector2 nodePos = { o.x + o.width * 0.5f, o.gapCenterY };
                    if (CheckCollisionCircles(pr.pos, pr.radius, nodePos, o.hazardRadius)) {
                        o.oracleDestroyed = true;
                        pr.active = false;
                        SpawnParticleBurst(nodePos, 24, pr.color, 80.0f, 280.0f, 5.5f, 0.45f);
                        SpawnCollectible(nodePos, CollectibleKind::MOTE_OF_LIGHT);
                        TriggerVanguardMedal("ORACLE SHATTERED!", 10, GOLD);
                        break;
                    }
                }
            }

            // Friendly projectile collision with Killable Patrol Enemies!
            if (pr.active) {
                for (auto& e : enemies_) {
                    if (e.active && CheckCollisionCircles(pr.pos, pr.radius, e.pos, e.radius)) {
                        e.health -= 1.0f;
                        pr.active = false;
                        if (e.health <= 0.0f) {
                            e.active = false;
                            SpawnParticleBurst(e.pos, 28, e.color, 70.0f, 260.0f, 5.5f, 0.45f);
                            SpawnCollectible(e.pos, CollectibleKind::MOTE_OF_LIGHT);
                            runGlimmerEarned_ += 10;
                            profile_.glimmer += 10;
                            TriggerVanguardMedal("COMBATANT ELIMINATED", 10, e.color);
                        }
                        break;
                    }
                }
            }

            if (pr.active && boss_.active && CheckCollisionCircles(pr.pos, pr.radius, boss_.pos, 58.0f)) {
                pr.active = false;
                if (!boss_.shieldImmune) {
                    boss_.health -= 18.0f;
                    SpawnParticleBurst(pr.pos, 18, pr.color, 70.0f, 240.0f, 5.0f, 0.35f);
                } else {
                    SpawnParticleBurst(pr.pos, 10, LIGHTGRAY, 40.0f, 120.0f, 3.5f, 0.25f);
                }
            }
        } else {
            if (CheckCollisionCircles(pr.pos, pr.radius, ghost_.pos, ghost_.radius * 0.80f)) {
                pr.active = false;
                HandleFatalImpact();
                if (state_ != ScreenState::PLAYING) return;
            }
        }
    }

    // Update Patrol Enemies & collision with Ghost
    for (auto& e : enemies_) {
        if (!e.active) continue;
        e.pos.x -= scrollSpeed_ * dt;
        e.bobTimer += 3.5f * dt;
        e.pos.y += std::sin(e.bobTimer) * 45.0f * dt;

        if (e.pos.x < -80.0f) {
            e.active = false;
            continue;
        }

        // Check fatal collision with Ghost unless phased or invincible
        if (CheckCollisionCircles(ghost_.pos, ghost_.radius * 0.82f, e.pos, e.radius * 0.85f)) {
            if (ghost_.phaseTimer > 0.0f || ghost_.superActiveTimer > 0.0f) {
                // Ghost vaporizes enemy during Phase / Super
                e.active = false;
                SpawnParticleBurst(e.pos, 25, GetSubclassAccentColor(), 80.0f, 260.0f, 5.0f, 0.4f);
                SpawnCollectible(e.pos, CollectibleKind::MOTE_OF_LIGHT);
                runGlimmerEarned_ += 10;
                profile_.glimmer += 10;
            } else {
                HandleFatalImpact();
                if (state_ != ScreenState::PLAYING) return;
            }
        }
    }

    // Companion Turret Drone logic (orbits Ghost & auto-fires at nearest target)
    ghost_.companionAngle += 3.2f * dt;
    ghost_.companionShootTimer -= dt;
    if (ghost_.companionShootTimer <= 0.0f) {
        ghost_.companionShootTimer = (static_cast<LightSubclass>(profile_.selectedSubclass) == LightSubclass::PRISMATIC_TRANSCENDENT) ? 0.65f : 1.25f;
        Vector2 dronePos = {
            ghost_.pos.x + std::cos(ghost_.companionAngle) * 42.0f,
            ghost_.pos.y + std::sin(ghost_.companionAngle) * 32.0f
        };
        // Target nearest obstacle node, enemy, or boss
        Vector2 target = { dronePos.x + 400.0f, dronePos.y };
        float closestDist = 9999.0f;
        for (const auto& e : enemies_) {
            if (e.active && e.pos.x > dronePos.x) {
                float d = Vector2Distance(dronePos, e.pos);
                if (d < closestDist) { closestDist = d; target = e.pos; }
            }
        }
        for (const auto& o : obstacles_) {
            if (o.active && !o.oracleDestroyed && (o.kind == ObstacleKind::ROTATING_ORACLE || o.kind == ObstacleKind::TAKEN_BLIGHT_WELL)) {
                Vector2 np = { o.x + o.width * 0.5f, o.gapCenterY };
                if (np.x > dronePos.x) {
                    float d = Vector2Distance(dronePos, np);
                    if (d < closestDist) { closestDist = d; target = np; }
                }
            }
        }
        if (boss_.active && boss_.pos.x > dronePos.x) {
            float d = Vector2Distance(dronePos, boss_.pos);
            if (d < closestDist) { closestDist = d; target = boss_.pos; }
        }
        if (closestDist < 750.0f) {
            Vector2 shotDir = Vector2Normalize(Vector2Subtract(target, dronePos));
            SpawnProjectile(dronePos, Vector2Scale(shotDir, 820.0f), 8.0f, GetSubclassAccentColor(), true, true);
        }
    }

    if (boss_.active) {
        UpdateBossFixed(dt);
    }
}

void GhostGame::UpdateBossFixed(float dt) {
    boss_.pos.x = Lerp(boss_.pos.x, VIRTUAL_WIDTH - 165.0f, 2.5f * dt);
    if (ghost_.stasisFreezeTimer <= 0.0f) {
        boss_.phaseAngle += 1.9f * dt;
        boss_.targetY = VIRTUAL_HEIGHT * 0.5f + std::sin(boss_.phaseAngle) * 175.0f;
        boss_.pos.y = Lerp(boss_.pos.y, boss_.targetY, 4.0f * dt);
    }

    if (!boss_.shieldImmune) {
        boss_.health -= 11.5f * dt;
        if (std::fmod(runTime_, 0.28f) < dt) {
            SpawnProjectile({ ghost_.pos.x + 20.0f, ghost_.pos.y },
                            Vector2Scale(Vector2Normalize(Vector2Subtract(boss_.pos, ghost_.pos)), 780.0f),
                            7.0f, GetSubclassAccentColor(), true, true);
        }
    }

    if (ghost_.stasisFreezeTimer <= 0.0f) {
        boss_.attackTimer -= dt;
        if (boss_.attackTimer <= 0.0f) {
            boss_.attackTimer = isNightfallMode_ ? 1.85f : 2.35f;
            audio_.PlayLaserShot();

            Vector2 dirToGhost = Vector2Normalize(Vector2Subtract(ghost_.pos, boss_.pos));
            Color projColor = (boss_.bossType == 0) ? PURPLE :
                              (boss_.bossType == 1) ? RED :
                              (boss_.bossType == 2) ? GREEN : SKYBLUE;
            SpawnProjectile({ boss_.pos.x - 45.0f, boss_.pos.y }, Vector2Scale(dirToGhost, 440.0f), 10.0f, projColor, false, false);
        }
    }

    if (boss_.health <= 0.0f) {
        boss_.active = false;
        gatesSinceLastBoss_ = 0;
        profile_.totalBossesDefeated++;
        runGlimmerEarned_ += 120;
        profile_.glimmer += 120;
        screenShakeTrauma_ = 0.8f;
        flashColor_ = GOLD;
        flashAlpha_ = 0.55f;
        audio_.PlayExoticPickup();
        SpawnParticleBurst(boss_.pos, 90, GOLD, 100.0f, 480.0f, 8.0f, 1.0f);
        SpawnCollectible(boss_.pos, CollectibleKind::EXOTIC_ENGRAM);
        UnlockTriumph(6);
    }
}

void GhostGame::UpdateParticlesFixed(float dt) {
    for (auto& p : particles_) {
        if (!p.active) continue;
        p.pos = Vector2Add(p.pos, Vector2Scale(p.vel, dt));
        p.vel = Vector2Scale(p.vel, 1.0f - 2.2f * dt);
        p.life -= dt;
        if (p.life <= 0.0f) {
            p.active = false;
        }
    }
}

void GhostGame::RunFrame() {
    float frameDt = std::min(GetFrameTime(), 0.1f);
    accumulator_ += frameDt;

    UpdateFrameInput();

    int musicMode = (state_ == ScreenState::TITLE_FRONT_PAGE) ? 0 :
                    (state_ == ScreenState::PLAYING || state_ == ScreenState::PAUSED || state_ == ScreenState::LIGHT_FADED) ? 2 : 1;
    audio_.UpdateMusic(musicMode);

    while (accumulator_ >= FIXED_DT) {
        UpdateFixed(FIXED_DT);
        accumulator_ -= FIXED_DT;
    }

    RenderFrame();
}

} // namespace FlappyGhost
