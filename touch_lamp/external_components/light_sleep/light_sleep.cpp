#include "light_sleep.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace light_sleep {

static const char *const TAG = "light_sleep";

bool global_has_light_sleep = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

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

void LightSleepComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Light sleep:");
  if (this->sleep_duration_.has_value()) {
    uint32_t duration = *this->sleep_duration_ / 1000;
    ESP_LOGCONFIG(TAG, "  Sleep Duration: %" PRIu32 " ms", duration);
  }
  if (this->run_duration_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Run Duration: %" PRIu32 " ms", *this->run_duration_);
  }
  this->dump_config_platform_();
}

void LightSleepComponent::loop() {
  if (this->next_enter_light_sleep_)
    this->begin_sleep();
}

// float LightSleepComponent::get_loop_priority()  {
//   return -100.0f;  // run after everything else is ready
// }

void LightSleepComponent::set_sleep_duration(uint32_t time_ms) { this->sleep_duration_ = uint64_t(time_ms) * 1000; }

void LightSleepComponent::set_run_duration(uint32_t time_ms) { this->run_duration_ = time_ms; }

void LightSleepComponent::begin_sleep(bool manual) {
  if (this->prevent_ && !manual) {
    this->next_enter_light_sleep_ = true;
    return;
  }

  if (!this->prepare_to_sleep_()) {
    return;
  }

  ESP_LOGI(TAG, "Beginning sleep");
  if (this->sleep_duration_.has_value()) {
    ESP_LOGI(TAG, "Sleeping for %" PRId64 "us", *this->sleep_duration_);
  }
  App.run_safe_shutdown_hooks();

  this->light_sleep_();
}

// float LightSleepComponent::get_setup_priority() { return setup_priority::LATE; }

void LightSleepComponent::prevent_light_sleep() { this->prevent_ = true; }

void LightSleepComponent::allow_light_sleep() { this->prevent_ = false; }

}  // namespace light_sleep
}  // namespace esphome
