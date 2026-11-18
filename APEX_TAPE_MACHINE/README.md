# APEX TAPE MACHINE — Gen 1

**Manufacturer:** APEX Audio Labs  
**Plugin:** APEX Tape Machine  
**Type:** Stereo insert effect  
**Primary target:** Original Akai MPC Live (32-bit ARM / Cortex-A17), MPC OS 3.10.1+

An original behavioral model inspired by a two-inch, 24-track analog tape workflow, with tape saturation, program-dependent compression, variable-delay wow/flutter, tape noise, track crosstalk and an integrated acoustic echo chamber. It is not an exact measured clone of any particular recorder and is not affiliated with Studer.

## MPC touchscreen interface

The plugin has three native MPC pages with amber analog-style controls and Q-Link mappings:

- **TAPE:** Input, Output, Drive, Tape Mix, Bias, Compression, Path (Input / Sync / Repro), 15/30 IPS, tape formula (250 / 456 / 900 / GP9).
- **MOTION:** Wow, Flutter, Hiss, Crosstalk, HF Shape, Tape On/Off.
- **CHAMBER:** Send, Wet/Dry Mix, Size, Decay, Chamber On/Off, Damping, Pre-delay, Width.

## Factory programs

1. 90s Hip-Hop
2. Dusty Boom Bap
3. Drum Bus Glue
4. Thick Bass
5. Vocal Chamber
6. Hot Tape Print
7. Open Reel Master
8. DJ Mixtape
9. APEX Default

## Build and test status

- MPC SDK native host simulator: **19 passed, 0 warnings, 0 failures** on the latest completed validation run.
- SDK binary validation: ARM32 output confirmed as 32-bit ARM, correct VST entry point, required shared-library set and GLIBC compatibility.
- Touchscreen UI: three tabs and all parameter handles / Q-Link maps validated.
- Physical MPC Live Gen 1 test: **not yet performed**. The final confirmation must happen on the user's hardware.

The SDK's normal package build also creates an AArch64 companion binary. Gen 1 is the supported test target for this first release; Gen 2 hardware behavior has not been tested or signed off.

## Installation

1. Unzip the MPC package.
2. Copy the whole folder named **APEX Audio Labs - VST - APEX Tape Machine** into a folder named **VST2 Plugins** at the root of a USB drive or SD card.
3. On the MPC Live, use MPC OS 3.10.1 or newer.
4. In Preferences → Enhancements (Beta), enable **Enable Enhancements** and **External Plugin Support**.
5. Restart the MPC and find the effect under **External Plugins** in an insert-effect slot.

## Modeling notes

The tape stage models stereo tape behavior and the interaction of a stereo pair of tracks; crosstalk provides a linked-track character. Input / Sync / Repro path selection is present; Sync and Repro currently use preliminary distinct playback voicings that still require measured calibration. It does not instantiate 24 independent audio channels. Wow/flutter use short variable-delay modulation. The echo chamber is an original damped feedback/early-reflection network, not an impulse-response capture of a particular physical room.

## DSP implementation

Original APEX C++ DSP; SDK framework integration follows the official inMusic MPC Plugin Builder SDK. Runtime audio processing allocates no memory and performs no file I/O in the process callback.
