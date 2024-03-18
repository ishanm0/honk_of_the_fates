# Honk of the Fates (ECE 167 Final Project)

The goal of this project is to run a playable game of rock-paper-scissors crossed with laser tag as a wand duel.

## Setup

The project requires at least two STM32 boards with the proper components attached and a macOS or Linux computer. (Note: we have not tested on Linux, use at your own risk.)
Our repo contains all of the necessary files except for the Python Bluetooth interface library, which must be installed from Adafruit.

Install the Adafruit library:
- [Linux](https://github.com/adafruit/Adafruit_Python_BluefruitLE?tab=readme-ov-file#linux--raspberry-pi-requirements)
- [macOS](https://github.com/adafruit/Adafruit_Python_BluefruitLE?tab=readme-ov-file#mac-osx-requirements)

## Usage

1) Upload the project to the STM32 boards, and power the RGB LEDs (the LEDs must be powered from a separate USB power source to get enough current at 5V).

2) Run the main_interface.py Python script. It will prompt you for a player count, input 2 if you just have two boards.
3) The Python script will now attempt to connect to the boards, and will notify you when all boards are connected.
3) Each player can now select a colour by rotating the rotary encoder, then pressing any button on the wand to confirm.
4) Once all players have selected a colour, the computer will start the game and you can cast spells at will.
Casting spells is done by aiming the wand at your opponent's IR receiver and pressing one of the buttons on the wand. Each of the three buttons is associated with a different symbol, each of which counters one other symbol and is countered by one other symbol (similar to the hand game rock-paper-scissors).