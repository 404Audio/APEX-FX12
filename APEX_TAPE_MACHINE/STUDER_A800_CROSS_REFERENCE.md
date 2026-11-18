# APEX Tape Machine — Studer A800 Cross-Reference and Fidelity Plan

**Reference target:** Studer A800 Mk II / Mk III, 2-inch, 24-track configuration.
**Purpose:** engineering reference and implementation gap analysis. This document does not claim Studer endorsement, an exact measured clone, or access to a particular machine's calibration data.

## Primary references

1. Universal Audio, *Studer A800 Multichannel Tape Recorder Manual* (updated 2025-04-02): https://help.uaudio.com/hc/en-us/articles/4419513099156-Studer-A800-Multichannel-Tape-Recorder-Manual
   - Useful for the modeled A800 circuit paths, 7.5/15/30 IPS behavior, 250/456/900/GP9 tape formulations, calibration levels, NAB/CCIR/AES EQ, bias, HF record EQ, Sync/Repro playback EQ, hiss and hum.
   - This is a third-party software modeling reference, not a substitute for Studer's hardware service data.
2. Studer, *A800 Service Information, Audio Adjustments* (SI 66/81 E): https://www.reeltoreel.nl/studer/Public/Products/Recording_Analog/A800/Technical_Info/SI_066-81_E_A800_TD_Audio_Adj.pdf
   - Primary service reference for calibration and bias adjustment procedures. The bias procedure searches the 10 kHz record/reproduce response and sets the bias drop relative to maximum according to the applicable speed configuration.
3. A800 technical specifications compiled from the service manual: https://audiogearspecs.com/gear/studer-a800/
   - 15 IPS: 30 Hz–20 kHz ±2 dB; 30 IPS: 50 Hz–20 kHz ±2 dB.
   - Wow/flutter: 0.04% at 15 and 30 IPS; 0.06% at 7.5 IPS, peak weighted DIN 45507.
   - 1% maximum third-harmonic distortion at 1 kHz at operating level; bias oscillator 240 kHz; erase oscillator 80 kHz.
4. Additional machine history/specification context: https://retrorecordingsxr.com/tape-machines/?tape_brand=studer&tape_model=a800

## A800 behavior that the model must account for

- **Transport speed:** three distinct speeds (7.5, 15, 30 IPS), not only 15/30. Speed must alter the tape/head frequency response, low-frequency head bump, high-frequency behavior, noise and distortion profile.
- **Tape formulation:** expose 250, 456, 900 and GP9. Each stock needs its own calibrated operating point, saturation onset, hysteresis/nonlinear response, bias interaction and noise profile. Do not represent formulas as arbitrary gain/headroom presets.
- **Calibration / fluxivity:** model +3, +6, +7.5 and +9 calibration choices where appropriate. UA's documented recommended mapping is 250=+3, 456=+6, 900=+9, GP9=+9. The service reference operating-level figures must be kept distinct from this plugin calibration abstraction.
- **Record bias:** model HF bias as an ultrasonic record-head bias mechanism, not a generic audible tone control. It must interact with tape stock, speed, record EQ and nonlinear record response. Provide an Auto Cal behavior and a manual override.
- **Record/playback path:** separate Input/Thru, Sync and Repro behavior. Record electronics, record head/tape, and playback head/electronics are not interchangeable with one waveshaper.
- **Equalization:** NAB and CCIR options at 7.5/15 IPS; AES emphasis at 30 IPS. Implement paired record pre-emphasis and playback de-emphasis rather than a single generic HF low-pass.
- **Head bump and response:** speed-dependent low-frequency resonance and measured response targets. Do not fake the A800's head bump with a static bass boost.
- **Noise:** separate tape hiss (in the tape/replay path) from mains hum (post tape circuitry); hum frequency follows NAB 60 Hz / CCIR 50 Hz behavior. Noise level/timbre depends on stock, speed, calibration, EQ and path.
- **Crosstalk:** represent adjacent track coupling as a calibrated, frequency-dependent effect; current simple stereo crossfeed is only a placeholder.
- **Wow/flutter:** the hardware specification is a low transport-error target, not an invitation to add exaggerated chorus. Preserve a calibrated/near-clean setting and make creative modulation a clearly separate range.
- **Do not conflate tape compression with a conventional compressor.** The A800's tape compression is a level-dependent magnetic recording behavior. The current envelope-based compressor is only a prototype placeholder and should not be described as a faithful A800 mechanism.

## Current prototype gap (source audit, v0.1)

The current DSP has a basic saturator, envelope-based gain reduction, a simplified HF low-pass, variable-delay modulation, a simple stereo crossfeed, noise, and an original echo-chamber effect. It currently exposes only 15/30 IPS and 456/900/GP9; it does not yet expose 7.5 IPS, 250, calibration/fluxivity, NAB/CCIR/AES, Auto Cal, Input/Sync/Repro path selection, separate HF record and playback calibration, independent hum, or measured head-bump curves. Its current chamber is an added APEX effect, not part of the A800 reference machine.

**Therefore v0.1 is an A800-inspired prototype, not yet a Studer A800-faithful sonic model.** Do not market it as an exact replica until the missing behaviors are implemented and compared against repeatable reference captures.

## Implementation order

1. Lock reference target and calibration assumptions: A800 Mk II/III, 2-inch 24-track, stock-specific calibration.
2. Replace generic drive/comp with a stable, oversampled magnetic-tape record model (hysteresis/level-dependent saturation and bias interaction); validate aliasing and CPU on MPC Live Gen 1.
3. Add all three IPS speeds, four tape formulas, calibration/fluxivity and Auto Cal mapping.
4. Implement paired NAB/CCIR/AES record/replay EQ and speed-dependent head bump/response.
5. Add Input/Sync/Repro path topology and separate record/playback calibration controls.
6. Separate hiss and hum, calibrate their path placement and stock/speed dependence; refine crosstalk and transport modulation.
7. Keep the APEX Chamber as an optional independent module that can be bypassed completely for A800-only comparisons.
8. Run level-matched null/listening tests against authorized A800 reference captures or a calibrated hardware unit at each speed/formula/path. Document sample rate, flux level, EQ, tape stock, alignment, source level and capture chain. Do not tune by memory or marketing descriptions alone.

## Acceptance targets

- Native host simulator remains clean (finite output, stable parameter/state round-trip, no clipping in nominal test).
- ARM32 package validates and stays within measured MPC Live Gen 1 CPU/memory budget.
- At nominal calibration, response and distortion are compared against the cited A800 reference specifications and documented service alignment conditions.
- Listening comparisons are level matched and use the same input, speed, tape stock, calibration, EQ and path.
- Any value that cannot be established from service documentation or measured captures is labeled as an APEX design choice, not as an authentic Studer setting.
