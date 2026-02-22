# IT8951E (M5Paper / IT8951)

ESPHome external component for the M5Paper IT8951 panel.

## Display platform

```yaml
spi:
  clk_pin: GPIO14
  mosi_pin: GPIO12
  miso_pin: GPIO13

display:
  - platform: it8951e
    id: m5paper_display
    model: M5EPD
    cs_pin: GPIO15
    reset_pin: GPIO23
    reset_duration: 100ms
    busy_pin: GPIO27
    rotation: 0
    reversed: false
    update_interval: never
    lambda: |-
      it.printf(20, 20, id(my_font), "Hello IT8951E");
```

`spi_id` is optional (provided by `spi.spi_device_schema()`), so both styles are accepted:

```yaml
display:
  - platform: it8951e
    spi_id: my_spi_bus
    cs_pin: GPIO15
    reset_pin: GPIO23
    busy_pin: GPIO27
```

## Actions

- `it8951e.clear`
- `it8951e.draw` (DU update)
- `it8951e.updateslow` (GC16 update)
- `it8951e.update` (explicit mode)

Explicit update action:

```yaml
- component.update: m5paper_display
- it8951e.update:
    id: m5paper_display
    mode: GL16
    full: false
```

Supported `mode` values:

- `INIT`
- `DU`
- `GC16`
- `GL16`
- `GLR16`
- `GLD16`
- `DU4`
- `A2`
- `NONE`
