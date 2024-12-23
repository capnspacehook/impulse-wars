#pragma once
#ifndef IMPULSE_WARS_TYPES_H
#define IMPULSE_WARS_TYPES_H

#include "box2d/box2d.h"

#include "include/cc_array.h"
#include "include/cc_slist.h"

#include "settings.h"

#define _MAX_DRONES 4

// 1 is added for the invalid entity type
const uint8_t NUM_WALL_TYPES = 4;
const uint8_t NUM_ENTITY_TYPES = NUM_WALL_TYPES + 2;

enum entityType
{
    INVALID_ENTITY = 0,
    STANDARD_WALL_ENTITY = 1,
    BOUNCY_WALL_ENTITY = 2,
    DEATH_WALL_ENTITY = 3,
    PROJECTILE_ENTITY = 4,
    DRONE_ENTITY = 5,
    // this needs to be last so map cell type observations can be
    // calculated correctly
    WEAPON_PICKUP_ENTITY = 5 + _MAX_DRONES + 1,
};

// the category bit that will be set on each entity's shape; this is
// used to control what entities can collide with each other
enum shapeCategory
{
    WALL_SHAPE = 1,
    FLOATING_WALL_SHAPE = 2,
    PROJECTILE_SHAPE = 4,
    WEAPON_PICKUP_SHAPE = 8,
    DRONE_SHAPE = 16,
};

// general purpose entity object
typedef struct entity
{
    enum entityType type;
    void *entity;
} entity;

#define _NUM_WEAPONS 5
const uint8_t NUM_WEAPONS = _NUM_WEAPONS;

enum weaponType
{
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
    SNIPER_WEAPON,
    SHOTGUN_WEAPON,
    IMPLODER_WEAPON,
};

typedef struct mapEntry
{
    const char *layout;
    const uint8_t columns;
    const uint8_t rows;
    const uint8_t floatingStandardWalls;
    const uint8_t floatingBouncyWalls;
    const uint8_t floatingDeathWalls;
    const uint16_t weaponPickups;
    const enum weaponType defaultWeapon;
} mapEntry;

// a cell in the map; ent will be NULL if the cell is empty
typedef struct mapCell
{
    entity *ent;
    b2Vec2 pos;
} mapCell;

typedef struct mapBounds
{
    b2Vec2 min;
    b2Vec2 max;
} mapBounds;

typedef struct cachedPos
{
    b2Vec2 pos;
    bool valid;
} cachedPos;

typedef struct wallEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    cachedPos pos;
    b2Vec2 extent;
    bool isFloating;
    enum entityType type;
} wallEntity;

typedef struct weaponInformation
{
    const enum weaponType type;
    const bool isPhysicsBullet;
    const uint8_t numProjectiles;
    const float recoilMagnitude;
    const float coolDown;
    const float maxDistance;
    const float radius;
    const float density;
    const float invMass;
    const uint8_t maxBounces;
} weaponInformation;

typedef struct weaponPickupEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    enum weaponType weapon;
    float respawnWait;
    uint8_t floatingWallsTouching;
    uint16_t mapCellIdx;
} weaponPickupEntity;

typedef struct droneEntity droneEntity;

typedef struct projectileEntity
{
    uint8_t droneIdx;

    b2BodyId bodyID;
    b2ShapeId shapeID;
    weaponInformation *weaponInfo;
    cachedPos pos;
    b2Vec2 lastPos;
    float distance;
    uint8_t bounces;
} projectileEntity;

typedef struct stepHitInfo
{
    bool shotHit[_MAX_DRONES];
    bool explosionHit[_MAX_DRONES];
} stepHitInfo;

typedef struct droneStats
{
    float distanceTraveled;
    float shotsFired[_NUM_WEAPONS];
    float shotsHit[_NUM_WEAPONS];
    float shotsTaken[_NUM_WEAPONS];
    float ownShotsTaken[_NUM_WEAPONS];
    float weaponsPickedUp[_NUM_WEAPONS];
    float shotDistances[_NUM_WEAPONS];
} droneStats;

typedef struct droneEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    weaponInformation *weaponInfo;
    int8_t ammo;
    float weaponCooldown;
    uint16_t heat;
    uint16_t charge;
    bool shotThisStep;

    uint8_t idx;
    cachedPos pos;
    b2Vec2 lastPos;
    b2Vec2 lastMove;
    b2Vec2 lastAim;
    b2Vec2 lastVelocity;
    stepHitInfo hitInfo;
    bool dead;
} droneEntity;

typedef struct observationInfo
{
    uint16_t obsSize;
    uint16_t scalarObsOffset;
    uint16_t droneObsOffset;
    uint16_t projectileObsOffset;
    uint16_t floatingWallObsOffset;
    uint16_t mapCellObsOffset;
} observationInfo;

typedef struct logEntry
{
    float reward[_MAX_DRONES];
    float length;
    droneStats stats[_MAX_DRONES];
    uint8_t winner;
} logEntry;

typedef struct logBuffer
{
    logEntry *logs;
    uint16_t size;
    uint16_t capacity;
} logBuffer;

typedef struct rayClient
{
    float scale;
    uint16_t width;
    uint16_t height;
    uint16_t halfWidth;
    uint16_t halfHeight;
} rayClient;

typedef struct env
{
    uint8_t numDrones;
    uint8_t numAgents;
    observationInfo obsInfo;

    float *obs;
    float *rewards;
    float *actions;
    uint8_t *terminals;

    uint64_t randState;
    bool needsReset;

    float episodeReward[_MAX_DRONES];
    uint16_t episodeLength;
    logBuffer *logs;
    droneStats stats[_MAX_DRONES];

    b2WorldId worldID;
    uint8_t columns;
    uint8_t rows;
    mapBounds bounds;
    weaponInformation *defaultWeapon;
    CC_Array *cells;
    CC_Array *walls;
    CC_Array *floatingWalls;
    CC_Array *drones;
    CC_Array *pickups;
    CC_SList *projectiles;

    // steps left until sudden death
    uint16_t stepsLeft;
    // steps left until the next set of sudden death walls are spawned
    uint16_t suddenDeathSteps;
    // the amount of sudden death walls that have been spawned
    uint8_t suddenDeathWallCounter;

    rayClient *client;

    // used for rendering explosions
    // TODO: use hitInfo
    uint8_t explosionSteps;
    b2ExplosionDef explosion;
} env;

#endif
