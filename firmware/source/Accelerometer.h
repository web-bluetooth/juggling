#include "mbed-drivers/mbed.h"
#include "mbed-drivers/I2C.h"

#ifndef __ACCELEROMETER_H__
#define __ACCELEROMETER_H__

/**
 * Relevant pin assignments
 */
#define MICROBIT_PIN_ACCEL_DATA_READY          P0_28

/*
 * I2C constants
 */
#define MMA8653_DEFAULT_ADDR    0x3A

/*
 * MMA8653 Register map (partial)
 */
#define MMA8653_STATUS          0x00
#define MMA8653_OUT_X_MSB       0x01
#define MMA8653_WHOAMI          0x0D
#define MMA8653_XYZ_DATA_CFG    0x0E
#define MMA8653_CTRL_REG1       0x2A
#define MMA8653_CTRL_REG2       0x2B
#define MMA8653_CTRL_REG3       0x2C
#define MMA8653_CTRL_REG4       0x2D
#define MMA8653_CTRL_REG5       0x2E


/**
 * MMA8653 constants
 */
#define MMA8653_WHOAMI_VAL      0x5A

#define MMA8653_SAMPLE_RANGES   3
#define MMA8653_SAMPLE_RATES    8

/*
 * Accelerometer events
 */
#define MICROBIT_ACCELEROMETER_EVT_DATA_UPDATE              1

/*
 * Gesture events
 */
#define MICROBIT_ACCELEROMETER_EVT_TILT_UP                  1
#define MICROBIT_ACCELEROMETER_EVT_TILT_DOWN                2
#define MICROBIT_ACCELEROMETER_EVT_TILT_LEFT                3
#define MICROBIT_ACCELEROMETER_EVT_TILT_RIGHT               4
#define MICROBIT_ACCELEROMETER_EVT_FACE_UP                  5
#define MICROBIT_ACCELEROMETER_EVT_FACE_DOWN                6
#define MICROBIT_ACCELEROMETER_EVT_FREEFALL                 7
#define MICROBIT_ACCELEROMETER_EVT_3G                       8
#define MICROBIT_ACCELEROMETER_EVT_6G                       9
#define MICROBIT_ACCELEROMETER_EVT_8G                       10
#define MICROBIT_ACCELEROMETER_EVT_SHAKE                    11

/*
 * Gesture recogniser constants
 */
#define MICROBIT_ACCELEROMETER_REST_TOLERANCE               200
#define MICROBIT_ACCELEROMETER_TILT_TOLERANCE               200
#define MICROBIT_ACCELEROMETER_FREEFALL_TOLERANCE           400
#define MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE              1000
#define MICROBIT_ACCELEROMETER_3G_TOLERANCE                 3072
#define MICROBIT_ACCELEROMETER_6G_TOLERANCE                 6144
#define MICROBIT_ACCELEROMETER_8G_TOLERANCE                 8192
#define MICROBIT_ACCELEROMETER_GESTURE_DAMPING              10
#define MICROBIT_ACCELEROMETER_SHAKE_DAMPING                10

#define MICROBIT_ACCELEROMETER_REST_THRESHOLD               (MICROBIT_ACCELEROMETER_REST_TOLERANCE * MICROBIT_ACCELEROMETER_REST_TOLERANCE)
#define MICROBIT_ACCELEROMETER_FREEFALL_THRESHOLD           (MICROBIT_ACCELEROMETER_FREEFALL_TOLERANCE * MICROBIT_ACCELEROMETER_FREEFALL_TOLERANCE)
#define MICROBIT_ACCELEROMETER_3G_THRESHOLD                 (MICROBIT_ACCELEROMETER_3G_TOLERANCE * MICROBIT_ACCELEROMETER_3G_TOLERANCE)
#define MICROBIT_ACCELEROMETER_6G_THRESHOLD                 (MICROBIT_ACCELEROMETER_6G_TOLERANCE * MICROBIT_ACCELEROMETER_6G_TOLERANCE)
#define MICROBIT_ACCELEROMETER_8G_THRESHOLD                 (MICROBIT_ACCELEROMETER_8G_TOLERANCE * MICROBIT_ACCELEROMETER_8G_TOLERANCE)
#define MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD        4

#define MICROBIT_OK             0
#define MICROBIT_I2C_ERROR      99

