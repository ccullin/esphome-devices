#ifdef USE_ESP32
#include "light_sleep.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h" // For App.delay()

namespace esphome {
namespace light_sleep {

static const char *const TAG = "light_sleep";
bool global_has_light_sleep = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)


void LightSleepComponent::set_wakeup_pin_mode(WakeupPinMode wakeup_pin_mode) {
  this->wakeup_pin_mode_ = wakeup_pin_mode;
}

#if !defined(USE_ESP32_VARIANT_ESP32C3) && !defined(USE_ESP32_VARIANT_ESP32C6)
void LightSleepComponent::set_ext1_wakeup(Ext1Wakeup ext1_wakeup) {
    this->ext1_wakeup_ = ext1_wakeup;
}

#if !defined(USE_ESP32_VARIANT_ESP32H2)
void LightSleepComponent::set_touch_wakeup(bool touch_wakeup) {
    this->touch_wakeup_ = touch_wakeup;
}
#endif

#endif

void LightSleepComponent::set_sleep_duration(uint32_t time_ms) { 
    this->sleep_duration_ = uint64_t(time_ms) * 1000; 
}


void LightSleepComponent::setup() {
  ESP_LOGCONFIG(TAG, "Running setup");
  global_has_light_sleep = true;

  const optional<uint32_t> run_duration = get_run_duration_();
  if (run_duration.has_value()) {
    ESP_LOGI(TAG, "Scheduling in %" PRIu32 " ms", *run_duration);
    this->set_timeout(*run_duration, [this]() { this->begin_sleep(); });
  } else {
    ESP_LOGD(TAG, "Not scheduling; no run duration configured");
  }
}

void LightSleepComponent::set_run_duration(WakeupCauseToRunDuration wakeup_cause_to_run_duration) {
  wakeup_cause_to_run_duration_ = wakeup_cause_to_run_duration;
}

void LighSleepComponent::dump_config_platform_() {
  if (wakeup_pin_ != nullptr) {
    LOG_PIN("  Wakeup Pin: ", this->wakeup_pin_);
  }
  if (this->wakeup_cause_to_run_duration_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Default Wakeup Run Duration: %" PRIu32 " ms",
                  this->wakeup_cause_to_run_duration_->default_cause);
    ESP_LOGCONFIG(TAG, "  Touch Wakeup Run Duration: %" PRIu32 " ms", this->wakeup_cause_to_run_duration_->touch_cause);
    ESP_LOGCONFIG(TAG, "  GPIO Wakeup Run Duration: %" PRIu32 " ms", this->wakeup_cause_to_run_duration_->gpio_cause);
  }
}

bool LightSleepComponent::prepare_to_sleep_() {
  if (this->wakeup_pin_mode_ == WAKEUP_PIN_MODE_KEEP_AWAKE && this->wakeup_pin_ != nullptr &&
      this->wakeup_pin_->digital_read()) {
    // Defer light sleep until inactive
    if (!this->next_enter_light_sleep_) {
      this->status_set_warning();
      ESP_LOGW(TAG, "Waiting for wakeup pin state change");
    }
    this->next_enter_light_sleep_ = true;
    return false;
  }
  return true;
}

void LightSleepComponent::light_sleep_() {
#if !defined(USE_ESP32_VARIANT_ESP32C3) && !defined(USE_ESP32_VARIANT_ESP32C6) && !defined(USE_ESP32_VARIANT_ESP32H2)
  if (this->sleep_duration_.has_value())
    esp_sleep_enable_timer_wakeup(*this->sleep_duration_);
  if (this->wakeup_pin_ != nullptr) {
    bool level = !this->wakeup_pin_->is_inverted();
    if (this->wakeup_pin_mode_ == WAKEUP_PIN_MODE_INVERT_WAKEUP && this->wakeup_pin_->digital_read()) {
      level = !level;
    }
    esp_sleep_enable_ext0_wakeup(gpio_num_t(this->wakeup_pin_->get_pin()), level);
  }
  if (this->ext1_wakeup_.has_value()) {
    esp_sleep_enable_ext1_wakeup(this->ext1_wakeup_->mask, this->ext1_wakeup_->wakeup_mode);
  }

  if (this->touch_wakeup_.has_value() && *(this->touch_wakeup_)) {
    esp_sleep_enable_touchpad_wakeup();
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  }
#endif
  esp_light_sleep_start();
}

void LightSleepComponent::being_sleep(bool manual) {
    ESP_LOGD(TAG, "Preparing to enter light sleep...");

    if (this->prevent_ && !manual) {
        this->next_enter_light_sleep_ = true;
        return;
    }

    if (!this->prepare_to_sleep_()) {
        return;
    }


    ESP_LOGI(TAG, "Beginning sleep");
    // Clear any pending wakeups from previous cycles to ensure clean state
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    // 1. Configure Wakeup Sources
    // Timer Wakeup
    if (this->sleep_duration_.has_value()) {
        esp_sleep_enable_timer_wakeup(*this->sleep_duration_);
        ESP_LOGD(TAG, "Timer wakeup enabled for %llu us.", *this->sleep_duration_);
    }
    App.run_safe_shutdown_hooks();

    this->light_sleep_();
}

void LightSleepComponent::prevent_light_sleep() {
    this->prevent_ = true;
}

void LightSleepComponent::allow_light_sleep() {
    this->prevent_ = false;
}


} // namespace light_sleep
} // namespace esphome

#endif