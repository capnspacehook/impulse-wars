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
    const uint8_t srcRow = srcCellIdx % e->columns;
    const uint8_t srcCol = srcCellIdx / e->columns;
    const uint8_t destRow = destCellIdx % e->columns;
    const uint8_t destCol = destCellIdx / e->columns;
    return (destCol * e->rows * e->columns * e->rows) + (destRow * e->rows * e->columns) + (srcCol * e->columns) + srcRow;
}

void pathfindBFS(const env *e, uint8_t *flatPaths, uint16_t destCell) {
    uint8_t(*paths)[e->columns] = (uint8_t(*)[e->columns])flatPaths;
    int8_t(*buffer)[3] = (int8_t(*)[3])e->mapPathing[e->mapIdx].pathBuffer;

    uint16_t start = 0;
    uint16_t end = 1;

    const mapCell *cell = (mapCell *)safe_array_get_at(e->cells, destCell);
    if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
        return;
    }
    const int8_t destRow = destCell % e->columns;
    const int8_t destCol = destCell / e->columns;

    buffer[start][0] = 8;
    buffer[start][1] = destRow;
    buffer[start][2] = destCol;
    while (start < end) {
        const int8_t direction = buffer[start][0];
        const int8_t startRow = buffer[start][1];
        const int8_t startCol = buffer[start][2];
        start++;

        if (startRow < 0 || startRow >= e->rows || startCol < 0 || startCol >= e->columns || paths[startCol][startRow] != UINT8_MAX) {
            continue;
        }
        int16_t cellIdx = cellIndex(e, startRow, startCol);
        const mapCell *cell = (mapCell *)safe_array_get_at(e->cells, cellIdx);
        if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
            paths[startCol][startRow] = 8;
            continue;
        }

        paths[startCol][startRow] = direction;

        buffer[end][0] = 6; // up
        buffer[end][1] = startRow;
        buffer[end][2] = startCol + 1;
        end++;

        buffer[end][0] = 2; // down
        buffer[end][1] = startRow;
        buffer[end][2] = startCol - 1;
        end++;

        buffer[end][1] = startRow - 1;
        buffer[end][2] = startCol;
        end++;

        buffer[end][0] = 4; // left
        buffer[end][1] = startRow + 1;
        buffer[end][2] = startCol;
        end++;

        buffer[end][0] = 5; // up left
        buffer[end][1] = startRow + 1;
        buffer[end][2] = startCol + 1;
        end++;

        buffer[end][0] = 3; // down left
        buffer[end][1] = startRow + 1;
        buffer[end][2] = startCol - 1;
        end++;

        buffer[end][0] = 1; // down right
        buffer[end][1] = startRow - 1;
        buffer[end][2] = startCol - 1;
        end++;

        buffer[end][0] = 7; // up right
        buffer[end][1] = startRow - 1;
        buffer[end][2] = startCol + 1;
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

