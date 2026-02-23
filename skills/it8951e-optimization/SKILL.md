# IT8951E Optimization Skill

Use this skill when tuning ESPHome + IT8951E devices for performance,
responsiveness, image quality, and battery life.

## Objectives

1. Maximize display quality for the content type (greyscale fidelity, no ghosting).
2. Minimize energy use without sacrificing UX.
3. Keep the ESPHome scheduler alive (no watchdog hits or long-op storms).
4. Produce auditable, predictable behaviour that does not require a test harness
   to verify.

---

## Inputs Required

- Device YAML (and any included files) — **required**.
- Display usage intent: static dashboard, interactive UI, or mixed — **required**.
- Serial run logs containing `IT8951E_DEMO_REPORT` lines — **optional**;
  use only when the YAML-based analysis is insufficient to diagnose a
  specific anomaly (see §Diagnostic Measurements below).

The skill can produce a complete, correct tuning plan from the YAML alone.
Log capture is a secondary diagnostic tool, not a prerequisite.

---

## Step 1 — Static YAML Audit (no hardware needed)

Read the YAML and check each item.  Most problems are visible here.

### Driver configuration

| Check | Red flag | Correct setting |
|-------|----------|-----------------|
| `sleep_when_done` | Missing or `false` | `sleep_when_done: true` — IT8951E draws ~150 mW active; sleeping between updates is the single biggest battery saving |
| `full_update_every` | Any value > 0 when modes are managed explicitly | `full_update_every: 0` — the auto-GC16 counter fires unexpectedly when waveforms are controlled in YAML; disable it |
| `model` | Missing | `model: M5EPD` — explicit model selection avoids future default-value surprises |
| `reset_duration` | < 100 ms | `reset_duration: 200ms` for M5Paper panels; too short causes unreliable SPI init |
| `reversed` | Wrong polarity | If colours are inverted on the physical panel, set `reversed: true` and use normal `Color(255,255,255)` / `Color(0,0,0)` in the lambda rather than inverting colours manually — simpler and less error-prone |
| `update_interval` | Any value other than `never` | `update_interval: never` — all updates must be driven explicitly; polling conflicts with manual waveform control |

### Render call pattern

The most common correctness bug is calling `component.update` and expecting a
quality waveform.

`component.update` calls the driver's `update()` method which **always uses DU
(binary black/white)** and **resets the dirty-rect tracker** to empty immediately
after.  Any subsequent `it8951e.update` with `full: false` will find an empty
dirty region and silently exit without writing to the panel.

**Correct two-step pattern:**

```yaml
- component.update: my_display        # 1. runs draw lambda → DU to panel → dirty rect cleared
- it8951e.update:
    id: my_display
    mode: GLD16                       # 2. re-sends full in-memory buffer at quality waveform
    full: true                        # REQUIRED — dirty rect is empty after step 1
```

`full: true` bypasses the dirty-rect check and sends the complete buffer.
The DU in step 1 acts as a fast "ghost-clear" flash — a standard e-paper
technique to discharge residual pixel charge before the quality render.

**Red flags in YAML:**

- `component.update` with no follow-up `it8951e.update` → DU-only renders,
  no greyscale, ghosting accumulates.
- `it8951e.update full: false` immediately after `component.update` → silent
  no-op; nothing reaches the panel after the DU.
- `it8951e.update` without a preceding `component.update` → stale buffer;
  the draw lambda has not run.

### Power rail management

| Pattern | Problem | Better |
|---------|---------|--------|
| `output: platform: gpio` + `output.turn_on` in a boot script | Raw GPIO with no deep-sleep hold | `m5paper` component — turns rails on at `setup_priority::BUS`, calls `gpio_hold_en` so rails survive deep-sleep wake cycles |

If using the `m5paper` component, remove any `output.turn_on: main_power` /
`output.turn_on: battery_power` calls — they are redundant and add latency to
every refresh cycle.

### Automation hygiene

- **Redraw storms:** `script mode: restart` + `component.update` in a tight
  loop will hammer the panel.  Ensure `delay` is bounded below (≥ 30 s for
  static dashboards).
- **Transition-driven updates:** for time-driven dashboards, subscribe to
  `next_transition_at` / `next_transition_kind` attributes and compute the
  sleep interval from that rather than polling at a fixed rate.  See boot
  sequence example below.
- **Page-switch latency:** calling `epaper_display.update()` (DU) directly in a
  C++ lambda gives immediate visual feedback for button presses.  A follow-up
  quality render then fires on the next `manage_run_and_sleep` iteration (~5 s).
  This DU-then-quality cadence is intentional and correct.

---

## Step 2 — Mode Selection

### Waveform reference

