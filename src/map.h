#ifndef IMPULSE_WARS_MAP_H
#define IMPULSE_WARS_MAP_H

#include <errno.h>
#include <string.h>

#include "env.h"
#include "settings.h"

// clang-format off

const char boringLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

const mapEntry boringMap = {
    .layout = boringLayout,
    .columns = 21,
    .rows = 21,
    .floatingStandardWalls = 0,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 0,
    .weaponPickups = 8,
    .defaultWeapon = STANDARD_WEAPON,
};

const char prototypeArenaLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','d','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','w','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','W','W','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','D','D','W','W','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','d','O','D','D','D','D','D','O','O','O','O','O','O','O','D',
    'D','O','w','O','O','O','O','D','D','D','D','D','O','O','O','O','w','O','O','D',
    'D','O','O','O','O','O','O','D','D','D','D','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','W','W','O','O','O','d','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','w','O','O','D',
    'D','O','O','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

const mapEntry prototypeArenaMap = {
    .layout = prototypeArenaLayout,
    .columns = 20,
    .rows = 20,
    .floatingStandardWalls = 0,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 0,
    .weaponPickups = 8,
    .defaultWeapon = STANDARD_WEAPON,
};

const char snipersLayout[] = {
    'B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','D','D','B','O','B','D','D','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','B','B','B','B','O','B','B','B','B','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','B','B','B','B','O','B','B','B','B','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','D','D','B','O','B','D','D','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B',
};

const mapEntry snipersMap = {
    .layout = snipersLayout,
    .columns = 21,
    .rows = 21,
    .floatingStandardWalls = 0,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 0,
    .weaponPickups = 6,
    .defaultWeapon = SNIPER_WEAPON,
};

const char roomsLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','W','O','O','O','W','D','D','D','D','D','W','O','O','O','W','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

const mapEntry roomsMap = {
    .layout = roomsLayout,
    .columns = 21,
    .rows = 21,
    .floatingStandardWalls = 3,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 3,
    .weaponPickups = 12,
    .defaultWeapon = SHOTGUN_WEAPON,
};

const char xArenaLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','d','O','O','O','O','O','O','O','O','O','d','O','D',
    'D','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','W','W','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','W','W','D','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','D','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','D',
    'D','O','O','O','O','O','D','D','O','O','w','O','O','O','O','D','W','W','O','O','O','O','D',
    'D','O','O','O','O','W','W','D','D','O','O','O','O','O','D','D','W','W','O','O','w','O','D',
    'D','O','w','O','O','W','W','D','O','O','O','O','d','O','O','D','D','O','O','O','O','O','D',
    'D','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','D','D','D','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','D','W','W','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','W','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','w','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','D',
    'D','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

const mapEntry xArena = {
    .layout = xArenaLayout,
    .columns = 23,
    .rows = 23,
    .floatingStandardWalls = 0,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 0,
    .weaponPickups = 12,
    .defaultWeapon = STANDARD_WEAPON,
};

const char crossBounceLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','B','B','B','B','O','O','O','O','B','D','D','D','D','B','O','O','O','O','B','B','B','B','D',
    'D','B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B','D',
    'D','B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B','D',
    'D','B','O','O','B','B','O','O','O','O','O','w','d','O','O','O','O','O','B','B','O','O','B','D',
    'D','O','O','O','B','D','D','O','O','O','O','O','O','O','O','O','O','D','D','B','O','O','O','D',
    'D','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','B','O','O','B','D','O','O','O','O','O','O','O','O','D',
    'D','B','O','O','O','O','O','O','D','D','B','O','O','B','D','D','O','O','O','O','O','O','B','D',
    'D','D','O','O','O','O','O','O','B','B','B','O','O','B','B','B','O','O','O','O','O','O','D','D',
    'D','D','O','O','d','O','O','O','O','O','O','O','O','O','O','O','O','O','O','w','O','O','D','D',
    'D','D','O','O','w','O','O','O','O','O','O','O','O','O','O','O','O','O','O','d','O','O','D','D',
    'D','D','O','O','O','O','O','O','B','B','B','O','O','B','B','B','O','O','O','O','O','O','D','D',
    'D','B','O','O','O','O','O','O','D','D','B','O','O','B','D','D','O','O','O','O','O','O','B','D',
    'D','O','O','O','O','O','O','O','O','D','B','O','O','B','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','D',
    'D','O','O','O','B','D','D','O','O','O','O','O','O','O','O','O','O','D','D','B','O','O','O','D',
    'D','B','O','O','B','B','O','O','O','O','O','d','w','O','O','O','O','O','B','B','O','O','B','D',
    'D','B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B','D',
    'D','B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B','D',
    'D','B','B','B','B','O','O','O','O','B','D','D','D','D','B','O','O','O','O','B','B','B','B','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

const mapEntry crossBounce = {
    .layout = crossBounceLayout,
    .columns = 24,
    .rows = 24,
    .floatingStandardWalls = 0,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 0,
    .weaponPickups = 12,
    .defaultWeapon = STANDARD_WEAPON,// TODO: make this exploding weapon
};

