#ifndef IMPULSE_WARS_GAME_H
#define IMPULSE_WARS_GAME_H

#include "env.h"
#include "helpers.h"
#include "settings.h"
#include "types.h"

// these functions call each other so need to be forward declared
void destroyProjectile(env *e, projectileEntity *projectile, const bool processExplosions, const bool full);
void createExplosion(env *e, droneEntity *drone, const enum weaponType *type, const b2ExplosionDef *def);

static inline bool entityTypeIsWall(const enum entityType type) {
    return type == STANDARD_WALL_ENTITY || type == BOUNCY_WALL_ENTITY || type == DEATH_WALL_ENTITY;
}

static inline int16_t cellIndex(const env *e, const int8_t col, const int8_t row) {
    return col + (row * e->columns);
}

// discretizes an entity's position into a cell index; -1 is returned if
// the position is out of bounds of the map
static inline int16_t entityPosToCellIdx(const env *e, const b2Vec2 pos) {
    const float cellX = pos.x + (((float)e->columns * WALL_THICKNESS) / 2.0f);
    const float cellY = pos.y + (((float)e->rows * WALL_THICKNESS) / 2.0f);
    const int8_t cellCol = cellX / WALL_THICKNESS;
    const int8_t cellRow = cellY / WALL_THICKNESS;
    const int16_t cellIdx = cellIndex(e, cellCol, cellRow);
    // set the cell to -1 if it's out of bounds
    // TODO: this is a box2d issue, investigate more
    if (cellIdx < 0 || (uint16_t)cellIdx >= cc_array_size(e->cells)) {
        DEBUG_LOGF("invalid cell index: %d from position: (%f, %f)", cellIdx, pos.x, pos.y);
        return -1;
    }
    return cellIdx;
}

typedef struct overlapCtx {
    const enum entityType *type;
    bool overlaps;
} overlapCtx;

bool overlapCallback(b2ShapeId shapeID, void *context) {
    if (!b2Shape_IsValid(shapeID)) {
        return true;
    }

    overlapCtx *ctx = (overlapCtx *)context;
    if (ctx->type == NULL) {
        ctx->overlaps = true;
        return false;
    }
    const entity *ent = (entity *)b2Shape_GetUserData(shapeID);
    if (ent->type == *ctx->type) {
        ctx->overlaps = true;
        return false;
    }
    return true;
}

// returns true if the given position overlaps with a bounding box with
// a height and width of distance with shape categories specified in maskBits;
// if type is set only entities that match a shape category in maskBits *and*
// that have the entity type specified will be considered
bool isOverlappingAABB(env *e, const b2Vec2 pos, const float distance, const enum shapeCategory category, const uint64_t maskBits, const enum entityType *type) {
    b2AABB bounds = {
        .lowerBound = {.x = pos.x - distance, .y = pos.y - distance},
        .upperBound = {.x = pos.x + distance, .y = pos.y + distance},
    };
    b2QueryFilter filter = {
        .categoryBits = category,
        .maskBits = maskBits,
    };
    overlapCtx ctx = {.type = type, .overlaps = false};
    b2World_OverlapAABB(e->worldID, bounds, filter, overlapCallback, &ctx);
    return ctx.overlaps;
}

// returns true if the given position overlaps with a circle
// with shape categories specified in maskBits;
// if type is set only entities that match a shape category in maskBits *and*
// that have the entity type specified will be considered
bool isOverlappingCircle(env *e, const b2Vec2 pos, const float radius, const enum shapeCategory category, const uint64_t maskBits, const enum entityType *type) {
    const b2Circle circle = {.center = b2Vec2_zero, .radius = radius};
    const b2Transform transform = {.p = pos, .q = b2Rot_identity};
    b2QueryFilter filter = {
        .categoryBits = category,
        .maskBits = maskBits,
    };
    overlapCtx ctx = {.type = type, .overlaps = false};
    b2World_OverlapCircle(e->worldID, &circle, transform, filter, overlapCallback, &ctx);
    return ctx.overlaps;
}

// returns true and sets emptyPos to the position of an empty cell
// that is an appropriate distance away from other entities if one exists
bool findOpenPos(env *e, const enum shapeCategory type, b2Vec2 *emptyPos) {
    uint8_t checkedCells[BITNSLOTS(MAX_CELLS)] = {0};
    const size_t nCells = cc_array_size(e->cells);
    uint16_t attempts = 0;

    while (true) {
        if (attempts == nCells) {
            return false;
        }
        const int cellIdx = randInt(&e->randState, 0, nCells - 1);
        if (bitTest(checkedCells, cellIdx)) {
            continue;
        }
        bitSet(checkedCells, cellIdx);
        attempts++;

        const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        if (cell->ent != NULL) {
            continue;
        }

        // ensure drones don't spawn too close to walls or other drones
        if (type == WEAPON_PICKUP_SHAPE) {
            if (isOverlappingAABB(e, cell->pos, PICKUP_SPAWN_DISTANCE, WEAPON_PICKUP_SHAPE, WEAPON_PICKUP_SHAPE, NULL)) {
                continue;
            }
        } else if (type == DRONE_SHAPE) {
            const enum entityType deathWallType = DEATH_WALL_ENTITY;
            if (isOverlappingAABB(e, cell->pos, DRONE_DEATH_WALL_SPAWN_DISTANCE, DRONE_SHAPE, WALL_SHAPE | DRONE_SHAPE, &deathWallType)) {
                continue;
            }
            if (isOverlappingAABB(e, cell->pos, DRONE_WALL_SPAWN_DISTANCE, DRONE_SHAPE, WALL_SHAPE | DRONE_SHAPE, NULL)) {
                continue;
            }
            if (isOverlappingAABB(e, cell->pos, DRONE_DRONE_SPAWN_DISTANCE, DRONE_SHAPE, DRONE_SHAPE, NULL)) {
                continue;
            }
        }

        if (!isOverlappingAABB(e, cell->pos, MIN_SPAWN_DISTANCE, type, FLOATING_WALL_SHAPE | WEAPON_PICKUP_SHAPE | DRONE_SHAPE, NULL)) {
            *emptyPos = cell->pos;
            return true;
        }
    }
}

entity *createWall(const env *e, const float posX, const float posY, const float width, const float height, uint16_t cellIdx, const enum entityType type, const bool floating) {
    ASSERT(entityTypeIsWall(type));

    if (floating) {
        cellIdx = -1;
    }
    const b2Vec2 pos = (b2Vec2){.x = posX, .y = posY};

    b2BodyDef wallBodyDef = b2DefaultBodyDef();
    wallBodyDef.position = pos;
    if (floating) {
        wallBodyDef.type = b2_dynamicBody;
        wallBodyDef.linearDamping = FLOATING_WALL_DAMPING;
        wallBodyDef.angularDamping = FLOATING_WALL_DAMPING;
        wallBodyDef.isAwake = false;
    }
    b2BodyId wallBodyID = b2CreateBody(e->worldID, &wallBodyDef);
    b2Vec2 extent = {.x = width / 2.0f, .y = height / 2.0f};
    b2ShapeDef wallShapeDef = b2DefaultShapeDef();
    wallShapeDef.density = WALL_DENSITY;
    wallShapeDef.restitution = STANDARD_WALL_RESTITUTION;
    wallShapeDef.filter.categoryBits = WALL_SHAPE;
    wallShapeDef.filter.maskBits = FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | WEAPON_PICKUP_SHAPE | DRONE_SHAPE;
    if (floating) {
        wallShapeDef.filter.categoryBits = FLOATING_WALL_SHAPE;
        wallShapeDef.filter.maskBits |= WALL_SHAPE | WEAPON_PICKUP_SHAPE;
        wallShapeDef.enableSensorEvents = true;
    }

    if (type == BOUNCY_WALL_ENTITY) {
        wallShapeDef.restitution = BOUNCY_WALL_RESTITUTION;
    }
    if (type == DEATH_WALL_ENTITY) {
        wallShapeDef.enableContactEvents = true;
    }

    wallEntity *wall = (wallEntity *)fastCalloc(1, sizeof(wallEntity));
    wall->bodyID = wallBodyID;
    wall->pos = pos;
    wall->rot = b2Rot_identity;
    wall->velocity = b2Vec2_zero;
    wall->extent = extent;
    wall->mapCellIdx = cellIdx;
    wall->isFloating = floating;
    wall->type = type;
    wall->isSuddenDeath = e->suddenDeathWallsPlaced;

    entity *ent = (entity *)fastCalloc(1, sizeof(entity));
    ent->type = type;
    ent->entity = wall;

    wallShapeDef.userData = ent;
    const b2Polygon wallPolygon = b2MakeBox(extent.x, extent.y);
    wall->shapeID = b2CreatePolygonShape(wallBodyID, &wallShapeDef, &wallPolygon);
    b2Body_SetUserData(wall->bodyID, ent);

    if (floating) {
        cc_array_add(e->floatingWalls, wall);
    } else {
        cc_array_add(e->walls, wall);
    }

    return ent;
}

