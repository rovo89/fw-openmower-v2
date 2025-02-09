//
// Created by clemens on 31.07.24.
//

#include "imu_service.hpp"

#include <lsm6ds3tr-c_reg.h>

static SPIConfig spi_config = {
    false,
    false,
    nullptr,
    nullptr,
    LINE_IMU_CS,
    SPI_CFG1_MBR_DIV32 | SPI_CFG1_DSIZE_0 | SPI_CFG1_DSIZE_1 | SPI_CFG1_DSIZE_2,
    SPI_CFG2_COMM_FULL_DUPLEX | SPI_CFG2_CPOL | SPI_CFG2_CPHA,
};

static stmdev_ctx_t dev_ctx{};

bool ImuService::Configure() {
  return true;
}
void ImuService::OnStart() {
}
void ImuService::OnStop() {
}
void ImuService::OnCreate() {
  // Acquire Bus and never let it go, there's only the one IMU connected to it.
  spiAcquireBus(&SPID_IMU);
  spiStart(&SPID_IMU, &spi_config);

  dev_ctx.write_reg =
      [](void *, uint8_t reg, const uint8_t *bufp, uint16_t len) {
        spiSelect(&SPID_IMU);
        spiSend(&SPID_IMU, 1, &reg);
        spiSend(&SPID_IMU, len, bufp);
        spiUnselect(&SPID_IMU);
        return (int32_t)0;
      },
  dev_ctx.read_reg = [](void *, uint8_t reg, uint8_t *bufp, uint16_t len) {
    reg |= 0x80;

    spiSelect(&SPID_IMU);
    spiSend(&SPID_IMU, 1, &reg);
    spiReceive(&SPID_IMU, len, bufp);
    spiUnselect(&SPID_IMU);
    return (int32_t)0;
  };
  uint8_t whoamI = 0;
  lsm6ds3tr_c_device_id_get(&dev_ctx, &whoamI);

  if (whoamI == 0x6a) {
    imu_found = true;
  } else {
    imu_found = false;
    return;
  }

  /* Restore default configuration */
  lsm6ds3tr_c_reset_set(&dev_ctx, PROPERTY_ENABLE);

  uint8_t rst = 0;
  int rst_count = 0;
  do {
    lsm6ds3tr_c_reset_get(&dev_ctx, &rst);
    rst_count++;
  } while (rst);

  /* Enable Block Data Update */
  lsm6ds3tr_c_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  /* Set Output Data Rate */
  lsm6ds3tr_c_xl_data_rate_set(&dev_ctx, LSM6DS3TR_C_XL_ODR_833Hz);
  lsm6ds3tr_c_gy_data_rate_set(&dev_ctx, LSM6DS3TR_C_GY_ODR_833Hz);
  /* Set full scale */
  lsm6ds3tr_c_xl_full_scale_set(&dev_ctx, LSM6DS3TR_C_2g);
  lsm6ds3tr_c_gy_full_scale_set(&dev_ctx, LSM6DS3TR_C_2000dps);
  /* Configure filtering chain(No aux interface) */
  /* Accelerometer - analog filter */
  lsm6ds3tr_c_xl_filter_analog_set(&dev_ctx, LSM6DS3TR_C_XL_ANA_BW_400Hz);
  /* Accelerometer - LPF1 path ( LPF2 not used )*/
  // lsm6ds3tr_c_xl_lp1_bandwidth_set(&dev_ctx, LSM6DS3TR_C_XL_LP1_ODR_DIV_4);
  /* Accelerometer - LPF1 + LPF2 path */
  lsm6ds3tr_c_xl_lp2_bandwidth_set(&dev_ctx, LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_100);
}
void ImuService::tick() {
  if (!imu_found) {
    return;
  }
  lsm6ds3tr_c_reg_t reg;
  lsm6ds3tr_c_status_reg_get(&dev_ctx, &reg.status_reg);

  if (reg.status_reg.xlda) {
    /* Read magnetic field data */
    memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
    lsm6ds3tr_c_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
    axes[0] = lsm6ds3tr_c_from_fs2g_to_mg(data_raw_acceleration[0]) / 1000.0;
    axes[1] = -lsm6ds3tr_c_from_fs2g_to_mg(data_raw_acceleration[1]) / 1000.0;
    axes[2] = -lsm6ds3tr_c_from_fs2g_to_mg(data_raw_acceleration[2]) / 1000.0;
  }

  if (reg.status_reg.gda) {
    /* Read magnetic field data */
    memset(data_raw_angular_rate, 0x00, 3 * sizeof(int16_t));
    lsm6ds3tr_c_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate);
    axes[3] = M_PI * lsm6ds3tr_c_from_fs2000dps_to_mdps(data_raw_angular_rate[0]) / 180000.0;
    axes[4] = -M_PI * lsm6ds3tr_c_from_fs2000dps_to_mdps(data_raw_angular_rate[1]) / 180000.0;
    axes[5] = -M_PI * lsm6ds3tr_c_from_fs2000dps_to_mdps(data_raw_angular_rate[2]) / 180000.0;
  }

  /*if (reg.status_reg.tda) {
    // Read temperature data
    memset(&data_raw_temperature, 0x00, sizeof(int16_t));
    lsm6ds3tr_c_temperature_raw_get(&dev_ctx, &data_raw_temperature);
    temperature_degC = lsm6ds3tr_c_from_lsb_to_celsius(
                         data_raw_temperature );
  }*/

  SendAxes(axes, 9);
}
