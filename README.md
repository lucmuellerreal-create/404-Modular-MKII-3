# 404 Modular MKII — v0.3 (Windows VST3 source)

Unofficial, independently written SP-404 MKII-inspired multi-effects plugin. Not affiliated with Roland. It does **not** contain Roland's original DSP or an exact emulation.

## Included
- 20 simultaneous, independently switchable effects: Vinyl Sim, Cassette, 303 Vinyl, Lo-Fi, Bit Crusher, Isolator, Filter + Drive, Wah, DJFX Looper, Scatter, Chorus, Flanger, Phaser, Delay, Tape Echo, Reverb, Compressor, Saturator, Ring Mod, Tremolo.
- Four knobs per effect: MIX / CHAR / MOVE / TONE (the last three are generalized macro controls, not exact hardware parameters).
- Five starter presets, global bypass, output gain, project state saving and DAW automation.
- Scrollable effect cards (5 rows, 3 visible). All effects currently run in a fixed series.
- DSP refinements to chorus/flanger modulation interpolation, tape echo, reverb damping, vinyl processing, and DJFX loop addressing.

## Build on Windows
1. Install **Visual Studio 2022** with **Desktop development with C++**, **CMake**, and **Git**.
2. Open **Developer PowerShell for VS 2022** in the extracted project directory.
3. Run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

4. Find the VST3 bundle under `build/SP404_Modular_artefacts/Release/VST3/` and copy it to `C:\Program Files\Common Files\VST3` (administrator permission may be needed).
5. Rescan plugins in your DAW.

JUCE is fetched from GitHub during configuration; an internet connection is required for the initial build.

## Known limitations
**Source package only; not compiled or tested in a Windows DAW in this environment.** This is a feature-complete *prototype*, not a verified production-ready release. The DSP is an approximation; some modules are intentionally simple (especially Scatter, DJFX Looper, Reverb). No BPM sync, drag-and-drop reorder, oversampling, advanced preset management, or full hardware-accurate control set. High-feedback settings may create loud output; start with low monitor volume. The plugin is stereo-only. No external Roland samples, artwork, or trademarks are included.
