# Destiny: The Flappy Ghost // Light & Darkness

A high-performance, cross-platform (Windows & Android) C++20 arcade action game built with **Raylib 5.0**. Pilot an articulated polyhedral Ghost across 4 procedural Destiny sectors, harvest Motes of Light, attune across 6 Light and Darkness subclasses, weave through Vex laser grids, command tactical Combat Drones, and conquer Nightfall strikes.

> **Studio:** Sebiny Labs  
> **IP Disclaimer:** Non-commercial fan tribute created for educational and portfolio purposes. *Destiny*, *Ghost*, *The Traveler*, and related IP are trademarks and copyright of **Bungie, Inc.**

---

## Game Modes

Destiny: The Flappy Ghost features two distinct flight protocols tailored for both casual exploration and hardcore combat:

### 1. Vanguard Patrol (Pure Flight & Precision)
- **Focus:** Classic aerial navigation, precision barrier weaving, and paracausal movement.
- **Rules:** Weapons and Combat Drones are deactivated. Focus entirely on navigating treacherous terrain, timing jumps, tactical blinks, and unleashing Super abilities.
- **Hostiles:** Enemies appear as extremely rare "Easter egg" cosmic encounters, keeping the run smooth, zen, and skill-focused.
- **Entry:** Unlocked and playable from your very first launch directly from the Director.

### 2. Nightfall Strike (High-Intensity Combat)
- **Focus:** Full-scale combat operations against enemy swarms, elite barrier projections, and raid bosses.
- **Unlock Condition:** Unlocked once you purchase your first **Combat Drone** from The Tower Hangar (starting at 200 Glimmer).
- **Combat Mechanics:** Equip your chosen Combat Drone and manually trigger tactical fire with `[C / X]` or the on-screen drone button.
- **Rewards:** High risk, high reward — earns an automatic **+1.5x Glimmer Multiplier** on all harvested Motes of Light!

---

## Guardian Subclasses (Light & Darkness)

Attune between 6 distinct subclass configurations in the **Subclass Armory**, each with custom tactical movement abilities, particle signatures, and screen-clearing Supers:

| Subclass | Affinity | Tactical Ability | Super Ability |
| :--- | :--- | :--- | :--- |
| **Voidwalker** | Void Light | **Blink:** Short-range phase teleport with 0.32s intangibility | **Nova Pulse:** Singularity blast vaporizing active barriers into Motes |
| **Dawnblade** | Solar Light | **Icarus Dash:** Forward momentum stabilizer & 3-way flare hover | **Daybreak Well:** Radiant aura melting obstacles on contact |
| **Stormcaller** | Arc Light | **Ionic Blink:** High-voltage dash firing a piercing Arc bolt ahead | **Chaos Reach:** Continuous high-energy Arc beam across the screen |
| **Shadebinder** | Stasis Darkness | **Frost Pulse:** Freezes moving Vex lasers and boss fire for 2.5s | **Winter's Wrath:** Shatters all frozen barriers into Motes of Light |
| **Threadrunner** | Strand Darkness | **Grapple:** Launches a grapple line ahead to swing & phase upward | **Needlestorm:** Unleashes 9 homing Strand Threadlings |
| **Transcendent** | Prismatic | **Prismatic Surge:** Freezes obstacles & fires twin piercing bolts | **Transcendence:** Screen-wide barrier purge + 12 homing missiles |

---

## The Tower & Persistent Progression (`flappy_ghost_save.dat`)

Visit **The Tower** to equip and unlock gear with harvested Glimmer:
- **12 Procedural Ghost Shells:** Including *Generalist Shell*, *Sagira's Shell*, *Tyrant Warmind*, *Harpy Vex*, *Ghost of the Deep*, and *Flawless Trials*.
- **4 Manual-Fire Combat Drones:**
  1. *Arc Sweeper* (Rapid twin Arc sparks)
  2. *Solar Valkyrie* (3-way fan of heavy Solar flares)
  3. *Void Tachyon* (High-velocity piercing singularity bolts)
  4. *Stasis Hailstorm* (Glacial cryo-shards that freeze obstacles on impact)
- **Shaders & Exotic Mods:** Custom palettes (*Midnight Exigent*, *Calus' Selected*, *Gambit Jadestone*) and exotic perks (Mote Magnetism, Overshield Recharging).
- **Vanguard Triumphs:** 10 trackable lore achievements with toast pop-ups and Glimmer bounties.

---

## 4 Procedural Destiny Biomes

1. **Old Russia Cosmodrome:** Rusting launch gantries, Fallen Ketch silhouettes, and crisp dawn starfields.
2. **The Vault of Glass:** Shifting brass pillars, cycling Vex sync lasers, and Golden Oracles.
3. **The Hellmouth of Luna:** Cracked Hive obsidian towers, green Soulfire lanterns, and Shrieker void wards.
4. **The Ascendant Plane:** Taken blights, gravitational distortion wells, and negative-space portals.

---

## Controls

| Action | PC (Keyboard & Mouse) | Android / Touch |
| :--- | :--- | :--- |
| **Flap / Jump** | `SPACE` / `W` / `UP` / `Left Click` | Tap Screen |
| **Tactical Ability / Blink** | `SHIFT` / `E` / `Right Click` | Tap `BLINK / DASH` HUD Button |
| **Cast Super Ability** | `F` / `Q` | Tap `SUPER` HUD Button |
| **Fire Combat Drone (Nightfall)** | `C` / `X` | Tap `DRONE FIRE` HUD Button |
| **Director / Cancel** | `ESC` | Back Button / UI Button |
| **Mute Soundtrack** | `M` | Audio Toggle |

---

## Audio & Soundtrack Architecture

Features a built-in multi-track ambient and orchestral music engine streaming iconic Destiny themes:
- **Title / Front Page:** *Inner Light* (Destiny 2 Original Soundtrack)
- **Director / Subclass Armory:** *Deep Stone Lullaby* & *Journey*
- **Action & Nightfall Run:** *The Last Array* & *Sepiks Redux*
- **Procedural Sound Synthesizer:** All 12 sound effects (Ghost chirps, Mote chimes, Blink swooshes, Super detonations) are generated dynamically via procedural C++ waveforms with zero missing asset risks.

---

## Technical Specifications

- **Engine:** Raylib 5.0 in modern C++20.
- **Physics Loop:** Deterministic fixed-timestep accumulator running at `120 Hz`.
- **Memory Management:** Zero runtime heap allocations during gameplay; pre-allocated object pools for obstacles, projectiles, and 1,024 particle nodes.
- **PRNG:** Deterministic `PCG32` generator ensuring predictable trajectory verification and seed generation.

---

## Building & Running

### Windows (CMake + MinGW / Ninja)
```cmd
cmake -B build -G Ninja
ninja -C build
.\build\FlappyGhost.exe
```

### Android APK Build
Requires Android SDK + NDK (`r26b`+):
```cmd
cmd.exe /c Build_Android_APK.bat
```
Output: `build_android/FlappyGhost.apk` ready for sideloading or USB debugging via ADB.
