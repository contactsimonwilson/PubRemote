# Pubmote Quick Start Guide

## First-Time Setup

1. Ensure you have completed the Hardware Prerequisites and Software Prerequisites for a deployment from the README.
2. Configure Pubmote settings
    - On the Pubmote, navigate to the main menu by swiping down from the top of the main screen
    - Select "Settings" and swipe through the options to set your preferences
    - Select "Save" to apply your settings
3. Pair the remote to your VESC Express by following the [Pairing Instructions](#pairing-instructions)

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