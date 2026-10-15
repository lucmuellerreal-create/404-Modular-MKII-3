# 404 Modular MKII — stable foundation (source)

Independent JUCE 8 VST3 effect, not affiliated with Roland. Eight working prototype DSP modules: Vinyl Sim, Lo-Fi, Isolator, Drive, Chorus, Tape Echo, Reverb, Tremolo. Every module has bypass, mix and amount. Global output, bypass and DAW project-state persistence. Dark interface. Fixed module order.

## Windows build without local programming tools

Upload **the contents of this directory** to the root of a GitHub repository, preserving `.github/workflows/windows-vst3.yml`, `Source/`, and `CMakeLists.txt`. Actions → Windows VST3 Build → Run workflow. A successful run should expose an artifact containing `404 Modular MKII.vst3` (a Windows VST3 directory bundle). Unzip and copy that bundle to `C:\Program Files\Common Files\VST3\`, then rescan plugins in your DAW.

## Important limitations

This source has not been compiled on Windows or DAW-tested in this environment. A green Actions build is required before installation; audio and UI still need hands-on testing. These effects are **original approximations**, not bit-identical SP-404 MKII effects. The DSP is a first stable *foundation*, not the complete SP-404 effect catalog. Delay and reverb are simple algorithms, not studio-quality final designs. Parameters are loaded per block, so abrupt automation can cause zipper noise. No BPM sync, oversampling, presets browser, or rearrangeable chain yet.

## Local build

Requires CMake >=3.22, Git, Visual Studio 2022 C++ build tools, internet access to fetch JUCE 8.0.10:

```
cmake -S . -B build -A x64
cmake --build build --config Release
```

## If Actions fails

Open the failing step and copy the first `error` or `CMake Error` lines, not just `exit code 1`. `Node.js 20 deprecated` warnings from checkout v4 are not usually the compilation failure.
