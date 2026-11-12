#pragma once
#include "Types.hpp"
#include "DeterministicRNG.hpp"
#include "SoundSynth.hpp"
#include <array>
#include <string>

namespace FlappyGhost {

class GhostGame {
public:
    GhostGame();
    ~GhostGame();

    void Init();
    void RunFrame();
    void Shutdown();

    static const std::array<GhostShellDef, NUM_SHELLS>& GetShellCatalog();
    static const std::array<ExoticPerkDef, NUM_EXOTIC_PERKS>& GetExoticCatalog();
    static const std::array<TriumphDef, NUM_TRIUMPHS>& GetTriumphCatalog();
    static const std::array<ShaderDef, NUM_SHADERS>& GetShaderCatalog();
    static const std::array<CombatDroneDef, NUM_DRONES>& GetDroneCatalog();

    float GetVirtualWidth() const { return virtualWidth_; }
    float GetVirtualHeight() const { return 720.0f; }
    void UpdateVirtualResolution();

private:
    float virtualWidth_ = 1280.0f;
    // Persistence
    void LoadSaveData();
    void SaveData() const;
    void UnlockTriumph(int triumphIdx);
    void RefreshDailyBounties();
    void ProgressDailyBounty(int bountyType, int count = 1);
    void TriggerVanguardMedal(const char* medalName, int glimmerBonus, Color col = GOLD);
    bool HasUnlockedAnyDrone() const;

    // Run lifecycle
    void StartNewRun(bool dailyNightfall);
    void TriggerFlap();
    void TriggerTacticalAbility();
    void TriggerSuperAbility();
    void TriggerDroneFire();
    void HandleFatalImpact();

    // Simulation (Fixed Timestep)
    void UpdateFrameInput();
    void UpdateFixed(float dt);
    void UpdatePlayingFixed(float dt);
    void UpdateBossFixed(float dt);
    void UpdateParticlesFixed(float dt);

    // Procedural generation
    void SpawnNextObstacle();
    void SpawnCollectible(Vector2 pos, CollectibleKind kind);
    void SpawnPatrolEnemy(Vector2 pos, EnemyType type);
    void SpawnBossEncounter();
    void SpawnParticleBurst(Vector2 pos, int count, Color color, float speedMin, float speedMax, float size, float life);
    void SpawnProjectile(Vector2 pos, Vector2 vel, float radius, Color color, bool friendly, bool homing = false,
                         float damage = 16.0f, bool stripShield = false, bool isNovaBomb = false);

    // Helpers
    BiomeSector GetCurrentBiome() const;
    Color GetSubclassAccentColor() const;
    const char* GetBiomeName(BiomeSector b) const;
    Vector2 GetVirtualMousePos() const;
    Vector2 ToVirtualCoords(Vector2 screenPos) const;
    bool IsButtonClicked(Rectangle rect) const;
    bool IsRectHovered(Rectangle rect) const;

    // Rendering (implemented in GhostRenderer.cpp)
    void RenderFrame();
    void RenderStarfieldAndBackdrop() const;
    void RenderBiomeEnvironment() const;
    void RenderObstacles() const;
    void RenderCollectibles() const;
    void RenderPatrolEnemies() const;
    void RenderBoss() const;
    void RenderProjectiles() const;
    void RenderParticles() const;
    void RenderGhostRig(Vector2 pos, float radius, float rotDeg, float expand, float spinAngle, Vector2 eyeOffset, int shellIdx, float alpha = 1.0f) const;
    void RenderHud() const;
    void RenderSplashIntro();
    void RenderTitleFrontPage();
    void RenderTitleDirector();
    void RenderSubclassArmory();
    void RenderShellHangar();
    void RenderTriumphsLog();
    void RenderSettingsView();
    void RenderCareerStats();
    void RenderPauseOverlay();
    void RenderLightFadedOverlay();
    void RenderBottomBrandingFooter() const;
    void RenderTriumphToast() const;
    void SetScreenState(ScreenState nextState);

private:
    RenderTexture2D targetRenderTex_{};
    SoundSynth audio_{};
    SaveProfile profile_{};
    PCG32 rng_{};

    ScreenState state_ = ScreenState::SPLASH_INTRO;
    float screenTransitionCooldown_ = 0.0f;
    float splashTimer_ = 0.0f;
    Texture2D splashLogo_{};
    std::array<DailyBounty, NUM_DAILY_BOUNTIES> dailyBounties_{};
    int activeBountyDateSeed_ = 0;
    int towerTab_ = 0; // 0 = Shells I, 1 = Shells II, 2 = Shaders, 3 = Xur Exotic Curios
    bool isNightfallMode_ = false;
    uint64_t activeSeed_ = 0;

    float accumulator_ = 0.0f;
    float globalTime_ = 0.0f;
    float runTime_ = 0.0f;
    float distanceMeters_ = 0.0f;
    float scrollSpeed_ = 310.0f;
    float nextObstacleSpawnX_ = 0.0f;
    int scoreGates_ = 0;
    int streakGates_ = 0;
    int runMotes_ = 0;
    int runGlimmerEarned_ = 0;
    int gatesSinceLastBoss_ = 0;

    // Screen juice
    float screenShakeTrauma_ = 0.0f;
    float flashAlpha_ = 0.0f;
    Color flashColor_ = WHITE;
    float biomeBannerTimer_ = 0.0f;
    BiomeSector lastBiome_ = BiomeSector::COSMODROME;
    std::array<BiomeSector, NUM_BIOMES> biomeOrder_{};

    float lastTapTime_ = -10.0f;

    // Mobile Multi-Touch & Input Feedback
    mutable std::array<int, 10> prevTouchIds_{};
    mutable int prevTouchCount_ = 0;
    float touchFeedbackTac_ = 0.0f;
    float touchFeedbackSup_ = 0.0f;
    float touchFeedbackDrn_ = 0.0f;
    float touchFeedbackPause_ = 0.0f;
    // Settings state & confirmation modal
    bool showResetConfirm_ = false;
    ScreenState settingsPrevState_ = ScreenState::TITLE_DIRECTOR;
    std::string activeInputDevice_ = "KEYBOARD & MOUSE";
    float shaderScrollY_ = 0.0f;
    float triumphScrollY_ = 0.0f;
    bool listDragging_ = false;
    bool listDragMoved_ = false;
    Vector2 listDragLastPos_{ 0.0f, 0.0f };
    float listDragTotalDy_ = 0.0f;

    GhostPlayer ghost_{};
    BossState boss_{};
    TriumphToast activeToast_{};
    VanguardMedalBanner activeMedal_{};

    // Pre-allocated Object Pools
    std::array<Obstacle, MAX_OBSTACLES> obstacles_{};
    std::array<Collectible, MAX_MOTES> collectibles_{};
    std::array<PatrolEnemy, MAX_PATROL_ENEMIES> enemies_{};
    std::array<Particle, MAX_PARTICLES> particles_{};
    std::array<Projectile, MAX_PROJECTILES> projectiles_{};
    std::array<StarBg, MAX_STARS> stars_{};
};

} // namespace FlappyGhost
