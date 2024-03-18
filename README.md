# Honk of the Fates (final project ECE 167)

The goal of this project is to run a playable game of rock-paper-sissors crossed with laser tag. 

## Setup

The project requires at least two STM32 boards with the proper components attached and an Apple or Linux computer running a modern OS.
Our repo contains all of the necessary files except for the Python library which must be installed from Adafruit.

Install the Adafruit library from: [the adafruit website](https://github.com/adafruit/Adafruit_Python_BluefruitLE)

## Usage

Run the python script on your laptop and power on the STM32 boards as well as the RGB LEDs (which must be powered off of a separate USB power source to get enough current at 5V)

1) The python script will prompt you for a player count. Input 2 if you just have two boards.

Once connected the gameplay steps are as follows:
1) First you will select a colour by roating the rotary encoder and pressing any button on the wand.
2) Once all player have selected a colour, the computer will start the game and you can cast spells at will.
Casting spells is done by aiming the wand at your oponnents chest and pressing one of the buttons on the wand. Each of the three buttons is associated with a different symbol, each of which counters one other symbol and is countered by one other symbol (similar to the hand game rock-paper-sissors).