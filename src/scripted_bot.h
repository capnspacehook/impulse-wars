#ifndef IMPULSE_WARS_SCRIPTED_BOT_H
#define IMPULSE_WARS_SCRIPTED_BOT_H

#include "game.h"
#include "types.h"

const uint8_t NUM_NEAR_WALLS = 3;
const uint8_t NUM_NEAR_PICKUPS = 1;

const float WALL_AVOID_DISTANCE = 3.9f;
const float WALL_DANGER_DISTANCE = 3.0f;
const float WALL_DANGER_SPEED = 5.0f;

static inline uint32_t pathOffset(const env *e, uint16_t srcCellIdx, uint16_t destCellIdx) {
    const uint8_t srcCol = srcCellIdx % e->map->columns;
    const uint8_t srcRow = srcCellIdx / e->map->columns;
    const uint8_t destCol = destCellIdx % e->map->columns;
    const uint8_t destRow = destCellIdx / e->map->columns;
    return (destRow * e->map->rows * e->map->columns * e->map->rows) + (destCol * e->map->rows * e->map->columns) + (srcRow * e->map->columns) + srcCol;
}

void pathfindBFS(const env *e, uint8_t *flatPaths, uint16_t destCellIdx) {
    uint8_t(*paths)[e->map->columns] = (uint8_t(*)[e->map->columns])flatPaths;
    int8_t(*buffer)[3] = (int8_t(*)[3])e->mapPathing[e->mapIdx].pathBuffer;

    uint16_t start = 0;
    uint16_t end = 1;

    const mapCell *cell = safe_array_get_at(e->cells, destCellIdx);
    if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
        return;
    }
    const int8_t destCol = destCellIdx % e->map->columns;
    const int8_t destRow = destCellIdx / e->map->columns;

    buffer[start][0] = 8;
    buffer[start][1] = destCol;
    buffer[start][2] = destRow;
    while (start < end) {
        const int8_t direction = buffer[start][0];
        const int8_t startCol = buffer[start][1];
        const int8_t startRow = buffer[start][2];
        start++;

        if (startCol < 0 || startCol >= e->map->columns || startRow < 0 || startRow >= e->map->rows || paths[startRow][startCol] != UINT8_MAX) {
            continue;
        }
        int16_t cellIdx = cellIndex(e, startCol, startRow);
        const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
            paths[startRow][startCol] = 8;
            continue;
        }

        paths[startRow][startCol] = direction;

        buffer[end][0] = 6; // up
        buffer[end][1] = startCol;
        buffer[end][2] = startRow + 1;
        end++;

        buffer[end][0] = 2; // down
        buffer[end][1] = startCol;
        buffer[end][2] = startRow - 1;
        end++;

        buffer[end][0] = 0; // right
        buffer[end][1] = startCol - 1;
        buffer[end][2] = startRow;
        end++;

        buffer[end][0] = 4; // left
        buffer[end][1] = startCol + 1;
        buffer[end][2] = startRow;
        end++;

        buffer[end][0] = 5; // up left
        buffer[end][1] = startCol + 1;
        buffer[end][2] = startRow + 1;
        end++;

        buffer[end][0] = 3; // down left
        buffer[end][1] = startCol + 1;
        buffer[end][2] = startRow - 1;
        end++;

        buffer[end][0] = 1; // down right
        buffer[end][1] = startCol - 1;
        buffer[end][2] = startRow - 1;
        end++;

        buffer[end][0] = 7; // up right
        buffer[end][1] = startCol - 1;
        buffer[end][2] = startRow + 1;
        end++;
    }
}

float shotRecoilDistance(const env *e, const droneEntity *drone, const b2Vec2 vel, const b2Vec2 direction, const float steps) {
    float speed = drone->weaponInfo->recoilMagnitude * DRONE_INV_MASS;
    if (!b2VecEqual(vel, b2Vec2_zero)) {
        speed = b2Length(b2MulAdd(vel, speed, direction));
    }

    const float damping = 1.0f + DRONE_LINEAR_DAMPING * e->deltaTime;
    return speed * (damping / DRONE_LINEAR_DAMPING) * (1.0f - powf(1.0f / (damping), steps));
}

