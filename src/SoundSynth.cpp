#include "SoundSynth.hpp"
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace FlappyGhost {

namespace {
constexpr int SAMPLE_RATE = 44100;
constexpr float TWO_PI = 6.28318530718f;

template <typename Fn>
Sound BuildSynthSound(float durationSec, Fn&& sampleGenerator) {
    int frameCount = static_cast<int>(durationSec * SAMPLE_RATE);
    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(frameCount);
    wave.sampleRate = SAMPLE_RATE;
    wave.sampleSize = 16;
    wave.channels = 1;

    short* data = static_cast<short*>(RL_MALLOC(sizeof(short) * frameCount));
    for (int i = 0; i < frameCount; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE);
        float progress = static_cast<float>(i) / static_cast<float>(frameCount);
        float s = sampleGenerator(t, progress);
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        data[i] = static_cast<short>(s * 28000.0f);
    }
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}
} // namespace

void SoundSynth::Init() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        initialized_ = false;
        return;
    }
    initialized_ = true;

    // 1. Ghost Flap: Subtle crystalline upward chirp + soft sub pulse
    sndFlap_ = BuildSynthSound(0.11f, [](float t, float p) {
        float env = (1.0f - p) * (1.0f - p);
        float freq = 340.0f + 420.0f * p;
        return 0.35f * env * (0.7f * std::sin(TWO_PI * freq * t) + 0.3f * std::sin(TWO_PI * freq * 2.0f * t));
    });

    // 2. Blink / Phase Dash: Void warp sweep down then up with modulation
    sndBlink_ = BuildSynthSound(0.28f, [](float t, float p) {
        float env = std::sin(p * 3.14159f);
        float freq = 680.0f - 460.0f * std::sin(p * 3.14159f) + 220.0f * p;
        float mod = std::sin(TWO_PI * 45.0f * t);
        return 0.45f * env * std::sin(TWO_PI * freq * t + 2.5f * mod);
    });

    // 3. Mote of Light Pickup: Harmonic crystalline fifth chime
    sndMote_ = BuildSynthSound(0.22f, [](float t, float p) {
        float env = (1.0f - p);
        float f1 = (p < 0.35f) ? 880.0f : 1318.5f;
        float f2 = f1 * 1.5f;
        return 0.38f * env * (0.65f * std::sin(TWO_PI * f1 * t) + 0.35f * std::sin(TWO_PI * f2 * t));
    });

    // 4. Exotic Engram Pickup: Rich major triad shimmer chord
    sndExotic_ = BuildSynthSound(0.55f, [](float t, float p) {
        float env = (1.0f - p) * std::min(1.0f, p * 12.0f);
        float c1 = std::sin(TWO_PI * 587.33f * t);
        float c2 = std::sin(TWO_PI * 739.99f * t);
        float c3 = std::sin(TWO_PI * 880.0f * t);
        float c4 = std::sin(TWO_PI * 1174.66f * t);
        return 0.45f * env * 0.25f * (c1 + c2 + c3 + c4);
    });

    // 5. Super Activation: Massive resonant Light bass drop + high harmonic ring
    sndSuper_ = BuildSynthSound(0.65f, [](float t, float p) {
        float env = (1.0f - p * 0.85f);
        float subFreq = 160.0f - 95.0f * p;
        float highFreq = 660.0f + 330.0f * std::sin(p * 3.14159f);
        float s = 0.55f * std::sin(TWO_PI * subFreq * t) +
                  0.30f * std::sin(TWO_PI * highFreq * t) +
                  0.15f * std::sin(TWO_PI * highFreq * 1.5f * t);
        return 0.55f * env * s;
    });

    // 6. Overshield Break: Glass shatter + energy discharge
    sndShieldBreak_ = BuildSynthSound(0.32f, [](float t, float p) {
        float env = (1.0f - p);
        float freq = 920.0f - 650.0f * p;
        float noise = (static_cast<float>(std::rand() % 2000) / 1000.0f - 1.0f);
        return 0.45f * env * (0.6f * std::sin(TWO_PI * freq * t) + 0.4f * noise);
    });

    // 7. Light Faded (Death): Somber minor chord descent
    sndDeath_ = BuildSynthSound(0.75f, [](float t, float p) {
        float env = (1.0f - p);
        float slide = 1.0f - 0.18f * p;
        float c1 = std::sin(TWO_PI * 220.0f * slide * t);
        float c2 = std::sin(TWO_PI * 261.63f * slide * t);
        float c3 = std::sin(TWO_PI * 329.63f * slide * t);
        return 0.5f * env * 0.33f * (c1 + c2 + c3);
    });

    // 8. Boss Warning Alert: Deep Vex/Fallen horn pulse
    sndBossAlert_ = BuildSynthSound(0.50f, [](float t, float p) {
        float env = std::sin(p * 3.14159f);
        float freq = 145.0f + 15.0f * std::sin(TWO_PI * 18.0f * t);
        float saw = 2.0f * (t * freq - std::floor(0.5f + t * freq));
        return 0.40f * env * (0.6f * saw + 0.4f * std::sin(TWO_PI * freq * 2.0f * t));
    });

    // 9. Laser / Oracle Blast
    sndLaser_ = BuildSynthSound(0.15f, [](float t, float p) {
        float env = 1.0f - p;
        float freq = 1100.0f - 820.0f * p;
        return 0.32f * env * std::sin(TWO_PI * freq * t);
    });

    // 10. Destiny UI Click
    sndUi_ = BuildSynthSound(0.06f, [](float t, float p) {
        float env = (1.0f - p) * (1.0f - p);
        return 0.30f * env * std::sin(TWO_PI * 1480.0f * t);
    });

    // Load Official Destiny Soundtrack MP3 Streams (View from Orbit + What We Fight For)
    auto TryLoadTrack = [](const char* fileName, Music& outMusic) -> bool {
        const char* appDir = GetApplicationDirectory();
        const std::string candidates[4] = {
            std::string("assets/") + fileName,
            std::string(appDir ? appDir : "") + "assets/" + fileName,
            std::string(appDir ? appDir : "") + "../assets/" + fileName,
            std::string(fileName)
        };
        for (const auto& path : candidates) {
            Music m = LoadMusicStream(path.c_str());
            if (m.stream.buffer != nullptr) {
                m.looping = true;
                ::SetMusicVolume(m, 0.55f);
                outMusic = m;
                return true;
            }
        }
        return false;
    };

    hasTitleMusic_   = TryLoadTrack("music_title.mp3",   bgmTitle_);
    hasOrbitMusic_   = TryLoadTrack("music_orbit.mp3",   bgmOrbit_);
    hasActionMusic1_ = TryLoadTrack("music_action.mp3",  bgmAction1_);
    hasActionMusic2_ = TryLoadTrack("music_action2.mp3", bgmAction2_);

    ApplyVolumes();
}

