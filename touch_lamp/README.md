# ESPHome customisation to support LEDC Light Sleep

## expressif esp-idf framework

version 5.5.0 is required for LEDC support of the RTC compatible clock and ledc sleep mode. as of June 2025 this was not available in the defaults version of esphome.

esphome:
  name: touch_lamp
  platformio_options:
    platform:
      - https://github.com/platformio/platform-espressif32.git
    platform_packages:
      - pioarduino/framework-espidf@https://github.com/pioarduino/esp-idf/releases/download/v5.5.0.20250510/esp-idf-v5.5.0.zip

## required Custom (external) Components 

two custom elements are required
### LEDC_SLEEP
LEDC_SLEEP is an extended version of LEDC and is a backward compatible replacement with the addition of Frequency and Resolution feature exposure for LED brightness configurations, and the additoion of CLK_CFG and SLEEP_MODE to enable LEDC to continue to function in light_sleep mode.

example of an Output Component configuration.
output:
  - platform: ledc_sleep # <-- Your new custom output platform name
    id: my_custom_led_output # unique name referenced in automations
    pin: GPIO23
    frequency: 500Hz
    resolution: 10
    clk_cfg: RC_FAST_CLK # RC_FAST_CLK for LIGHT_SLEEP
    sleep_mode: KEEP_ALIVE # KEEP_ALIVE for LIGHT_SLEEP

### LIGHT_SLEEP
lIGHT_SLEEP is similiar to DEEP_SLEEP, but specific system peripherals can continue to run, and the system continues to run from where it left off when light_sleep was started.

The custom Light_Sleep mirrors that of Deep_Sleep and has been designed to operate on the ESP32 and inconjunction with LEDC_SLEEP.  however the funcational is much broader than this but not extensively tested beyond this.

**example 1 of light sleep compoment configuration:**

light_sleep:
  id: my_light_sleep_config
  touch_wakeup: true

  **example 2 of light sleep compoment configuration:**

light_sleep:
  id: my_light_sleep_config
    wakeup_pin:
    number: GPIO33
    allow_other_uses: true
