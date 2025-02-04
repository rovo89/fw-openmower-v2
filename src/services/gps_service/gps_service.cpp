#include "gps_service.hpp"

#include <drivers/gps/nmea_gps_driver.h>
#include <drivers/gps/ublox_gps_driver.h>

void GpsService::OnCreate() {
}

bool GpsService::Configure() {
  if (strncmp(Protocol.value, "UBX", Protocol.length) == 0) {
    protocol_ = UBX;
  } else if (strncmp(Protocol.value, "NMEA", Protocol.length) == 0) {
    protocol_ = NMEA;
  } else {
    return false;
  }
  return true;
}

void GpsService::OnStart() {
  using namespace xbot::driver::gps;
  if (protocol_ == UBX) {
    gps_driver_ = new UbxGpsDriver();
  } else {
    gps_driver_ = new NmeaGpsDriver();
  }

  gps_driver_->SetStateCallback(
      etl::delegate<void(const GpsDriver::GpsState&)>::create<GpsService, &GpsService::GpsStateCallback>(*this));

  // TODO: Make port configurable.
  gps_driver_->StartDriver(&UARTD8, Baudrate.value);
}

void GpsService::OnStop() {
  delete gps_driver_;
}

void GpsService::tick() {
}

bool GpsService::OnRTCMChanged(const uint8_t* new_value, uint32_t length) {
  gps_driver_->SendRTCM(new_value, length);
  return true;
}

void GpsService::GpsStateCallback(const GpsDriver::GpsState& state) {
  StartTransaction();
  double position[3] = {state.pos_lat, state.pos_lon, state.pos_height};
  SendPosition(position, 3);
  SendPositionHorizontalAccuracy(state.position_h_accuracy);
  SendPositionVerticalAccuracy(state.position_v_accuracy);
  CommitTransaction();
}