void destroyWall(const env *e, wallEntity *wall, const bool full) {
    entity *ent = (entity *)b2Shape_GetUserData(wall->shapeID);
    fastFree(ent);

    if (full) {
        mapCell *cell = safe_array_get_at(e->cells, wall->mapCellIdx);
        cell->ent = NULL;
    }

    b2DestroyBody(wall->bodyID);
    fastFree(wall);
}

enum weaponType randWeaponPickupType(env *e) {
    while (true) {
        const enum weaponType type = (enum weaponType)randInt(&e->randState, STANDARD_WEAPON + 1, NUM_WEAPONS - 1);
        if (type != e->defaultWeapon->type) {
            return type;
        }
    }
}

void createWeaponPickup(env *e) {
    b2BodyDef pickupBodyDef = b2DefaultBodyDef();
    b2Vec2 pos;
    if (!findOpenPos(e, WEAPON_PICKUP_SHAPE, &pos)) {
        ERROR("no open position for weapon pickup");
    }
    pickupBodyDef.position = pos;
    b2BodyId pickupBodyID = b2CreateBody(e->worldID, &pickupBodyDef);
    b2ShapeDef pickupShapeDef = b2DefaultShapeDef();
    pickupShapeDef.filter.categoryBits = WEAPON_PICKUP_SHAPE;
    pickupShapeDef.filter.maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | WEAPON_PICKUP_SHAPE | DRONE_SHAPE;
    pickupShapeDef.isSensor = true;

    weaponPickupEntity *pickup = (weaponPickupEntity *)fastCalloc(1, sizeof(weaponPickupEntity));
    pickup->bodyID = pickupBodyID;
    pickup->weapon = randWeaponPickupType(e);
    pickup->respawnWait = 0.0f;
    pickup->floatingWallsTouching = 0;
    pickup->pos = pos;

    entity *ent = (entity *)fastCalloc(1, sizeof(entity));
    ent->type = WEAPON_PICKUP_ENTITY;
    ent->entity = pickup;

    const int16_t cellIdx = entityPosToCellIdx(e, pos);
    if (cellIdx == -1) {
        ERRORF("invalid position for weapon pickup spawn: (%f, %f)", pos.x, pos.y);
    }
    pickup->mapCellIdx = cellIdx;
    mapCell *cell = safe_array_get_at(e->cells, cellIdx);
    cell->ent = ent;

    pickupShapeDef.userData = ent;
    const b2Polygon pickupPolygon = b2MakeBox(PICKUP_THICKNESS / 2.0f, PICKUP_THICKNESS / 2.0f);
    pickup->shapeID = b2CreatePolygonShape(pickupBodyID, &pickupShapeDef, &pickupPolygon);
    b2Body_SetUserData(pickup->bodyID, ent);

    cc_array_add(e->pickups, pickup);
}

void destroyWeaponPickup(const env *e, weaponPickupEntity *pickup) {
    entity *ent = (entity *)b2Shape_GetUserData(pickup->shapeID);
    fastFree(ent);

    mapCell *cell = safe_array_get_at(e->cells, pickup->mapCellIdx);
    cell->ent = NULL;

    b2DestroyBody(pickup->bodyID);

    fastFree(pickup);
}

void createDrone(env *e, const uint8_t idx) {
    b2BodyDef droneBodyDef = b2DefaultBodyDef();
    droneBodyDef.type = b2_dynamicBody;
    if (!findOpenPos(e, DRONE_SHAPE, &droneBodyDef.position)) {
        ERROR("no open position for drone");
    }
    droneBodyDef.fixedRotation = true;
    droneBodyDef.linearDamping = DRONE_LINEAR_DAMPING;
    b2BodyId droneBodyID = b2CreateBody(e->worldID, &droneBodyDef);
    b2ShapeDef droneShapeDef = b2DefaultShapeDef();
    droneShapeDef.density = DRONE_DENSITY;
    droneShapeDef.friction = 0.0f;
    droneShapeDef.restitution = 0.3f;
    droneShapeDef.filter.categoryBits = DRONE_SHAPE;
    droneShapeDef.filter.maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | WEAPON_PICKUP_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE;
    droneShapeDef.enableContactEvents = true;
    droneShapeDef.enableSensorEvents = true;
    const b2Circle droneCircle = {.center = b2Vec2_zero, .radius = DRONE_RADIUS};

    droneEntity *drone = (droneEntity *)fastCalloc(1, sizeof(droneEntity));
    drone->bodyID = droneBodyID;
    drone->weaponInfo = e->defaultWeapon;
    drone->ammo = weaponAmmo(e->defaultWeapon->type, drone->weaponInfo->type);
    drone->weaponCooldown = 0.0f;
    drone->heat = 0;
    drone->chargingWeapon = false;
    drone->weaponCharge = 0.0f;
    drone->energyLeft = DRONE_ENERGY_MAX;
    drone->lightBraking = false;
    drone->heavyBraking = false;
    drone->chargingBurst = false;
    drone->burstCharge = 0.0f;
    drone->burstCooldown = 0.0f;
    drone->energyFullyDepleted = false;
    drone->energyFullyDepletedThisStep = false;
    drone->energyRefillWait = 0.0f;
    drone->shotThisStep = false;
    drone->diedThisStep = false;
    drone->idx = idx;
    drone->initalPos = droneBodyDef.position;
    drone->pos = droneBodyDef.position;
    drone->lastPos = b2Vec2_zero;
    drone->lastMove = b2Vec2_zero;
    drone->lastAim = (b2Vec2){.x = 0.0f, .y = -1.0f};
    drone->velocity = b2Vec2_zero;
    drone->lastVelocity = b2Vec2_zero;
    drone->dead = false;
    memset(&drone->stepInfo, 0x0, sizeof(droneStepInfo));
    memset(&drone->inLineOfSight, 0x0, sizeof(drone->inLineOfSight));

    entity *ent = (entity *)fastCalloc(1, sizeof(entity));
    ent->type = DRONE_ENTITY;
    ent->entity = drone;

    droneShapeDef.userData = ent;
    drone->shapeID = b2CreateCircleShape(droneBodyID, &droneShapeDef, &droneCircle);
    b2Body_SetUserData(drone->bodyID, ent);

    cc_array_add(e->drones, drone);
}

void destroyDrone(droneEntity *drone) {
    entity *ent = (entity *)b2Shape_GetUserData(drone->shapeID);
    fastFree(ent);

    b2DestroyBody(drone->bodyID);
    fastFree(drone);
}

void killDrone(env *e, droneEntity *drone) {
    if (drone->dead) {
        return;
    }
    DEBUG_LOGF("drone %d died", drone->idx);

    drone->dead = true;
    drone->diedThisStep = true;
    if (e->numDrones == 2) {
        return;
    }

    b2Body_Disable(drone->bodyID);
    drone->lightBraking = false;
    drone->heavyBraking = false;
    drone->chargingBurst = false;
    drone->energyFullyDepleted = false;
    drone->shotThisStep = false;
}

void createProjectile(env *e, droneEntity *drone, const b2Vec2 normAim) {
    ASSERT_VEC_NORMALIZED(normAim);

    b2BodyDef projectileBodyDef = b2DefaultBodyDef();
    projectileBodyDef.type = b2_dynamicBody;
    projectileBodyDef.fixedRotation = true;
    projectileBodyDef.isBullet = drone->weaponInfo->isPhysicsBullet;
    projectileBodyDef.enableSleep = false;
    float radius = drone->weaponInfo->radius;
    projectileBodyDef.position = b2MulAdd(drone->pos, 1.0f + (radius * 1.5f), normAim);
    b2BodyId projectileBodyID = b2CreateBody(e->worldID, &projectileBodyDef);
    b2ShapeDef projectileShapeDef = b2DefaultShapeDef();
    projectileShapeDef.enableContactEvents = true;
    projectileShapeDef.density = drone->weaponInfo->density;
    projectileShapeDef.friction = 0.0f;
    projectileShapeDef.restitution = 1.0f;
    projectileShapeDef.filter.categoryBits = PROJECTILE_SHAPE;
    projectileShapeDef.filter.maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE;
    const b2Circle projectileCircle = {.center = b2Vec2_zero, .radius = radius};

    b2ShapeId projectileShapeID = b2CreateCircleShape(projectileBodyID, &projectileShapeDef, &projectileCircle);

    // add lateral drone velocity to projectile
    b2Vec2 forwardVel = b2MulSV(b2Dot(drone->velocity, normAim), normAim);
    b2Vec2 lateralVel = b2Sub(drone->velocity, forwardVel);
    lateralVel = b2MulSV(projectileShapeDef.density / DRONE_MOVE_AIM_DIVISOR, lateralVel);
    b2Vec2 aim = weaponAdjustAim(&e->randState, drone->weaponInfo->type, drone->heat, normAim);
    b2Vec2 fire = b2MulAdd(lateralVel, weaponFire(&e->randState, drone->weaponInfo->type), aim);
    b2Body_ApplyLinearImpulseToCenter(projectileBodyID, fire, true);

    projectileEntity *projectile = (projectileEntity *)fastCalloc(1, sizeof(projectileEntity));
    projectile->droneIdx = drone->idx;
    projectile->bodyID = projectileBodyID;
    projectile->shapeID = projectileShapeID;
    projectile->weaponInfo = drone->weaponInfo;
    projectile->pos = projectileBodyDef.position;
    projectile->lastPos = projectileBodyDef.position;
    projectile->velocity = b2Body_GetLinearVelocity(projectileBodyID);
    projectile->lastSpeed = b2Length(projectile->velocity);
    projectile->distance = 0.0f;
    projectile->bounces = 0;
    projectile->needsToBeDestroyed = false;
    cc_slist_add(e->projectiles, projectile);

    entity *ent = (entity *)fastCalloc(1, sizeof(entity));
    ent->type = PROJECTILE_ENTITY;
    ent->entity = projectile;

    b2Body_SetUserData(projectile->bodyID, ent);
    b2Shape_SetUserData(projectile->shapeID, ent);

    if (projectile->weaponInfo->proximityDetonates) {
        projectile->sensorID = weaponSensor(projectile->bodyID, projectile->weaponInfo->type);
        b2Shape_SetUserData(projectile->sensorID, ent);
    }
}

