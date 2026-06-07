#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32
#include <esp_sleep.h>
#endif

#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/time.h"
#endif

#include <cinttypes>

namespace esphome {
namespace light_sleep {

#ifdef USE_ESP32

/** The values of this enum define what should be done if light sleep is set up with a wakeup pin on the ESP32
 * and the scenario occurs that the wakeup pin is already in the wakeup state.
 */
enum WakeupPinMode {
  WAKEUP_PIN_MODE_IGNORE = 0,  ///< Ignore the fact that we will wake up when going into light sleep.
  WAKEUP_PIN_MODE_KEEP_AWAKE,  ///< As long as the wakeup pin is still in the wakeup state, keep awake.

  /** Automatically invert the wakeup level. For example if we were set up to wake up on HIGH, but the pin
   * is already high when attempting to enter light sleep, re-configure light sleep to wake up on LOW level.
   */
  WAKEUP_PIN_MODE_INVERT_WAKEUP,
};

#if defined(USE_ESP32) && !defined(USE_ESP32_VARIANT_ESP32C3)
struct Ext1Wakeup {
  uint64_t mask;
  esp_sleep_ext1_wakeup_mode_t wakeup_mode;
};
#endif

struct WakeupCauseToRunDuration {
  // Run duration if woken up by timer or any other reason besides those below.
  uint32_t default_cause;
  // Run duration if woken up by touch pads.
  uint32_t touch_cause;
  // Run duration if woken up by GPIO pins.
  uint32_t gpio_cause;
};

#endif

template<typename... Ts> class EnterLightSleepAction;

template<typename... Ts> class PreventLightSleepAction;

/** This component allows setting up the node to go into light sleep mode to conserve battery.
 *
 * To set this component up, first set *when* the light sleep should trigger using set_run_cycles
 * and set_run_duration, then set how long the light sleep should last using set_sleep_duration and optionally
 * on the ESP32 set_wakeup_pin.
 */
class LightSleepComponent : public Component {
 public:
  /// Set the duration in ms the component should sleep once it's in light sleep mode.
  void set_sleep_duration(uint32_t time_ms);
#if defined(USE_ESP32)
  /** Set the pin to wake up to on the ESP32 once it's in light sleep mode.
   * Use the inverted property to set the wakeup level.
   */
  void set_wakeup_pin(InternalGPIOPin *pin) { this->wakeup_pin_ = pin; }

  void set_wakeup_pin_mode(WakeupPinMode wakeup_pin_mode);
#endif

#if defined(USE_ESP32)
#if !defined(USE_ESP32_VARIANT_ESP32C3)

  void set_ext1_wakeup(Ext1Wakeup ext1_wakeup);

  void set_touch_wakeup(bool touch_wakeup);

#endif
  // Set the duration in ms for how long the code should run before entering
  // light sleep mode, according to the cause the ESP32 has woken.
  void set_run_duration(WakeupCauseToRunDuration wakeup_cause_to_run_duration);
#endif

  /// Set a duration in ms for how long the code should run before entering light sleep mode.
  void set_run_duration(uint32_t time_ms);

  void setup() override;
  void dump_config() override;
  void loop() override;
  // float get_loop_priority() override;
  // float get_setup_priority() override;

  /// Helper to enter light sleep mode
  void begin_sleep(bool manual = false);

  void prevent_light_sleep();
  void allow_light_sleep();

 protected:
  // Returns nullopt if no run duration is set. Otherwise, returns the run
  // duration before entering light sleep.
  optional<uint32_t> get_run_duration_() const;

  void dump_config_platform_();
  bool prepare_to_sleep_();
  void light_sleep_();

  optional<uint64_t> sleep_duration_;
#ifdef USE_ESP32
  InternalGPIOPin *wakeup_pin_;
  WakeupPinMode wakeup_pin_mode_{WAKEUP_PIN_MODE_IGNORE};

#if !defined(USE_ESP32_VARIANT_ESP32C3)
  optional<Ext1Wakeup> ext1_wakeup_;
#endif

  optional<bool> touch_wakeup_;
  optional<WakeupCauseToRunDuration> wakeup_cause_to_run_duration_;
#endif
  optional<uint32_t> run_duration_;
  bool next_enter_light_sleep_{false};
  bool prevent_{false};
};

extern bool global_has_light_sleep;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template<typename... Ts> class EnterLightSleepAction : public Action<Ts...> {
 public:
  EnterLightSleepAction(LightSleepComponent *light_sleep) : light_sleep_(light_sleep) {}
  TEMPLATABLE_VALUE(uint32_t, sleep_duration);

#ifdef USE_TIME
  void set_until(uint8_t hour, uint8_t minute, uint8_t second) {
    this->hour_ = hour;
    this->minute_ = minute;
    this->second_ = second;
  }

  void set_time(time::RealTimeClock *time) { this->time_ = time; }
#endif

  void play(Ts... x) override {
    if (this->sleep_duration_.has_value()) {
      this->light_sleep_->set_sleep_duration(this->sleep_duration_.value(x...));
    }
#ifdef USE_TIME

    if (this->hour_.has_value()) {
      auto time = this->time_->now();
      const uint32_t timestamp_now = time.timestamp;

      bool after_time = false;
      if (time.hour > this->hour_) {
        after_time = true;
      } else {
        if (time.hour == this->hour_) {
          if (time.minute > this->minute_) {
            after_time = true;
          } else {
            if (time.minute == this->minute_) {
              if (time.second > this->second_) {
                after_time = true;
              }
            }
          }
        }
      }

      time.hour = *this->hour_;
      time.minute = *this->minute_;
      time.second = *this->second_;
      time.recalc_timestamp_utc();

      time_t timestamp = time.timestamp;  // timestamp in local time zone

      if (after_time)
        timestamp += 60 * 60 * 24;

      int32_t offset = ESPTime::timezone_offset();
      timestamp -= offset;  // Change timestamp to utc
      const uint32_t ms_left = (timestamp - timestamp_now) * 1000;
      this->light_sleep_->set_sleep_duration(ms_left);
    }
#endif
    this->light_sleep_->begin_sleep(true);
  }

 protected:
  LightSleepComponent *light_sleep_;
#ifdef USE_TIME
  optional<uint8_t> hour_;
  optional<uint8_t> minute_;
  optional<uint8_t> second_;

  time::RealTimeClock *time_;
#endif
};

template<typename... Ts> class PreventLightSleepAction : public Action<Ts...>, public Parented<LightSleepComponent> {
 public:
  // void play(Ts... x) override { this->parent_->prevent_light_sleep(); }
  void play(const Ts &... x) override { this->parent_->prevent_light_sleep(); }
};

template<typename... Ts> class AllowLightSleepAction : public Action<Ts...>, public Parented<LightSleepComponent> {
 public:
  // void play(Ts... x) override { this->parent_->allow_light_sleep(); }
  void play(const Ts &... x) override { this->parent_->allow_light_sleep(); }
};

}  // namespace light_sleep
}  // namespace esphome
