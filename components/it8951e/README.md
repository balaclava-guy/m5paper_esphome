# it8951e (M5Paper / IT8951)

This is an ESPHome external component implementing an IT8951-based e-paper display
(M5Paper "M5EPD" panel class).

## Display platform

Example:

```yaml
spi:
  clk_pin: GPIO14
  mosi_pin: GPIO12
  miso_pin: GPIO13

display:
  - platform: it8951e
    id: epaper_display
    cs_pin: GPIO15
    reset_pin: GPIO23
    busy_pin: GPIO27
    rotation: 0
    reversed: false
    reset_duration: 200ms
    update_interval: never
    lambda: |-
      it.fill(Color(0,0,0));