b2ShapeProxy makeDistanceProxyFromType(const enum entityType type, bool *isCircle) {
    b2ShapeProxy proxy = {0};
    switch (type) {
    case DRONE_ENTITY:
        *isCircle = true;
        proxy.radius = DRONE_RADIUS;
        break;
    case WEAPON_PICKUP_ENTITY:
        proxy.count = 4;
        proxy.points[0] = (b2Vec2){.x = -PICKUP_THICKNESS / 2.0f, .y = -PICKUP_THICKNESS / 2.0f};
        proxy.points[1] = (b2Vec2){.x = -PICKUP_THICKNESS / 2.0f, .y = +PICKUP_THICKNESS / 2.0f};
        proxy.points[2] = (b2Vec2){.x = +PICKUP_THICKNESS / 2.0f, .y = -PICKUP_THICKNESS / 2.0f};
        proxy.points[3] = (b2Vec2){.x = +PICKUP_THICKNESS / 2.0f, .y = +PICKUP_THICKNESS / 2.0f};
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        proxy.count = 4;
        proxy.points[0] = (b2Vec2){.x = -FLOATING_WALL_THICKNESS / 2.0f, .y = -FLOATING_WALL_THICKNESS / 2.0f};
        proxy.points[1] = (b2Vec2){.x = -FLOATING_WALL_THICKNESS / 2.0f, .y = +FLOATING_WALL_THICKNESS / 2.0f};
        proxy.points[2] = (b2Vec2){.x = +FLOATING_WALL_THICKNESS / 2.0f, .y = -FLOATING_WALL_THICKNESS / 2.0f};
        proxy.points[3] = (b2Vec2){.x = +FLOATING_WALL_THICKNESS / 2.0f, .y = +FLOATING_WALL_THICKNESS / 2.0f};
        break;
    default:
        ERRORF("unknown entity type for shape distance: %d", type);
    }

    return proxy;
}

b2ShapeProxy makeDistanceProxy(const entity *ent, bool *isCircle) {
    if (ent->type == PROJECTILE_ENTITY) {
        b2ShapeProxy proxy = {0};
        const projectileEntity *proj = (projectileEntity *)ent->entity;
        proxy.radius = proj->weaponInfo->radius;
        return proxy;
    }

    return makeDistanceProxyFromType(ent->type, isCircle);
}

// simplified and copied from box2d/src/shape.c
float getShapeProjectedPerimeter(const b2ShapeId shapeID, const b2Vec2 line) {
    if (b2Shape_GetType(shapeID) == b2_circleShape) {
        const b2Circle circle = b2Shape_GetCircle(shapeID);
        return circle.radius * 2.0f;
    }

    const b2Polygon polygon = b2Shape_GetPolygon(shapeID);
    const b2Vec2 *points = polygon.vertices;
    int count = polygon.count;
    B2_ASSERT(count > 0);
    float value = b2Dot(points[0], line);
    float lower = value;
    float upper = value;
    for (int i = 1; i < count; ++i) {
        value = b2Dot(points[i], line);
        lower = b2MinFloat(lower, value);
        upper = b2MaxFloat(upper, value);
    }

    return upper - lower;
}

// explodes projectile and ensures any other projectiles that are caught
// in the explosion are also destroyed if necessary
void createProjectileExplosion(env *e, projectileEntity *projectile, const bool initalProjectile, const bool destroyProjectiles) {
    b2ExplosionDef explosion;
    weaponExplosion(projectile->weaponInfo->type, &explosion);
    explosion.position = projectile->pos;
    explosion.maskBits = FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE;
    droneEntity *parentDrone = safe_array_get_at(e->drones, projectile->droneIdx);
    cc_array_add(e->explodingProjectiles, projectile);
    createExplosion(e, parentDrone, &projectile->weaponInfo->type, &explosion);

    if (e->client != NULL) {
        explosionInfo *explInfo = fastCalloc(1, sizeof(explosionInfo));
        explInfo->def = explosion;
        explInfo->renderSteps = UINT16_MAX;
        cc_array_add(e->explosions, explInfo);
    }

    if (!initalProjectile) {
        return;
    }
    // destroy all other projectiles that were caught in the explosion
    // and also exploded now that the initial projectile has been destroyed
    for (uint8_t i = 1; i < cc_array_size(e->explodingProjectiles); i++) {
        projectileEntity *proj = safe_array_get_at(e->explodingProjectiles, i);
        if (destroyProjectiles) {
            destroyProjectile(e, proj, false, true);
        } else {
            proj->needsToBeDestroyed = true;
        }
    }
    if (destroyProjectiles) {
        cc_array_remove_all(e->explodingProjectiles);
    } else {
        // if we're not destroying the projectiles now, we need to remove the initial projectile
        // from the list of exploding projectiles so it's not destroyed twice
        const enum cc_stat res = cc_array_remove(e->explodingProjectiles, projectile, NULL);
        MAYBE_UNUSED(res);
        ASSERT(res == CC_OK);
    }
}

typedef struct explosionCtx {
    env *e;
    const bool isBurst;
    droneEntity *parentDrone;
    const enum weaponType *weaponType;
    const b2ExplosionDef *def;
} explosionCtx;

