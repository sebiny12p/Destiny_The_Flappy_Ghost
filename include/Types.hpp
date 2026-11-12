#pragma once
#include "raylib.h"
#include <array>
#include <cstdint>
#include <string>

namespace FlappyGhost {

constexpr int VIRTUAL_WIDTH = 1280;
constexpr int VIRTUAL_HEIGHT = 720;
constexpr float FIXED_DT = 1.0f / 120.0f;

constexpr size_t MAX_OBSTACLES = 32;
constexpr size_t MAX_MOTES = 64;
constexpr size_t MAX_PARTICLES = 1024;
constexpr size_t MAX_PROJECTILES = 64;
constexpr size_t MAX_STARS = 180;
constexpr size_t NUM_SHELLS = 12;
constexpr size_t NUM_SUBCLASSES = 6;
constexpr size_t NUM_EXOTIC_PERKS = 4;
constexpr size_t NUM_TRIUMPHS = 16;
constexpr size_t MAX_PATROL_ENEMIES = 16;
constexpr size_t NUM_SHADERS = 12;
constexpr size_t NUM_DRONES = 4;
constexpr size_t NUM_DAILY_BOUNTIES = 3;

struct DailyBounty {
    int id = 0;
    const char* title = "";
    const char* description = "";
    int targetCount = 10;
    int currentProgress = 0;
    int rewardGlimmer = 60;
    bool completed = false;
    bool claimed = false;
};

enum class ScreenState {
    SPLASH_INTRO,      // Sebiny Labs Logo Flash Intro
    TITLE_FRONT_PAGE,  // "Press Any Key / Tap to Enter Destiny" Title Page
    TITLE_DIRECTOR,    // Vanguard Director Navigation
    SUBCLASS_ARMORY,
    SHELL_HANGAR,
    TRIUMPHS_LOG,
    SETTINGS_VIEW,
    CAREER_STATS,      // Vanguard Career Dossier & Lifetime Flight Telemetry
    PLAYING,
    PAUSED,
    LIGHT_FADED // Game Over screen
};

enum class LightSubclass : int {
    VOID_WALKER = 0,        // Blink Teleport + Nova Pulse shockwave Super
    SOLAR_DAWN = 1,         // Icarus Hover Glide + Daybreak Solar Aura Super
    ARC_CALLER = 2,         // Ionic Chain Dash + Chaos Reach Laser Beam Super
    STASIS_SHADEBINDER = 3, // Frost Pulse Freeze + Winter's Wrath Shatter Super
    STRAND_RUNNER = 4,      // Grapple Swing + Woven Mail + Needlestorm Threadlings Super
    PRISMATIC_TRANSCENDENT = 5 // Transcendence: Combined Light & Darkness dual blast + Turret Buddy
};

enum class EnemyType : int {
    FALLEN_SHANK = 0,       // Cosmodrome hovering drone
    VEX_SUPPLICANT = 1,     // Vault of Glass harpy unit
    HIVE_THRALL = 2,        // Hellmouth leaping shadow beast
    TAKEN_PSION = 3         // Ascendant realm multiplying anomaly
};

struct PatrolEnemy {
    bool active = false;
    EnemyType type = EnemyType::FALLEN_SHANK;
    Vector2 pos = { 0.0f, 0.0f };
    Vector2 vel = { 0.0f, 0.0f };
    float health = 1.0f;
    float maxHealth = 1.0f;
    float radius = 18.0f;
    float bobTimer = 0.0f;
    Color color = RED;
};

enum class ExoticPerk : int {
    NONE = 0,
    GJALLARHORN_WOLFPACK = 1, // Firing abilities spawns homing Wolfpack micro-rockets
    CRIMSON_REDRIX = 2,       // Collecting 10 Motes regenerates +1 Overshield charge
    RISKRUNNER_CONDUCTOR = 3  // Near-misses on barriers charge +15% Super energy & chain lightning
};

enum class BiomeSector : int {
    COSMODROME = 0,             // Fallen territory: rusted golden-age steel, tripmines
    VAULT_OF_GLASS = 1,         // Vex network: radiolarian fluid, laser barriers, Oracles
    HELLMOUTH = 2,              // Hive catacombs: gothic bone pillars, soulfire traps
    ASCENDANT_PLANE = 3,        // Taken/Pyramid realm: gravitational blights, resonance beams
    DREAMING_CITY = 4,          // Awoken twilight citadel: starlight amethyst & gilded spires
    EUROPA_BRAYTECH = 5,        // BrayTech exoscience: glacial ice-blue & arctic cobalt
    SAVATHUNS_THRONE_WORLD = 6, // Lucent Hive: alabaster bone & luminous swamp teal
    PALE_HEART = 7              // Inside The Traveler: prismatic gold, rose & celestial cyan
};
inline constexpr std::size_t NUM_BIOMES = 8;

enum class ObstacleKind : int {
    STANDARD_GATE = 0,      // Upper & lower biome pillars
    MOVING_VEX_LASER = 1,   // Oscillating gap + pulsing laser emitter
    ROTATING_ORACLE = 2,    // Central hazard inside a wide gap that can be scanned/blasted
    TAKEN_BLIGHT_WELL = 3   // Gravitational anomaly that pulls/warps the Ghost trajectory
};

enum class CollectibleKind : int {
    MOTE_OF_LIGHT = 0,      // Charges Super & gives Glimmer
    EXOTIC_ENGRAM = 1       // Golden dodecahedron: +50 Glimmer, instant Overshield & Super boost
};

struct GhostShellDef {
    int id;
    const char* name;
    const char* loreSubtitle;
    int glimmerCost;
    Color primaryColor;
    Color secondaryColor;
    Color coreColor;
    Color trailColor;
    int finShape;
};

struct ExoticPerkDef {
    int id;
    const char* name;
    const char* description;
    int glimmerCost;
    Color accentColor;
};

struct UpgradeLevels {
    int lightMagnetLevel = 0;    // Increases Mote pickup radius (0..5)
    int superEfficiencyLevel = 0;// Increases Super charge per Mote & duration (0..5)
    int phaseCooldownLevel = 0;  // Reduces Blink/Dash tactical cooldown (0..5)
    int overshieldCapLevel = 0;  // Starts run with Light Overshield charge (0..3)
};

struct TriumphDef {
    int id;
    const char* title;
    const char* description;
    int rewardGlimmer;
};

struct ShaderDef {
    int id;
    const char* name;
    int glimmerCost;
    Color primary;
    Color secondary;
    Color core;
};

struct CombatDroneDef {
    int id;
    const char* name;
    const char* description;
    int glimmerCost;
    float fireCooldown;
    Color color;
};

struct SaveProfile {
    uint32_t magic = 0x47485358; // 'GHSX' v5
    int glimmer = 60;
    int bestScoreStandard = 0;
    int bestScoreNightfall = 0;
    int totalMotesCollected = 0;
    int totalBossesDefeated = 0;
    int totalGatesCleared = 0;
    int equippedShell = 0;
    int selectedSubclass = 0;
    int equippedExoticPerk = 0;
    int equippedShader = 0;
    int equippedDrone = -1; // -1 = None unlocked yet; unlocking first drone unlocks Nightfall mode
    std::array<bool, NUM_SHELLS> unlockedShells = {
        true, false, false, false, false, false,
        false, false, false, false, false, false
    };
    std::array<bool, NUM_SUBCLASSES> unlockedSubclasses = {
        true, true, true, false, false, false // Darkness = 500G, Prismatic = 1500G
    };
    std::array<bool, NUM_SHADERS> unlockedShaders = {
        true, false, false, false, false, false,
        false, false, false, false, false, false
    };
    std::array<bool, NUM_DRONES> unlockedDrones = {
        false, false, false, false
    };
    std::array<bool, NUM_EXOTIC_PERKS> unlockedExotics = { true, false, false, false };
    std::array<bool, NUM_TRIUMPHS> unlockedTriumphs = {
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false
    };
    UpgradeLevels upgrades{};

