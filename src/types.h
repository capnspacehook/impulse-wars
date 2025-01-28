#ifndef IMPULSE_WARS_TYPES_H
#define IMPULSE_WARS_TYPES_H

#include "box2d/box2d.h"

#include "include/cc_array.h"
#include "include/cc_slist.h"

#include "settings.h"

#define _MAX_DRONES 4

const uint8_t NUM_WALL_TYPES = 3;

enum entityType {
    STANDARD_WALL_ENTITY,
    BOUNCY_WALL_ENTITY,
    DEATH_WALL_ENTITY,
    WEAPON_PICKUP_ENTITY,
    PROJECTILE_ENTITY,
    DRONE_ENTITY,
};

// the category bit that will be set on each entity's shape; this is
// used to control what entities can collide with each other
enum shapeCategory {
    WALL_SHAPE = 1,
    FLOATING_WALL_SHAPE = 2,
    PROJECTILE_SHAPE = 4,
    WEAPON_PICKUP_SHAPE = 8,
    DRONE_SHAPE = 16,
};

// general purpose entity object
typedef struct entity {
    enum entityType type;
    void *entity;
} entity;

#define _NUM_WEAPONS 6
const uint8_t NUM_WEAPONS = _NUM_WEAPONS;

enum weaponType {
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
    SNIPER_WEAPON,
    SHOTGUN_WEAPON,
    IMPLODER_WEAPON,
    ACCELERATOR_WEAPON,
};

typedef struct mapEntry {
    const char *layout;
    const uint8_t columns;
    const uint8_t rows;
    const uint8_t randFloatingStandardWalls;
    const uint8_t randFloatingBouncyWalls;
    const uint8_t randFloatingDeathWalls;
    const bool hasSetFloatingWalls;
    const uint16_t weaponPickups;
    const enum weaponType defaultWeapon;
} mapEntry;

// a cell in the map; ent will be NULL if the cell is empty
typedef struct mapCell {
    entity *ent;
    b2Vec2 pos;
} mapCell;

typedef struct mapBounds {
    b2Vec2 min;
    b2Vec2 max;
} mapBounds;

typedef struct wallEntity {
    b2BodyId bodyID;
    b2ShapeId shapeID;
    b2Vec2 pos;
    b2Rot rot;
    b2Vec2 velocity;
    b2Vec2 extent;
    int16_t mapCellIdx;
    bool isFloating;
    enum entityType type;
    bool isSuddenDeath;
} wallEntity;

typedef struct weaponInformation {
    const enum weaponType type;
    const bool isPhysicsBullet;
    const uint8_t numProjectiles;
    const float fireMagnitude;
    const float recoilMagnitude;
    const float charge;
    const float coolDown;
    const float maxDistance;
    const float radius;
    const float density;
    const float invMass;
    const uint8_t maxBounces;
    const bool destroyedOnDroneHit;
    const float energyRefill;
} weaponInformation;

typedef struct weaponPickupEntity {
    b2BodyId bodyID;
    b2ShapeId shapeID;
    enum weaponType weapon;
    float respawnWait;
    uint8_t floatingWallsTouching;
    b2Vec2 pos;
    uint16_t mapCellIdx;
} weaponPickupEntity;

typedef struct droneEntity droneEntity;

typedef struct projectileEntity {
    uint8_t droneIdx;

    b2BodyId bodyID;
    b2ShapeId shapeID;
    weaponInformation *weaponInfo;
    b2Vec2 pos;
    b2Vec2 lastPos;
    b2Vec2 velocity;
    float lastSpeed;
    float distance;
    uint8_t bounces;
} projectileEntity;

typedef struct droneStepInfo {
    bool firedShot;
    bool pickedUpWeapon;
    enum weaponType prevWeapon;
    uint8_t shotHit[_MAX_DRONES];
    bool explosionHit[_MAX_DRONES];
    uint8_t shotTaken[_MAX_DRONES];
    bool explosionTaken[_MAX_DRONES];
    bool ownShotTaken;
} droneStepInfo;