// b2World_Explode doesn't support filtering on shapes of the same category,
// so we have to do it manually
// mostly copied from box2d/src/world.c
bool explodeCallback(b2ShapeId shapeID, void *context) {
    if (!b2Shape_IsValid(shapeID)) {
        return true;
    }

    const explosionCtx *ctx = context;
    const entity *entity = b2Shape_GetUserData(shapeID);
    DEBUG_LOGF("exploding entity type %d", entity->type);
    droneEntity *drone = NULL;
    wallEntity *wall = NULL;
    projectileEntity *projectile = NULL;

    // the explosion shouldn't affect the parent drone
    if (ctx->isBurst && entity->type == DRONE_ENTITY) {
        drone = entity->entity;
        if (drone->idx == ctx->parentDrone->idx) {
            if (!ctx->isBurst) {
                drone->stepInfo.ownShotTaken = true;
                ctx->e->stats[drone->idx].ownShotsTaken[*ctx->weaponType]++;
                DEBUG_LOGF("drone %d hit itself with explosion from weapon %d", drone->idx, *ctx->weaponType);
            }
            return true;
        }
        ctx->parentDrone->stepInfo.explosionHit[drone->idx] = true;
        if (ctx->isBurst) {
            ctx->e->stats[ctx->parentDrone->idx].burstsHit++;
        } else {
            ctx->e->stats[ctx->parentDrone->idx].shotsHit[*ctx->weaponType]++;
            ctx->e->stats[drone->idx].shotsTaken[*ctx->weaponType]++;
        }
        DEBUG_LOGF("drone %d hit drone %d with burst", ctx->parentDrone->idx, drone->idx);
        drone->stepInfo.explosionTaken[ctx->parentDrone->idx] = true;
        DEBUG_LOGF("drone %d hit burst from drone %d", drone->idx, ctx->parentDrone->idx);
    }
    bool isStaticWall = false;
    bool isFloatingWall = false;
    if (entityTypeIsWall(entity->type)) {
        wall = (wallEntity *)entity->entity;
        isStaticWall = !wall->isFloating;
        isFloatingWall = wall->isFloating;
    }
    // normal explosions don't affect static walls
    if (!ctx->isBurst && isStaticWall) {
        return true;
    }

    b2BodyId bodyID = b2Shape_GetBody(shapeID);
    ASSERT(b2Body_IsValid(bodyID));
    b2Transform transform = b2Body_GetTransform(bodyID);

    bool isCircle = false;
    b2DistanceInput input;
    input.proxyA = makeDistanceProxy(entity, &isCircle);
    input.proxyB = b2MakeProxy(&ctx->def->position, 1, 0.0f);
    input.transformA = transform;
    input.transformB = b2Transform_identity;
    input.useRadii = isCircle;

    b2SimplexCache cache = {0};
    const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);
    // don't consider for static walls so burst pushback isn't as
    // surprising to players
    if (output.distance > ctx->def->radius + ctx->def->falloff || (isStaticWall && output.distance > ctx->def->radius)) {
        return true;
    }

    const b2Vec2 closestPoint = output.pointA;
    b2Vec2 direction;
    if (isStaticWall) {
        direction = b2Normalize(b2Sub(ctx->def->position, closestPoint));
    } else {
        direction = b2Normalize(b2Sub(closestPoint, ctx->def->position));
    }

    b2Vec2 localLine = b2InvRotateVector(transform.q, b2LeftPerp(direction));
    float perimeter = getShapeProjectedPerimeter(shapeID, localLine);
    float scale = 1.0f;
    // ignore falloff for projectiles to avoid slowing them down to a crawl
    if (output.distance > ctx->def->radius && entity->type != PROJECTILE_ENTITY) {
        scale = clamp((ctx->def->radius + ctx->def->falloff - output.distance) / ctx->def->falloff);
    }

    float magnitude = (ctx->def->impulsePerLength + b2Length(ctx->parentDrone->lastVelocity)) * perimeter * scale;
    if (isStaticWall) {
        // reduce the magnitude when pushing a drone away from a wall
        magnitude = log2f(magnitude) * 7.5f;
    }
    const b2Vec2 impulse = b2MulSV(magnitude, direction);

    if (isStaticWall) {
        b2Body_ApplyLinearImpulseToCenter(ctx->parentDrone->bodyID, impulse, true);
    } else {
        b2Body_ApplyLinearImpulseToCenter(bodyID, impulse, true);
        switch (entity->type) {
        case STANDARD_WALL_ENTITY:
        case BOUNCY_WALL_ENTITY:
        case DEATH_WALL_ENTITY:
            wall->velocity = b2Body_GetLinearVelocity(wall->bodyID);
            if (isFloatingWall) {
                // floating walls are the only bodies that can rotate
                b2Body_ApplyAngularImpulse(bodyID, magnitude, true);
            }
            break;
        case PROJECTILE_ENTITY:
            projectile = entity->entity;
            // mine launcher projectiles explode when caught in another
            // explosion, explode this mine only once
            if (projectile->weaponInfo->type == MINE_LAUNCHER_WEAPON) {
                if (!cc_array_contains(ctx->e->explodingProjectiles, projectile)) {
                    createProjectileExplosion(ctx->e, projectile, false, false);
                }
                break;
            }

            projectile->velocity = b2Body_GetLinearVelocity(projectile->bodyID);
            projectile->lastSpeed = b2Length(projectile->velocity);
            break;
        case DRONE_ENTITY:
            drone = entity->entity;
            drone->lastVelocity = drone->velocity;
            drone->velocity = b2Body_GetLinearVelocity(drone->bodyID);
            break;
        default:
            ERRORF("unknown entity type for burst impulse %d", entity->type);
        }
    }

    return true;
}

void createExplosion(env *e, droneEntity *drone, const enum weaponType *type, const b2ExplosionDef *def) {
    const float fullRadius = def->radius + def->falloff;
    b2AABB aabb = {
        .lowerBound.x = def->position.x - fullRadius,
        .lowerBound.y = def->position.y - fullRadius,
        .upperBound.x = def->position.x + fullRadius,
        .upperBound.y = def->position.y + fullRadius,
    };

    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.categoryBits = PROJECTILE_SHAPE;
    filter.maskBits = def->maskBits;

    explosionCtx ctx = {
        .e = e,
        .isBurst = type == NULL,
        .parentDrone = drone,
        .weaponType = type,
        .def = def,
    };
    b2World_OverlapAABB(e->worldID, aabb, filter, explodeCallback, &ctx);
}

void destroyProjectile(env *e, projectileEntity *projectile, const bool processExplosions, const bool full) {
    // explode projectile if necessary
    if (processExplosions && projectile->weaponInfo->explosive) {
        createProjectileExplosion(e, projectile, true, full);
    }

    entity *ent = (entity *)b2Shape_GetUserData(projectile->shapeID);
    fastFree(ent);

    b2DestroyBody(projectile->bodyID);

    if (full) {
        const enum cc_stat res = cc_slist_remove(e->projectiles, projectile, NULL);
        MAYBE_UNUSED(res);
        ASSERT(res == CC_OK);
    } else {
        // only add to the stats if we are not clearing the environment,
        // otherwise this projectile's distance will be counted twice
        e->stats[projectile->droneIdx].shotDistances[projectile->droneIdx] += projectile->distance;
    }

    fastFree(projectile);
}

void createSuddenDeathWalls(env *e, const b2Vec2 startPos, const b2Vec2 size) {
    int16_t endIdx;
    uint8_t indexIncrement;
    if (size.y == WALL_THICKNESS) {
        const b2Vec2 endPos = (b2Vec2){.x = startPos.x + size.x, .y = startPos.y};
        endIdx = entityPosToCellIdx(e, endPos);
        if (endIdx == -1) {
            ERRORF("invalid position for sudden death wall: (%f, %f)", endPos.x, endPos.y);
        }
        indexIncrement = 1;
    } else {
        const b2Vec2 endPos = (b2Vec2){.x = startPos.x, .y = startPos.y + size.y};
        endIdx = entityPosToCellIdx(e, endPos);
        if (endIdx == -1) {
            ERRORF("invalid position for sudden death wall: (%f, %f)", endPos.x, endPos.y);
        }
        indexIncrement = e->rows;
    }
    const int16_t startIdx = entityPosToCellIdx(e, startPos);
    if (startIdx == -1) {
        ERRORF("invalid position for sudden death wall: (%f, %f)", startPos.x, startPos.y);
    }
    for (uint16_t i = startIdx; i <= endIdx; i += indexIncrement) {
        mapCell *cell = safe_array_get_at(e->cells, i);
        if (cell->ent != NULL) {
            if (cell->ent->type == WEAPON_PICKUP_ENTITY) {
                weaponPickupEntity *pickup = (weaponPickupEntity *)cell->ent->entity;
                pickup->respawnWait = PICKUP_RESPAWN_WAIT;
            } else {
                continue;
            }
        }
        entity *ent = createWall(e, cell->pos.x, cell->pos.y, WALL_THICKNESS, WALL_THICKNESS, i, DEATH_WALL_ENTITY, false);
        cell->ent = ent;
    }
}

// TODO: fix for maps with different columns and rows
void handleSuddenDeath(env *e) {
    ASSERT(e->suddenDeathSteps == 0);

    // create new walls that will close in on the arena
    e->suddenDeathWallCounter++;
    e->suddenDeathWallsPlaced = true;

    // TODO: these magic numbers can probably be simplified somehow
    createSuddenDeathWalls(
        e,
        (b2Vec2){
            .x = e->bounds.min.x + ((e->suddenDeathWallCounter - 1) * WALL_THICKNESS),
            .y = e->bounds.min.y + ((WALL_THICKNESS * (e->suddenDeathWallCounter - 1)) + (WALL_THICKNESS / 2)),
        },
        (b2Vec2){
            .x = WALL_THICKNESS * (e->columns - (e->suddenDeathWallCounter * 2) - 1),
            .y = WALL_THICKNESS,
        });
    createSuddenDeathWalls(
        e,
        (b2Vec2){
            .x = e->bounds.min.x + ((e->suddenDeathWallCounter - 1) * WALL_THICKNESS),
            .y = e->bounds.max.y - ((WALL_THICKNESS * (e->suddenDeathWallCounter - 1)) + (WALL_THICKNESS / 2)),
        },
        (b2Vec2){
            .x = WALL_THICKNESS * (e->columns - (e->suddenDeathWallCounter * 2) - 1),
            .y = WALL_THICKNESS,
        });
    createSuddenDeathWalls(
        e,
        (b2Vec2){
            .x = e->bounds.min.x + ((e->suddenDeathWallCounter - 1) * WALL_THICKNESS),
            .y = e->bounds.min.y + (e->suddenDeathWallCounter * WALL_THICKNESS),
        },
        (b2Vec2){
            .x = WALL_THICKNESS,
            .y = WALL_THICKNESS * (e->rows - (e->suddenDeathWallCounter * 2) - 2),
        });
    createSuddenDeathWalls(
        e,
        (b2Vec2){
            .x = e->bounds.min.x + ((e->columns - e->suddenDeathWallCounter - 2) * WALL_THICKNESS),
            .y = e->bounds.min.y + (e->suddenDeathWallCounter * WALL_THICKNESS),
        },
        (b2Vec2){
            .x = WALL_THICKNESS,
            .y = WALL_THICKNESS * (e->rows - (e->suddenDeathWallCounter * 2) - 2),
        });

    // mark drones as dead if they touch a newly placed wall
    bool droneDead = false;
    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (isOverlappingAABB(e, drone->pos, DRONE_RADIUS, DRONE_SHAPE, WALL_SHAPE, NULL)) {
            killDrone(e, drone);
            droneDead = true;
        }
    }
    if (droneDead && e->numDrones == 2) {
        return;
    }

    // make floating walls static bodies if they are now overlapping with
    // a newly placed wall, but destroy them if they are fully inside a wall
    CC_ArrayIter floatingWallIter;
    cc_array_iter_init(&floatingWallIter, e->floatingWalls);
    wallEntity *wall;
    while (cc_array_iter_next(&floatingWallIter, (void **)&wall) != CC_ITER_END) {
        int16_t cellIdx = entityPosToCellIdx(e, wall->pos);
        if (cellIdx == -1) {
            ERRORF("floating wall is out of bounds at %f, %f", wall->pos.x, wall->pos.y);
        }

        const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
            // floating wall is overlapping with a wall, destroy it
            const enum cc_stat res = cc_array_iter_remove(&floatingWallIter, NULL);
            MAYBE_UNUSED(res);
            ASSERT(res == CC_OK);

            const b2Vec2 wallPos = wall->pos;
            MAYBE_UNUSED(wallPos);
            destroyWall(e, wall, false);
            DEBUG_LOGF("destroyed floating wall at %f, %f", wallPos.x, wallPos.y);
            continue;
        }
    }

    // detroy all projectiles that are now overlapping with a newly placed wall
    CC_SListIter projectileIter;
    cc_slist_iter_init(&projectileIter, e->projectiles);
    projectileEntity *projectile;
    while (cc_slist_iter_next(&projectileIter, (void **)&projectile) != CC_ITER_END) {
        const int16_t cellIdx = entityPosToCellIdx(e, projectile->pos);
        if (cellIdx == -1) {
            continue;
        }
        const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
            cc_slist_iter_remove(&projectileIter, NULL);
            destroyProjectile(e, projectile, false, false);
        }
    }
}

