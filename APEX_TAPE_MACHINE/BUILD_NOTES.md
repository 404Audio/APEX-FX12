# Build notes
- Project identity: APEX Audio Labs / APEX Tape Machine / uid AtM1.
- Target this first for original MPC Live (ARM32 only).
- SDK project source is under APEX_TAPE_MACHINE/.
- Once integrated into a checked-out SDK tree: python tools/build.py plugins/APEX-Tape-Machine --target native (test), then --target arm32 (final Gen 1 binary).
- After build, run .sdk-tools/host_sim ... test, then package only after validator and test pass.
