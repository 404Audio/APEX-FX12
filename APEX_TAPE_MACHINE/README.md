# APEX TAPE MACHINE — Gen 1
**Manufacturer:** APEX Audio Labs  
**Plugin:** APEX Tape Machine  
**Type:** Stereo insert effect  
**Target:** Original Akai MPC Live (ARM32 / Cortex-A17), MPC OS 3.10.1+

A Studer A800-inspired two-inch, 24-track tape coloration effect with an integrated original acoustic echo-chamber algorithm. It is a behavioral model, not an exact measured clone or an affiliation with Studer.

## UI pages
- **TAPE:** Input, Output, Drive, Tape Mix, Bias, Compression, Speed, Formula
- **MOTION:** Wow, Flutter, Hiss, Crosstalk, HF response
- **CHAMBER:** Enable, Send, Mix, Size, Decay, Damping, Pre-delay, Width

## Factory programs
90s Hip-Hop Warmth, Dusty Boom Bap, Drum Bus Glue, Thick Bass, Vocal Tracking, Hot Print, Open Reel Master, DJ Mixtape, Chamber Vocal, A800 Default.

## Status
Initial SDK-native effect implementation. The tape and echo algorithms are real-time safe and use fixed buffers allocated in prepare(). Build, host-simulator, ARM32 packaging and hardware test remain to be run using the official SDK toolchain.