void droneMove(droneEntity *drone, b2Vec2 direction) {
    ASSERT_VEC_BOUNDED(direction);

    // if energy is fully depleted halve movement until energy starts
    // to refill again
    if (drone->energyFullyDepleted && drone->energyRefillWait != 0.0f) {
        direction = b2MulSV(0.5f, direction);
        drone->lastMove = direction;
    }
    b2Vec2 force = b2MulSV(DRONE_MOVE_MAGNITUDE, direction);
    b2Body_ApplyForceToCenter(drone->bodyID, force, true);
}

void droneChangeWeapon(const env *e, droneEntity *drone, const enum weaponType newWeapon) {
    // only top up ammo if the weapon is the same
    if (drone->weaponInfo->type != newWeapon) {
        drone->weaponCooldown = 0.0f;
        drone->weaponCharge = 0.0f;
        drone->heat = 0;
    }
    drone->weaponInfo = weaponInfos[newWeapon];
    drone->ammo = weaponAmmo(e->defaultWeapon->type, drone->weaponInfo->type);
}

void droneShoot(env *e, droneEntity *drone, const b2Vec2 aim, const bool chargingWeapon) {
    ASSERT(drone->ammo != 0);

    drone->shotThisStep = true;
    // TODO: rework heat to only increase when projectiles are fired,
    // and only cool down after the next shot was skipped
    drone->heat++;
    if (drone->weaponCooldown != 0.0f) {
        return;
    }
    const bool weaponNeedsCharge = drone->weaponInfo->charge != 0.0f;
    if (weaponNeedsCharge && chargingWeapon) {
        drone->chargingWeapon = true;
        drone->weaponCharge = fminf(drone->weaponCharge + e->deltaTime, drone->weaponInfo->charge);
    }
    // if the weapon needs to be charged, only fire the weapon if it's
    // fully charged and the agent released the trigger
    if (weaponNeedsCharge && (chargingWeapon || drone->weaponCharge < drone->weaponInfo->charge)) {
        return;
    }

    if (drone->ammo != INFINITE) {
        drone->ammo--;
    }
    drone->weaponCooldown = drone->weaponInfo->coolDown;
    drone->chargingWeapon = false;
    drone->weaponCharge = 0.0f;

    b2Vec2 normAim = drone->lastAim;
    if (!b2VecEqual(aim, b2Vec2_zero)) {
        normAim = b2Normalize(aim);
    }
    ASSERT_VEC_NORMALIZED(normAim);
    b2Vec2 recoil = b2MulSV(-drone->weaponInfo->recoilMagnitude, normAim);
    b2Body_ApplyLinearImpulseToCenter(drone->bodyID, recoil, true);

    for (int i = 0; i < drone->weaponInfo->numProjectiles; i++) {
        createProjectile(e, drone, normAim);

        e->stats[drone->idx].shotsFired[drone->weaponInfo->type]++;
        DEBUG_LOGF("drone %d fired %d weapon", drone->idx, drone->weaponInfo->type);
    }
    drone->stepInfo.firedShot = true;

    if (drone->ammo == 0) {
        droneChangeWeapon(e, drone, e->defaultWeapon->type);
        drone->weaponCooldown = drone->weaponInfo->coolDown;
    }
}

void droneBrake(env *e, droneEntity *drone, const bool lightBrake, const bool heavyBrake) {
    // if the drone isn't braking or energy is fully depleted, return
    // unless the drone was braking during the last step
    if ((!lightBrake && !heavyBrake) || drone->energyFullyDepleted) {
        if (drone->lightBraking || drone->heavyBraking) {
            drone->lightBraking = false;
            drone->heavyBraking = false;
            b2Body_SetLinearDamping(drone->bodyID, DRONE_LINEAR_DAMPING);
            if (drone->energyRefillWait == 0.0f && !drone->chargingBurst) {
                drone->energyRefillWait = DRONE_ENERGY_REFILL_WAIT;
            }
        }
        return;
    }
    ASSERT(!drone->energyFullyDepleted);

    // apply additional brake damping and decrease energy
    if (lightBrake) {
        if (!drone->lightBraking) {
            drone->lightBraking = true;
            b2Body_SetLinearDamping(drone->bodyID, DRONE_LINEAR_DAMPING * DRONE_LIGHT_BRAKE_COEF);
        }
        drone->energyLeft = fmaxf(drone->energyLeft - (DRONE_LIGHT_BRAKE_DRAIN_RATE * e->deltaTime), 0.0f);
        e->stats[drone->idx].lightBrakeTime += e->deltaTime;
    } else if (heavyBrake) {
        if (!drone->heavyBraking) {
            drone->heavyBraking = true;
            b2Body_SetLinearDamping(drone->bodyID, DRONE_LINEAR_DAMPING * DRONE_HEAVY_BRAKE_COEF);
        }
        drone->energyLeft = fmaxf(drone->energyLeft - (DRONE_HEAVY_BRAKE_DRAIN_RATE * e->deltaTime), 0.0f);
        e->stats[drone->idx].heavyBrakeTime += e->deltaTime;
    }

    // if energy is empty but burst is being charged, let burst functions
    // handle energy refill
    if (drone->energyLeft == 0.0f && !drone->chargingBurst) {
        drone->energyFullyDepleted = true;
        drone->energyFullyDepletedThisStep = true;
        drone->energyRefillWait = DRONE_ENERGY_REFILL_EMPTY_WAIT;
        e->stats[drone->idx].energyEmptied++;
    }

    if (e->client != NULL) {
        brakeTrailPoint *trailPoint = fastCalloc(1, sizeof(brakeTrailPoint));
        trailPoint->pos = drone->pos;
        trailPoint->heavyBrake = heavyBrake;
        trailPoint->lifetime = UINT16_MAX;
        cc_array_add(e->brakeTrailPoints, trailPoint);
    }
}

void droneChargeBurst(env *e, droneEntity *drone) {
    if (drone->energyFullyDepleted || drone->burstCooldown != 0.0f || (!drone->chargingBurst && drone->energyLeft < DRONE_BURST_BASE_COST)) {
        return;
    }

    // take energy and put it into burst charge
    if (drone->chargingBurst) {
        drone->burstCharge = fminf(drone->burstCharge + (DRONE_BURST_CHARGE_RATE * e->deltaTime), DRONE_ENERGY_MAX);
        drone->energyLeft = fmaxf(drone->energyLeft - (DRONE_BURST_CHARGE_RATE * e->deltaTime), 0.0f);
    } else {
        drone->burstCharge = fminf(drone->burstCharge + DRONE_BURST_BASE_COST, DRONE_ENERGY_MAX);
        drone->energyLeft = fmaxf(drone->energyLeft - DRONE_BURST_BASE_COST, 0.0f);
        drone->chargingBurst = true;
    }

    if (drone->energyLeft == 0.0f) {
        drone->energyFullyDepleted = true;
        e->stats[drone->idx].energyEmptied++;
    }
}

