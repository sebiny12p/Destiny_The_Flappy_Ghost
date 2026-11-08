# Destiny: Flappy Ghost — Lightbearer Protocol

A procedural, cross-platform (PC & Android) C++20 arcade action game built with **Raylib 5.0**. Pilot an articulated polyhedral Ghost across 4 procedural Destiny sectors, harvest Motes of Light, attune between Void, Solar, and Arc subclasses, phase through barriers with Blink, and scan Vex Confluxes to shatter Raid Boss immunity shields.

> **Studio:** Sebiny Labs  
> **IP Disclaimer:** Non-commercial fan tribute created for educational and portfolio purposes. *Destiny*, *Ghost*, *The Traveler*, and related IP are trademarks and copyright of **Bungie, Inc.**

---

## Key Engineering & Gameplay Features

- **3 Attunable Light Subclasses:**
  - **Voidwalker:** *Blink* short-range forward teleport with phase intangibility + *Nova Pulse* Super that vaporizes all on-screen barriers into Motes of Light.
  - **Dawnblade:** *Icarus Dash* vertical momentum stabilizer with 3-way Solar flare volley & hover glide + *Daybreak Well* Super aura.
  - **Stormcaller:** *Ionic Blink* chain-lightning dash + *Chaos Reach* continuous screen-wide Arc laser beam.
- **4 Procedural Destiny Biomes:**
  1. *Cosmodrome Ruins* (Fallen territory)
  2. *The Vault of Glass* (Moving Vex laser gates & destructible Golden Oracles)
  3. *The Hellmouth Catacombs* (Hive bone pillars & Shrieker turrets)
  4. *The Ascendant Plane* (Taken Blight gravitational wells that warp Ghost trajectories)
- **Raid Boss Encounters:** Every 20 gates, an Ultra Boss (*Sepiks Prime*, *The Templar*, *Savathun's Song*, or *Taken Subjugator*) arrives with an immune shield. Scan 3 Vex Confluxes in mid-flight to break its shield and channel Light strikes.
- **Tower Hangar & Persistent Progression (`flappy_ghost_save.dat`):**
  - Unlock 6 custom procedural Ghost Shells (*Generalist*, *Sagira's Shell*, *Tyrant Warmind*, *Harpy Vex*, *Ghost of the Deep*, *Flawless Trials*).
  - Upgrade Mote Magnetism, Paracausal Capacitor, Phase Blink Cooldown, and Vanguard Overshields.
  - 10 unlockable Vanguard **Triumphs** with toast notifications and Glimmer rewards.
- **Engine Hygiene:**
  - **Fixed-Timestep Accumulator (`120 Hz`):** Deterministic physics independent of monitor refresh rate.
  - **Zero Runtime Heap Allocations:** Pre-allocated `std::array` object pools for obstacles, collectibles, projectiles, and 1,024 particles.
  - **Deterministic `PCG32` PRNG:** Powers the **Daily Nightfall Strike** seeded by calendar date (`YYYY-MM-DD`).
  - **Procedural Waveform Audio Synthesizer:** All 10 sound effects (Ghost chirps, Mote fifth chimes, Exotic triad chords, Void Blink sweeps, Super bass drops) are synthesized mathematically in C++ at startup with zero external audio files.

---

## Controls

| Action | PC (Keyboard / Mouse) | Android / Touch |
| :--- | :--- | :--- |
| **Flap / Ascend** | `SPACE` / `W` / `UP` / `Left Click` | Tap Screen |
| **Blink / Tactical Dash** | `SHIFT` / `E` / `Right Click` | Tap `BLINK / DASH` HUD Button |
| **Cast Super Ability** | `F` / `Q` | Tap `CAST SUPER` HUD Button |
| **Ghost Scan Pulse** | `C` | Automatic on Conflux proximity |
| **Pause / Director** | `ESC` / `P` | Back Button |
| **Mute Audio** | `M` | — |

---

## Build & Run (PC & Android)

### Windows (CMake + Ninja / MinGW / MSVC)
```powershell
cmake -B build -G Ninja
cmake --build build
.\Run_FlappyGhost.bat
```
