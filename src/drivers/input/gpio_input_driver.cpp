#include "gpio_input_driver.hpp"

#include <drivers/input/gpio_input_driver.hpp>

#include "InputConfig_types.h"
#include "ulog.h"
#include "zcbor_helpers.h"

namespace xbot::driver::input {

Input* GpioInputDriver::AddInput(const InputConfig& input_config) {
  ZCBOR_REQUIRE_PRESENT(input_config, line);
  GpioInput& input = inputs_.emplace_back();
  input.name = to_string_view(input_config.name);
  input.line = input.line;
  return &input;
}

bool GpioInputDriver::OnStart() {
  for (auto& input : inputs_) {
    palSetLineMode(input.line, PAL_MODE_INPUT);
  }
  return true;
}

void GpioInputDriver::tick() {
  for (auto& input : inputs_) {
    input.Update(palReadLine(input.line) ^ input.invert);
  }
}

}  // namespace xbot::driver::input
