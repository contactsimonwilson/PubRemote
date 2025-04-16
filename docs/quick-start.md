# Pubmote Quick Start Guide

## First-Time Setup

1. Ensure you have completed the [Hardware Prerequisites](/README.md#hardware-prerequisites) and [Software Prerequisites](/README.md#software-prerequisites) for a deployment, from the README.
    - Flash your ESP32 using the included flashing tool at [pubmote.techfoundry.nz](https://pubmote.techfoundry.nz/)
2. Configure Pubmote settings
    - On the Pubmote, navigate to the main menu by swiping down from the top of the main screen
    - Select "Settings" and swipe through the options to set your preferences
    - Select "Save" to apply your settings
3. Calibrate your Pubmote
    - On the Pubmote, navigate to the main menu by swiping down from the top of the main screen
    - Select "Calibration"
    - Select "Start" and move through the steps, selecting "Next" to continue each step, and "Save" to store your calibration.
        - For "Move stick to center", allow the joystick to sit steady.
        - For "Move stick to min/max", move the joystick smoothly to its limits of motion in all directions.
        - For "Move stick within deadband", move the joystick just slightly off-center. This will set the inner radius the Pubmote treats as no input.
        - For "Set expo factor", you may leave this at 1.00 or adjust for more or less input to tilt ratio.
        - For "Invert Y", you may check or uncheck this to flip the direction of the Y axis input.
        - View the graph for testing and save your calibration.
4. Pair the remote to your VESC Express by following the [Pairing Instructions](#pairing-instructions)
5. Go ride!

## Pairing Instructions

> [!CAUTION]
> VESC Express -> Bluetooth -> Bluetooth Mode: "Enabled and Encrypted" will cause connection failures with Pubmote! It must be set to "Enabled" or "Enabled with Scripting"
> ![alt text](bt_encrypted.png)

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
    - On the Pubmote, navigate to the main menu by swiping down from the top of the main screen
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

## Usage

While the homescreen is active, tilting the joystick will apply remote input to your VESC.

Some common scenarios where a tilt remote may be useful:
- Going up a hill, raising the nose angle
- Going down a hill, lowering the nose angle
- Accelerating quickly, raising the nose angle
- Decelerating quickly, lowering the nose angle
- Riding into headwind, lowering the nose angle to require less leaning into the wind
- Landing drops more level, lowering the nose
- Bonks or climbs over large objects, to gain clearance on the approach and exit
- Putting your friends through a Bucking Bronco mini game on your board
- Nose slides
- Tail drags
- Balance recovery
- And whatever else you might come up with!

## Common Issues and Mistakes

### The Pubmote won't connect
1. If VESC Express -> Bluetooth -> Bluetooth Mode is set to "Enabled and Encrypted", it will cause connection failures with Pubmote!

The fix: It must be set to "Enabled" or "Enabled with Scripting".

2. If VESC Express -> WiFi -> WiFi Mode is set to "Station Mode", it will cause connection failures with Pubmote.

The fix: It must be set to "Access Point"

3. If VESC Controller -> Refloat Cfg -> Remote ->
  - Remote Type is not set to UART
  - Tiltback Angle Limit is set to 0 °
  - Tiltback Speed is set to 0 °/s
  - Input Deadband is set to a very high %

The fix: Ensure Remote Type of  UART, Tiltback Angle Limit of >0 °, Tiltback Speed of >0 °/s, and a relatively low Input Deadband,

### The direction of the tilt is backwards
The fix: Re-run calibration and check "Invert Y"
