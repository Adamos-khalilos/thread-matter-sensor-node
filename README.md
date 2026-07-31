# Thread/Matter Sensor Node

A low-power, secure IoT sensor node built on ESP32-C6, FreeRTOS, and the
Matter-over-Thread protocol stack. Part of an embedded systems portfolio
project — see the full architecture write-up in `docs/` (add your notes
there as you go).

## Stack

- **MCU:** ESP32-C6 (native 802.15.4 + Wi-Fi + BLE)
- **RTOS:** FreeRTOS (via ESP-IDF)
- **Protocol:** Thread (network layer) + Matter (application layer)
- **SDK:** ESP-IDF v5.5.x + ESP-Matter
- **Sensor:** BME280 / SHT40 (I2C)

## Project Structure

```
thread-matter-sensor-node/
├── CMakeLists.txt              # top-level build config
├── main/
│   ├── CMakeLists.txt
│   ├── app_main.c              # creates tasks, wires up queues
│   ├── common/
│   │   └── app_queues.h        # shared queue handles + message types
│   └── tasks/
│       ├── sensor_task.c/.h    # samples the sensor, sleeps between reads
│       ├── radio_task.c/.h     # handles Matter/Thread comms
│       └── power_task.c/.h     # manages deep sleep + wake scheduling
└── README.md
```

## Architecture (at a glance)

```
 sensor_task  --(queue)-->  radio_task  --(Thread/Matter)--> Border Router
      |                          |
      +---------(events)---------+
                   |
              power_task
        (decides sleep/wake timing)
```

Each task is isolated — `sensor_task` never touches the radio directly, and
`radio_task` never touches the sensor directly. They only communicate through
queues defined in `app_queues.h`. This mirrors how production firmware is
structured and makes each part independently testable.

## Build Instructions

Once ESP-IDF is installed and the environment is activated:

```bash
idf.py set-target esp32c6
idf.py build
idf.py -p COMx flash monitor   # replace COMx with your board's port
```

(Board not connected yet? `idf.py build` alone will confirm your toolchain
and code compile correctly — no hardware required for that step.)

## Status / TODO

- [ ] Get `idf.py build` passing with this skeleton (no hardware needed)
- [ ] Wire in real I2C driver for the sensor once the board arrives
- [ ] Integrate ESP-Matter temperature-sensor example into `radio_task`
- [ ] Add deep sleep logic in `power_task`
- [ ] Measure real current draw once running on a coin cell
- [ ] Add OTA update support
- [ ] Design custom PCB