void SoundSynth::ApplyVolumes() {
    if (!initialized_) return;
    float effectiveMusic = muted_ ? 0.0f : (masterVol_ * musicVol_);
    float effectiveSfx = muted_ ? 0.0f : (masterVol_ * sfxVol_);

    if (hasTitleMusic_)   ::SetMusicVolume(bgmTitle_, effectiveMusic);
    if (hasOrbitMusic_)   ::SetMusicVolume(bgmOrbit_, effectiveMusic);
    if (hasActionMusic1_) ::SetMusicVolume(bgmAction1_, effectiveMusic);
    if (hasActionMusic2_) ::SetMusicVolume(bgmAction2_, effectiveMusic);

    SetSoundVolume(sndFlap_, effectiveSfx * 0.90f);
    SetSoundVolume(sndBlink_, effectiveSfx * 0.85f);
    SetSoundVolume(sndMote_, effectiveSfx * 0.80f);
    SetSoundVolume(sndExotic_, effectiveSfx * 0.95f);
    SetSoundVolume(sndSuper_, effectiveSfx * 1.0f);
    SetSoundVolume(sndShieldBreak_, effectiveSfx * 0.90f);
    SetSoundVolume(sndDeath_, effectiveSfx * 0.95f);
    SetSoundVolume(sndBossAlert_, effectiveSfx * 0.90f);
    SetSoundVolume(sndLaser_, effectiveSfx * 0.85f);
    SetSoundVolume(sndUi_, effectiveSfx * 0.80f);
}

void SoundSynth::Shutdown() {
    if (!initialized_) return;
    if (hasTitleMusic_)   UnloadMusicStream(bgmTitle_);
    if (hasOrbitMusic_)   UnloadMusicStream(bgmOrbit_);
    if (hasActionMusic1_) UnloadMusicStream(bgmAction1_);
    if (hasActionMusic2_) UnloadMusicStream(bgmAction2_);
    UnloadSound(sndFlap_);
    UnloadSound(sndBlink_);
    UnloadSound(sndMote_);
    UnloadSound(sndExotic_);
    UnloadSound(sndSuper_);
    UnloadSound(sndShieldBreak_);
    UnloadSound(sndDeath_);
    UnloadSound(sndBossAlert_);
    UnloadSound(sndLaser_);
    UnloadSound(sndUi_);
    CloseAudioDevice();
    initialized_ = false;
}

