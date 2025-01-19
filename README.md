[![Publish Release](https://github.com/contactsimonwilson/PubRemote/actions/workflows/release.yml/badge.svg)](https://github.com/contactsimonwilson/PubRemote/actions/workflows/release.yml)

# Pubmote AKA Pubremote

Welcome to Pubmote!

Pubmote is a feature-rich, ESP-NOW based remote control for VESC based onewheels. Pubmote connects to a VESC Express receiver running the Float Accessories package, which provides a feature-rich platform and easy configuration experience.

## Getting started

### Hardware Prerequisites
- VESC Express receiver. Options include:
  - Building your own
  - [Trampa VESC Express Module](https://trampaboards.com/vesc-express--p-34857.html)
  - [AvaSpark RGB Mini](https://avaspark.com/products/avaspark-rgb-mini)
  - [CustomWheel VESC Express Module](https://customwheel.shop/accesories/vesc-express-module-wifi-bt)
  - And many others...
- [ESP32S3 development board](https://www.waveshare.com/esp32-s3-touch-lcd-1.28.htm)
  - The ["Cowmote"](https://cowpowersystems.com/product/1) from ExcessRacoon uses the [LilyGo T-Display S3 Amoled (1.43in)](https://lilygo.cc/products/t-display-s3-amoled-1-64?variant=44507650556085)
  - Or for an easy assembly process, consider buying a kit from [Avaspark](https://avaspark.com/products/pubmote-dev-kit)
- A Joystick; One of either:
  - [Nintendo Switch joystick](https://vi.aliexpress.com/item/1005006746686389.html)
  - [PS5 hall joystick](https://vi.aliexpress.com/item/1005005916919152.html)

### Software Prerequisites

**Simple Deployment, No Development**

Flash your ESP32 using the included flashing tool at [pubmote.techfoundry.nz](https://pubmote.techfoundry.nz)

**Advanced Deployment**
- IDE or other code editor
  - SquareLine Studio
  - VS Code
  - Something else...
- PlatformIO

> [!TIP]
> If SL Studio repeatedly fails on startup because of a font error, try clearing the font bin files.

## Issues

Create an issue on GitHub or in the PubRemote channel within the PubWheel Discord server

## Pairing Instructions

To ensure you can get your PubRemote paired and running, follow these simple steps after you have your Pubmote hardware assembled and have flashed the latest Pubmote software to your ESP32:

1. Install Float Accessories on your VESC Express. Available from [Syler's vesc_pkg repository](https://github.com/Relys/vesc_pkg). To install this, either:
    - Clone the repository and build the float_accessories package yourself
    - Download a relatively recent build from [pubmote.techfoundry.nz](https://pubmote.techfoundry.nz) and install using the VESC Tool custom package installer
2. Configure your VESC Express wifi settings
    - Navigate to VESC Express > WiFi > WiFi Mode
    - Set this to "Station"
3. Configure your VESC Express Float Accessories settings
    - Navigate to App UI > Settings > Pubmote Enabled
    - Ensure this is checked
    - Save and restart as necessary
4. Launch Pubmote pairing
    - On the Pubmote, navigate to the settings menu
    - Select "Pairing" and start the pairing on the Pubmote
5. Launch VESC Express Pubmote pairing
    - Navigate to App UI > Config > Pair Pubmote
6. Confirm the pairing code on both the Pubmote and VESC Express

You should now be connected!
