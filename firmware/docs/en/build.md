There are essentially three ways to build your NodeMCU firmware: cloud build service, Docker image, dedicated Linux environment (possibly VM).

## Tools

### Cloud Build Service
<del>NodeMCU "application developers" just need a ready-made firmware. There's a [cloud build service](http://nodemcu-build.com/) with a nice UI and configuration options for them.</del>

Not available yet.

### Docker Image
<del>Occasional NodeMCU firmware hackers don't need full control over the complete tool chain. They might not want to setup a Linux VM with the build environment. Docker to the rescue. Give [Docker NodeMCU build](https://hub.docker.com/r/marcelstoer/nodemcu-build/) a try.</del>

Not available yet.

### Linux Build Environment
NodeMCU firmware developers commit or contribute to the project on GitHub and might want to build their own full fledged build environment with the complete tool chain.

Run the following command for a new checkout from scratch. This will fetch the nodemcu repo, checkout the `dev-esp32` branch and finally pull all submodules:

```
git clone --branch dev-esp32 --recurse-submodules https://github.com/nodemcu/nodemcu-firmware.git nodemcu-firmware-esp32
```

The `make` command initiates the build process, which will start with the configuration menu to set the build options.

!!! important

    GNU make version 4.0 or higher is required for a successful build. Versions 3.8.2 and below will produce an incomplete firmware image.

Updating your clone from upstream needs an additional command to update the submodules as well:
```
git pull origin dev-esp32
git submodule update --recursive
```

## Build Options

All configuration options are accessed from the file `sdkconfig`. It's advisable to set it up with the interactive `make menuconfig` - on a fresh checkout you're prompted to run through it by default.

The most notable options are described in the following sections.

### Select Modules

Follow the menu path
```
Component config --->
  NodeMCU modules --->
```
Tick or untick modules as required.

### UART default bit rate

Follow the menu path
```
Component config --->
  Platform config --->
    UART console default bit rate --->
```

### CPU Frequency

Follow the menu path
```
Component config --->
  ESP32-specific --->
    CPU frequency --->
```

### Stack Size

If you experience random crashes then increase the stack size and feed back your observation on the project's issues list.

Follow the menu path
```
Component config --->
  ESP32-specific --->
    Main task stack size --->
```

### Flashing Options

Default settings for flashing the firmware with esptool.py are also configured with menuconfig:

```
Serial flasher config --->
  Default serial port
  Default baud rate
  Flash SPI mode --->
  Detect flash size when flashing bootloader --->
```
