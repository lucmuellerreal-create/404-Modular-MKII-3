# Windows VST3 build
This archive does NOT contain a compiled .vst3 file.

## Automated Windows build using GitHub Actions
1. Create a new GitHub repository and upload the contents of this folder (including .github/workflows/windows-vst3.yml).
2. Open the repository's Actions tab and select "Windows VST3 Build" -> "Run workflow".
3. If the build succeeds, download the artifact "404-Modular-MKII-Windows-VST3" and extract its ZIP.
4. Copy the extracted `.vst3` directory into `C:\Program Files\Common Files\VST3\` and rescan plugins in your DAW.

The build has not been tested and may fail. JUCE is downloaded by CMake. This is an unofficial SP-404-inspired prototype, not an original Roland algorithm implementation.
