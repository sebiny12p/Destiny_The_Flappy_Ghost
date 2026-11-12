# Destiny: The Flappy Ghost

A high-performance, cross-platform (Windows & Android) C++20 arcade action game built with **Raylib 5.0**. Pilot an articulated polyhedral Ghost across **8 shuffled Destiny biomes**, harvest Motes of Light, attune across 6 Light and Darkness subclasses, weave through Vex laser grids, command 4 distinct Combat Drones, battle **8 unique Nightfall bosses**, unlock 12 exotic Shaders & 16 Vanguard Triumphs, and conquer Nightfall strikes.

> **Studio:** Sebiny Labs  
> **IP Disclaimer:** Non-commercial fan tribute created for educational and portfolio purposes. *Destiny*, *Ghost*, *The Traveler*, and related IP are trademarks and copyright of **Bungie, Inc.**

---

## What's New in the Latest Release

- 🎨 **Expanded Shader Wardrobe (12 Exotic Shaders):** Scrollable Tower catalog featuring *Midnight Exigent*, *Calus' Selected*, *Gambit Jadestone*, *Neopop Wave*, *Amethyst Veil*, *Desert Gold*, *Bergusian Night*, *Carminica*, *Cinderchar*, *Butterbark*, *Boreal Defiant*, and *Monochromatic*.
- 🏆 **16 Vanguard Triumphs:** Scrollable triumph dossier with real-time progress bars, completion checkmarks, toast pop-ups, and Glimmer bounties.
- 📊 **Lifetime Career Stats Dossier:** Real-time telemetry tracking lifetime flight distance, motes harvested, supers unleashed, nightfalls conquered, favorite shell, and favorite subclass.
- 🎵 **Expanded Soundtrack Jukebox:** 4 authentic Destiny action tracks (*The Last Array*, *Sepiks Redux*, *Shadowkeep / Hashladun*, and *The Witch Queen*) with in-game track selector and mute toggles.
- ⚙️ **System Settings & Accessibility:** Fullscreen toggle (`F11 / Alt+Enter`), left-handed touch controls, screen flash accessibility toggles, and native gamepad support.

---

## Game Modes

Destiny: The Flappy Ghost features two distinct flight protocols tailored for both casual exploration and hardcore combat:

### 1. Vanguard Patrol (Pure Flight & Precision)
- **Focus:** Pure skill-based aerial navigation, barrier weaving, and tactical blinks.
- **Rules:** Supers, Combat Drones, and lethal Oracles/Taken Blights are disabled so your Patrol high score strictly rewards precision flying and tactical timing.
- **Hostiles:** Enemies appear only as rare scenic encounters.
- **Entry:** Unlocked and playable from your very first launch directly from the Director.

### 2. Nightfall Strike (High-Intensity Combat & Supers)
- **Focus:** Full-scale combat operations featuring **Super Abilities**, Combat Drones, Oracles, Taken Blights, enemy swarms, and Raid Bosses.
- **Unlock Condition:** Unlocked once you purchase your first **Combat Drone** from The Tower Hangar (starting at 200 Glimmer).
- **Combat Mechanics:** Charge Super energy from Motes & combat, shatter Boss Overshields with Supers, and fire your Drone with `[C / X]` or the on-screen button.
- **Rewards:** High risk, high reward — earns **+1.5x Glimmer** on Motes of Light!

---

## Guardian Subclasses (Light & Darkness)

Tactical abilities work in every mode; Super abilities are tuned specifically for **Nightfall Strike** combat:

| Subclass | Affinity | Tactical Ability | Nightfall Super Ability |
| :--- | :--- | :--- | :--- |
| **Voidwalker** | Void Light | **Blink:** Short phase teleport (0.32s) | **Nova Bomb:** Massive Void orb shattering shields & clearing 1 hazard |
| **Dawnblade** | Solar Light | **Icarus Dash:** Hover glide & hazard phase | **Daybreak:** Twin homing Solar blades on every flap + aura shield |
| **Stormcaller** | Arc Light | **Ionic Blink:** High-voltage phase dash | **Chaos Reach:** Aimable Arc laser melting enemies, Oracles & Bosses |
| **Shadebinder** | Stasis Darkness | **Frost Pulse:** Freezes lasers & boss fire | **Winter's Wrath:** Freezes hazards & shatters all foes/shots into Motes |
| **Threadrunner** | Strand Darkness | **Grapple:** Forward-upward grapple swing | **Needlestorm:** 12+ homing Threadlings hunting foes & Bosses |
| **Transcendent** | Prismatic | **Prismatic Surge:** Freezes lasers + phase | **Transcendence:** 4x Tactical recharge + dual Light/Dark bursts |

---

## The Tower & Persistent Progression (`flappy_ghost_save.dat`)

