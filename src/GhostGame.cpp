#include "GhostGame.hpp"

#undef VIRTUAL_WIDTH
#define VIRTUAL_WIDTH virtualWidth_

#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <vector>
#include <cstring>

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
    { 6, "GODSLAYER PROTOCOL", "Defeat an Ultra Boss Encounter in Nightfall Strike mode.", 200 },
    { 7, "NIGHTFALL VANGUARD", "Score 20 or higher in Nightfall Strike mode.", 200 },
    { 8, "EXOTIC ARCHIVIST", "Collect a golden Exotic Engram in mid-flight.", 100 },
    { 9, "FASHION ENDGAME", "Unlock at least 3 custom Ghost Shells in The Tower.", 250 },
    { 10, "WARMIND PROTOCOL", "Vaporize 50 patrol combatants or Fallen Skiffs with Drone or Super.", 200 },
    { 11, "LIGHT HARVESTER", "Collect 100 Glimmer motes in a single patrol sortie.", 150 },
    { 12, "GRANDMASTER ASCENT", "Survive past Gate 60 in a single continuous flight sortie.", 300 },
    { 13, "DISCIPLE OF LIGHT", "Unlock and master all 6 Paracausal Subclasses in The Tower.", 350 },
    { 14, "THE TRAVELER'S CHOSEN", "Synthesize over 3,000 Lifetime Glimmer in your Vault.", 250 },
    { 15, "EVA'S RUNWAY", "Collect and equip at least 6 custom Shaders from Eva Levante.", 250 }
}};
const std::array<ShaderDef, NUM_SHADERS> SHADER_CATALOG = {{
    { 0, "DEFAULT TRAWLER", 0, { 235, 238, 245, 255 }, { 215, 95, 35, 255 }, { 120, 225, 255, 255 } },
    { 1, "CALUS'S SELECTED", 300, { 245, 245, 250, 255 }, { 215, 175, 55, 255 }, { 165, 45, 145, 255 } },
    { 2, "SUPERBLACK", 500, { 18, 18, 22, 255 }, { 32, 34, 40, 255 }, { 255, 255, 255, 255 } },
    { 3, "GAMBIT JADESTONE", 450, { 35, 75, 50, 255 }, { 65, 255, 120, 255 }, { 95, 255, 160, 255 } },
    { 4, "PHOTO FINISH RGB", 650, { 45, 45, 55, 255 }, { 255, 75, 180, 255 }, { 65, 230, 255, 255 } },
    { 5, "NEOPOP WAVE", 400, { 40, 50, 75, 255 }, { 255, 120, 220, 255 }, { 80, 255, 240, 255 } },
    { 6, "CHATTERWHITE", 550, { 250, 252, 255, 255 }, { 210, 220, 230, 255 }, { 255, 255, 255, 255 } },
    { 7, "AMETHYST VEIL", 600, { 22, 22, 28, 255 }, { 85, 35, 125, 255 }, { 145, 75, 215, 255 } },
    { 8, "LUMINOUS VOID", 700, { 42, 24, 72, 255 }, { 215, 160, 45, 255 }, { 185, 80, 255, 255 } },
    { 9, "BUTTERBARK", 450, { 98, 62, 34, 255 }, { 215, 175, 65, 255 }, { 245, 240, 225, 255 } },
    { 10, "CRUCIBLE REDJACK", 500, { 180, 28, 36, 255 }, { 36, 38, 44, 255 }, { 255, 215, 215, 255 } },
    { 11, "MONOCHROMATIC", 650, { 15, 15, 18, 255 }, { 245, 245, 250, 255 }, { 180, 185, 195, 255 } }
}};
const std::array<CombatDroneDef, NUM_DRONES> DRONE_CATALOG = {{
    { 0, "ARC SOUL SENTINEL",
      "Manual-fire Arc buddy ([C / X]). Fires a rapid 3-round burst of homing Arc bolts.",
      200, 0.45f, { 75, 225, 255, 255 } },
    { 1, "HELLION SOLAR MORTAR",
      "Manual-fire Solar buddy ([C / X]). Launches twin high-impact Solar mortar flares.",
      450, 0.60f, { 255, 145, 35, 255 } },
    { 2, "BRIARBINDS VOID SOUL",
      "Manual-fire Void buddy ([C / X]). Unleashes 3 homing Void seeker orbs that shatter targets.",
      750, 0.55f, { 180, 85, 255, 255 } },
    { 3, "NTTE CHRONAL PORTAL",
      "Manual-fire Time Chasm ([C / X]). Fires rapid high-velocity timeline beam volleys.",
      1100, 0.35f, { 130, 245, 255, 255 } }
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

const std::array<CombatDroneDef, NUM_DRONES>& GhostGame::GetDroneCatalog() {
    return DRONE_CATALOG;
}

bool GhostGame::HasUnlockedAnyDrone() const {
    for (bool u : profile_.unlockedDrones) {
        if (u) return true;
    }
    return false;
}

void GhostGame::UpdateVirtualResolution() {
    float sw = static_cast<float>(GetScreenWidth());
    float sh = static_cast<float>(GetScreenHeight());
    if (sw > 0.0f && sh > 0.0f) {
        float targetW = std::max(1280.0f, std::round(720.0f * (sw / sh)));
        if (targetRenderTex_.id == 0 || std::abs(virtualWidth_ - targetW) > 1.0f) {
            virtualWidth_ = targetW;
            if (targetRenderTex_.id != 0) {
                UnloadRenderTexture(targetRenderTex_);
            }
            targetRenderTex_ = LoadRenderTexture(static_cast<int>(virtualWidth_), 720);
            SetTextureFilter(targetRenderTex_.texture, TEXTURE_FILTER_BILINEAR);
        }
    } else if (targetRenderTex_.id == 0) {
        virtualWidth_ = 1280.0f;
        targetRenderTex_ = LoadRenderTexture(1280, 720);
        SetTextureFilter(targetRenderTex_.texture, TEXTURE_FILTER_BILINEAR);
    }
}

void GhostGame::Init() {
    UpdateVirtualResolution();

    audio_.Init();
    LoadSaveData();
    audio_.SetMasterVolume(profile_.masterVolume);
    audio_.SetMusicVolume(profile_.musicVolume);
    audio_.SetSfxVolume(profile_.sfxVolume);
    SetTargetFPS(profile_.targetFps > 0 ? profile_.targetFps : 120);

    auto TryLoadTex = [](const std::initializer_list<const char*>& paths) -> Texture2D {
        for (const char* p : paths) {
#if !defined(PLATFORM_ANDROID)
            if (!FileExists(p)) continue;
#endif
            Texture2D tex = LoadTexture(p);
            if (tex.id != 0) return tex;
        }
        return Texture2D{};
    };
    splashLogo_ = TryLoadTex({ "assets/sebiny_splash_logo.png", "sebiny_splash_logo.png", "../assets/sebiny_splash_logo.png" });
    RefreshDailyBounties();

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
    if (splashLogo_.id != 0) {
        UnloadTexture(splashLogo_);
        splashLogo_.id = 0;
    }
    if (targetRenderTex_.id != 0) {
        UnloadRenderTexture(targetRenderTex_);
    }
}

void GhostGame::LoadSaveData() {
    std::ifstream in(SAVE_FILE_PATH, std::ios::binary);
    if (in.is_open()) {
        in.seekg(0, std::ios::end);
        std::streamsize sz = in.tellg();
        in.seekg(0, std::ios::beg);
        if (sz >= 32) {
            std::vector<char> buf(static_cast<size_t>(sz));
            in.read(buf.data(), sz);
            uint32_t magic = *reinterpret_cast<uint32_t*>(buf.data());
            if (magic == 0x47485358) {
                SaveProfile loaded{};
                size_t copyBytes = std::min(sizeof(SaveProfile), static_cast<size_t>(sz));
                std::memcpy(&loaded, buf.data(), copyBytes);
                profile_ = loaded;
                profile_.magic = 0x47485358;
                profile_.equippedShell = std::clamp(profile_.equippedShell, 0, static_cast<int>(NUM_SHELLS) - 1);
                profile_.selectedSubclass = std::clamp(profile_.selectedSubclass, 0, static_cast<int>(NUM_SUBCLASSES) - 1);
                if (!profile_.unlockedSubclasses[profile_.selectedSubclass]) {
                    profile_.selectedSubclass = 0;
                }
                profile_.equippedExoticPerk = std::clamp(profile_.equippedExoticPerk, 0, static_cast<int>(NUM_EXOTIC_PERKS) - 1);
                profile_.equippedShader = std::clamp(profile_.equippedShader, 0, static_cast<int>(NUM_SHADERS) - 1);
                if (profile_.equippedDrone >= 0) {
                    profile_.equippedDrone = std::clamp(profile_.equippedDrone, 0, static_cast<int>(NUM_DRONES) - 1);
                    if (!profile_.unlockedDrones[profile_.equippedDrone]) {
                        profile_.equippedDrone = -1;
                    }
                }
                audio_.SetMasterVolume(profile_.masterVolume);
                audio_.SetMusicVolume(profile_.musicVolume);
                audio_.SetSfxVolume(profile_.sfxVolume);
                audio_.SetMusicTrackPreference(profile_.musicTrackChoice);
                SetTargetFPS(profile_.targetFps);
                return;
            }
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

void GhostGame::RefreshDailyBounties() {
    std::time_t t = std::time(nullptr);
    std::tm* lt = std::localtime(&t);
    int dateKey = (lt->tm_year + 1900) * 10000 + (lt->tm_mon + 1) * 100 + lt->tm_mday;
    if (activeBountyDateSeed_ == dateKey) return;
    activeBountyDateSeed_ = dateKey;

    PCG32 bountyRng(static_cast<uint64_t>(dateKey) * 987654321ULL);

    struct BountyTemplate {
        const char* title;
        const char* desc;
        int target;
        int reward;
    };
    static const BountyTemplate POOL[] = {
        { "SURGE OPERATIVE", "Unleash Super abilities 3 times during flight.", 3, 60 },
        { "QUANTUM NAVIGATOR", "Clear 20 barrier gateways without wipeout.", 20, 75 },
        { "MOTE SYNTHESIS", "Harvest 35 Motes of Light in Patrol or Nightfall.", 35, 60 },
        { "PHASE DRIFTER", "Execute 6 successful Blink or Dash tactical maneuvers.", 6, 50 },
        { "DRONE ARTILLERY", "Eliminate 8 combatants or drones in Nightfall.", 8, 80 },
        { "LIGHT BEARER", "Survive a single run past 150 meters.", 150, 70 },
        { "PARACAUSAL WARPING", "Trigger 4 Stasis Frost Pulses or Strand Grapples.", 4, 60 },
        { "VANGUARD INITIATE", "Earn a 5x or higher barrier streak.", 5, 50 }
    };
    constexpr int POOL_SIZE = sizeof(POOL) / sizeof(POOL[0]);
    int chosen[NUM_DAILY_BOUNTIES];
    for (size_t i = 0; i < NUM_DAILY_BOUNTIES; ++i) {
        int idx = 0;
        bool duplicate = false;
        do {
            idx = bountyRng.RangeInt(0, POOL_SIZE - 1);
            duplicate = false;
            for (size_t j = 0; j < i; ++j) {
                if (chosen[j] == idx) { duplicate = true; break; }
            }
        } while (duplicate);
        chosen[i] = idx;
        dailyBounties_[i].id = idx;
        dailyBounties_[i].title = POOL[idx].title;
        dailyBounties_[i].description = POOL[idx].desc;
        dailyBounties_[i].targetCount = POOL[idx].target;
        dailyBounties_[i].currentProgress = 0;
        dailyBounties_[i].rewardGlimmer = POOL[idx].reward;
        dailyBounties_[i].completed = false;
        dailyBounties_[i].claimed = false;
    }
}

void GhostGame::ProgressDailyBounty(int bountyType, int count) {
    for (auto& b : dailyBounties_) {
        if (b.completed || b.claimed) continue;
        if (b.id == bountyType) {
            b.currentProgress += count;
            if (b.currentProgress >= b.targetCount) {
                b.currentProgress = b.targetCount;
                b.completed = true;
                b.claimed = true;
                profile_.glimmer += b.rewardGlimmer;
                profile_.totalGlimmerEarnedLifetime += b.rewardGlimmer;
                profile_.totalBountiesCompleted++;
                SaveData();
                activeToast_.active = true;
                activeToast_.title = std::string("BOUNTY CLAIMED: ") + b.title;
                activeToast_.subtitle = std::string("+") + std::to_string(b.rewardGlimmer) + " Glimmer deposited!";
                activeToast_.timer = 4.0f;
                audio_.PlayExoticPickup();
            }
        }
    }
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

    SetScreenState(ScreenState::PLAYING);
    profile_.totalRunsStarted++;
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
    if (isNightfallMode_ && profile_.equippedDrone >= 0 && profile_.equippedDrone < static_cast<int>(NUM_DRONES)) {
        ghost_.droneMaxCooldown = DRONE_CATALOG[profile_.equippedDrone].fireCooldown;
    }

    boss_ = BossState{};
    for (auto& o : obstacles_) o.active = false;
    for (auto& c : collectibles_) c.active = false;
    for (auto& e : enemies_) e.active = false;
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
    return ToVirtualCoords(GetMousePosition());
}

Vector2 GhostGame::ToVirtualCoords(Vector2 screenPos) const {
    float sw = static_cast<float>(GetScreenWidth());
    float sh = static_cast<float>(GetScreenHeight());
    if (sw <= 0.0f || sh <= 0.0f) return { 0.0f, 0.0f };
    return {
        screenPos.x * (virtualWidth_ / sw),
        screenPos.y * (720.0f / sh)
    };
}

bool GhostGame::IsRectHovered(Rectangle rect) const {
    if (CheckCollisionPointRec(GetVirtualMousePos(), rect)) return true;
    int tc = GetTouchPointCount();
    for (int i = 0; i < tc; ++i) {
        if (CheckCollisionPointRec(ToVirtualCoords(GetTouchPosition(i)), rect)) return true;
    }
    return false;
}

void GhostGame::SetScreenState(ScreenState nextState) {
    if (state_ == nextState) return;
    state_ = nextState;
    screenTransitionCooldown_ = 0.25f; // Prevent input bleeding across screen transitions
}

bool GhostGame::IsButtonClicked(Rectangle rect) const {
    if (screenTransitionCooldown_ > 0.0f) return false;
    // Generous touch padding for thumbs on mobile devices
    Rectangle padded = { rect.x - 10.0f, rect.y - 10.0f, rect.width + 20.0f, rect.height + 20.0f };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetVirtualMousePos(), padded)) {
        return true;
    }
    int tc = GetTouchPointCount();
    for (int i = 0; i < tc; ++i) {
        int tid = GetTouchPointId(i);
        bool isNew = true;
        for (int p = 0; p < prevTouchCount_; ++p) {
            if (prevTouchIds_[p] == tid) { isNew = false; break; }
        }
        if (isNew && CheckCollisionPointRec(ToVirtualCoords(GetTouchPosition(i)), padded)) {
            return true;
        }
    }
    return false;
}

void GhostGame::TriggerFlap() {
    ghost_.vel.y = -435.0f;
    ghost_.shellExpand = 1.0f;
    audio_.PlayFlap();

    const auto& shell = SHELL_CATALOG[profile_.equippedShell];
    SpawnParticleBurst({ ghost_.pos.x - 14.0f, ghost_.pos.y + 6.0f }, 9, shell.trailColor, 40.0f, 160.0f, 4.5f, 0.38f);
}

void GhostGame::TriggerTacticalAbility() {
    if (ghost_.tacticalCooldown > 0.0f) return;

    profile_.totalBlinkTeleports++;
    ghost_.tacticalCooldown = ghost_.tacticalMaxCooldown;
    ghost_.shellExpand = 1.6f;
    audio_.PlayBlink();
    ProgressDailyBounty(3, 1); // PHASE DRIFTER
    LightSubclass sub = static_cast<LightSubclass>(profile_.selectedSubclass);
    if (sub == LightSubclass::STASIS_SHADEBINDER || sub == LightSubclass::STRAND_RUNNER) {
        ProgressDailyBounty(6, 1); // PARACAUSAL WARPING
    }

    Color accent = GetSubclassAccentColor();

    // Exotic Perk: Gjallarhorn Wolfpack spawns 3 homing micro-rockets on every tactical ability in Nightfall!
    if (isNightfallMode_ && static_cast<ExoticPerk>(profile_.equippedExoticPerk) == ExoticPerk::GJALLARHORN_WOLFPACK) {
        for (int w = -1; w <= 1; ++w) {
            SpawnProjectile({ ghost_.pos.x + 18.0f, ghost_.pos.y },
                            { 620.0f, w * 140.0f }, 9.0f, GOLD, true, true);
        }
    }

    if (sub == LightSubclass::VOID_WALKER) {
        // Voidwalker Blink: Focused short phase through an immediate hazard (0.32s phase, slight vertical nudge)
        // No longer warps entire world back or teleports past multiple obstacles automatically
        ghost_.phaseTimer = 0.32f;
        ghost_.vel.y = -90.0f;
        SpawnParticleBurst(ghost_.pos, 22, accent, 70.0f, 240.0f, 5.0f, 0.35f);
        flashColor_ = accent;
        flashAlpha_ = 0.25f;
    } else if (sub == LightSubclass::SOLAR_DAWN) {
        ghost_.phaseTimer = 0.65f;
        ghost_.vel.y = 0.0f;
        SpawnParticleBurst(ghost_.pos, 32, accent, 100.0f, 360.0f, 6.5f, 0.50f);
        if (isNightfallMode_) {
            for (int i = -1; i <= 1; ++i) {
                SpawnProjectile({ ghost_.pos.x + 20.0f, ghost_.pos.y },
                                { 650.0f, i * 110.0f }, 12.0f, accent, true, false);
            }
        }
    } else if (sub == LightSubclass::ARC_CALLER) {
        ghost_.phaseTimer = 0.50f;
        ghost_.vel.y = -150.0f;
        SpawnParticleBurst(ghost_.pos, 26, accent, 120.0f, 380.0f, 5.5f, 0.40f);
        if (isNightfallMode_) {
            SpawnProjectile({ ghost_.pos.x + 24.0f, ghost_.pos.y }, { 850.0f, 0.0f }, 15.0f, accent, true, false);
        }
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
        if (isNightfallMode_) {
            SpawnProjectile({ ghost_.pos.x + 20.0f, ghost_.pos.y }, { 700.0f, 0.0f }, 11.0f, accent, true, true);
        }
    } else if (sub == LightSubclass::PRISMATIC_TRANSCENDENT) {
        // Prismatic Transcendence Surge: Freezes laser obstacles + phase surge!
        ghost_.phaseTimer = 0.70f;
        ghost_.stasisFreezeTimer = 2.0f;
        ghost_.vel.y = -220.0f;
        SpawnParticleBurst(ghost_.pos, 42, accent, 120.0f, 420.0f, 7.0f, 0.60f);
        if (isNightfallMode_) {
            SpawnProjectile({ ghost_.pos.x + 22.0f, ghost_.pos.y - 14.0f }, { 880.0f, -40.0f }, 14.0f, SKYBLUE, true, true);
            SpawnProjectile({ ghost_.pos.x + 22.0f, ghost_.pos.y + 14.0f }, { 880.0f,  40.0f }, 14.0f, { 255, 140, 40, 255 }, true, true);
        }
        flashColor_ = accent;
        flashAlpha_ = 0.40f;
    }
}

void GhostGame::TriggerSuperAbility() {
    if (ghost_.superEnergy < 100.0f || ghost_.superActiveTimer > 0.0f) return;

    profile_.totalSupersActivated++;
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
    ProgressDailyBounty(0, 1); // SURGE OPERATIVE

    LightSubclass sub = static_cast<LightSubclass>(profile_.selectedSubclass);
    SpawnParticleBurst(ghost_.pos, 60, GetSubclassAccentColor(), 120.0f, 520.0f, 8.0f, 0.8f);

    if (sub == LightSubclass::VOID_WALKER) {
        // Nova Bomb: Fires a concentrated heavy Void singularity projectile ahead that detonates on the nearest obstacle/enemy/boss
        // Clears a single focused hazard area rather than wiping the entire screen
        Color subCol = GetSubclassAccentColor();
        SpawnProjectile({ ghost_.pos.x + 24.0f, ghost_.pos.y }, { 680.0f, 0.0f }, 22.0f, subCol, true, true);
        for (auto& o : obstacles_) {
            if (o.active && o.x > ghost_.pos.x && o.x < ghost_.pos.x + 280.0f) {
                SpawnParticleBurst({ o.x + o.width * 0.5f, o.gapCenterY }, 25, subCol, 70.0f, 260.0f, 5.5f, 0.45f);
                SpawnCollectible({ o.x + o.width * 0.5f, o.gapCenterY }, CollectibleKind::MOTE_OF_LIGHT);
                o.active = false;
                scoreGates_++;
                profile_.totalGatesCleared++;
                break; // Only destroys the immediate obstacle ahead
            }
        }
        if (boss_.active) {
            boss_.health -= 28.0f;
            if (boss_.shieldImmune && boss_.health <= boss_.maxHealth * 0.65f) boss_.shieldImmune = false;
        }
    } else if (sub == LightSubclass::STASIS_SHADEBINDER || (!isNightfallMode_ && sub == LightSubclass::STRAND_RUNNER)) {
        // Winter's Wrath Shatter vaporizes active barriers into Motes of Light!
        for (auto& o : obstacles_) {
            if (o.active && o.x > 0.0f && o.x < VIRTUAL_WIDTH + 100.0f) {
                SpawnParticleBurst({ o.x + o.width * 0.5f, o.gapCenterY }, 35, GetSubclassAccentColor(), 90.0f, 350.0f, 7.0f, 0.6f);
                SpawnCollectible({ o.x + o.width * 0.5f, o.gapCenterY }, CollectibleKind::MOTE_OF_LIGHT);
                o.active = false;
                scoreGates_++;
                profile_.totalGatesCleared++;
            }
        }
        if (boss_.active) {
            boss_.health -= 45.0f;
            if (boss_.shieldImmune && boss_.health <= boss_.maxHealth * 0.65f) boss_.shieldImmune = false;
        }
    } else if (sub == LightSubclass::STRAND_RUNNER) {
        // Needlestorm: Unleashes 9 homing Strand Threadlings!
        for (int i = 0; i < 9; ++i) {
            float vy = (i - 4) * 65.0f;
            SpawnProjectile({ ghost_.pos.x + 20.0f, ghost_.pos.y },
                            { 640.0f, vy }, 10.0f, GetSubclassAccentColor(), true, true);
        }
    } else if (sub == LightSubclass::PRISMATIC_TRANSCENDENT) {
        // Transcendence: Full screen purge of barriers & vaporizes all enemies into Glimmer
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
        if (isNightfallMode_) {
            for (int i = 0; i < 12; ++i) {
                float vy = (i - 6) * 55.0f;
                SpawnProjectile({ ghost_.pos.x + 20.0f, ghost_.pos.y },
                                { 720.0f, vy }, 12.0f, (i % 2 == 0) ? SKYBLUE : Color{ 255, 120, 220, 255 }, true, true);
            }
        }
        if (boss_.active) {
            boss_.health -= 55.0f;
            if (boss_.shieldImmune && boss_.health <= boss_.maxHealth * 0.65f) boss_.shieldImmune = false;
        }
    }
}

void GhostGame::TriggerDroneFire() {
    if (!isNightfallMode_ || profile_.equippedDrone < 0 || profile_.equippedDrone >= static_cast<int>(NUM_DRONES)) {
        return;
    }
    if (ghost_.droneCooldown > 0.0f) return;

    const auto& drone = DRONE_CATALOG[profile_.equippedDrone];
    ghost_.droneMaxCooldown = drone.fireCooldown;
    ghost_.droneCooldown = drone.fireCooldown;
    ghost_.dronePulseAnim = 1.0f;
    audio_.PlayLaserShot();

    Vector2 dronePos = {
        ghost_.pos.x + std::cos(ghost_.companionAngle) * 42.0f,
        ghost_.pos.y + std::sin(ghost_.companionAngle) * 32.0f
    };
    SpawnParticleBurst(dronePos, 14, drone.color, 60.0f, 220.0f, 4.5f, 0.28f);

    if (profile_.equippedDrone == 0) {
        // Arc Soul Sentinel: 3-round burst of homing Arc bolts
        for (int i = -1; i <= 1; ++i) {
            SpawnProjectile({ dronePos.x + 10.0f, dronePos.y + i * 8.0f },
                            { 840.0f, i * 65.0f }, 9.0f, drone.color, true, true);
        }
    } else if (profile_.equippedDrone == 1) {
        // Hellion Solar Mortar: Twin high-damage Solar flares
        SpawnProjectile({ dronePos.x + 12.0f, dronePos.y - 10.0f }, { 780.0f, -35.0f }, 14.0f, drone.color, true, true);
        SpawnProjectile({ dronePos.x + 12.0f, dronePos.y + 10.0f }, { 780.0f,  35.0f }, 14.0f, GOLD, true, true);
    } else if (profile_.equippedDrone == 2) {
        // Briarbeinds Void Soul: 3 homing Void seeker orbs
        for (int i = -1; i <= 1; ++i) {
            SpawnProjectile({ dronePos.x + 10.0f, dronePos.y },
                            { 720.0f, i * 120.0f }, 11.0f, drone.color, true, true);
        }
    } else {
        // NTTE Chronal Portal: Rapid 4-bolt timeline volley
        for (int i = 0; i < 4; ++i) {
            float vy = (i - 1.5f) * 45.0f;
            SpawnProjectile({ dronePos.x + 12.0f + i * 8.0f, dronePos.y },
                            { 960.0f, vy }, 10.0f, drone.color, true, true);
        }
    }
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

    SetScreenState(ScreenState::LIGHT_FADED);
    screenShakeTrauma_ = 0.85f;
    flashColor_ = { 220, 40, 40, 255 };
    flashAlpha_ = 0.65f;
    audio_.PlayDeath();
    SpawnParticleBurst(ghost_.pos, 75, WHITE, 60.0f, 420.0f, 7.0f, 1.1f);

    profile_.totalDistanceFlown += scoreGates_;
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
    } else if (isNightfallMode_ && (biome == BiomeSector::VAULT_OF_GLASS || biome == BiomeSector::HELLMOUTH) && roll < 0.72f) {
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

    if (rng_.Chance(0.08f)) {
        SpawnCollectible({ midX, midY }, CollectibleKind::EXOTIC_ENGRAM);
    } else if (rng_.Chance(0.78f)) {
        SpawnCollectible({ midX, midY }, CollectibleKind::MOTE_OF_LIGHT);
        if (rng_.Chance(0.35f)) {
            SpawnCollectible({ slot->x + slot->width * 0.5f, slot->baseGapY }, CollectibleKind::MOTE_OF_LIGHT);
        }
    }

    // Spawn Combatant Enemies:
    // - Nightfall Strike: Frequent tactical combatants (65% chance)
    // - Vanguard Patrol: Rare easter eggs (only ~4% chance, harmless scenery/easter egg that can be phased through or vaporized by Super)
    bool spawnEnemy = isNightfallMode_ ? (scoreGates_ >= 2 && rng_.Chance(0.65f))
                                       : (scoreGates_ >= 8 && rng_.Chance(0.04f));
    if (spawnEnemy) {
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
            c.radius = (kind == CollectibleKind::EXOTIC_ENGRAM) ? 18.0f : 14.0f;
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
    float frameDt = GetFrameTime();
    if (touchFeedbackTac_ > 0.0f) touchFeedbackTac_ -= frameDt;
    if (touchFeedbackSup_ > 0.0f) touchFeedbackSup_ -= frameDt;
    if (touchFeedbackDrn_ > 0.0f) touchFeedbackDrn_ -= frameDt;
    if (touchFeedbackPause_ > 0.0f) touchFeedbackPause_ -= frameDt;

    if (IsKeyPressed(KEY_M)) {
        audio_.ToggleMute();
    }

    if (IsKeyPressed(KEY_F11) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
        ToggleFullscreen();
    }

    if (state_ == ScreenState::PLAYING) {
        // Music Jukebox Skip: N or Gamepad Right Stick Click
        if (IsKeyPressed(KEY_N) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_THUMB))) {
            int nextIdx = audio_.CycleNextActionTrack();
            activeToast_.title = "NOW PLAYING:";
            activeToast_.subtitle = audio_.GetActionTrackName(nextIdx);
            activeToast_.timer = 2.8f;
            activeToast_.active = true;
            audio_.PlayUiSelect();
        }

        // Collect new touch points & mouse click
        struct TouchEvent {
            Vector2 pos;
        };
        std::vector<TouchEvent> newPresses;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            newPresses.push_back({ GetVirtualMousePos() });
        }

        int tc = GetTouchPointCount();
        for (int i = 0; i < tc; ++i) {
            int tid = GetTouchPointId(i);
            bool isNew = true;
            for (int p = 0; p < prevTouchCount_; ++p) {
                if (prevTouchIds_[p] == tid) { isNew = false; break; }
            }
            if (isNew) {
                newPresses.push_back({ ToVirtualCoords(GetTouchPosition(i)) });
            }
        }

        // Gamepad Controller Integration (Player 0)
        bool hasGamepad = IsGamepadAvailable(0);
        if (hasGamepad) {
            activeInputDevice_ = GetGamepadName(0);
            if (activeInputDevice_.empty()) activeInputDevice_ = "GAMEPAD CONTROLLER";
        } else if (GetTouchPointCount() > 0) {
            activeInputDevice_ = "TOUCH SCREEN";
        } else if (GetKeyPressed() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            activeInputDevice_ = "KEYBOARD & MOUSE";
        }

        // On-screen HUD Touch Buttons & Hitboxes (with generous touch-padding)
        Rectangle pauseBtn = { VIRTUAL_WIDTH - 362.0f, 26.0f, 54.0f, 54.0f };
        Rectangle pauseHitbox = { pauseBtn.x - 14.0f, pauseBtn.y - 14.0f, pauseBtn.width + 28.0f, pauseBtn.height + 28.0f };

        float btnStartX = profile_.leftHandedControls ? 20.0f : (VIRTUAL_WIDTH - 378.0f);
        Rectangle drnBtn = { btnStartX, VIRTUAL_HEIGHT - 92.0f, 112.0f, 62.0f };
        Rectangle tacBtn = { btnStartX + 122.0f, VIRTUAL_HEIGHT - 92.0f, 112.0f, 62.0f };
        Rectangle supBtn = { btnStartX + 244.0f, VIRTUAL_HEIGHT - 92.0f, 112.0f, 62.0f };
        float pad = 16.0f;
        Rectangle drnHitbox = { drnBtn.x - pad, drnBtn.y - pad, drnBtn.width + pad * 2.0f, drnBtn.height + pad * 2.0f };
        Rectangle tacHitbox = { tacBtn.x - pad, tacBtn.y - pad, tacBtn.width + pad * 2.0f, tacBtn.height + pad * 2.0f };
        Rectangle supHitbox = { supBtn.x - pad, supBtn.y - pad, supBtn.width + pad * 2.0f, supBtn.height + pad * 2.0f };

        bool touchedPause = false;
        bool touchedFlap = false;

        for (const auto& ev : newPresses) {
            if (CheckCollisionPointRec(ev.pos, pauseHitbox)) {
                touchFeedbackPause_ = 0.25f;
                touchedPause = true;
                break;
            }
            if (isNightfallMode_ && CheckCollisionPointRec(ev.pos, drnHitbox)) {
                touchFeedbackDrn_ = 0.25f;
                TriggerDroneFire();
                continue;
            }
            if (CheckCollisionPointRec(ev.pos, tacHitbox)) {
                touchFeedbackTac_ = 0.25f;
                TriggerTacticalAbility();
                continue;
            }
            if (CheckCollisionPointRec(ev.pos, supHitbox)) {
                touchFeedbackSup_ = 0.25f;
                TriggerSuperAbility();
                continue;
            }
            touchedFlap = true;
        }

        bool padPause = hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT);
        if (touchedPause || padPause || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
            SetScreenState(ScreenState::PAUSED);
            audio_.PlayUiSelect();
            return;
        }

        bool padFlap = hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        if (touchedFlap || padFlap || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
            TriggerFlap();
            lastTapTime_ = globalTime_;
        }

        bool padTac = hasGamepad && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1));
        if (padTac || IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT) ||
            IsKeyPressed(KEY_E) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            TriggerTacticalAbility();
        }

        bool padSup = hasGamepad && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1));
        if (padSup || IsKeyPressed(KEY_F) || IsKeyPressed(KEY_Q)) {
            TriggerSuperAbility();
        }

        bool padDrn = hasGamepad && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2));
        if (isNightfallMode_ && (padDrn || IsKeyPressed(KEY_C) || IsKeyPressed(KEY_X))) {
            TriggerDroneFire();
        }
    } else if (state_ == ScreenState::SPLASH_INTRO) {
        splashTimer_ += GetFrameTime();
        bool hasGamepad = IsGamepadAvailable(0);
        bool padSkip = hasGamepad && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT));
        if (splashTimer_ >= 2.4f || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0 || GetKeyPressed() > 0 || padSkip) {
            SetScreenState(ScreenState::TITLE_FRONT_PAGE);
        }
    } else if (state_ == ScreenState::TITLE_FRONT_PAGE) {
        bool hasGamepad = IsGamepadAvailable(0);
        bool padStart = hasGamepad && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT));
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_F) || IsKeyPressed(KEY_E) || padStart) {
            SetScreenState(ScreenState::TITLE_DIRECTOR);
            audio_.PlayUiSelect();
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0) {
            Rectangle enterArea = { VIRTUAL_WIDTH / 2.0f - 260.0f, 540.0f, 520.0f, 60.0f };
            Vector2 p = (GetTouchPointCount() > 0) ? ToVirtualCoords(GetTouchPosition(0)) : GetVirtualMousePos();
            if (CheckCollisionPointRec(p, enterArea)) {
                SetScreenState(ScreenState::TITLE_DIRECTOR);
                audio_.PlayUiSelect();
            }
        }
    } else if (state_ == ScreenState::TITLE_DIRECTOR) {
        if (screenTransitionCooldown_ <= 0.0f) {
            bool hasGamepad = IsGamepadAvailable(0);
            // Launch Vanguard Patrol: Space, Enter, or Gamepad (A) / Start
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
                (hasGamepad && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)))) {
                StartNewRun(false);
            }
            // Launch Nightfall Strike: N or Gamepad (X)
            else if (IsKeyPressed(KEY_N) || (hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))) {
                if (HasUnlockedAnyDrone()) {
                    StartNewRun(true);
                } else {
                    towerTab_ = 2; // Open directly to Combat Drones tab in The Tower!
                    activeToast_.active = true;
                    activeToast_.title = "NIGHTFALL REQUIRES A COMBAT DRONE";
                    activeToast_.subtitle = "Unlock your first manual-fire Combat Drone in The Tower (200G)!";
                    activeToast_.timer = 4.0f;
                    SetScreenState(ScreenState::SHELL_HANGAR);
                    audio_.PlayUiSelect();
                }
            }
            // Subclass Armory: U or Gamepad (LB)
            else if (IsKeyPressed(KEY_U) || (hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1))) {
                SetScreenState(ScreenState::SUBCLASS_ARMORY);
                audio_.PlayUiSelect();
            }
            // The Tower: T or Gamepad (Y)
            else if (IsKeyPressed(KEY_T) || (hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP))) {
                SetScreenState(ScreenState::SHELL_HANGAR);
                audio_.PlayUiSelect();
            }
            // Triumphs Record: R or Gamepad (RB)
            else if (IsKeyPressed(KEY_R) || (hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1))) {
                SetScreenState(ScreenState::TRIUMPHS_LOG);
                audio_.PlayUiSelect();
            }
            // Career Stats: C
            else if (IsKeyPressed(KEY_C)) {
                SetScreenState(ScreenState::CAREER_STATS);
                audio_.PlayUiSelect();
            }
            // Settings: S or Gamepad Select/Back (MIDDLE_LEFT)
            else if (IsKeyPressed(KEY_S) || (hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT))) {
                settingsPrevState_ = ScreenState::TITLE_DIRECTOR;
                SetScreenState(ScreenState::SETTINGS_VIEW);
                audio_.PlayUiSelect();
            }
            // Back to Title: Escape or Gamepad (B)
            else if (IsKeyPressed(KEY_ESCAPE) || (hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
                SetScreenState(ScreenState::TITLE_FRONT_PAGE);
                audio_.PlayUiSelect();
            }
        }
    } else if (state_ == ScreenState::PAUSED) {
        bool hasGamepad = IsGamepadAvailable(0);
        bool padResume = hasGamepad && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
        bool padBack = hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE) || padResume) {
            SetScreenState(ScreenState::PLAYING);
            audio_.PlayUiSelect();
        } else if (padBack) {
            SaveData();
            SetScreenState(ScreenState::TITLE_DIRECTOR);
            audio_.PlayUiSelect();
        }
    } else if (state_ == ScreenState::LIGHT_FADED) {
        bool hasGamepad = IsGamepadAvailable(0);
        bool padRestart = hasGamepad && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT));
        bool padDirector = hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R) || padRestart) {
            StartNewRun(isNightfallMode_);
        } else if (IsKeyPressed(KEY_ESCAPE) || padDirector) {
            SetScreenState(ScreenState::TITLE_DIRECTOR);
            audio_.PlayUiSelect();
        }
    } else if (state_ != ScreenState::TITLE_DIRECTOR && state_ != ScreenState::TITLE_FRONT_PAGE) {
        bool hasGamepad = IsGamepadAvailable(0);
        bool padBack = hasGamepad && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        if (IsKeyPressed(KEY_ESCAPE) || padBack) {
            if (state_ == ScreenState::SETTINGS_VIEW) {
                SetScreenState(settingsPrevState_);
            } else {
                SetScreenState(ScreenState::TITLE_DIRECTOR);
            }
            audio_.PlayUiSelect();
        }
    }

    // Store active touch IDs for frame transition detection
    int curTc = GetTouchPointCount();
    prevTouchCount_ = std::min(curTc, static_cast<int>(prevTouchIds_.size()));
    for (int i = 0; i < prevTouchCount_; ++i) {
        prevTouchIds_[i] = GetTouchPointId(i);
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
    if (ghost_.droneCooldown > 0.0f) ghost_.droneCooldown = std::max(0.0f, ghost_.droneCooldown - dt);
    if (ghost_.dronePulseAnim > 0.0f) ghost_.dronePulseAnim = std::max(0.0f, ghost_.dronePulseAnim - 4.5f * dt);

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
            if (boss_.active) {
                boss_.health -= 26.0f * dt;
                if (boss_.shieldImmune && boss_.health <= boss_.maxHealth * 0.65f) {
                    boss_.shieldImmune = false;
                }
            }
        } else if (isNightfallMode_ && (sub == LightSubclass::SOLAR_DAWN || sub == LightSubclass::STRAND_RUNNER) && std::fmod(runTime_, 0.22f) < dt) {
            SpawnProjectile({ ghost_.pos.x + 22.0f, ghost_.pos.y },
                            { 760.0f, rng_.RangeFloat(-80.0f, 80.0f) }, 11.0f, GetSubclassAccentColor(), true, true);
        }
    } else {
        float trickle = 1.4f + profile_.upgrades.superEfficiencyLevel * 0.25f;
        ghost_.superEnergy = std::min(100.0f, ghost_.superEnergy + trickle * dt);
    }

    // Ghost Physics
    float gravity = 1180.0f;
    bool holdGlide = IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0 ||
                     (IsGamepadAvailable(0) && IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
    if (static_cast<LightSubclass>(profile_.selectedSubclass) == LightSubclass::SOLAR_DAWN &&
        ghost_.vel.y > 40.0f && holdGlide) {
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
            ProgressDailyBounty(1, 1); // QUANTUM NAVIGATOR
            if (streakGates_ >= 5) ProgressDailyBounty(7, 1); // VANGUARD INITIATE
            runGlimmerEarned_ += isNightfallMode_ ? 2 : 1;
            profile_.glimmer += isNightfallMode_ ? 2 : 1;

            if (scoreGates_ >= 10) UnlockTriumph(0);
            if (scoreGates_ >= 60) UnlockTriumph(12); // GRANDMASTER ASCENT
            if (runGlimmerEarned_ >= 100) UnlockTriumph(11); // LIGHT HARVESTER
            if (profile_.totalGlimmerEarnedLifetime >= 3000) UnlockTriumph(14); // THE TRAVELER'S CHOSEN

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

            if (isNightfallMode_ && !boss_.active && gatesSinceLastBoss_ >= 20) {
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
                ProgressDailyBounty(2, 1); // MOTE SYNTHESIS
                int glim = isNightfallMode_ ? 3 : 2;
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
            }
        }

        if (c.pos.x < -80.0f) {
            c.active = false;
        }
    }

    for (auto& pr : projectiles_) {
        if (!pr.active) continue;

        // Homing Drone / Wolfpack / Strand Threadling steering
        if (pr.friendly && pr.homing) {
            Vector2 bestTarget = { pr.pos.x + 300.0f, pr.pos.y };
            float bestDist = 99999.0f;
            for (const auto& e : enemies_) {
                if (e.active && e.pos.x > pr.pos.x - 20.0f) {
                    float d = Vector2Distance(pr.pos, e.pos);
                    if (d < bestDist) { bestDist = d; bestTarget = e.pos; }
                }
            }
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
                Vector2 desired = Vector2Scale(Vector2Normalize(Vector2Subtract(bestTarget, pr.pos)), 780.0f);
                pr.vel = Vector2Lerp(pr.vel, desired, 9.5f * dt);
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

            // Friendly projectile collision with Killable Nightfall Enemies!
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
                boss_.health -= 16.0f;
                SpawnParticleBurst(pr.pos, 20, pr.color, 70.0f, 240.0f, 5.0f, 0.35f);
                if (boss_.shieldImmune && boss_.health <= boss_.maxHealth * 0.65f) {
                    boss_.shieldImmune = false;
                    audio_.PlayShieldBreak();
                    screenShakeTrauma_ = 0.45f;
                    TriggerVanguardMedal("BOSS SHIELD SHATTERED!", 25, SKYBLUE);
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

    // Update Nightfall Enemies & collision with Ghost
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

    // Orbit Combat Drone in Nightfall mode (manual-fire via [C / X] or on-screen [FIRE DRONE] button)
    if (isNightfallMode_ && profile_.equippedDrone >= 0) {
        ghost_.companionAngle += 3.2f * dt;
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

    if (ghost_.stasisFreezeTimer <= 0.0f) {
        boss_.attackTimer -= dt;
        if (boss_.attackTimer <= 0.0f) {
            boss_.attackTimer = 1.95f;
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
    if (screenTransitionCooldown_ > 0.0f) {
        screenTransitionCooldown_ = std::max(0.0f, screenTransitionCooldown_ - frameDt);
    }

    UpdateFrameInput();

    int musicMode = (state_ == ScreenState::SPLASH_INTRO || state_ == ScreenState::TITLE_FRONT_PAGE) ? 0 :
                    (state_ == ScreenState::PLAYING || state_ == ScreenState::PAUSED || state_ == ScreenState::LIGHT_FADED) ? 2 : 1;
    audio_.UpdateMusic(musicMode);

    while (accumulator_ >= FIXED_DT) {
        UpdateFixed(FIXED_DT);
        accumulator_ -= FIXED_DT;
    }

    RenderFrame();
}

} // namespace FlappyGhost