class Accelerometer {
public:
    Accelerometer(AccelerometerService* aAccelService)
        : accelService(aAccelService)
    {
        i2c = new I2C(YOTTA_CFG_HARDWARE_PINS_I2C_SDA0, YOTTA_CFG_HARDWARE_PINS_I2C_SCL0);

        uint8_t buffer[1];
        if (read(MMA8653_WHOAMI, buffer, 1) != MICROBIT_OK) {
            // printf("WHOAMI command failed\r\n");
            return;
        }
        if (buffer[0] != MMA8653_WHOAMI_VAL) {
            // printf("WHOAMI returned wrong value, was %x\r\n", buffer[0]);
            return;
        }
        // printf("Accelerometer identified successfully!\r\n");

        // Now configure the accelerometer accordingly.
        // First place the device into standby mode, so it can be configured.
        if (write(MMA8653_CTRL_REG1, 0x00) != MICROBIT_OK) {
            // printf("MMA8653_CTRL_REG1 Write failed\r\n");
            return;
        }

        // Enable high precisiosn mode. This consumes a bit more power, but still only 184 uA!
        if (write(MMA8653_CTRL_REG2, 0x10) != MICROBIT_OK) {
            // printf("MMA8653_CTRL_REG2 Write failed\r\n");
            return;
        }

        // Enable the INT1 interrupt pin.
        if (write(MMA8653_CTRL_REG4, 0x01) != MICROBIT_OK) {
            // printf("MMA8653_CTRL_REG4 Write failed\r\n");
            return;
        }

        // Select the DATA_READY event source to be routed to INT1
        if (write(MMA8653_CTRL_REG5, 0x01) != MICROBIT_OK) {
            // printf("MMA8653_CTRL_REG5 Write failed\r\n");
            return;
        }

        // Configure for the selected g range.
        if (write(MMA8653_XYZ_DATA_CFG, 0x01) != MICROBIT_OK) {
            // printf("MMA8653_XYZ_DATA_CFG Write failed\r\n");
            return;
        }

        // Bring the device back online, with 10bit wide samples at the requested frequency.
        if (write(MMA8653_CTRL_REG1, 0x18 | 0x01) != MICROBIT_OK) {
            // printf("MMA8653_CTRL_REG1 Write failed\r\n");
            return;
        }

        // printf("Configuration has been written. Accelerometer is ready!\r\n");

        minar::Scheduler::postCallback(
            mbed::util::FunctionPointer0<void>(this, &Accelerometer::read_accel_irq).bind()
        ).period(minar::milliseconds(1000 / 60));
    }

private:
    void read_accel_irq(void)
    {
        // probably not safe but who gives a...
        int8_t data[6];
        // this cast feels very unsafe, but i stole it from microbit code
        // and those guys actually went to uni, so let's just keep it
        if (read(MMA8653_OUT_X_MSB, (uint8_t*)data, 6) != MICROBIT_OK) {
            // printf("MMA8653_OUT_X_MSB read failed\r\n");
            return;
        }

        int16_t x, y, z;

        // read MSB values...
        x = data[0];
        y = data[2];
        z = data[4];

        // Normalize the data in the 0..1024 range.
        x *= 8;
        y *= 8;
        z *= 8;

        // Invert the x and y axes, so that the reference frame aligns with micro:bit expectations
        x = -x;
        y = -y;

        // Add in LSB values.
        x += (data[1] / 64);
        y += (data[3] / 64);
        z += (data[5] / 64);

        // Scale into millig (approx!)
        x *= 4;
        y *= 4;
        z *= 4;

        // // printf("Accel data %d %d %d\r\n", x, y, z);

        // update BLE data
        accelService->SetData(x, y, z);
    }

    int read(uint8_t reg, uint8_t* buffer, int length)
    {
        char regBuffer[1] = { reg };
        if (i2c->write(MMA8653_DEFAULT_ADDR, regBuffer, 1, true) != 0) {
            return MICROBIT_I2C_ERROR;
        }
        if (i2c->read(MMA8653_DEFAULT_ADDR, (char*)buffer, length) != 0) {
            return MICROBIT_I2C_ERROR;
        }
        return MICROBIT_OK;
    }

    int write(uint8_t reg, uint8_t value)
    {
        char regBuffer[2] = { reg, value };
        if (i2c->write(MMA8653_DEFAULT_ADDR, regBuffer, 2, true) != 0) {
            return MICROBIT_I2C_ERROR;
        }
        return MICROBIT_OK;
    }

    I2C* i2c;
    AccelerometerService* accelService;
};

#endif /* #ifndef __ACCELEROMETER_H__ */
