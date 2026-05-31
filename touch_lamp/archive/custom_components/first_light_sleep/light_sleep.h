#ifndef ESPHOME_LIGHT_SLEEP_COMPONENT_H__
#define ESPHOME_LIGHT_SLEEP_COMPONENT_H__

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h" // For InternalGPIOPin
#include "esphome/core/helpers.h" // For optional

#ifdef USE_ESP32 // Ensure these are only compiled for ESP32
#include <esp_sleep.h>
#endif

namespace esphome {
namespace light_sleep {

#ifdef USE_ESP32

/** The values of this enum define what should be done if light sleep is set up with a wakeup pin on the ESP32
 * and the scenario occurs that the wakeup pin is already in the wakeup state.
 */
enum WakeupPinMode {
  WAKEUP_PIN_MODE_IGNORE = 0,  ///< Ignore the fact that we will wake up when going into deep sleep.
  WAKEUP_PIN_MODE_KEEP_AWAKE,  ///< As long as the wakeup pin is still in the wakeup state, keep awake.

  /** Automatically invert the wakeup level. For example if we were set up to wake up on HIGH, but the pin
   * is already high when attempting to enter deep sleep, re-configure deep sleep to wake up on LOW level.
   */
  WAKEUP_PIN_MODE_INVERT_WAKEUP,
};

#if defined(USE_ESP32) && !defined(USE_ESP32_VARIANT_ESP32C3)
struct Ext1Wakeup {
  uint64_t mask;
  esp_sleep_ext1_wakeup_mode_t wakeup_mode;
};
#endif // USE_ESP32

struct WakeupCauseToRunDuration {
  // Run duration if woken up by timer or any other reason besides those below.
  uint32_t default_cause;
  // Run duration if woken up by touch pads.
  uint32_t touch_cause;
  // Run duration if woken up by GPIO pins.
  uint32_t gpio_cause;
};

template<typename... Ts> class EnterLightSleepAction;

template<typename... Ts> class PreventDeepSleepAction;

/** This component allows setting up the node to go into light sleep mode to conserve battery.
 */
class LightSleepComponent : public Component {
 public:
  // Set the duration in ms the component should sleep. Converted to us internally.
  void set_sleep_duration(uint32_t time_ms);
#ifdef USE_ESP32
  /** Set the pin to wake up to on the ESP32 once it's in light sleep mode.
   * Use the inverted property to set the wakeup level.
   */
  void set_wakeup_pin(InternalGPIOPin *pin) { this->wakeup_pin_ = pin; }
  void set_wakeup_pin_mode(WakeupPinMode wakeup_pin_mode);

  #if !defined(USE_ESP32_VARIANT_ESP32C3)
    void set_ext1_wakeup(Ext1Wakeup ext1_wakeup);
    void set_touch_wakeup(bool touch_wakeup);
  #endif

#endif

  void setup() override;
  void dump_config() override;
  void loop() override {} // Light sleep components typically don't need a loop
  float get_setup_priority() const override { return setup_priority::AFTER_BLUETOOTH; } // Ensure peripherals are set up

  /// Helper to enter light sleep mode
  void begin_sleep(bool manual = false);
  
  void prevent_light_sleep();
  void allow_light_sleep();

 protected:
  optional<uint64_t> sleep_duration_; // Stored in microseconds internally
#ifdef USE_ESP32
  InternalGPIOPin *wakeup_pin_{nullptr};
  WakeupPinMode wakeup_pin_mode_{WAKEUP_PIN_MODE_IGNORE};
  #if !defined(USE_ESP32_VARIANT_ESP32C3)
    optional<Ext1Wakeup> ext1_wakeup_;
  #endif

  optional<bool> touch_wakeup_;
#endif
};

// --- Automation Action ---
template<typename... Ts> class EnterLightSleepAction : public Action<Ts...> {
 public:
  EnterLightSleepAction(LightSleepComponent *light_sleep) : light_sleep_(light_sleep) {}
  // Optional override for sleep_duration when the action is called
  TEMPLATABLE_VALUE(uint32_t, sleep_duration_ms);

  void play(Ts... x) override {
    // If sleep_duration is provided in the action, set it dynamically
    if (this->sleep_duration_ms_.has_value()) {
      this->light_sleep_->set_sleep_duration(this->sleep_duration_ms_.value(x...));
    }
    this->light_sleep_->begin_sleep(true); // Trigger sleep
  }

 protected:
  optional<uint32_t> get_run_duration_() const;

  void dump_config_platform_();
  bool prepare_to_sleep_();
  void deep_sleep_();

  optional<uint64_t> sleep_duration_;
  #ifdef USE_ESP32
  InternalGPIOPin *wakeup_pin_;
  WakeupPinMode wakeup_pin_mode_{WAKEUP_PIN_MODE_IGNORE};
  optional<bool> touch_wakeup_;
  optional<WakeupCauseToRunDuration> wakeup_cause_to_run_duration_;
  #endif
  
  optional<uint32_t> run_duration_;
  bool next_enter_deep_sleep_{false};
  bool prevent_{false};

  LightSleepComponent *light_sleep_;
};

}  // namespace light_sleep
}  // namespace esphome

#endif // ESPHOME_LIGHT_SLEEP_COMPONENT_H__