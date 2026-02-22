# IT8951E Optimization Skill

Use this skill when tuning ESPHome + IT8951E devices for performance, responsiveness, image quality, and battery life.

## Objectives

1. Minimize UI latency and long-blocking operations.
2. Preserve acceptable ghosting/artifact behavior.
3. Reduce average energy use without breaking UX.
4. Produce repeatable measurements and tuning decisions.

## Inputs Required

- Device YAML (and any included files).
- Serial run logs containing `IT8951E_DEMO_REPORT` lines.
- Display usage intent (interactive UI, static dashboard, mixed).

## Measurement Protocol (Baseline First)

1. Run a full cycle of all update modes in a fixed sequence.
2. Capture these metrics per mode:
   - `duration_ms`
   - `last_update_ms`
   - `max_update_ms`
   - `fps` (for A2 or animation modes)
   - counts of long-operation warnings (`script`, `interval`, etc.)
   - `task_wdt` events
3. Repeat after each tuning pass and compare deltas.

## Quick Interpretation Rules

- `task_wdt > 0`: hard fail; fix blocking first.
- Many 4s+ long-op warnings with low `last_update_ms`: bottleneck is usually automation/render lambda work, not the panel command itself.
- A2 much slower than expected: dynamic region rendering or update cadence is too heavy.
- INIT/full update slow is expected; use sparingly.

## Driver-Side Tuning Checklist

- Use bulk SPI writes (`write_array`) instead of per-byte transfers where possible.
- Stream in chunks and yield periodically (`delay(1)` cadence) to keep scheduler alive.
- Keep partial update path hot; avoid forcing full refresh unless required.
- Keep `sleep_when_done: true` for power-sensitive deployments.
- Keep a bounded full-refresh policy (periodic cleanup) to control ghosting.
- Ensure busy waits have sane timeouts and clear logging.

## YAML/Automation Tuning Checklist

- Avoid redraw storms (`script` restart loops + rapid `component.update`).
- Separate static and dynamic regions; redraw only changed windows.
- Use fast modes for interaction (`DU`, `DU4`, `A2`) and quality modes (`GC16`, `GL*`) for settled views.
- Avoid full-window re-render in tight intervals.
- Use explicit phase progress and fewer full-canvas operations in animation loops.

## Battery Tuning Checklist

- Keep panel asleep between updates.
- Reduce Wi-Fi activity when not needed (especially in benchmark/demo cycles).
- Prefer partial updates over full updates for frequent changes.
- Tune update cadence to the minimum acceptable UX rate.

## Mode Selection Heuristic

- `DU`: fastest black/white text or cursor changes.
- `DU4`: fast limited grayscale UI changes.
- `A2`: motion/animation windows only.
- `GL16/GLR16/GLD16`: mixed content quality/perf tradeoff.
- `GC16`: highest grayscale quality; slower.
- `INIT`: recovery/cleanup only.

## Acceptance Gates

- Zero `task_wdt` events.
- Long-op warnings reduced vs baseline (or bounded and understood).
- Stable per-mode durations across repeated runs.
- No severe regressions in ghosting after partial-update tuning.
- Power profile improves for target usage pattern.

## Output Template

Return:

1. Baseline metrics table by mode.
2. Bottleneck diagnosis (driver vs automation vs network).
3. Prioritized tuning plan (top 3 actions).
4. Post-change metrics + regression check.
5. Final recommended mode policy for the target device.
