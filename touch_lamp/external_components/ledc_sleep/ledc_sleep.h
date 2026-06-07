// In custom_components/ledc_sleep/ledc_sleep.h

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h" // For GPIOPin and InternalGPIOPin
#include "esphome/components/output/float_output.h"
#include "driver/ledc.h"         // Defines ledc_clk_cfg_t, ledc_sleep_mode_t, etc.
#include "soc/clk_tree_defs.h" // For LEDC_AUTO_CLK etc.

namespace esphome {
namespace ledc_sleep {

class LEDC_SleepComponent : public output::FloatOutput, public Component {
 public:
  // Constructor: Correctly initialized member
  explicit LEDC_SleepComponent(esphome::InternalGPIOPin *pin);

  // Setters
  void set_frequency(float frequency) { this->frequency_ = frequency; }
  void set_resolution(int resolution) { this->resolution_ = resolution; }
  void set_clk_cfg(ledc_clk_cfg_t clk_cfg) { this->clk_cfg_ = clk_cfg; }
  void set_sleep_mode(ledc_sleep_mode_t sleep_mode) { this->sleep_mode_ = sleep_mode; }

  void write_state(float state) override;
  void setup() override;
  void dump_config() override;
  void loop() override; // <--- ADDED: Empty loop method

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  esphome::InternalGPIOPin *pin_; // <--- Initialized in constructor
  float frequency_;
  int resolution_;
  ledc_clk_cfg_t clk_cfg_;
  ledc_sleep_mode_t sleep_mode_;

  ledc_mode_t speed_mode_{LEDC_LOW_SPEED_MODE};
  ledc_timer_t timer_num_{LEDC_TIMER_0};
  ledc_channel_t channel_num_{LEDC_CHANNEL_0};
};

} // namespace ledc_sleep
} // namespace esphome