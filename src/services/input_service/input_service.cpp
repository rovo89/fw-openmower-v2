#include "input_service.hpp"

#include <etl/string.h>

#include <xbot-service/Lock.hpp>

#include "globals.hpp"
#include "ulog.h"
#include "zcbor_helpers.h"

using xbot::service::Lock;

bool InputService::OnStart() {
  // Start drivers.
  for (auto& driver : drivers_) {
    if (!driver.second->OnStart()) {
      ULOG_ERROR("Failed to start input driver %d", driver.first);
      return false;
    }
  }

  // FIXME: This should be read from the config.
  const InputConfig* input_configs = nullptr;

  // Parse input configurations and pass them to the drivers.
  Lock lk(&mutex);
  all_inputs_.clear();
  for (size_t i = 0; i < 1; i++) {
    auto& input_config = input_configs[i];
    auto it = drivers_.find(input_config.driver_choice);
    if (it == drivers_.end()) {
      ULOG_ERROR("Unknown input driver %d for %s", input_config.driver_choice, to_string_view(input_config.name));
      continue;
    }
    Input* input = it->second->AddInput(input_config);
    all_inputs_.push_back(input);
  }

  return true;
}

void InputService::tick() {
  // TODO: Check if these can run at the same frequency.
  for (auto& driver : drivers_) {
    driver.second->tick();
  }
}