// TODO: switch between personalities when episode begins:
// - aggressive
// - defensive
// - lazy
agentActions scriptedBotActions(env *e, droneEntity *drone) {
    agentActions actions = {0};
    if (e->sittingDuck) {
        return actions;
    }

    // find the nearest death wall or floating wall
    nearEntity nearWalls[MAX_NEAR_WALLS] = {0};
    nearEntity nearPickups[MAX_WEAPON_PICKUPS] = {0};
    findNearWallsAndPickups(e, drone, nearWalls, NUM_NEAR_WALLS, nearPickups, NUM_NEAR_PICKUPS);

    // find the distance between the closest points on the drone and the nearest wall
    const b2Vec2 pos = getCachedPos(drone->bodyID, &drone->pos);
    b2Vec2 nearestWallPos = b2Vec2_zero;
    float nearestWallDistance = FLT_MAX;
    for (uint8_t i = 0; i < NUM_NEAR_WALLS; i++) {
        const wallEntity *wall = (wallEntity *)nearWalls[i].entity;
        if (wall->type == DEATH_WALL_ENTITY) {
            bool isCircle = false;
            b2DistanceInput input;
            input.proxyA = makeDistanceProxyFromType(DRONE_ENTITY, &isCircle);
            input.proxyB = makeDistanceProxyFromType(wall->type, &isCircle);
            input.transformA = (b2Transform){.p = pos, .q = b2Rot_identity};
            input.transformB = (b2Transform){.p = wall->pos.pos, .q = b2Rot_identity};
            input.useRadii = isCircle;
            b2SimplexCache cache = {0};
            const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);

            nearestWallPos = wall->pos.pos;
            nearestWallDistance = output.distance;
            break;
        }
    }

    for (uint8_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
        wallEntity *floatingWall = safe_array_get_at(e->floatingWalls, i);
        if (floatingWall->type != DEATH_WALL_ENTITY) {
            continue;
        }
        const b2Vec2 floatingWallPos = getCachedPos(floatingWall->bodyID, &floatingWall->pos);
        const float floatingWallDistance = b2Distance(floatingWallPos, pos);
        if (floatingWallDistance < nearestWallDistance) {
            bool isCircle = false;
            b2DistanceInput input;
            input.proxyA = makeDistanceProxyFromType(DRONE_ENTITY, &isCircle);
            input.proxyB = makeDistanceProxyFromType(floatingWall->type, &isCircle);
            input.transformA = (b2Transform){.p = pos, .q = b2Rot_identity};
            input.transformB = b2Body_GetTransform(floatingWall->bodyID);
            input.useRadii = isCircle;
            b2SimplexCache cache = {0};
            const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);

            nearestWallPos = floatingWallPos;
            nearestWallDistance = output.distance;
        }
    }

    // move away from a death wall if we're too close
    const b2Vec2 vel = b2Body_GetLinearVelocity(drone->bodyID);
    const b2Vec2 wallDirection = b2Normalize(b2Sub(nearestWallPos, pos));
    const b2Vec2 invWallDirection = b2MulSV(-1.0f, wallDirection);
    if (nearestWallDistance <= WALL_AVOID_DISTANCE) {
        const float speedToWall = b2Dot(vel, wallDirection);
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
            const float recoilDistance = shotRecoilDistance(e, drone, vel, invWallDirection, shotWait);
            const bool safeShot = safeToFire(e, pos, invWallDirection, recoilDistance);
            // e->debugPoint = b2MulAdd(pos, recoilDistance, invWallDirection);
            if (safeShot) {
                actions.aim = wallDirection;
                actions.shoot = true;
                return actions;
            }
        }
    }

    if (drone->weaponInfo->type == STANDARD_WEAPON) {
        const weaponPickupEntity *pickup = (weaponPickupEntity *)nearPickups[0].entity;
        moveTo(e, &actions, pos, pickup->pos);
        return actions;
    }

    droneEntity *agentDrone = (droneEntity *)safe_array_get_at(e->drones, 0);
    const b2Vec2 agentDronePos = getCachedPos(agentDrone->bodyID, &agentDrone->pos);
    const b2Vec2 agentDroneDirection = b2Normalize(b2Sub(agentDronePos, pos));
    const b2Vec2 invAgentDroneDirection = b2MulSV(-1.0f, agentDroneDirection);

    float shotWait;
    if (drone->ammo > 1) {
        shotWait = ((drone->weaponInfo->coolDown + drone->weaponInfo->charge) / e->deltaTime) * 1.5f;
    } else {
        shotWait = ((e->defaultWeapon->coolDown + e->defaultWeapon->charge) / e->deltaTime) * 1.5f;
    }
    const float recoilDistance = shotRecoilDistance(e, drone, vel, invAgentDroneDirection, shotWait);
    const bool safeShot = safeToFire(e, pos, invAgentDroneDirection, recoilDistance);

    // e->debugPoint = b2MulAdd(pos, recoilDistance, invAgentDroneDirection);

    if (drone->inLineOfSight[0] && safeShot) {
        actions.move.x += agentDroneDirection.x;
        actions.move.y += agentDroneDirection.y;
        actions.move = b2Normalize(actions.move);
        actions.aim = agentDroneDirection;
        actions.shoot = true;
    } else {
        moveTo(e, &actions, pos, agentDronePos);
    }

    return actions;
}

#endif
