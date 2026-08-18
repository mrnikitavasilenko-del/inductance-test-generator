# Inductance Test Generator

Firmware for a bench instrument that finds out how much inductance a coil *actually* has —
and catches the moment its core saturates — by hitting it with a real current pulse instead
of a small-signal LCR-meter probe.

The device charges a capacitor bank to a set voltage, then fires a current pulse through the
inductor under test while sampling current and voltage at 50 kHz to compute
`L = U·dt/dI` in real time. A windowed, hysteresis-filtered saturation detector watches the
effective inductance collapse as the core saturates and cuts the pulse before the current
runs away — with a short extra hold so the saturation "knee" is actually visible on a
scope trace, not just electronically detected.

## Hardware

- **MCU**: STM32F107VCT6 (Cortex-M3)
- 6-button input (Stop / Start / I± / L±), 6-digit multiplexed 7-segment display
- Adjustable setpoints: current limit 10–300 A (5 A steps), target inductance
  10 µH – 50 mH (step size auto-scales with magnitude)
- Safety interlock output, independent of firmware state

## How the safety cutoff works

- **Overcurrent**: hard, immediate cutoff once measured current reaches the setpoint
- **Saturation**: `dI` is measured over a sliding window (not sample-to-sample, which is too
  noisy at high inductance) with real elapsed time from a hardware tick counter; a cutoff
  only fires after several consecutive below-threshold windows, so a single noisy sample
  can't trip it
- A hard 500 ms pulse-length backstop catches the case where current oscillates in an
  undamped LC ring without ever crossing either threshold

## Build

Open in **STM32CubeIDE** (`.project`/`.cproject`/`.mxproject` included) and build, or run
`make` from the generated `Debug` directory.

## Layout

- `Core/` — state machine (charge → measure → discharge), ADC/TIM setup
- `7Segment_Lib/` — bit-banged multiplexed 7-segment display driver
- `Drivers/` — vendored ST HAL/CMSIS