typedef struct droneStats {
    float reward;
    float distanceTraveled;
    float absDistanceTraveled;
    float shotsFired[_NUM_WEAPONS];
    float shotsHit[_NUM_WEAPONS];
    float shotsTaken[_NUM_WEAPONS];
    float ownShotsTaken[_NUM_WEAPONS];
    float weaponsPickedUp[_NUM_WEAPONS];
    float shotDistances[_NUM_WEAPONS];
    float lightBrakeTime;
    float heavyBrakeTime;
    float totalBursts;
    float burstsHit;
    float energyEmptied;
    float wins;
} droneStats;

typedef struct droneEntity {
    b2BodyId bodyID;
    b2ShapeId shapeID;
    weaponInformation *weaponInfo;
    int8_t ammo;
    float weaponCooldown;
    uint16_t heat;
    bool chargingWeapon;
    float weaponCharge;
    float energyLeft;
    bool lightBraking;
    bool heavyBraking;
    bool chargingBurst;
    float burstCharge;
    float burstCooldown;
    bool energyFullyDepleted;
    bool energyFullyDepletedThisStep;
    float energyRefillWait;
    bool shotThisStep;
    bool diedThisStep;

    uint8_t idx;
    b2Vec2 initalPos;
    b2Vec2 pos;
    b2Vec2 lastPos;
    b2Vec2 lastMove;
    b2Vec2 lastAim;
    b2Vec2 velocity;
    b2Vec2 lastVelocity;
    bool inLineOfSight[_MAX_DRONES];
    droneStepInfo stepInfo;
    bool dead;
} droneEntity;

typedef struct logEntry {
    float length;
    droneStats stats[_MAX_DRONES];
} logEntry;

typedef struct logBuffer {
    logEntry *logs;
    uint16_t size;
    uint16_t capacity;
} logBuffer;

typedef struct nearEntity {
    void *entity;
    float distance;
} nearEntity;

typedef struct rayClient {
    float scale;
    uint16_t width;
    uint16_t height;
    uint16_t halfWidth;
    uint16_t halfHeight;
} rayClient;

typedef struct brakeTrailPoint {
    b2Vec2 pos;
    bool heavyBrake;
    uint16_t lifetime;
} brakeTrailPoint;

typedef struct explosionInfo {
    b2ExplosionDef def;
    uint16_t renderSteps;
} explosionInfo;

typedef struct agentActions {
    b2Vec2 move;
    b2Vec2 aim;
    bool chargingWeapon;
    bool shoot;
    bool brakeLight;
    bool brakeHeavy;
    bool chargingBurst;
    bool burst;
    bool discardWeapon;
} agentActions;

typedef struct pathingInfo {
    uint8_t *paths;
    int8_t *pathBuffer;
} pathingInfo;

typedef struct env {
    uint8_t numDrones;
    uint8_t numAgents;
    bool sittingDuck;
    bool isTraining;

    uint16_t obsBytes;
    uint16_t mapObsBytes;

    uint8_t *obs;
    float *rewards;
    bool discretizeActions;
    float *contActions;
    int32_t *discActions;
    uint8_t *terminals;
    uint8_t *truncations;

    uint8_t frameRate;
    float deltaTime;
    uint8_t frameSkip;
    uint8_t box2dSubSteps;
    uint64_t randState;
    bool needsReset;

    uint16_t episodeLength;
    logBuffer *logs;
    droneStats stats[_MAX_DRONES];

    b2WorldId worldID;
    int8_t mapIdx;
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

    pathingInfo *mapPathing;

    uint16_t totalSteps;
    uint16_t totalSuddenDeathSteps;
    // steps left until sudden death
    uint16_t stepsLeft;
    // steps left until the next set of sudden death walls are spawned
    uint16_t suddenDeathSteps;
    // the amount of sudden death walls that have been spawned
    uint8_t suddenDeathWallCounter;
    bool suddenDeathWallsPlaced;

    rayClient *client;
    float renderScale;
    CC_Array *brakeTrailPoints;
    // used for rendering explosions
    CC_Array *explosions;
    b2Vec2 debugPoint;
    bool humanInput;
    uint8_t humanDroneInput;
} env;

#endif
