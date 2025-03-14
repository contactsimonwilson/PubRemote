[![Publish Release](https://github.com/contactsimonwilson/PubRemote/actions/workflows/release.yml/badge.svg)](https://github.com/contactsimonwilson/PubRemote/actions/workflows/release.yml)

# Pubmote AKA Pubremote

Welcome to Pubmote!

Pubmote is a feature-rich, ESP-NOW based remote control for VESC based onewheels. Pubmote connects to a VESC Express receiver running the Float Accessories package, which provides a feature-rich platform and easy configuration experience.

## Getting started

### Hardware Prerequisites

#### VESC Express receiver. Options:
- Building your own
- [Trampa VESC Express Module](https://trampaboards.com/vesc-express--p-34857.html)
- [AvaSpark RGB Mini](https://avaspark.com/products/avaspark-rgb-mini)
- [CustomWheel VESC Express Module](https://customwheel.shop/accesories/vesc-express-module-wifi-bt)
- And many others...

#### ESP32S3-based controller with a display, and a case. Options:
- [WaveShare 1.43in Amoled display](https://www.waveshare.com/esp32-s3-touch-amoled-1.43.htm?sku=30106) (recommended)
  - [Leaf Blaster case by Markoblaster](https://www.printables.com/model/1191785-leaf-blaster-pubmote-waveshare-14)
  - Leaf Blaster remix case by ZiNc (TBD)
- [WaveShare 1.28in LCD display](https://www.waveshare.com/esp32-s3-touch-lcd-1.28.htm)
  - [Case by ThePoro](https://www.printables.com/model/835158-pubmote)
  - Full development kit from [Avaspark](https://avaspark.com/products/pubmote-dev-kit) including a case, display, joystick, and other parts
- The ["Cowmote"](https://cowpowersystems.com/product/1) from ExcessRacoon uses the [LilyGo T-Display S3 Amoled (1.43in)](https://lilygo.cc/products/t-display-s3-amoled-1-64?variant=44507650556085)
  - [SnowMote](https://www.printables.com/model/1143449-snowmote-case-for-pubmote-project) by ZiNc

#### A Joystick. Options:
- [PS5 hall joystick](https://www.aliexpress.us/item/3256806823053436.html)
- [Nintendo Switch joystick](https://vi.aliexpress.com/item/1005006746686389.html)

### Software Prerequisites

#### Simple Deployment, No Development

Flash your ESP32 using the included flashing tool at [pubmote.techfoundry.nz](https://pubmote.techfoundry.nz)

#### Advanced Deployment
- IDE or other code editor
  - SquareLine Studio
  - VS Code
  - Something else...
- PlatformIO

> [!TIP]
> If SL Studio repeatedly fails on startup because of a font error, try clearing the font bin files.

## Issues

[Create an issue](https://github.com/contactsimonwilson/PubRemote/issues) on GitHub or post in the PubRemote channel within the PubWheel Discord server

## Pairing Instructions

To ensure you can get your PubRemote paired and running, follow these simple steps after you have your Pubmote hardware assembled and have flashed the latest Pubmote software to your ESP32:

1. Install Float Accessories on your VESC Express. Available from [Syler's vesc_pkg repository](https://github.com/Relys/vesc_pkg). To install this, either:
    - Clone the repository and build the float_accessories package yourself
    - Download a relatively recent build from [pubmote.techfoundry.nz](https://pubmote.techfoundry.nz) and install using the VESC Tool custom package installer
2. Configure your VESC Express wifi settings
    - Navigate to VESC Express > WiFi > WiFi Mode
    - Set this to "Access Point"
3. Configure your VESC Express Float Accessories settings
    - Navigate to App UI > Settings > Pubmote Enabled
    - Ensure this is checked
    - Save and restart as necessary
4. Launch Pubmote pairing
    - On the Pubmote, navigate to the settings menu by swiping down from the top of the main screen
    - Select "Pairing" and start the pairing on the Pubmote
5. Launch VESC Express Pubmote pairing
    - Navigate to App UI > Config > Pair Pubmote
6. Confirm the pairing code on both the Pubmote and VESC Express

You should now be connected!

## Package-Side Setup

On Refloat:
1. Navigate to Refloat Cfg > Remote
2. Ensure Remote Type is set to "UART"
3. Ensure Tiltback Angle Limit is above 0 degrees
4. Ensure Tiltback Speed is above 0 degrees/second
5. Ensure Input Deadband is below 100%, but at least 1%.
6. Set Throttle Current Maximum to 0, unless you intend to use remote throttle for fun