Visit **The Tower** to equip and unlock gear with harvested Glimmer:
- **12 Procedural Ghost Shells:** Including *Generalist Shell*, *Sagira's Shell*, *Tyrant Warmind*, *Harpy Vex*, *Ghost of the Deep*, and *Flawless Trials*.
- **4 Distinct Manual-Fire Combat Drones (Nightfall):**
  1. *Arc Soul Sentinel* (Agile 3-round homing Arc burst)
  2. *Hellion Solar Mortar* (Parabolic arcing 30-dmg Solar AoE artillery clearing nearby foes & shots)
  3. *Briarbinds Void Soul* (Shield-breaker Void seekers that shatter Boss Overshields & vaporize enemy fire)
  4. *NTTE Chronal Portal* (Ultra-fast straight-line piercing precision Chronal rail-beams)
- **12 Exotic Shaders:** Full scrollable palette wardrobe with live polyhedral shell preview.
- **16 Vanguard Triumphs:** Scrollable triumph ledger with lore bounties and achievement toasts.
- **Exotic Mods:** Mote Magnetism, Overshield Recharging, and Glimmer Boosters.

---

## 8 Shuffled Destiny Biomes & Unique Nightfall Bosses

Every run shuffles the biome order so each flight starts in a fresh realm and cycles through all 8 sectors:

1. **Cosmodrome Ruins (Fallen):** Rusted Golden-Age steel (`Sepiks Prime` ? 3-bolt Void spread fan).
2. **The Vault of Glass (Vex Network):** Teal radiolarian brass & Vex lasers (`The Templar` ? rotating shield arcs & dual laser rails).
3. **The Hellmouth Catacombs (Hive):** Gothic jade bone pillars (`Savathun's Song` ? opening Shrieker petals & homing Void seekers).
4. **The Ascendant Realm (Taken):** Deep obsidian violet & Nightfall Taken Blights (`Taken Subjugator` ? vertical blinks & Axion darts).
5. **The Dreaming City (Awoken Citadel):** Twilight amethyst & gilded spires (`Dul Incaru` ? 4-way starlight burst).
6. **Europa // BrayTech Exoscience:** Glacial ice-blue & cryo lasers (`Atraks-1` ? aggressive darting & fast Stasis twin-lances).
7. **Savathun's Throne World (Lucent):** Alabaster bone & luminous teal (`Savathun` ? moth wings & 4-bolt Lucent Blade Storm).
8. **The Pale Heart (Inside The Traveler):** Prismatic gold/rose/cyan resonance (`The Witness` ? monolith rings & 5-way Prismatic Barrage).
---

## Controls

| Action | PC (Keyboard & Mouse) | Gamepad (Xbox / PS) | Android / Touch |
| :--- | :--- | :--- | :--- |
| **Flap / Jump** | `SPACE` / `W` / `UP` / `Left Click` | `A` / `Cross` | Tap Screen |
| **Tactical Ability / Blink** | `SHIFT` / `E` / `Right Click` | `B` / `Circle` or `LB` / `L1` | Tap `[BLINK / DASH]` Button |
| **Cast Super Ability** | `F` / `Q` | `Y` / `Triangle` or `RB` / `R1` | Tap `[SUPER]` Button |
| **Fire Combat Drone (Nightfall)** | `C` / `X` | `RT` / `R2` or `X` / `Square` | Tap `[DRONE FIRE]` Button |
| **Open Director / Pause** | `ESC` | `START` / `Options` | Tap Top-Right `[DIRECTOR]` Button |
| **Soundtrack Selector** | `N` (Next Track) / `M` (Mute) | Via Director Settings | Via Director Settings |
| **Toggle Fullscreen** | `F11` or `ALT + ENTER` | — | Native Immersive Fullscreen |

---

## Technical Specifications

- **Engine:** Raylib 5.0 in modern C++20.
- **Physics Loop:** Deterministic fixed-timestep accumulator running at `120 Hz`.
- **Memory Management:** Zero runtime heap allocations during gameplay; pre-allocated object pools for obstacles, projectiles, and 1,024 particle nodes.
- **PRNG:** Deterministic `PCG32` generator ensuring predictable trajectory verification and seed generation.
- **Cross-Platform:** Native builds for Windows 64-bit and Android `arm64-v8a` (OpenGL ES 2.0 / 3.0).

---

## Building & Deliverables

### Windows Executable & Standalone Package
```cmd
cmake -B build -G Ninja
ninja -C build
.\build\FlappyGhost.exe
```
Standalone distribution archive: `FlappyGhost_Windows.zip` (contains `.exe` and self-contained `assets/` folder).

### Android APK Build
Requires Android SDK + NDK (`r26b`+):
```cmd
cmd.exe /c Build_Android_APK.bat
```
Output: `FlappyGhost.apk` (aligned and signed with v2/v3 signatures, ready for direct USB sideloading).
