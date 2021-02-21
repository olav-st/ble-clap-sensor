# BLE Clap Sensor
Wireless clap sensor using Bluetooth Low Energy (BLE). The sensor is based on an [NRF52-DK](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52-DK) running the [Zephyr](https://www.zephyrproject.org/) RTOS.

<img alt="A photo of the sensor" src="https://i.imgur.com/nvGAB7c.jpeg" width="400">

## Hardware
* NRF52-DK
* Analog Microphone
* 1.1 kΩ Resistor

## Compiling
The project can be compiled using either the [PlatformIO extension for Visual Studio Code](https://platformio.org/install/ide) or the [PlatformIO CLI](https://platformio.org/install/cli). To compile using the CLI, follow the instructions below:

Clone the repository:

```sh
git clone https://github.com/olav-st/ble-clap-sensor
cd ble-clap-sensor
```

Compile the project:

`platformio run`

And flash the compiled code onto the NRF52-DK:

`platformio -t upload`