bool safeToFire(const env *e, const b2Vec2 pos, const b2Vec2 direction, const float recoilDistance) {
    const b2Vec2 rayEnd = b2MulAdd(pos, recoilDistance, direction);
    const b2Vec2 translation = b2Sub(rayEnd, pos);
    const b2Circle projCircle = {.center = b2Vec2_zero, .radius = DRONE_RADIUS};
    const b2Transform projTransform = {.p = pos, .q = b2Rot_identity};
    const b2QueryFilter filter = {.categoryBits = DRONE_SHAPE, .maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | DRONE_SHAPE};

    castCircleCtx ctx = {0};
    b2World_CastCircle(e->worldID, &projCircle, projTransform, translation, filter, castCircleCallback, &ctx);
    if (!ctx.hit) {
        return true;
    } else {
        const entity *ent = b2Shape_GetUserData(ctx.shapeID);
        if (ent->type == STANDARD_WALL_ENTITY || ent->type == BOUNCY_WALL_ENTITY || ent->type == DRONE_ENTITY) {
            return true;
        }
    }

    return false;
}

void moveTo(env *e, agentActions *actions, const b2Vec2 srcPos, const b2Vec2 dstPos) {
    int16_t srcIdx = entityPosToCellIdx(e, srcPos);
    if (srcIdx == -1) {
        return;
    }
    int16_t dstIdx = entityPosToCellIdx(e, dstPos);
    if (dstIdx == -1) {
        return;
    }

    uint32_t pathIdx = pathOffset(e, srcIdx, dstIdx);
    uint8_t *paths = e->mapPathing[e->mapIdx].paths;
    uint8_t direction = paths[pathIdx];
    if (direction == UINT8_MAX) {
        uint32_t bfsIdx = pathOffset(e, 0, dstIdx);
        pathfindBFS(e, &paths[bfsIdx], dstIdx);
        direction = paths[pathIdx];
    }
    if (direction >= 8) {
        return;
    }
    actions->move.x += discMoveToContMoveMap[0][direction];
    actions->move.y += discMoveToContMoveMap[1][direction];
    actions->move = b2Normalize(actions->move);
}

void scriptedBotShoot(droneEntity *drone, agentActions *actions) {
    actions->shoot = true;
    if (drone->chargingWeapon && drone->weaponCharge == drone->weaponInfo->charge) {
        actions->chargingWeapon = false;
    }
}

bool shouldShootAtAgent(const env *e, const droneEntity *drone, const droneEntity *agentDrone, const b2Vec2 agentDroneDirection) {
    const b2Vec2 invAgentDroneDirection = b2MulSV(-1.0f, agentDroneDirection);

    float shotWait;
    if (drone->ammo > 1) {
        shotWait = ((drone->weaponInfo->coolDown + drone->weaponInfo->charge) / e->deltaTime) * 1.5f;
    } else {
        shotWait = ((e->defaultWeapon->coolDown + e->defaultWeapon->charge) / e->deltaTime) * 1.5f;
    }
    const float recoilDistance = shotRecoilDistance(e, drone, drone->velocity, invAgentDroneDirection, shotWait);
    const bool safeShot = safeToFire(e, drone->pos, invAgentDroneDirection, recoilDistance);
    // e->debugPoint = b2MulAdd(pos, recoilDistance, invAgentDroneDirection);
    if (!safeShot) {
        return false;
    }

    // cast a circle that's the size of a projectile of the current weapon
    const float agentDroneDistance = b2Distance(agentDrone->pos, drone->pos);
    const b2Vec2 castEnd = b2MulAdd(drone->pos, agentDroneDistance, agentDroneDirection);
    const b2Vec2 translation = b2Sub(castEnd, drone->pos);
    const b2Circle projCircle = {.center = b2Vec2_zero, .radius = drone->weaponInfo->radius};
    const b2Transform projTransform = {.p = drone->pos, .q = b2Rot_identity};
    const b2QueryFilter filter = {.categoryBits = PROJECTILE_SHAPE, .maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | DRONE_SHAPE};

    castCircleCtx ctx = {0};
    b2World_CastCircle(e->worldID, &projCircle, projTransform, translation, filter, castCircleCallback, &ctx);
    if (!ctx.hit) {
        return false;
    }
    ASSERT(b2Shape_IsValid(ctx.shapeID));
    const entity *ent = b2Shape_GetUserData(ctx.shapeID);
    if (ent == NULL || ent->type != DRONE_ENTITY) {
        return false;
    }

    return true;
}

#ifndef AUTOPXD

