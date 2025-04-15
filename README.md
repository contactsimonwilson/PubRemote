[![Publish Release](https://github.com/contactsimonwilson/PubRemote/actions/workflows/release.yml/badge.svg)](https://github.com/contactsimonwilson/PubRemote/actions/workflows/release.yml)

# Pubmote AKA Pubremote

Welcome to Pubmote!

Pubmote is a feature-rich, ESP-NOW based remote control for VESC based onewheels. Pubmote connects to a VESC Express receiver running the Float Accessories package, which provides a feature-rich platform and easy configuration experience.

## Getting started

### Hardware Prerequisites

#### ESP32S3-based controller with a display, and a case. Options:
- **Recommended:** [WaveShare 1.43in Amoled display](https://www.waveshare.com/esp32-s3-touch-amoled-1.43.htm?sku=30106) (see: [example build](/docs/builds/leaf-blaster.md))
  - [ZiNc Leaf Blaster remix case](https://www.printables.com/model/1265591) by ZiNc
  - [Leaf Blaster case](https://www.printables.com/model/1191785) by Markoblaster
- [WaveShare 1.28in LCD display](https://www.waveshare.com/esp32-s3-touch-lcd-1.28.htm)
  - [VX4 Case](https://www.printables.com/model/835158-pubmote) by ThePoro
  - Full development kit from [Avaspark](https://avaspark.com/products/pubmote-dev-kit) including a case, display, joystick, and other parts
- The ["Cowmote"](https://cowpowersystems.com/product/1) from ExcessRacoon uses the [LilyGo T-Display S3 Amoled (1.43in)](https://lilygo.cc/products/t-display-s3-amoled-1-64?variant=44507650556085) (see: [example build](/docs/builds/snowmote.md))
  - [SnowMote case](https://www.printables.com/model/1143449) by ZiNc
  - [Finger Blaster case](https://www.printables.com/model/1159060) by Markoblaster

#### A Joystick. Options:
- [PS5 hall joystick](https://www.aliexpress.us/item/3256806823053436.html)
- [Nintendo Switch joystick](https://vi.aliexpress.com/item/1005006746686389.html)

#### VESC Express receiver. Options:
- Building your own
- [Trampa VESC Express Module](https://trampaboards.com/vesc-express--p-34857.html)
- [AvaSpark RGB Mini](https://avaspark.com/products/avaspark-rgb-mini)
- [CustomWheel VESC Express Module](https://customwheel.shop/accesories/vesc-express-module-wifi-bt)
- And many others...

### Software Prerequisites

#### Simple Deployment, No Development

Flash your ESP32 using the included flashing tool at [pubmote.techfoundry.nz](https://pubmote.techfoundry.nz/)

#### Advanced Deployment
- IDE or other code editor
  - SquareLine Studio
  - VS Code
  - Something else...
- PlatformIO

> [!TIP]
> If SL Studio repeatedly fails on startup because of a font error, try clearing the font bin files.

## Issues

[Create an issue](https://github.com/contactsimonwilson/PubRemote/) on GitHub or post in the Pubmote channel within the PubWheel Discord server

## Get Started Using PubMote

For instructions on first-time setup, pairing, and usage, see the [quick start guide](/docs/quick-start.md).
