#pragma once
#include "raylib.h"

namespace FlappyGhost {

class SoundSynth {
public:
    void Init();
    void Shutdown();

    void PlayFlap() const;
    void PlayBlink() const;
    void PlayMotePickup() const;
    void PlayExoticPickup() const;
    void PlaySuperActivate() const;
    void PlayShieldBreak() const;
    void PlayDeath() const;
    void PlayBossAlert() const;
    void PlayLaserShot() const;
    void PlayUiSelect() const;
    void UpdateMusic(int musicMode); // 0 = Title Theme, 1 = Orbit/Menu, 2 = Action/Gameplay

    bool IsMuted() const { return muted_; }
    void ToggleMute() { muted_ = !muted_; }
    void SetMasterVolume(float vol) { masterVol_ = vol; ApplyVolumes(); }
    void SetMusicVolume(float vol) { musicVol_ = vol; ApplyVolumes(); }
    void SetSfxVolume(float vol) { sfxVol_ = vol; ApplyVolumes(); }
    float GetMasterVolume() const { return masterVol_; }
    float GetMusicVolume() const { return musicVol_; }
    float GetSfxVolume() const { return sfxVol_; }
    void ApplyVolumes();

private:
    bool initialized_ = false;
    bool muted_ = false;
    float masterVol_ = 1.0f;
    float musicVol_ = 0.55f;
    float sfxVol_ = 0.80f;
    int currentMode_ = -1;
    int currentActionTrackIdx_ = 0;

    bool hasTitleMusic_ = false;
    bool hasOrbitMusic_ = false;
    bool hasActionMusic1_ = false;
    bool hasActionMusic2_ = false;

    Sound sndFlap_{};
    Sound sndBlink_{};
    Sound sndMote_{};
    Sound sndExotic_{};
    Sound sndSuper_{};
    Sound sndShieldBreak_{};
    Sound sndDeath_{};
    Sound sndBossAlert_{};
    Sound sndLaser_{};
    Sound sndUi_{};
    Music bgmTitle_{};
    Music bgmOrbit_{};
    Music bgmAction1_{};
    Music bgmAction2_{};
};

} // namespace FlappyGhost