// TODO: switch between personalities when episode begins:
// - aggressive
// - defensive
// - lazy
agentActions scriptedBotActions(env *e, droneEntity *drone) {
    agentActions actions = {0};
    if (e->sittingDuck) {
        return actions;
    }
    actions.chargingWeapon = true;

    // find the nearest death wall or floating wall
    nearEntity nearWalls[MAX_NEAREST_WALLS] = {0};
    findNearWalls(e, drone, nearWalls, NUM_NEAR_WALLS);

    // find the distance between the closest points on the drone and the nearest wall
    b2Vec2 nearestWallPos = b2Vec2_zero;
    float nearestWallDistance = FLT_MAX;
    for (uint8_t i = 0; i < NUM_NEAR_WALLS; i++) {
        const wallEntity *wall = nearWalls[i].entity;
        if (wall->type == DEATH_WALL_ENTITY) {
            bool isCircle = false;
            b2DistanceInput input;
            input.proxyA = makeDistanceProxyFromType(DRONE_ENTITY, &isCircle);
            input.proxyB = makeDistanceProxyFromType(wall->type, &isCircle);
            input.transformA = (b2Transform){.p = drone->pos, .q = b2Rot_identity};
            input.transformB = (b2Transform){.p = wall->pos, .q = b2Rot_identity};
            input.useRadii = isCircle;
            b2SimplexCache cache = {0};
            const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);

            nearestWallPos = wall->pos;
            nearestWallDistance = output.distance;
            break;
        }
    }

    for (uint8_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
        wallEntity *floatingWall = safe_array_get_at(e->floatingWalls, i);
        if (floatingWall->type != DEATH_WALL_ENTITY) {
            continue;
        }
        const float floatingWallDistance = b2Distance(floatingWall->pos, drone->pos);
        if (floatingWallDistance < nearestWallDistance) {
            bool isCircle = false;
            b2DistanceInput input;
            input.proxyA = makeDistanceProxyFromType(DRONE_ENTITY, &isCircle);
            input.proxyB = makeDistanceProxyFromType(floatingWall->type, &isCircle);
            input.transformA = (b2Transform){.p = drone->pos, .q = b2Rot_identity};
            input.transformB = b2Body_GetTransform(floatingWall->bodyID);
            input.useRadii = isCircle;
            b2SimplexCache cache = {0};
            const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);

            nearestWallPos = floatingWall->pos;
            nearestWallDistance = output.distance;
        }
    }

    // move away from a death wall if we're too close
    const b2Vec2 wallDirection = b2Normalize(b2Sub(nearestWallPos, drone->pos));
    const b2Vec2 invWallDirection = b2MulSV(-1.0f, wallDirection);
    if (nearestWallDistance <= WALL_AVOID_DISTANCE) {
        const float speedToWall = b2Dot(drone->velocity, wallDirection);
        DEBUG_LOGF("speedToWall: %f", speedToWall);
        if (nearestWallDistance <= WALL_DANGER_DISTANCE || speedToWall > 0.1f) {
            actions.move = invWallDirection;

            // shoot to move away faster from a death wall if we're too close and it's safe
            float shotWait;
            if (drone->ammo > 1) {
                shotWait = ((drone->weaponInfo->coolDown + drone->weaponInfo->charge) / e->deltaTime) * 1.5f;
            } else {
                shotWait = ((e->defaultWeapon->coolDown + e->defaultWeapon->charge) / e->deltaTime) * 1.5f;
            }
            const float recoilDistance = shotRecoilDistance(e, drone, drone->velocity, invWallDirection, shotWait);
            const bool safeShot = safeToFire(e, drone->pos, invWallDirection, recoilDistance);
            // e->debugPoint = b2MulAdd(pos, recoilDistance, invWallDirection);
            if (safeShot) {
                actions.aim = wallDirection;
                scriptedBotShoot(drone, &actions);
                return actions;
            }
        }
    }

    // get a weapon if the standard weapon is active
    if (drone->weaponInfo->type == STANDARD_WEAPON) {
        nearEntity nearPickups[MAX_WEAPON_PICKUPS] = {0};
        for (uint8_t i = 0; i < cc_array_size(e->pickups); i++) {
            weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
            const nearEntity nearEnt = {
                .entity = pickup,
                .distanceSquared = b2DistanceSquared(pickup->pos, drone->pos),
            };
            nearPickups[i] = nearEnt;
        }
        insertionSort(nearPickups, cc_array_size(e->pickups));

        const weaponPickupEntity *pickup = nearPickups[0].entity;
        moveTo(e, &actions, drone->pos, pickup->pos);
        return actions;
    }

    droneEntity *agentDrone = safe_array_get_at(e->drones, 0);
    const b2Vec2 agentDroneDirection = b2Normalize(b2Sub(agentDrone->pos, drone->pos));
    if (shouldShootAtAgent(e, drone, agentDrone, agentDroneDirection)) {
        actions.move.x += agentDroneDirection.x;
        actions.move.y += agentDroneDirection.y;
        actions.move = b2Normalize(actions.move);
        actions.aim = agentDroneDirection;
        scriptedBotShoot(drone, &actions);
    } else {
        moveTo(e, &actions, drone->pos, agentDrone->pos);
    }

    return actions;
}

#endif

#endif
