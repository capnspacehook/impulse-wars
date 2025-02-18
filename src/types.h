#ifndef IMPULSE_WARS_TYPES_H
#define IMPULSE_WARS_TYPES_H

#include "box2d/box2d.h"

#include "include/cc_array.h"

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

#define _NUM_WEAPONS 8
const uint8_t NUM_WEAPONS = _NUM_WEAPONS;

enum weaponType {
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
    SNIPER_WEAPON,
    SHOTGUN_WEAPON,
    IMPLODER_WEAPON,
    ACCELERATOR_WEAPON,
    FLAK_CANNON_WEAPON,
    MINE_LAUNCHER_WEAPON,
};

typedef struct mapBounds {
    b2Vec2 min;
    b2Vec2 max;
} mapBounds;

typedef struct nearEntity {
    uint16_t idx;
    void *entity;
    float distanceSquared;
} nearEntity;

typedef struct mapEntry {
    const char *layout;
    const uint8_t columns;
    const uint8_t rows;
    const uint8_t randFloatingStandardWalls;
    const uint8_t randFloatingBouncyWalls;
    const uint8_t randFloatingDeathWalls;
    // are there any floating walls that have consistent starting positions
    const bool hasSetFloatingWalls;
    const uint16_t weaponPickups;
    const enum weaponType defaultWeapon;

    mapBounds bounds;
    mapBounds spawnQuads[4];
    bool *droneSpawns;
    uint8_t *packedLayout;
    nearEntity *nearestWalls;
} mapEntry;

// a cell in the map; ent will be NULL if the cell is empty
typedef struct mapCell {
    entity *ent;
    b2Vec2 pos;
} mapCell;

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
    // should the body be treated as a bullet by box2d; if so CCD
    // (continuous collision detection) will be enabled to prevent
    // tunneling through static bodies which is expensive so it's
    // only enabled for fast moving projectiles
    const bool isPhysicsBullet;
    // can the projectile ever be stationary? if so, it should be
    // allowed to sleep to save on physics updates
    const bool canSleep;
    const uint8_t numProjectiles;
    const float fireMagnitude;
    const float recoilMagnitude;
    const float damping;
    const float charge;
    const float coolDown;
    const float maxDistance;
    const float radius;
    const float density;
    const float invMass;
    const float initialSpeed;
    const uint8_t maxBounces;
    const bool explosive;
    const bool destroyedOnDroneHit;
    const bool explodesOnDroneHit;
    const bool proximityDetonates;
    const float energyRefill;
    const float spawnWeight;
} weaponInformation;

typedef struct weaponPickupEntity {
    b2BodyId bodyID;
    b2ShapeId shapeID;
    enum weaponType weapon;
    float respawnWait;
    // how many floating walls are touching this pickup
    uint8_t floatingWallsTouching;
    b2Vec2 pos;
    int16_t mapCellIdx;

    entity *ent;
    bool bodyDestroyed;
} weaponPickupEntity;

typedef struct droneEntity droneEntity;

typedef struct projectileEntity {
    uint8_t droneIdx;

    b2BodyId bodyID;
    b2ShapeId shapeID;
    // used for proximity explosive projectiles
    b2ShapeId sensorID;
    weaponInformation *weaponInfo;
    b2Vec2 pos;
    int16_t mapCellIdx;
    b2Vec2 lastPos;
    b2Vec2 velocity;
    float speed;
    float lastSpeed;
    float distance;
    uint8_t bounces;
    bool inContact;
    bool setMine;
    bool needsToBeDestroyed;
} projectileEntity;

// used to keep track of what happened each step for reward purposes
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

// stats for the whole episode
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
    float brakeTime;
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
    bool braking;
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
    int16_t mapCellIdx;
    b2Vec2 lastPos;
    b2Vec2 lastMove;
    b2Vec2 lastAim;
    b2Vec2 velocity;
    b2Vec2 lastVelocity;
    droneStepInfo stepInfo;
    bool dead;
} droneEntity;

typedef struct logEntry {
    float length;
    float ties;
    droneStats stats[_MAX_DRONES];
} logEntry;

typedef struct logBuffer {
    logEntry *logs;
    uint16_t size;
    uint16_t capacity;
} logBuffer;

typedef struct rayClient {
    float scale;
    uint16_t width;
    uint16_t height;
    uint16_t halfWidth;
    uint16_t halfHeight;
} rayClient;

typedef struct brakeTrailPoint {
    b2Vec2 pos;
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
    bool brake;
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
    int8_t pinnedMapIdx;
    int8_t mapIdx;
    mapEntry *map;
    int8_t lastSpawnQuad;
    uint8_t spawnedWeaponPickups[_NUM_WEAPONS];
    weaponInformation *defaultWeapon;
    CC_Array *cells;
    CC_Array *walls;
    CC_Array *floatingWalls;
    CC_Array *drones;
    CC_Array *pickups;
    CC_Array *projectiles;
    CC_Array *explodingProjectiles;

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
    uint8_t connectedControllers;
} env;

#endif
