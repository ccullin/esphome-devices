// In custom_components/ledc_sleep/ledc_sleep.cpp

#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "ledc_sleep.h" 
#include "esp_sleep.h"  // for testing of RTC power domain

#include <driver/ledc.h>  
#include <soc/clk_tree_defs.h> 

#ifdef USE_ESP32 

namespace esphome {
namespace ledc_sleep {

static const char *const TAG = "ledc_sleep";

// Constructor: Matches header, correctly initializes pin_
LEDC_SleepComponent::LEDC_SleepComponent(esphome::InternalGPIOPin *pin) : pin_(pin) {}

void LEDC_SleepComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LEDC Sleep output on GPIO%d...", this->pin_->get_pin());
  this->pin_->setup(); // Setup the pin as output

  const ledc_timer_t selected_ledc_timer = LEDC_TIMER_0; // Explicitly use Timer 0

  // Configuration for LEDC timer
  ledc_timer_config_t ledc_timer = {
      .speed_mode = this->speed_mode_,
      .duty_resolution = (ledc_timer_bit_t)this->resolution_,
      .timer_num = selected_ledc_timer,
      .freq_hz = (uint32_t)this->frequency_,
      .clk_cfg = this->clk_cfg_,
      .deconfigure = false, // Keep this for now, it's a specific IDF 5.x field.
  };
  esp_err_t err_timer = ledc_timer_config(&ledc_timer);
  if (err_timer != ESP_OK) {
    ESP_LOGE(TAG, "LEDC Timer config failed: %s", esp_err_to_name(err_timer));
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "LEDC Timer %d configured.", (int)selected_ledc_timer);


  ledc_channel_config_t ledc_channel = {}; // Initializes all members to zero/false

  ledc_channel.gpio_num = this->pin_->get_pin();
  ledc_channel.speed_mode = this->speed_mode_;
  ledc_channel.channel = this->channel_num_; // This is typically LEDC_CHANNEL_0 for the first output
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.timer_sel = selected_ledc_timer; // THIS MUST MATCH THE TIMER USED ABOVE!
  ledc_channel.duty = 0; // Set initial duty to 0
  ledc_channel.hpoint = 0;
  ledc_channel.flags.output_invert = 0; // Explicitly set, for consistency with native code

  ledc_channel.sleep_mode = this->sleep_mode_; // Use your class member for sleep_mode

  esp_err_t err_channel = ledc_channel_config(&ledc_channel);
  if (err_channel != ESP_OK) {
    ESP_LOGE(TAG, "LEDC Channel config failed: %s", esp_err_to_name(err_channel));
    this->mark_failed();
    return;
  }

  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_ON);

  ESP_LOGI(TAG, "LEDC Channel %d configured on GPIO%d with sleep mode %d.",
           (int)this->channel_num_, this->pin_->get_pin(), (int)this->sleep_mode_);
}

void LEDC_SleepComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LEDC Sleep Output:");
  LOG_PIN("  Pin: ", this->pin_); // Using LOG_PIN as you had it.
  ESP_LOGCONFIG(TAG, "  Frequency: %.1f Hz", this->frequency_);
  ESP_LOGCONFIG(TAG, "  Resolution: %d bits", this->resolution_);

  const char *clk_source_str = "UNKNOWN";
  switch (this->clk_cfg_) {
    case LEDC_AUTO_CLK:         clk_source_str = "AUTO_CLK"; break;
    case LEDC_USE_APB_CLK:      clk_source_str = "APB_CLK"; break;
    case LEDC_USE_RC_FAST_CLK:  clk_source_str = "RC_FAST_CLK"; break;
    case LEDC_USE_REF_TICK:     clk_source_str = "REF_TICK"; break;
    // case LEDC_USE_RTC8M_CLK:    clk_source_str = "RTC8M_CLK"; break; // REMOVED: Duplicate value with RC_FAST_CLK
    // case LEDC_USE_XTAL_CLK:     clk_source_str = "XTAL_CLK"; break; // REMOVED: Not declared in scope
    default:                    clk_source_str = "UNKNOWN/UNHANDLED"; break;
  }
  ESP_LOGCONFIG(TAG, "  Clock Source: %s", clk_source_str);

  const char *sleep_mode_str = "UNKNOWN";
  switch (this->sleep_mode_) {
    case LEDC_SLEEP_MODE_NO_ALIVE_NO_PD:    sleep_mode_str = "NO_ALIVE_NO_PD"; break;
    case LEDC_SLEEP_MODE_NO_ALIVE_ALLOW_PD: sleep_mode_str = "NO_ALIVE_ALLOW_PD"; break;
    case LEDC_SLEEP_MODE_KEEP_ALIVE:        sleep_mode_str = "KEEP_ALIVE"; break;
    case LEDC_SLEEP_MODE_INVALID:           sleep_mode_str = "INVALID"; break;
    default:                                sleep_mode_str = "UNKNOWN/UNHANDLED"; break;
  }
  ESP_LOGCONFIG(TAG, "  Sleep Mode: %s", sleep_mode_str);
}

void LEDC_SleepComponent::write_state(float state) {
  ESP_LOGV(TAG, "entering write_state (Requested state: %.2f)", state); // Log requested state
  uint32_t max_duty = (1 << this->resolution_) - 1;
  uint32_t duty = (uint32_t)(state * max_duty);

  // Ensure duty is within bounds
  if (duty > max_duty) duty = max_duty;
  // REMOVED: if (duty < 0) duty = 0; // redundant for uint32_t

  ledc_set_duty(this->speed_mode_, this->channel_num_, duty);
  ledc_update_duty(this->speed_mode_, this->channel_num_);

  ESP_LOGV(TAG, "Setting duty for GPIO%d to %.2f%% (raw: %lu / %lu)",
           this->pin_->get_pin(), state * 100.0f, duty, max_duty);
}

void LEDC_SleepComponent::loop() {
  // This component typically doesn't need a loop method as it's event-driven (write_state).
  // But it's good practice to have it as it inherits from Component.
}

} // namespace ledc_sleep
} // namespace esphome

#endif // USE_ESP32