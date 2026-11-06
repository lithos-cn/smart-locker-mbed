# Smart Locker Monitoring with Mbed

A two-person Embedded Processors course project by **Li Lushi and Long Junlin**, using a NUCLEO-L432KC, ADXL345 accelerometer, TMP102 temperature sensor, two buttons, LEDs and a piezo buzzer.

The system combines a fixed button sequence with temperature and movement alerts. It is a classroom monitoring prototype: password acceptance produces feedback; no physical lock actuator is controlled.

## Behavior

- Enter `1, 2, 1, 1` using the two active-low buttons. A correct sequence lights the green LED for one second; a wrong sequence lights the denied LED and sounds the buzzer.
- Each button is debounced independently for 50 ms, including its release. Overlapping button presses are rejected rather than assigned an arbitrary order.
- TMP102 temperature is checked every 250 ms. Values below 20 C or above 30 C activate the temperature alert.
- ADXL345 movement is checked every 100 ms. A change greater than 0.5 g relative to the startup baseline activates the motion alert.
- Environmental alarms continue during password feedback. Sensor initialization or communication faults activate the corresponding alert and buzzer.

## Wiring

All signal levels and pull-ups use **3.3 V**, with a shared ground. Use current-limiting resistors for LEDs and an appropriate driver if the buzzer exceeds a GPIO's current capability.

| Component signal | Board pin |
| --- | --- |
| ADXL345 MOSI / SDI | A6 / PA_7 |
| ADXL345 MISO / SDO | A5 / PA_6 |
| ADXL345 SCLK | A4 / PA_5 |
| ADXL345 CS | D12 / PB_4 |
| TMP102 SDA | D0 / PA_10 |
| TMP102 SCL | D1 / PA_9 |
| TMP102 ADD0 | GND, giving 7-bit address 0x48 |
| Button 1 | D2 to GND, internal pull-up |
| Button 2 | D3 to GND, internal pull-up |
| Temperature LED | A0 |
| Motion LED | D10 |
| Password accepted LED | D6 |
| Password denied LED | D11 |
| Passive piezo buzzer | D9, 1 kHz PWM |

The TMP102 I2C lines need pull-up resistors, which may already be fitted to the breakout board. Do not enable a USB device application on D2/D10 while using this wiring. Serial logging uses the board's default ST-LINK virtual COM port at 115200 baud.

## Startup and operation

Keep the board stationary during startup. The firmware allows sensor startup, then averages 32 accelerometer samples to establish the resting gravity vector. This avoids assuming that gravity always points along the Z axis. Failed startup checks remain an alarm condition; correct the wiring and reset to initialize again.

Password feedback is timed without blocking the sensing loop. Interrupt handlers only signal an event; button sampling, debouncing, password handling and bus transactions run in the main thread. A held button counts once and must be released before another digit is accepted. Button input during active password feedback is ignored, except that an ambiguous overlapping press is rejected.

Temperature thresholds are software checks. The TMP102 ALERT pin is unused. TMP102 uses signed 12-bit continuous conversion at 4 Hz; this firmware does not claim deep-sleep or measured power-saving performance. Movement detection compares the present acceleration vector with its fixed startup baseline, so changing the mounting orientation can intentionally trigger an alert. Short impacts between the 100 ms reads may be missed.

## Build and validation

See [BUILDING.md](BUILDING.md) for the pinned Mbed dependency and build steps, and [tests/README.md](tests/README.md) for native tests.

The native C++ control tests and driver tests pass, and the main program compiles against the supplied mock Mbed interfaces. An ARM target build and tests on the physical board have **not** been run for this revision. Host tests cannot verify real bus timing, wiring, sensor calibration, or audible output.

## Demonstration and attribution

[Course demonstration video](demo/course-demonstration.mp4) is retained from the original submission. It predates this maintenance revision and does not validate the revised firmware.

The saved course sources were revised to restore sensor integration, correct signed decoding and sensor mode configuration, remove blocking password feedback, and separate debounce state. A separate course team's example was consulted as a reference for the integration flow. That reference archive and the old project reports are not included. No separate open-source license is granted.

Technical references used for the maintenance work:

- [Analog Devices ADXL345 data sheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf)
- [Texas Instruments TMP102 data sheet](https://www.ti.com/lit/ds/symlink/tmp102.pdf)
- [Mbed OS 6.17.0 source](https://github.com/ARMmbed/mbed-os/tree/mbed-os-6.17.0)
