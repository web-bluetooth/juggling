/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BLE_ACCELEROMETER_SERVICE_H__
#define __BLE_ACCELEROMETER_SERVICE_H__

// these actually come from micro:bit, maybe we should change them
const uint16_t AccelServiceUuid = 0x1337;

// const uint8_t AccelServiceUuid[] = {
//     0xe9, 0x5d, 0x07, 0x53, 0x25, 0x1d, 0x47, 0x0a,
//     0xa0, 0x62, 0xfa, 0x19, 0x22, 0xdf, 0xa9, 0xa8
// };
const uint8_t AccelCharacteristicUuid[] = {
    0xe9, 0x5d, 0xca, 0x4b, 0x25, 0x1d, 0x47, 0x0a,
    0xa0, 0x62, 0xfa, 0x19, 0x22, 0xdf, 0xa9, 0xa8
};

class AccelerometerService {
public:
    AccelerometerService(BLE &_ble) :
        ble(_ble)
    {
        characteristic = new GattCharacteristic(AccelCharacteristicUuid, (uint8_t *)dataBuffer, 0,
            sizeof(dataBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

        GattCharacteristic *charTable[] = { characteristic };
        GattService buttonService(AccelServiceUuid, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));
        ble.gattServer().addService(buttonService);

        auto handle = characteristic->getValueHandle();
        ble.gattServer().write(handle, (uint8_t *)dataBuffer, sizeof(dataBuffer));
    }

    void SetData(uint16_t x, uint16_t y, uint16_t z)
    {
        dataBuffer[0] = x;
        dataBuffer[1] = y;
        dataBuffer[2] = z;

        auto handle = characteristic->getValueHandle();
        ble.gattServer().write(handle, (uint8_t *)dataBuffer, sizeof(dataBuffer));
    }

private:
    BLE &ble;
    GattCharacteristic* characteristic;
    uint16_t dataBuffer[3] = { 0x10, 0x11, 0x12 };
};

#endif /* #ifndef __BLE_ACCELEROMETER_SERVICE_H__ */
