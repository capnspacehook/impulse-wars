# Impulse Wars

A C reinforcement learning environment that is very closely based off of the game Retrograde Arena. 

## Controls

### Controller

- move: left stick
- aim: right stick
- shoot: right bottom trigger
- light brake: left bottom trigger
- hard brake: left top trigger
- burst: right top trigger or bottom face button
    - hold to charge and release to burst
- discard weapon: left face button

### Mouse and Keyboard

- move: WASD
- aim: mouse pointer
- shoot: left mouse button
- light brake: spacebar
- burst: right mouse button
    - hold to charge and release to burst

## Basics

- blue walls are standard walls
- yellow walls are bouncy
- only way to kill opponents is to push them into red death walls
- shooting them is the primary way to do this
- different weapons spawn randomly on the map and can be picked up
- shooting imposes recoil
- movement can't completely counteract recoil
- braking can help with recoil and help control movement
- braking consumes energy
- energy passively very slowly refills 
- energy can be refilled by picking up weapons and by hitting opponents
- if energy is fully depleted no energy can be used until it is fully replenished
- bursting can deflect projectiles and push opponents away
- bursting consumes energy and can be charged for greater effect

## Advanced

- movement speed is halved for a short period of time if energy is fully depleted
- while charging a burst the brake can't be used
- any energy gained or lost when charging a burst directly affects the burst charge
- a picked up weapon can be discarded at the expense of some energy

## Meta

- bursting can be used aggressively by running down opponents and bursting point blank
- this can be countered by braking and taking advantage of the opponents spent energy bursting
- excessive braking can be countered by powerful weapons
- powerful weapons can be countered by bursting to deflecting projectiles or braking

## Building

Build the Python module with `make`. You can then run the `main.py` file to train a policy or evaluate one. 

Python 3.11 is what I'm developing with, I make no promises for other versions. `scikit-core-build` is used to build the Python module, but will be installed automatically if the correct make command is invoked. `autopxd2` is used to generate declarations in a PXD file for the Cython code, which will automatically be installed as well. There are a few parts of my C headers that `autopxd2` fails to parse, but they are guarded by defines. 

## Structure

### Python

- `main.py` contains the main training/evaluation loop
- `policy.py` contains the neural network policy
- `impulse_wars.py` defines the Python environment
- `cy_impulse_wars.pyx` is the Cython wrapper between Python and C

### C environment (`src` directory)

- `include` directory contains a few deps from GitHub I converted to be header only
- `helpers.h` defines small helper functions and macros
- `types.h` defines most of the types used throughout the project. It's in it's own file to prevent circular dependencies
- `settings.h` defines general game and environment settings, as well as weapon handling settings/logic
- `map.h` contains all map layouts and map setup logic
- `game.h` contains the game logic
- `env.h` contains the RL environment logic
