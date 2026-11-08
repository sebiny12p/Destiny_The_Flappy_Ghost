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

private:
    bool initialized_ = false;
    bool muted_ = false;
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