    // User Settings & Accessibility (Persisted)
    float masterVolume = 1.0f;
    float musicVolume = 0.55f;
    float sfxVolume = 0.80f;
    float screenShakeScale = 1.0f; // 0.0f = Off, 0.5f = 50%, 1.0f = 100%
    bool screenFlashes = true;
    int targetFps = 120; // 30, 60, 120
    int touchOpacity = 2; // 0 = 40%, 1 = 70%, 2 = 100%
    bool leftHandedControls = false;
    int musicTrackChoice = 0; // 0 = Auto-Shuffle, 1 = Last Array, 2 = Sepiks, 3 = Shadowkeep, 4 = Deep Stone Crypt

    // Lifetime Career Telemetry
    int totalRunsStarted = 0;
    int totalDistanceFlown = 0;
    int totalGlimmerEarnedLifetime = 60;
    int totalSupersActivated = 0;
    int totalBlinkTeleports = 0;
    int totalBountiesCompleted = 0;
};

struct GhostPlayer {
    Vector2 pos = { 280.0f, 360.0f };
    Vector2 vel = { 0.0f, 0.0f };
    float radius = 20.0f;
    float rotation = 0.0f;
    float shellExpand = 0.0f;
    float shellSpinAngle = 0.0f;
    Vector2 eyeLookOffset = { 4.0f, 0.0f };

