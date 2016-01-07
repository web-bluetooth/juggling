#include "mbed-drivers/mbed.h"
#include "minar/minar.h"
#include "core-util/FunctionPointer.h"
#include "ble/BLE.h"
#include "AccelerometerService.h"
#include "Accelerometer.h"

using namespace mbed::util;

// so this is micro:bit specific. grid stuff.
static DigitalOut led_col_1(YOTTA_CFG_HARDWARE_PINS_COL1);
static DigitalOut led_row_1(YOTTA_CFG_HARDWARE_PINS_ROW1);

const static char DEVICE_NAME[] = "Juggler";
static const uint16_t uuid16_list[] = { 0x1337 };
static AccelerometerService* accelService;
static Accelerometer* accelerometer;

static void blinky_user(void) {
    led_row_1 = !led_row_1;
}

static void blinky_irq(void) {
    minar::Scheduler::postCallback(FunctionPointer0<void>(&blinky_user).bind());
}

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    // hmm.. is there UNUSED() in mbed?

    BLE::Instance().gap().startAdvertising(); // restart advertising
}

void onBleInitError(BLE &ble, ble_error_t error)
{
    (void)ble;
    (void)error;
   /* Initialization error handling should go here */
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    // blinky when BLE init is completed
    minar::Scheduler::postCallback(blinky_irq).period(minar::milliseconds(500));

    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        onBleInitError(ble, error);
        return;
    }

    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    ble.gap().onDisconnection(disconnectionCallback);

    // set up the bluetooth service, and init the accelerometer hardware
    accelService = new AccelerometerService(ble);
    // accelerometer shouldnt depend on the BLE service, need to make this evented
    accelerometer = new Accelerometer(accelService);

    /* Setup advertising. */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms */
    ble.gap().startAdvertising();
}

// hmm... how to do i2c

void app_start(int, char**) {
    // col1=false, row1=true to turn on led at (0,0)
    led_col_1 = false;

    // init the BLE stack
    BLE::Instance().init(bleInitComplete);
}