| Mode | Speed | Greyscale | Best use |
|------|-------|-----------|----------|
| `INIT` | Very slow (~3 s full) | None | Boot cleanup; removes factory ghost and accumulated residue; use once on first render |
| `DU` | Fast (~250 ms) | No (B/W only) | Ghost-clear flash before a quality render; cursor/overlay updates; immediate page-switch feedback |
| `DU4` | Fast | Limited (4 grey) | Anti-aliased menus; faster than GC16 with partial greyscale |
| `A2` | Fastest | No | Animation / motion windows only; accumulates heavy ghosting |
| `GL16` | Medium (~1.5 s) | Yes | **Sparse text on white background only** — degrades visibly on dark or mixed-background content |
| `GLD16` | Medium (~1.5 s) | Yes | **Mixed light/dark content** — balanced quality for dashboards with both light and dark regions |
| `GLR16` | Medium | Yes | Sparse content with improved artefact suppression vs GL16 |
| `GC16` | Slow (~2 s) | Yes (16 levels) | Highest greyscale fidelity; use for first render, manual refresh triggers, and full-screen dark-mode pages |
| `NONE` | Instant | — | Buffer write only; no panel update |

**Common mistake:** using `GL16` for pages with a dark (black-fill) background.
`GL16` is a "sparse content on white" waveform — it assumes a predominantly
white prior state.  Use `GLD16` or `GC16` for pages that fill with black.

### Prescriptive policy by usage pattern

**Static information dashboard** (updates ≤ a few times per day):

| Trigger | Mode | `full` |
|---------|------|--------|
| First boot | `INIT` → `GC16` | `true` |
| Scheduled / event-driven refresh | `GLD16` | `true` |
| Manual refresh (button / HA service) | `GC16` | `true` |
| Page switch (immediate feedback) | `DU` (via `component.update`) | — |

**Interactive UI** (frequent user-driven updates):

| Trigger | Mode | `full` |
|---------|------|--------|
| Fast text / cursor change | `DU` | `false` |
| Menu / overlay with greyscale | `DU4` | `false` |
| Settled full-screen view | `GC16` or `GLD16` | `false` or `true` |
| Periodic ghost cleanup | `GC16` | `true` |

---

## Step 3 — Boot Sequence

A clean boot sequence removes factory residue and sets a known panel state:

```yaml
globals:
  - id: display_initialized
    type: bool
    restore_value: false
    initial_value: 'false'

script:
  - id: manage_run_and_sleep
    mode: restart
    then:
      # ... wait for API / time sync ...
      - if:
          condition:
            lambda: 'return !id(display_initialized);'
          then:
            - lambda: 'id(display_initialized) = true;'
            - it8951e.clear:         # INIT waveform — clears ghost residue
                id: my_display
            - component.update: my_display
            - it8951e.update:
                id: my_display
                mode: GC16           # highest quality for first visible render
                full: true
          else:
            - component.update: my_display
            - it8951e.update:
                id: my_display
                mode: GLD16          # balanced quality for subsequent refreshes
                full: true
```

`restore_value: false` ensures the INIT sequence always runs on a hard reboot,
not just on first flash.

---

## Step 4 — Energy Checklist

Work through this list in order.  Each item has measurable impact.

1. **`sleep_when_done: true`** — highest impact; do this first.
2. **Eliminate unnecessary refreshes** — check for script restart loops that
   call `component.update` without a bounded delay.
3. **Event-driven scheduling** — replace fixed-interval polling with
   transition-based wake times (parse `next_transition_at`; sleep until then
   rather than waking every N seconds).
4. **Reduce Wi-Fi activity** — if the device does not need real-time HA
   connectivity between updates, consider reducing reconnect polling.
5. **Prefer partial updates** (interactive UI only) — `full: false` with a
   tight dirty rect transfers far fewer bytes over SPI and completes faster.
6. **`full_update_every: 0`** — disables spurious auto-GC16 full-screen
   writes that the counter would otherwise trigger unpredictably.

---

## Diagnostic Measurements (optional — use when YAML audit is inconclusive)

If a specific anomaly cannot be explained from the YAML (e.g., unexpectedly
slow updates, watchdog events, or visual artefacts that do not match predicted
waveform behaviour), collect serial logs from `it8951e_test_suite.yaml`.

### What to capture

Per mode, record:
- `duration_ms` — total wall time for the demo run
- `last_update_ms` — time of the final panel update
- `max_update_ms` — worst-case single update latency
- `fps` (A2 / animation modes only)
- Count of `slow display update` warnings (> 4 s threshold)
- `task_wdt` events (hard failure indicator)

### Interpretation

| Symptom | Likely cause |
|---------|-------------|
| `task_wdt > 0` | Hard scheduler block; check for unbounded busy-wait or SPI chunk size too large |
| Many slow-update warnings, low `last_update_ms` | Bottleneck is lambda render work, not panel I/O |
| A2 much slower than expected | Dynamic region too large; reduce bounding box |
| INIT unexpectedly fast | Panel may not be completing the waveform; check BUSY pin timeout logs |
| Quality mode looks same as DU | `full: false` with empty dirty rect — switch to `full: true` |
| Panel completely blank after update | `reversed` polarity mismatch — toggle `reversed:` and retest |

---

## Output Template

Return:

1. YAML audit findings — list every red flag found with the corrected setting.
2. Mode policy table — prescriptive waveform selection for each trigger in the
   device's usage pattern.
3. Energy delta — estimate of improvement from each change (qualitative is fine).
4. Boot sequence — complete `manage_run_and_sleep` snippet if not already correct.
5. Diagnostic steps — only if anomalies remain after the YAML fixes are applied.