void droneBurst(env *e, droneEntity *drone) {
    if (!drone->chargingBurst) {
        return;
    }

    const float radius = (DRONE_BURST_RADIUS_BASE * drone->burstCharge) + DRONE_BURST_RADIUS_MIN;
    b2ExplosionDef explosion = {
        .position = drone->pos,
        .radius = radius,
        .impulsePerLength = (DRONE_BURST_IMPACT_BASE * drone->burstCharge) + DRONE_BURST_IMPACT_MIN,
        .falloff = radius / 2.0f,
        .maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE,
    };
    createExplosion(e, drone, NULL, &explosion);

    drone->chargingBurst = false;
    drone->burstCharge = 0.0f;
    drone->burstCooldown = DRONE_BURST_COOLDOWN;
    if (drone->energyLeft == 0.0f) {
        drone->energyFullyDepletedThisStep = true;
        drone->energyRefillWait = DRONE_ENERGY_REFILL_EMPTY_WAIT;
    } else {
        drone->energyRefillWait = DRONE_ENERGY_REFILL_WAIT;
    }
    e->stats[drone->idx].totalBursts++;

    if (e->client != NULL) {
        explosionInfo *explInfo = fastCalloc(1, sizeof(explosionInfo));
        explInfo->def = explosion;
        explInfo->renderSteps = UINT16_MAX;
        cc_array_add(e->explosions, explInfo);
    }
}

void droneAddEnergy(droneEntity *drone, float energy) {
    // if a burst is charging, add the energy to the burst charge
    if (drone->chargingBurst) {
        drone->burstCharge = clamp(drone->burstCharge + energy);
    } else {
        drone->energyLeft = clamp(drone->energyLeft + energy);
    }
}

void droneDiscardWeapon(env *e, droneEntity *drone) {
    if (drone->weaponInfo->type == e->defaultWeapon->type || (drone->energyFullyDepleted && !drone->chargingBurst)) {
        return;
    }

    droneChangeWeapon(e, drone, e->defaultWeapon->type);
    droneAddEnergy(drone, -WEAPON_DISCARD_COST);
    if (drone->chargingBurst) {
        return;
    }

    if (drone->energyLeft == 0.0f) {
        drone->energyFullyDepleted = true;
        drone->energyFullyDepletedThisStep = true;
        drone->energyRefillWait = DRONE_ENERGY_REFILL_EMPTY_WAIT;
        e->stats[drone->idx].energyEmptied++;
    } else {
        drone->energyRefillWait = DRONE_ENERGY_REFILL_WAIT;
    }
}

typedef struct castCircleCtx {
    bool hit;
    b2ShapeId shapeID;
} castCircleCtx;

float castCircleCallback(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void *context) {
    // these parameters are required by the callback signature
    MAYBE_UNUSED(point);
    MAYBE_UNUSED(normal);
    if (!b2Shape_IsValid(shapeId)) {
        // skip this shape if it isn't valid
        return -1.0f;
    }

    castCircleCtx *ctx = (castCircleCtx *)context;
    ctx->hit = true;
    ctx->shapeID = shapeId;

    return fraction;
}

void droneStep(env *e, droneEntity *drone) {
    // manage weapon charge and heat
    if (drone->weaponCooldown != 0.0f) {
        drone->weaponCooldown = fmaxf(drone->weaponCooldown - e->deltaTime, 0.0f);
    }
    if (!drone->shotThisStep) {
        drone->weaponCharge = fmaxf(drone->weaponCharge - e->deltaTime, 0);
        drone->heat = fmaxf(drone->heat - 1, 0);
    } else {
        drone->shotThisStep = false;
    }
    ASSERT(!drone->shotThisStep);

    // manage drone energy
    if (drone->burstCooldown != 0.0f) {
        drone->burstCooldown = fmaxf(drone->burstCooldown - e->deltaTime, 0.0f);
    }
    if (drone->energyFullyDepletedThisStep) {
        drone->energyFullyDepletedThisStep = false;
    } else if (drone->energyRefillWait != 0.0f) {
        drone->energyRefillWait = fmaxf(drone->energyRefillWait - e->deltaTime, 0.0f);
    } else if (drone->energyLeft != DRONE_ENERGY_MAX && !drone->chargingBurst) {
        // don't start recharging energy until the burst charge is used
        drone->energyLeft = fminf(drone->energyLeft + (DRONE_ENERGY_REFILL_RATE * e->deltaTime), DRONE_ENERGY_MAX);
    }
    if (drone->energyLeft == DRONE_ENERGY_MAX) {
        drone->energyFullyDepleted = false;
    }

    const float distance = b2Distance(drone->lastPos, drone->pos);
    e->stats[drone->idx].distanceTraveled += distance;

    // update line of sight info for this drone
    for (uint8_t i = 0; i < e->numDrones; i++) {
        if (i == drone->idx || drone->inLineOfSight[i]) {
            continue;
        }

        // cast a circle that's the size of a projectile of the current weapon
        droneEntity *enemyDrone = safe_array_get_at(e->drones, i);
        const float enemyDistance = b2Distance(enemyDrone->pos, drone->pos);
        const b2Vec2 enemyDirection = b2Normalize(b2Sub(enemyDrone->pos, drone->pos));
        const b2Vec2 castEnd = b2MulAdd(drone->pos, enemyDistance, enemyDirection);
        const b2Vec2 translation = b2Sub(castEnd, drone->pos);
        const b2Circle projCircle = {.center = b2Vec2_zero, .radius = drone->weaponInfo->radius};
        const b2Transform projTransform = {.p = drone->pos, .q = b2Rot_identity};
        const b2QueryFilter filter = {.categoryBits = PROJECTILE_SHAPE, .maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | DRONE_SHAPE};

        castCircleCtx ctx = {0};
        b2World_CastCircle(e->worldID, &projCircle, projTransform, translation, filter, castCircleCallback, &ctx);
        if (!ctx.hit) {
            continue;
        }
        ASSERT(b2Shape_IsValid(ctx.shapeID));
        const entity *ent = b2Shape_GetUserData(ctx.shapeID);
        if (ent != NULL && ent->type == DRONE_ENTITY) {
            // get the drone entity in case another drone is between the
            // current drone and the enemy drone
            droneEntity *closestDrone = ent->entity;
            closestDrone->inLineOfSight[drone->idx] = true;
            drone->inLineOfSight[closestDrone->idx] = true;
        }
    }
}

void projectilesStep(env *e) {
    CC_SListIter iter;
    cc_slist_iter_init(&iter, e->projectiles);
    projectileEntity *projectile;
    while (cc_slist_iter_next(&iter, (void **)&projectile) != CC_ITER_END) {
        if (projectile->needsToBeDestroyed) {
            continue;
        }
        const float maxDistance = projectile->weaponInfo->maxDistance;
        const b2Vec2 distance = b2Sub(projectile->pos, projectile->lastPos);
        projectile->distance += b2Length(distance);

        if (maxDistance == INFINITE) {
            continue;
        }

        if (projectile->distance >= maxDistance) {
            // we have to destroy the projectile using the iterator so
            // we can continue to iterate correctly
            cc_slist_iter_remove(&iter, NULL);
            destroyProjectile(e, projectile, true, false);
            continue;
        }
    }

    // destroy all other projectiles that were caught in explosions of
    // projectiles destroyed above and also exploded now that the
    // initial projectiles have been destroyed
    for (uint8_t i = 0; i < cc_array_size(e->explodingProjectiles); i++) {
        projectileEntity *proj = safe_array_get_at(e->explodingProjectiles, i);
        destroyProjectile(e, proj, false, true);
    }
    cc_array_remove_all(e->explodingProjectiles);
}

void weaponPickupsStep(env *e) {
    CC_ArrayIter iter;
    cc_array_iter_init(&iter, e->pickups);
    weaponPickupEntity *pickup;

    // respawn weapon pickups at a random location as a random weapon type
    // once the respawn wait has elapsed
    while (cc_array_iter_next(&iter, (void **)&pickup) != CC_ITER_END) {
        if (pickup->respawnWait == 0.0f) {
            continue;
        }
        pickup->respawnWait = fmaxf(pickup->respawnWait - e->deltaTime, 0.0f);
        if (pickup->respawnWait != 0.0f) {
            continue;
        }

        b2Vec2 pos;
        if (!findOpenPos(e, WEAPON_PICKUP_SHAPE, &pos)) {
            const enum cc_stat res = cc_array_iter_remove(&iter, NULL);
            MAYBE_UNUSED(res);
            ASSERT(res == CC_OK);
            DEBUG_LOG("destroying weapon pickup");
            destroyWeaponPickup(e, pickup);
            continue;
        }
        b2Body_SetTransform(pickup->bodyID, pos, b2Rot_identity);
        pickup->pos = pos;
        pickup->weapon = randWeaponPickupType(e);

        DEBUG_LOGF("respawned weapon pickup at %f, %f", pos.x, pos.y);
        const int16_t cellIdx = entityPosToCellIdx(e, pos);
        if (cellIdx == -1) {
            ERRORF("invalid position for weapon pickup spawn: (%f, %f)", pos.x, pos.y);
        }
        pickup->mapCellIdx = cellIdx;
        mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        entity *ent = (entity *)b2Shape_GetUserData(pickup->shapeID);
        cell->ent = ent;
    }
}