    // Tactical ability
    float tacticalCooldown = 0.0f;
    float tacticalMaxCooldown = 3.2f;
    float phaseTimer = 0.0f;

    // Strand Grapple Anchor
    float grappleTimer = 0.0f;
    Vector2 grappleAnchor = { 0.0f, 0.0f };

    // Stasis Freeze field timer
    float stasisFreezeTimer = 0.0f;

    // Super ability (Nightfall Strike only)
    float superEnergy = 0.0f;
    float superActiveTimer = 0.0f;
    float superPulseCooldown = 0.0f;

    // Overshield
    int overshieldCharges = 0;
    float invulnerableTimer = 0.0f;

    // Manual Combat Drone (Nightfall Mode only)
    float companionAngle = 0.0f;
    float droneCooldown = 0.0f;
    float droneMaxCooldown = 0.45f;
    float dronePulseAnim = 0.0f;
};

struct Obstacle {
    bool active = false;
    bool scored = false;
    bool nearMissAwarded = false;
    ObstacleKind kind = ObstacleKind::STANDARD_GATE;
    BiomeSector biome = BiomeSector::COSMODROME;
    float x = 0.0f;
    float width = 76.0f;
    float gapCenterY = 360.0f;
    float gapHeight = 195.0f;
    float baseGapY = 360.0f;
    float moveSpeedY = 0.0f;
    float movePhase = 0.0f;
    float hazardRadius = 28.0f;
    float oracleHealth = 1.0f;
    bool oracleDestroyed = false;
    bool frozenByStasis = false;
};

struct Collectible {
    bool active = false;
    CollectibleKind kind = CollectibleKind::MOTE_OF_LIGHT;
    Vector2 pos = { 0.0f, 0.0f };
    Vector2 vel = { 0.0f, 0.0f };
    float radius = 14.0f;
    float bobPhase = 0.0f;
};

struct Projectile {
    bool active = false;
    bool friendly = false;
    bool homing = false;
    bool stripShield = false;
    bool isNovaBomb = false;
    float damage = 16.0f;
    Vector2 pos = { 0.0f, 0.0f };
    Vector2 vel = { 0.0f, 0.0f };
    float radius = 8.0f;
    float life = 3.0f;
    Color color = PURPLE;
};

struct Particle {
    bool active = false;
    Vector2 pos = { 0.0f, 0.0f };
    Vector2 vel = { 0.0f, 0.0f };
    float size = 4.0f;
    float life = 0.0f;
    float maxLife = 1.0f;
    Color color = WHITE;
    bool additive = true;
};

struct StarBg {
    Vector2 pos;
    float depth;
    float size;
    Color color;
};

struct BossState {
    bool active = false;
    int bossType = 0;
    std::string name = "SEPIKS PRIME, REBORN";
    Vector2 pos = { 1080.0f, 360.0f };
    float targetY = 360.0f;
    float health = 100.0f;
    float maxHealth = 100.0f;
    float attackTimer = 0.0f;
    float phaseAngle = 0.0f;
    int confluxesScanned = 0;
    bool shieldImmune = true;
};

struct TriumphToast {
    bool active = false;
    std::string title;
    std::string subtitle;
    float timer = 0.0f;
};

struct VanguardMedalBanner {
    bool active = false;
    std::string medalName;
    int bonusGlimmer = 0;
    float timer = 0.0f;
    Color color = GOLD;
};

} // namespace FlappyGhost