const char asteriskArenaLayout[]= {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','D','W','O','O','O','W','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','O','O','O','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','D','D','O','O','O','O','O','O','O','O','O','D','D','O','O','O','O','D',
    'D','O','O','O','O','W','W','D','O','O','O','O','O','O','O','D','W','W','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','D','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','W','W','D','O','O','O','O','O','O','O','D','W','W','O','O','O','O','D',
    'D','O','O','O','O','D','D','O','O','O','O','O','O','O','O','O','D','D','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','D','O','O','O','D','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','D','W','O','O','O','W','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

const mapEntry asteriskArena = {
    .layout = asteriskArenaLayout,
    .columns = 23,
    .rows = 23,
    .floatingStandardWalls = 0,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 0,
    .weaponPickups = 12,
    .defaultWeapon = STANDARD_WEAPON,
};

const char foamPitLayout[] = {
    'B','B','B','W','W','W','D','D','D','B','B','D','D','D','W','W','W','B','B','B',
    'B','O','O','O','O','O','O','O','D','B','B','D','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','B','B','O','O','O','O','O','O','O','O','B',
    'W','O','O','d','O','O','O','O','O','O','O','O','O','O','O','O','d','O','O','W',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','d','O','O','O','O','d','O','O','O','O','O','O','D',
    'D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D',
    'B','B','B','O','O','O','O','O','O','d','d','O','O','O','O','O','O','B','B','B',
    'B','B','B','O','O','O','O','O','O','d','d','O','O','O','O','O','O','B','B','B',
    'D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D',
    'D','O','O','O','O','O','O','d','O','O','O','O','d','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'W','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','W',
    'W','O','O','d','O','O','O','O','O','O','O','O','O','O','O','O','d','O','O','W',
    'B','O','O','O','O','O','O','O','O','B','B','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','D','B','B','D','O','O','O','O','O','O','O','B',
    'B','B','B','W','W','W','D','D','D','B','B','D','D','D','W','W','W','B','B','B',
};

const mapEntry foamPitMap = {
    .layout = foamPitLayout,
    .columns = 20,
    .rows = 20,
    .floatingStandardWalls = 0,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 0,
    .weaponPickups = 8,
    .defaultWeapon = MACHINEGUN_WEAPON,
};

// clang-format on

#define NUM_MAPS 8

#ifndef AUTOPXD
const mapEntry *maps[] = {
    (mapEntry *)&boringMap,
    (mapEntry *)&prototypeArenaMap,
    (mapEntry *)&snipersMap,
    (mapEntry *)&roomsMap,
    (mapEntry *)&xArena,
    (mapEntry *)&crossBounce,
    (mapEntry *)&asteriskArena,
    (mapEntry *)&foamPitMap,
};
#endif

void createMap(env *e, const int mapIdx) {
    const uint8_t columns = maps[mapIdx]->columns;
    const uint8_t rows = maps[mapIdx]->rows;
    const char *layout = maps[mapIdx]->layout;

    e->columns = columns;
    e->rows = rows;
    e->defaultWeapon = weaponInfos[maps[mapIdx]->defaultWeapon];
    if (e->isTraining && randFloat(&e->randState, 0.0f, 1.0f) < 0.25f) {
        e->defaultWeapon = weaponInfos[randInt(&e->randState, 0, NUM_WEAPONS - 1)];
    }

    uint16_t cellIdx = 0;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            char cellType = layout[col + (row * columns)];
            enum entityType wallType;
            float x = (col - (columns / 2.0f) + 0.5) * WALL_THICKNESS;
            float y = ((rows / 2.0f) - (rows - row) + 0.5f) * WALL_THICKNESS;

            b2Vec2 pos = {.x = x, .y = y};
            mapCell *cell = (mapCell *)fastMalloc(sizeof(mapCell));
            cell->ent = NULL;
            cell->pos = pos;
            cc_array_add(e->cells, cell);

            bool floating = false;
            float thickness = WALL_THICKNESS;
            switch (cellType) {
            case 'O':
                continue;
            case 'w':
                thickness = FLOATING_WALL_THICKNESS;
                floating = true;
            case 'W':
                wallType = STANDARD_WALL_ENTITY;
                break;
            case 'b':
                thickness = FLOATING_WALL_THICKNESS;
                floating = true;
            case 'B':
                wallType = BOUNCY_WALL_ENTITY;
                break;
            case 'd':
                thickness = FLOATING_WALL_THICKNESS;
                floating = true;
            case 'D':
                wallType = DEATH_WALL_ENTITY;
                break;
            default:
                ERRORF("unknown map layout cell %c", cellType);
            }

            entity *ent = createWall(e, x, y, thickness, thickness, cellIdx, wallType, floating);
            if (!floating) {
                cell->ent = ent;
            }
            cellIdx++;
        }
    }
}

void placeRandFloatingWall(env *e, const enum entityType wallType) {
    b2Vec2 pos;
    if (!findOpenPos(e, FLOATING_WALL_SHAPE, &pos)) {
        ERROR("failed to find open position for floating wall");
    }
    createWall(e, pos.x, pos.y, FLOATING_WALL_THICKNESS, FLOATING_WALL_THICKNESS, -1, wallType, true);
}

void placeRandFloatingWalls(env *e, const int mapIdx) {
    for (int i = 0; i < maps[mapIdx]->floatingStandardWalls; i++) {
        placeRandFloatingWall(e, STANDARD_WALL_ENTITY);
    }
    for (int i = 0; i < maps[mapIdx]->floatingBouncyWalls; i++) {
        placeRandFloatingWall(e, BOUNCY_WALL_ENTITY);
    }
    for (int i = 0; i < maps[mapIdx]->floatingDeathWalls; i++) {
        placeRandFloatingWall(e, DEATH_WALL_ENTITY);
    }
}

#endif