// only update positions and velocities of dynamic bodies if they moved
// this step
void handleBodyMoveEvents(const env *e) {
    b2BodyEvents events = b2World_GetBodyEvents(e->worldID);
    for (int i = 0; i < events.moveCount; i++) {
        const b2BodyMoveEvent *event = events.moveEvents + i;
        ASSERT(b2Body_IsValid(event->bodyId));
        entity *ent = (entity *)event->userData;
        if (ent == NULL) {
            continue;
        }

        wallEntity *wall;
        projectileEntity *proj;
        droneEntity *drone;

        switch (ent->type) {
        case STANDARD_WALL_ENTITY:
        case BOUNCY_WALL_ENTITY:
        case DEATH_WALL_ENTITY:
            wall = (wallEntity *)ent->entity;
            wall->pos = event->transform.p;
            wall->rot = event->transform.q;
            wall->velocity = b2Body_GetLinearVelocity(wall->bodyID);
            break;
        case PROJECTILE_ENTITY:
            proj = (projectileEntity *)ent->entity;
            proj->lastPos = proj->pos;
            proj->pos = event->transform.p;
            proj->velocity = b2Body_GetLinearVelocity(proj->bodyID);
            break;
        case DRONE_ENTITY:
            drone = (droneEntity *)ent->entity;
            drone->lastPos = drone->pos;
            drone->pos = event->transform.p;
            drone->lastVelocity = drone->velocity;
            drone->velocity = b2Body_GetLinearVelocity(drone->bodyID);
            break;
        default:
            ERRORF("unknown entity type for move event %d", ent->type);
        }
    }
}

// destroy the projectile if it has traveled enough or has bounced enough
// times, and update drone stats if a drone was hit
uint8_t handleProjectileBeginContact(env *e, const entity *proj, const entity *ent, const b2Manifold *manifold, const bool projIsShapeA) {
    projectileEntity *projectile = (projectileEntity *)proj->entity;
    // e (shape B in the collision) will be NULL if it's another
    // projectile that was just destroyed
    if (ent == NULL || (ent != NULL && ent->type == PROJECTILE_ENTITY)) {
        // explode mines when hit by a projectile
        if (projectile->weaponInfo->type == MINE_LAUNCHER_WEAPON) {
            uint8_t numDestroyed = 1;
            if (ent != NULL) {
                const projectileEntity *projectile2 = ent->entity;
                // if both entities are mines both will be destroyed
                if (projectile2->weaponInfo->type == MINE_LAUNCHER_WEAPON) {
                    numDestroyed = 2;
                }
            }
            destroyProjectile(e, projectile, true, true);
            return numDestroyed;
        }

        // always allow all other projectiles to bounce off each other
        return false;
    } else if (ent->type == BOUNCY_WALL_ENTITY) {
        // always allow projectiles to bounce off bouncy walls
        return false;
    } else if (ent->type != BOUNCY_WALL_ENTITY) {
        projectile->bounces++;
        if (ent->type == DRONE_ENTITY) {
            droneEntity *hitDrone = (droneEntity *)ent->entity;
            if (projectile->droneIdx != hitDrone->idx) {
                droneEntity *shooterDrone = safe_array_get_at(e->drones, projectile->droneIdx);

                droneAddEnergy(shooterDrone, projectile->weaponInfo->energyRefill);
                // add 1 so we can differentiate between no weapon and weapon 0
                shooterDrone->stepInfo.shotHit[hitDrone->idx] = projectile->weaponInfo->type + 1;
                e->stats[shooterDrone->idx].shotsHit[projectile->weaponInfo->type]++;
                DEBUG_LOGF("drone %d hit drone %d with weapon %d", shooterDrone->idx, hitDrone->idx, projectile->weaponInfo->type);
                hitDrone->stepInfo.shotTaken[shooterDrone->idx] = projectile->weaponInfo->type + 1;
                e->stats[hitDrone->idx].shotsTaken[projectile->weaponInfo->type]++;
                DEBUG_LOGF("drone %d hit by drone %d with weapon %d", hitDrone->idx, shooterDrone->idx, projectile->weaponInfo->type);
            } else {
                hitDrone->stepInfo.ownShotTaken = true;
                e->stats[hitDrone->idx].ownShotsTaken[projectile->weaponInfo->type]++;
                DEBUG_LOGF("drone %d hit by own weapon %d", hitDrone->idx, projectile->weaponInfo->type);
            }

            if (projectile->weaponInfo->destroyedOnDroneHit) {
                destroyProjectile(e, projectile, projectile->weaponInfo->explodesOnDroneHit, true);
                return 1;
            }
        } else if (projectile->weaponInfo->type == MINE_LAUNCHER_WEAPON && projectile->lastSpeed != 0.0f) {
            // if the mine is in explosion proximity of a drone now,
            // destroy it
            if (isOverlappingCircle(e, projectile->pos, MINE_LAUNCHER_PROXIMITY_RADIUS, PROJECTILE_SHAPE, DRONE_SHAPE, NULL)) {
                destroyProjectile(e, projectile, true, true);
                return 1;
            }

            // create a weld joint to stick the mine to the wall
            ASSERT(entityTypeIsWall(ent->type));
            wallEntity *wall = ent->entity;
            ASSERT(manifold->pointCount == 1);

            // disable fixed rotation so floating walls mines are attached to can rotate
            b2Body_SetFixedRotation(projectile->bodyID, false);

            b2WeldJointDef jointDef = b2DefaultWeldJointDef();
            jointDef.angularHertz = 100.0f;
            jointDef.bodyIdA = wall->bodyID;
            jointDef.bodyIdB = projectile->bodyID;
            if (projIsShapeA) {
                jointDef.localAnchorA = manifold->points[0].anchorB;
                jointDef.localAnchorB = manifold->points[0].anchorA;
            } else {
                jointDef.localAnchorA = manifold->points[0].anchorA;
                jointDef.localAnchorB = manifold->points[0].anchorB;
            }
            b2CreateWeldJoint(e->worldID, &jointDef);
            projectile->lastSpeed = 0.0f;
        }
    }
    const uint8_t maxBounces = projectile->weaponInfo->maxBounces;
    if (projectile->bounces == maxBounces) {
        destroyProjectile(e, projectile, true, true);
        return 1;
    }

    return 0;
}

void handleProjectileEndContact(const entity *proj, const entity *ent) {
    // ensure the projectile's speed doesn't change after bouncing off
    // something
    projectileEntity *projectile = (projectileEntity *)proj->entity;
    if (ent != NULL && ent->type == PROJECTILE_ENTITY) {
        const projectileEntity *projectile2 = (projectileEntity *)ent->entity;
        // don't change projectile speeds if two projectiles of the same
        // weapon type collide
        if (projectile->weaponInfo->type != projectile2->weaponInfo->type) {
            projectile->lastSpeed = b2Length(b2Body_GetLinearVelocity(projectile->bodyID));
            return;
        }
    }

    float speed = projectile->lastSpeed;
    if (projectile->weaponInfo->type == ACCELERATOR_WEAPON) {
        speed = projectile->lastSpeed * ACCELERATOR_BOUNCE_SPEED_COEF;
        projectile->lastSpeed = speed;
    }

    const b2Vec2 velocity = b2Body_GetLinearVelocity(projectile->bodyID);
    const b2Vec2 newVel = b2MulSV(projectile->lastSpeed, b2Normalize(velocity));
    b2Body_SetLinearVelocity(projectile->bodyID, newVel);
}

void handleContactEvents(env *e) {
    b2ContactEvents events = b2World_GetContactEvents(e->worldID);
    for (int i = 0; i < events.beginCount; ++i) {
        const b2ContactBeginTouchEvent *event = events.beginEvents + i;
        entity *e1 = NULL;
        entity *e2 = NULL;

        if (b2Shape_IsValid(event->shapeIdA)) {
            e1 = (entity *)b2Shape_GetUserData(event->shapeIdA);
            ASSERT(e1 != NULL);
        }
        if (b2Shape_IsValid(event->shapeIdB)) {
            e2 = (entity *)b2Shape_GetUserData(event->shapeIdB);
            ASSERT(e2 != NULL);
        }

        if (e1 != NULL) {
            if (e1->type == PROJECTILE_ENTITY) {
                uint8_t numDestroyed = handleProjectileBeginContact(e, e1, e2, &event->manifold, true);
                if (numDestroyed == 2) {
                    continue;
                } else if (numDestroyed == 1) {
                    e1 = NULL;
                }

            } else if (e1->type == DEATH_WALL_ENTITY && e2 != NULL && e2->type == DRONE_ENTITY) {
                droneEntity *drone = (droneEntity *)e2->entity;
                killDrone(e, drone);
            }
        }
        if (e2 != NULL) {
            if (e2->type == PROJECTILE_ENTITY) {
                handleProjectileBeginContact(e, e2, e1, &event->manifold, false);
            } else if (e2->type == DEATH_WALL_ENTITY && e1 != NULL && e1->type == DRONE_ENTITY) {
                droneEntity *drone = (droneEntity *)e1->entity;
                killDrone(e, drone);
            }
        }
    }

    for (int i = 0; i < events.endCount; ++i) {
        const b2ContactEndTouchEvent *event = events.endEvents + i;
        entity *e1 = NULL;
        entity *e2 = NULL;
        if (b2Shape_IsValid(event->shapeIdA)) {
            e1 = (entity *)b2Shape_GetUserData(event->shapeIdA);
            ASSERT(e1 != NULL);
        }
        if (b2Shape_IsValid(event->shapeIdB)) {
            e2 = (entity *)b2Shape_GetUserData(event->shapeIdB);
            ASSERT(e2 != NULL);
        }
        if (e1 != NULL && e1->type == PROJECTILE_ENTITY) {
            handleProjectileEndContact(e1, e2);
        }
        if (e2 != NULL && e2->type == PROJECTILE_ENTITY) {
            handleProjectileEndContact(e2, e1);
        }
    }
}

