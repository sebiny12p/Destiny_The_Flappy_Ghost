# Destiny: The Flappy Ghost // Light & Darkness

A high-performance, cross-platform (Windows & Android) C++20 arcade action game built with **Raylib 5.0**. Pilot an articulated polyhedral Ghost across 4 procedural Destiny sectors, harvest Motes of Light, attune across 6 Light and Darkness subclasses, weave through Vex laser grids, command tactical Combat Drones, unlock 12 exotic Shaders & 16 Vanguard Triumphs, and conquer Nightfall strikes.

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
- **12 Exotic Shaders:** Full scrollable palette wardrobe with live polyhedral shell preview.
- **16 Vanguard Triumphs:** Scrollable triumph ledger with lore bounties and achievement toasts.
- **Exotic Mods:** Mote Magnetism, Overshield Recharging, and Glimmer Boosters.

---

## 4 Procedural Destiny Biomes

1. **Old Russia Cosmodrome:** Rusting launch gantries, Fallen Ketch silhouettes, and crisp dawn starfields.
2. **The Vault of Glass:** Shifting brass pillars, cycling Vex sync lasers, and Golden Oracles.
3. **The Hellmouth of Luna:** Cracked Hive obsidian towers, green Soulfire lanterns, and Shrieker void wards.
4. **The Ascendant Plane:** Taken blights, gravitational distortion wells, and negative-space portals.

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
