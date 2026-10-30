# RIV VOCAL FORGE PRO

A JUCE 8 / C++17 vocal-processing VST3 project for Windows 11 and FL Studio.

## What this build contains

- VST3 and standalone targets (x64 on the supplied GitHub Actions runner).
- Simple and Advanced pages, dark interface with orange accents.
- 17 starting presets: Mid Control, Backing Vocal, Reggaeton, Trap and Voz Femenina.
- Compressor with threshold, ratio, attack and release controls.
- Seven-band parametric EQ with per-band frequency and gain controls.
- Input/output gain, gate, Air, de-esser approximation, reverb and delay.
- Up-to-15-minute signal-statistics analysis that adapts selected parameters; the user can stop early and apply.
- Host state recall and `.rivpreset` export/import.
- GitHub Actions workflow to build and package the Windows VST3.

## Build on GitHub

1. Extract the ZIP and upload the **contents** of `RIV_VOCAL_FORGE` to a new GitHub repository.
2. Push to `main`/`master`, or open **Actions** and run **Build RIV VOCAL FORGE PRO (Windows)**.
3. Wait for the workflow to finish, then download the `RIV-VOCAL-FORGE-Windows-x64` artifact.
4. Extract the artifact and copy `RIV VOCAL FORGE.vst3` to:
   `C:\Program Files\Common Files\VST3\`
5. In FL Studio, open **Options > Manage plugins** and run a scan.

The workflow fetches JUCE 8.0.8 from the official JUCE repository during configuration. A working GitHub runner needs internet access. No JUCE source is vendored in this repository.

## Local build (Visual Studio 2022)

Install Visual Studio 2022 with **Desktop development with C++**, CMake, and a Windows 10/11 SDK. Then run:

```powershell
cmake -S . -B build -A x64 -DRIV_FETCH_JUCE=ON
cmake --build build --config Release --target RIV_VOCAL_FORGE_VST3
```

## Important limitations — read before use

This is an extensible engineering build, not a finished commercial vocal suite. The voice analyzer measures signal statistics (RMS, peak and a simple brightness proxy); it is **not a trained neural AI model**, does not identify artists, and cannot guarantee a release-ready mix. The current gate and de-esser are intentionally described as basic approximations, not dedicated spectral noise reduction or advanced split-band de-essing. The compressor is a clean digital compressor, not a CLA-2A/LA-2A emulation. The reverb uses JUCE's algorithmic reverb, not a convolution plate model. The delay is a basic time-based effect and is not yet a full host-tempo-synced stereo delay with independent feedback filters. The EQ is static parametric EQ; dynamic EQ and a real-time spectrum display are not yet included. A professional pitch-correction/autotune engine, scale/key detection, and neural denoising are not included in this build.

Use conservative levels, test with your own recordings, and check latency/CPU behavior in your DAW before production use. The included presets are starting points; adjust them for the microphone, vocalist, room and arrangement.

## Licensing and distribution

JUCE is downloaded at build time. Review the current JUCE licence and comply with its terms before distributing binaries or source. This project does not include proprietary code from other audio products or recordings of named artists.