// set pickup to respawn somewhere else randomly if a drone touched it,
// mark the pickup as disabled if a floating wall is touching it
void handleWeaponPickupBeginTouch(env *e, const entity *sensor, entity *visitor) {
    weaponPickupEntity *pickup = (weaponPickupEntity *)sensor->entity;
    if (pickup->respawnWait != 0.0f || pickup->floatingWallsTouching != 0) {
        return;
    }

    switch (visitor->type) {
    case DRONE_ENTITY:
        pickup->respawnWait = PICKUP_RESPAWN_WAIT;
        mapCell *cell = safe_array_get_at(e->cells, pickup->mapCellIdx);
        ASSERT(cell->ent != NULL);
        cell->ent = NULL;

        droneEntity *drone = (droneEntity *)visitor->entity;
        drone->stepInfo.pickedUpWeapon = true;
        drone->stepInfo.prevWeapon = drone->weaponInfo->type;
        droneChangeWeapon(e, drone, pickup->weapon);

        e->stats[drone->idx].weaponsPickedUp[pickup->weapon]++;
        DEBUG_LOGF("drone %d picked up weapon %d", drone->idx, pickup->weapon);
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        pickup->floatingWallsTouching++;
        break;
    default:
        ERRORF("invalid weapon pickup begin touch visitor %d", visitor->type);
    }
}

void handleProjectileBeginTouch(env *e, const entity *sensor) {
    projectileEntity *projectile = sensor->entity;

    switch (projectile->weaponInfo->type) {
    case FLAK_CANNON_WEAPON:
        if (projectile->distance < FLAK_CANNON_SAFE_DISTANCE) {
            return;
        }
        destroyProjectile(e, projectile, true, true);
        break;
    case MINE_LAUNCHER_WEAPON:
        if (projectile->lastSpeed != 0.0f) {
            return;
        }
        destroyProjectile(e, projectile, true, true);
        break;
    default:
        ERRORF("invalid projectile type %d for begin touch event", sensor->type);
    }
}

// mark the pickup as enabled if no floating walls are touching it
void handleWeaponPickupEndTouch(const entity *sensor, entity *visitor) {
    weaponPickupEntity *pickup = (weaponPickupEntity *)sensor->entity;
    if (pickup->respawnWait != 0.0f) {
        return;
    }

    switch (visitor->type) {
    case DRONE_ENTITY:
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        pickup->floatingWallsTouching--;
        break;
    default:
        ERRORF("invalid weapon pickup end touch visitor %d", visitor->type);
    }
}

void handleSensorEvents(env *e) {
    b2SensorEvents events = b2World_GetSensorEvents(e->worldID);
    for (int i = 0; i < events.beginCount; ++i) {
        const b2SensorBeginTouchEvent *event = events.beginEvents + i;
        if (!b2Shape_IsValid(event->sensorShapeId)) {
            DEBUG_LOG("could not find sensor shape for begin touch event");
            continue;
        }
        entity *s = (entity *)b2Shape_GetUserData(event->sensorShapeId);
        ASSERT(s != NULL);

        if (!b2Shape_IsValid(event->visitorShapeId)) {
            DEBUG_LOG("could not find visitor shape for begin touch event");
            continue;
        }
        entity *v = (entity *)b2Shape_GetUserData(event->visitorShapeId);
        ASSERT(v != NULL);

        switch (s->type) {
        case WEAPON_PICKUP_ENTITY:
            handleWeaponPickupBeginTouch(e, s, v);
            break;
        case PROJECTILE_ENTITY:
            handleProjectileBeginTouch(e, s);
            break;
        default:
            ERRORF("unknown entity type %d for sensor begin touch event", s->type);
        }
    }

    for (int i = 0; i < events.endCount; ++i) {
        const b2SensorEndTouchEvent *event = events.endEvents + i;
        if (!b2Shape_IsValid(event->sensorShapeId)) {
            DEBUG_LOG("could not find sensor shape for end touch event");
            continue;
        }
        entity *s = (entity *)b2Shape_GetUserData(event->sensorShapeId);
        ASSERT(s != NULL);
        if (s->type != WEAPON_PICKUP_ENTITY) {
            continue;
        }

        if (!b2Shape_IsValid(event->visitorShapeId)) {
            DEBUG_LOG("could not find visitor shape for end touch event");
            continue;
        }
        entity *v = (entity *)b2Shape_GetUserData(event->visitorShapeId);
        ASSERT(v != NULL);

        handleWeaponPickupEndTouch(s, v);
    }
}

const uint8_t searchDirections[8][2] = {
    {0, -1},
    {1, -1},
    {1, 0},
    {1, 1},
    {0, 1},
    {-1, 1},
    {-1, 0},
    {-1, -1},
};

// insertion sort should be faster than quicksort for small arrays
void insertionSort(nearEntity arr[], uint8_t size) {
    for (int i = 1; i < size; i++) {
        nearEntity key = arr[i];
        int j = i - 1;

        while (j >= 0 && arr[j].distance > key.distance) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }

        arr[j + 1] = key;
    }
}

#ifndef AUTOPXD
void findNearWallsAndPickups(const env *e, droneEntity *drone, nearEntity nearWalls[], const uint8_t nWalls, nearEntity nearPickups[], const uint8_t nPickups) {
    const int16_t droneCellIdx = entityPosToCellIdx(e, drone->pos);
    int8_t droneCellCol = droneCellIdx % e->columns;
    int8_t droneCellRow = droneCellIdx / e->columns;

    uint8_t foundWalls = 0;
    uint8_t foundPickups = 0;
    int8_t xDirection = 1;
    int8_t yDirection = 0;
    int8_t col = droneCellCol;
    int8_t row = droneCellRow;
    uint8_t segmentLen = 1;
    uint8_t segmentPassed = 0;
    uint8_t segmentsCompleted = 0;

    // search for near walls and pickups in a spiral; pickups may be
    // scattered, so if we don't find them all here we'll just sort all
    // pickups by distance later
    while (segmentsCompleted % 5 != 0 || foundWalls < nWalls) {
        col += xDirection;
        row += yDirection;
        const int16_t cellIdx = cellIndex(e, col, row);
        segmentPassed++;

        if (segmentPassed == segmentLen) {
            // if we have completed a segment, change direction
            segmentsCompleted++;
            segmentPassed = 0;
            uint8_t temp = xDirection;
            xDirection = -yDirection;
            yDirection = temp;
            if (yDirection == 0) {
                // increase the segment length if necessary
                segmentLen++;
            }
        }

        if (cellIdx < 0 || col < 0 || col >= e->columns || row < 0 || row >= e->rows) {
            continue;
        }

        const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        if (cell->ent == NULL || (cell->ent->type != WEAPON_PICKUP_ENTITY && !entityTypeIsWall(cell->ent->type))) {
            continue;
        }

        nearEntity nearEntity = {
            .entity = cell->ent->entity,
            .distance = b2Distance(cell->pos, drone->pos),
        };
        if (cell->ent->type != WEAPON_PICKUP_ENTITY) {
            nearWalls[foundWalls++] = nearEntity;
        } else if (cell->ent->type == WEAPON_PICKUP_ENTITY) {
            nearPickups[foundPickups++] = nearEntity;
        }
    }

    ASSERTF(foundWalls >= nWalls, "foundWalls: %d", foundWalls);
    insertionSort(nearWalls, foundWalls);

    // if we didn't find enough pickups, add the rest of the pickups
    // and sort them by distance
    if (foundPickups < nPickups) {
        for (uint8_t i = 0; i < cc_array_size(e->pickups); i++) {
            weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
            if (i < foundPickups) {
                const weaponPickupEntity *nearPickup = (weaponPickupEntity *)nearPickups[i].entity;
                if (nearPickup == pickup) {
                    continue;
                }
            }

            nearEntity nearEntity = {
                .entity = pickup,
                .distance = b2Distance(pickup->pos, drone->pos),
            };
            nearPickups[i] = nearEntity;
        }
        foundPickups = cc_array_size(e->pickups);
    }
    insertionSort(nearPickups, foundPickups);
}
#endif

#endif