void SoundSynth::UpdateMusic(int musicMode) {
    if (!initialized_) return;
    if (muted_) {
        if (hasTitleMusic_   && IsMusicStreamPlaying(bgmTitle_))   PauseMusicStream(bgmTitle_);
        if (hasOrbitMusic_   && IsMusicStreamPlaying(bgmOrbit_))   PauseMusicStream(bgmOrbit_);
        if (hasActionMusic1_ && IsMusicStreamPlaying(bgmAction1_)) PauseMusicStream(bgmAction1_);
        if (hasActionMusic2_ && IsMusicStreamPlaying(bgmAction2_)) PauseMusicStream(bgmAction2_);
        return;
    }

    // Music mode changed (0 = Title, 1 = Orbit/Director, 2 = Action/Gameplay)
    if (musicMode != currentMode_) {
        // Stop previous non-matching tracks
        if (musicMode != 0 && hasTitleMusic_ && IsMusicStreamPlaying(bgmTitle_)) StopMusicStream(bgmTitle_);
        if (musicMode != 1 && hasOrbitMusic_ && IsMusicStreamPlaying(bgmOrbit_)) StopMusicStream(bgmOrbit_);
        // If exiting action mode, pause action so it doesn't restart from beginning on next run
        if (musicMode != 2) {
            if (hasActionMusic1_ && IsMusicStreamPlaying(bgmAction1_)) PauseMusicStream(bgmAction1_);
            if (hasActionMusic2_ && IsMusicStreamPlaying(bgmAction2_)) PauseMusicStream(bgmAction2_);
        }
        currentMode_ = musicMode;
    }

    if (musicMode == 0) {
        if (hasTitleMusic_) {
            if (!IsMusicStreamPlaying(bgmTitle_)) PlayMusicStream(bgmTitle_);
            UpdateMusicStream(bgmTitle_);
        }
    } else if (musicMode == 1) {
        if (hasOrbitMusic_) {
            if (!IsMusicStreamPlaying(bgmOrbit_)) PlayMusicStream(bgmOrbit_);
            UpdateMusicStream(bgmOrbit_);
        }
    } else if (musicMode == 2) {
        // Continuous rotating action tracks (does NOT restart on death / respawn)
        Music& activeAction = (currentActionTrackIdx_ == 0 || !hasActionMusic2_) ? bgmAction1_ : bgmAction2_;
        bool& hasActive = (currentActionTrackIdx_ == 0 || !hasActionMusic2_) ? hasActionMusic1_ : hasActionMusic2_;

        if (hasActive) {
            if (!IsMusicStreamPlaying(activeAction)) {
                PlayMusicStream(activeAction);
            }
            UpdateMusicStream(activeAction);

            // Rotate track when current track completes its playback cycle
            float played = GetMusicTimePlayed(activeAction);
            float total = GetMusicTimeLength(activeAction);
            if (total > 5.0f && played >= total - 0.25f && hasActionMusic2_) {
                StopMusicStream(activeAction);
                currentActionTrackIdx_ = 1 - currentActionTrackIdx_;
                Music& nextAction = (currentActionTrackIdx_ == 0) ? bgmAction1_ : bgmAction2_;
                PlayMusicStream(nextAction);
            }
        }
    }
}

void SoundSynth::PlayFlap() const { if (initialized_ && !muted_) PlaySound(sndFlap_); }
void SoundSynth::PlayBlink() const { if (initialized_ && !muted_) PlaySound(sndBlink_); }
void SoundSynth::PlayMotePickup() const { if (initialized_ && !muted_) PlaySound(sndMote_); }
void SoundSynth::PlayExoticPickup() const { if (initialized_ && !muted_) PlaySound(sndExotic_); }
void SoundSynth::PlaySuperActivate() const { if (initialized_ && !muted_) PlaySound(sndSuper_); }
void SoundSynth::PlayShieldBreak() const { if (initialized_ && !muted_) PlaySound(sndShieldBreak_); }
void SoundSynth::PlayDeath() const { if (initialized_ && !muted_) PlaySound(sndDeath_); }
void SoundSynth::PlayBossAlert() const { if (initialized_ && !muted_) PlaySound(sndBossAlert_); }
void SoundSynth::PlayLaserShot() const { if (initialized_ && !muted_) PlaySound(sndLaser_); }
void SoundSynth::PlayUiSelect() const { if (initialized_ && !muted_) PlaySound(sndUi_); }

} // namespace FlappyGhost
