#ifndef IMPULSE_WARS_ENV_H
#define IMPULSE_WARS_ENV_H

#include "game.h"
#include "map.h"
#include "scripted_agent.h"
#include "settings.h"
#include "types.h"

// autopdx can't parse raylib's headers for some reason, but that's ok
// because the Cython code doesn't need to directly use raylib anyway
// include the full render header if we're compiling C code, otherwise
// just declare the necessary functions the Cython code needs
#ifndef AUTOPXD
#include "render.h"
#else
rayClient *createRayClient();
void destroyRayClient(rayClient *client);
typedef struct Vector2 {
    float x;
    float y;
} Vector2;
#endif

const uint8_t THREE_BIT_MASK = 0x7;
const uint8_t FOUR_BIT_MASK = 0xf;

logBuffer *createLogBuffer(uint16_t capacity) {
    logBuffer *logs = fastCalloc(1, sizeof(logBuffer));
    logs->logs = fastCalloc(capacity, sizeof(logEntry));
    logs->size = 0;
    logs->capacity = capacity;
    return logs;
}

void destroyLogBuffer(logBuffer *buffer) {
    fastFree(buffer->logs);
    fastFree(buffer);
}

void addLogEntry(logBuffer *logs, logEntry *log) {
    if (logs->size == logs->capacity) {
        return;
    }
    logs->logs[logs->size] = *log;
    logs->size += 1;
}

logEntry aggregateAndClearLogBuffer(uint8_t numDrones, logBuffer *logs) {
    logEntry log = {0};
    if (logs->size == 0) {
        return log;
    }

    DEBUG_LOGF("aggregating logs, size: %d", logs->size);

    const float logSize = logs->size;
    for (uint16_t i = 0; i < logs->size; i++) {
        log.length += logs->logs[i].length / logSize;
        log.ties += logs->logs[i].ties / logSize;

        for (uint8_t j = 0; j < numDrones; j++) {
            log.stats[j].reward += logs->logs[i].stats[j].reward / logSize;
            log.stats[j].wins += logs->logs[i].stats[j].wins / logSize;

            for (uint8_t k = 0; k < NUM_WEAPONS; k++) {
                log.stats[j].distanceTraveled += logs->logs[i].stats[j].distanceTraveled / logSize;
                log.stats[j].absDistanceTraveled += logs->logs[i].stats[j].absDistanceTraveled / logSize;
                log.stats[j].shotsFired[k] += logs->logs[i].stats[j].shotsFired[k] / logSize;
                log.stats[j].shotsHit[k] += logs->logs[i].stats[j].shotsHit[k] / logSize;
                log.stats[j].shotsTaken[k] += logs->logs[i].stats[j].shotsTaken[k] / logSize;
                log.stats[j].ownShotsTaken[k] += logs->logs[i].stats[j].ownShotsTaken[k] / logSize;
                log.stats[j].weaponsPickedUp[k] += logs->logs[i].stats[j].weaponsPickedUp[k] / logSize;
                log.stats[j].shotDistances[k] += logs->logs[i].stats[j].shotDistances[k] / logSize;
                log.stats[j].brakeTime += logs->logs[i].stats[j].brakeTime / logSize;
                log.stats[j].totalBursts += logs->logs[i].stats[j].totalBursts / logSize;
                log.stats[j].burstsHit += logs->logs[i].stats[j].burstsHit / logSize;
                log.stats[j].energyEmptied += logs->logs[i].stats[j].energyEmptied / logSize;
            }
        }
    }

    logs->size = 0;
    return log;
}

// returns a cell index that is closest to pos that isn't cellIdx
uint16_t findNearestCell(const env *e, const b2Vec2 pos, const uint16_t cellIdx) {
    uint8_t cellOffsets[8][2] = {
        {-1, 0},  // left
        {1, 0},   // right
        {0, -1},  // up
        {0, 1},   // down
        {-1, -1}, // top-left
        {1, -1},  // top-right
        {-1, 1},  // bottom-left
        {1, 1},   // bottom-right
    };

    uint16_t closestCell = cellIdx;
    float minDistance = FLT_MAX;
    const uint8_t cellCol = cellIdx / e->map->columns;
    const uint8_t cellRow = cellIdx % e->map->columns;
    for (uint8_t i = 0; i < 8; i++) {
        const int8_t newCellCol = cellCol + cellOffsets[i][0];
        if (newCellCol < 0 || newCellCol >= e->map->columns) {
            continue;
        }
        const int8_t newCellRow = cellRow + cellOffsets[i][1];
        if (newCellRow < 0 || newCellRow >= e->map->rows) {
            continue;
        }
        const int16_t newCellIdx = cellIndex(e, newCellCol, newCellRow);
        const mapCell *cell = safe_array_get_at(e->cells, newCellIdx);
        if (minDistance != fminf(minDistance, b2DistanceSquared(pos, cell->pos))) {
            closestCell = newCellIdx;
        }
    }

    return closestCell;
}

// normalize a drone's ammo count, setting infinite ammo as no ammo
static inline float scaleAmmo(const env *e, const droneEntity *drone) {
    int8_t maxAmmo = weaponAmmo(e->defaultWeapon->type, drone->weaponInfo->type);
    float scaledAmmo = 0;
    if (drone->ammo != INFINITE) {
        scaledAmmo = scaleValue(drone->ammo, maxAmmo, true);
    }
    return scaledAmmo;
}

// fills a small 2D grid centered around the agent with discretized
// walls, floating walls, weapon pickups, and drone positions
void computeMapObs(env *e, const uint8_t agentIdx, const uint16_t obsStartOffset) {
    droneEntity *drone = safe_array_get_at(e->drones, agentIdx);
    const uint8_t droneCellCol = drone->mapCellIdx % e->map->columns;
    const uint8_t droneCellRow = drone->mapCellIdx / e->map->columns;

    const int8_t obsStartCol = droneCellCol - (MAP_OBS_COLUMNS / 2);
    const int8_t startCol = max(obsStartCol, 0);
    const int8_t obsStartRow = droneCellRow - (MAP_OBS_ROWS / 2);
    const int8_t startRow = max(obsStartRow, 0);

    const int8_t obsEndCol = droneCellCol + (MAP_OBS_COLUMNS / 2);
    const int8_t endCol = min(obsEndCol, e->map->columns - 1);
    const int8_t endRow = min(droneCellRow + (MAP_OBS_ROWS / 2), e->map->rows - 1);

    const int8_t obsColOffset = startCol - obsStartCol;
    const int8_t obsRowOffset = startRow - obsStartRow;
    uint16_t startOffset = obsStartOffset;
    if (obsColOffset == 0 && obsRowOffset != 0) {
        startOffset += obsRowOffset * MAP_OBS_COLUMNS;
    } else if (obsColOffset != 0 && obsRowOffset == 0) {
        startOffset += obsColOffset;
    } else if (obsColOffset != 0 && obsRowOffset != 0) {
        startOffset += obsColOffset + (obsRowOffset * MAP_OBS_COLUMNS);
    }
    uint16_t offset = startOffset;

    // compute map layout, and discretized positions of weapon pickups
    if (!e->suddenDeathWallsPlaced) {
        // copy precomputed map layout if sudden death walls haven't been placed
        const int8_t numCols = endCol - startCol + 1;
        for (int8_t row = startRow; row <= endRow; row++) {
            const int16_t cellIdx = cellIndex(e, startCol, row);
            memcpy(e->obs + offset, e->map->packedLayout + cellIdx, numCols * sizeof(uint8_t));
            offset += MAP_OBS_COLUMNS;
        }

        // compute discretized location of weapon pickups on grid
        for (size_t i = 0; i < cc_array_size(e->pickups); i++) {
            const weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
            const uint8_t cellCol = pickup->mapCellIdx % e->map->columns;
            if (cellCol < startCol || cellCol > endCol) {
                continue;
            }
            const uint8_t cellRow = pickup->mapCellIdx / e->map->columns;
            if (cellRow < startRow || cellRow > endRow) {
                continue;
            }

            offset = startOffset + ((cellCol - startCol) + ((cellRow - startRow) * MAP_OBS_COLUMNS));
            ASSERTF(offset <= startOffset + MAP_OBS_SIZE, "offset: %d", offset);
            e->obs[offset] |= 1 << 3;
        }
    } else {
        // sudden death walls have been placed so compute may layout manually
        const int8_t colPadding = obsColOffset + (obsEndCol - endCol);
        for (int8_t row = startRow; row <= endRow; row++) {
            for (int8_t col = startCol; col <= endCol; col++) {
                const int16_t cellIdx = cellIndex(e, col, row);
                const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
                if (cell->ent == NULL) {
                    offset++;
                    continue;
                }

                if (entityTypeIsWall(cell->ent->type)) {
                    e->obs[offset] = ((cell->ent->type + 1) & TWO_BIT_MASK) << 5;
                } else if (cell->ent->type == WEAPON_PICKUP_ENTITY) {
                    e->obs[offset] |= 1 << 3;
                }

                offset++;
            }
            offset += colPadding;
        }
        ASSERTF(offset <= startOffset + MAP_OBS_SIZE, "offset %u startOffset %u", offset, startOffset);
    }

    // compute discretized locations of floating walls on grid
    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
        const wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        const uint8_t cellCol = wall->mapCellIdx % e->map->columns;
        if (cellCol < startCol || cellCol > endCol) {
            continue;
        }
        const uint8_t cellRow = wall->mapCellIdx / e->map->columns;
        if (cellRow < startRow || cellRow > endRow) {
            continue;
        }

        offset = startOffset + ((cellCol - startCol) + ((cellRow - startRow) * MAP_OBS_COLUMNS));
        ASSERTF(offset <= startOffset + MAP_OBS_SIZE, "offset: %d", offset);
        e->obs[offset] = ((wall->type + 1) & TWO_BIT_MASK) << 5;
        e->obs[offset] |= 1 << 4;
    }

    // compute discretized location and index of drones on grid
    uint8_t newDroneIdx = 1;
    uint16_t droneCells[e->numDrones];
    memset(droneCells, 0x0, sizeof(droneCells));
    for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
        if (i == agentIdx) {
            continue;
        }

        // ensure drones do not share cells in the observation
        droneEntity *otherDrone = safe_array_get_at(e->drones, i);
        if (i != 0) {
            for (uint8_t j = 0; j < i; j++) {
                if (droneCells[j] == otherDrone->mapCellIdx) {
                    otherDrone->mapCellIdx = findNearestCell(e, otherDrone->pos, otherDrone->mapCellIdx);
                    break;
                }
            }
        }
        const uint8_t cellCol = otherDrone->mapCellIdx % e->map->columns;
        if (cellCol < startCol || cellCol > endCol) {
            continue;
        }
        const uint8_t cellRow = otherDrone->mapCellIdx / e->map->columns;
        if (cellRow < startRow || cellRow > endRow) {
            continue;
        }
        droneCells[i] = otherDrone->mapCellIdx;

        offset = startOffset + ((cellCol - startCol) + ((cellRow - startRow) * MAP_OBS_COLUMNS));
        ASSERTF(offset <= startOffset + MAP_OBS_SIZE, "offset: %d", offset);
        e->obs[offset] |= (newDroneIdx++ & THREE_BIT_MASK);
    }
}

#ifndef AUTOPXD
// computes observations for N nearest walls, floating walls, and weapon pickups
void computeNearMapObs(env *e, droneEntity *drone, float *scalarObs) {
    nearEntity nearWalls[NUM_NEAR_WALL_OBS];
    findNearWalls(e, drone, nearWalls, NUM_NEAR_WALL_OBS);

    uint16_t offset;

    // compute type and position of N nearest walls
    for (uint8_t i = 0; i < NUM_NEAR_WALL_OBS; i++) {
        const wallEntity *wall = nearWalls[i].entity;

        offset = NEAR_WALL_TYPES_OBS_OFFSET + i;
        ASSERTF(offset <= NEAR_WALL_POS_OBS_OFFSET, "offset: %d", offset);
        scalarObs[offset] = wall->type;

        // DEBUG_LOGF("wall %d cell %d", i, wall->mapCellIdx);

        offset = NEAR_WALL_POS_OBS_OFFSET + (i * NEAR_WALL_POS_OBS_SIZE);
        ASSERTF(offset <= FLOATING_WALL_TYPES_OBS_OFFSET, "offset: %d", offset);
        const b2Vec2 wallRelPos = b2Sub(wall->pos, drone->pos);

        scalarObs[offset++] = scaleValue(wallRelPos.x, MAX_X_POS, false);
        scalarObs[offset] = scaleValue(wallRelPos.y, MAX_Y_POS, false);
    }

    if (cc_array_size(e->floatingWalls) != 0) {
        // find N nearest floating walls
        nearEntity nearFloatingWalls[MAX_FLOATING_WALLS] = {0};
        for (uint8_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
            wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
            const nearEntity nearEnt = {
                .entity = wall,
                .distanceSquared = b2DistanceSquared(wall->pos, drone->pos),
            };
            nearFloatingWalls[i] = nearEnt;
        }
        insertionSort(nearFloatingWalls, cc_array_size(e->floatingWalls));

        // compute type, position, angle and velocity of N nearest floating walls
        for (uint8_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
            if (i == NUM_FLOATING_WALL_OBS) {
                break;
            }
            const wallEntity *wall = nearFloatingWalls[i].entity;

            const b2Transform wallTransform = b2Body_GetTransform(wall->bodyID);
            const b2Vec2 wallRelPos = b2Sub(wallTransform.p, drone->pos);
            const float angle = b2Rot_GetAngle(wallTransform.q);

            offset = FLOATING_WALL_TYPES_OBS_OFFSET + i;
            ASSERTF(offset <= FLOATING_WALL_INFO_OBS_OFFSET, "offset: %d", offset);
            scalarObs[offset] = wall->type + 1;

            // DEBUG_LOGF("floating wall %d cell %d", i, wall->mapCellIdx);

            offset = FLOATING_WALL_INFO_OBS_OFFSET + (i * FLOATING_WALL_INFO_OBS_SIZE);
            ASSERTF(offset <= WEAPON_PICKUP_TYPES_OBS_OFFSET, "offset: %d", offset);
            scalarObs[offset++] = scaleValue(wallRelPos.x, MAX_X_POS, false);
            scalarObs[offset++] = scaleValue(wallRelPos.y, MAX_Y_POS, false);
            scalarObs[offset++] = scaleValue(angle, MAX_ANGLE, false);
            scalarObs[offset++] = scaleValue(wall->velocity.x, MAX_SPEED, false);
            scalarObs[offset] = scaleValue(wall->velocity.y, MAX_SPEED, false);
        }
    }

    if (cc_array_size(e->pickups) != 0) {
        // find N nearest weapon pickups
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

        // compute type and location of N nearest weapon pickups
        for (uint8_t i = 0; i < cc_array_size(e->pickups); i++) {
            if (i == NUM_WEAPON_PICKUP_OBS) {
                break;
            }
            const weaponPickupEntity *pickup = nearPickups[i].entity;

            offset = WEAPON_PICKUP_TYPES_OBS_OFFSET + i;
            ASSERTF(offset <= WEAPON_PICKUP_POS_OBS_OFFSET, "offset: %d", offset);
            scalarObs[offset] = pickup->weapon + 1;

            // DEBUG_LOGF("pickup %d cell %d", i, pickup->mapCellIdx);

            offset = WEAPON_PICKUP_POS_OBS_OFFSET + (i * WEAPON_PICKUP_POS_OBS_SIZE);
            ASSERTF(offset <= PROJECTILE_TYPES_OBS_OFFSET, "offset: %d", offset);
            const b2Vec2 pickupRelPos = b2Sub(pickup->pos, drone->pos);
            scalarObs[offset++] = scaleValue(pickupRelPos.x, MAX_X_POS, false);
            scalarObs[offset] = scaleValue(pickupRelPos.y, MAX_Y_POS, false);
        }
    }
}
#endif

void computeObs(env *e) {
    for (uint8_t agentIdx = 0; agentIdx < e->numAgents; agentIdx++) {
        droneEntity *agentDrone = safe_array_get_at(e->drones, agentIdx);
        // if the drone is dead, only compute observations if it died
        // this step and it isn't out of bounds
        if (agentDrone->dead && (!agentDrone->diedThisStep || agentDrone->mapCellIdx == -1)) {
            continue;
        }

        // compute discrete map observations
        uint16_t mapObsOffset = e->obsBytes * agentIdx;
        memset(e->obs + mapObsOffset, 0x0, e->obsBytes);
        const uint16_t mapObsStart = mapObsOffset;
        computeMapObs(e, agentIdx, mapObsOffset);

        // compute continuous scalar observations
        uint16_t scalarObsOffset = 0;
        const uint16_t scalarObsStart = mapObsStart + e->mapObsBytes;
        float *scalarObs = (float *)(e->obs + scalarObsStart);

        computeNearMapObs(e, agentDrone, scalarObs);

        // compute type and location of N projectiles
        for (size_t i = 0; i < cc_array_size(e->projectiles); i++) {
            // TODO: handle better
            if (i == NUM_PROJECTILE_OBS) {
                break;
            }
            const projectileEntity *projectile = safe_array_get_at(e->projectiles, i);

            scalarObsOffset = PROJECTILE_TYPES_OBS_OFFSET + i;
            ASSERTF(scalarObsOffset <= PROJECTILE_POS_OBS_OFFSET, "offset: %d", scalarObsOffset);
            scalarObs[scalarObsOffset] = projectile->weaponInfo->type + 1;

            scalarObsOffset = PROJECTILE_POS_OBS_OFFSET + (i * PROJECTILE_INFO_OBS_SIZE);
            ASSERTF(scalarObsOffset <= ENEMY_DRONE_OBS_OFFSET, "offset: %d", scalarObsOffset);
            const b2Vec2 projectileRelPos = b2Sub(projectile->pos, agentDrone->pos);
            scalarObs[scalarObsOffset++] = projectile->droneIdx + 1;
            scalarObs[scalarObsOffset++] = scaleValue(projectileRelPos.x, MAX_X_POS, false);
            scalarObs[scalarObsOffset++] = scaleValue(projectileRelPos.y, MAX_Y_POS, false);
            scalarObs[scalarObsOffset++] = scaleValue(projectile->velocity.x, MAX_SPEED, false);
            scalarObs[scalarObsOffset] = scaleValue(projectile->velocity.y, MAX_SPEED, false);
        }

        // compute enemy drone observations
        bool hitShot = false;
        bool tookShot = false;
        uint8_t processedDrones = 0;
        for (uint8_t i = 0; i < e->numDrones; i++) {
            if (i == agentIdx) {
                continue;
            }

            if (agentDrone->stepInfo.shotHit[i]) {
                hitShot = true;
            }
            if (agentDrone->stepInfo.shotTaken[i]) {
                tookShot = true;
            }

            droneEntity *enemyDrone = safe_array_get_at(e->drones, i);
            if (enemyDrone->dead) {
                processedDrones++;
                continue;
            }

            const b2Vec2 enemyDroneRelPos = b2Sub(enemyDrone->pos, agentDrone->pos);
            const float enemyDroneDistance = b2Distance(enemyDrone->pos, agentDrone->pos);
            const b2Vec2 enemyDroneAccel = b2Sub(enemyDrone->velocity, enemyDrone->lastVelocity);
            const b2Vec2 enemyDroneRelNormPos = b2Normalize(b2Sub(enemyDrone->pos, agentDrone->pos));
            const float enemyDroneAimAngle = atan2f(enemyDrone->lastAim.y, enemyDrone->lastAim.x);
            float enemyDroneBraking = 0.0f;
            if (enemyDrone->braking) {
                enemyDroneBraking = 1.0f;
            }

            const uint16_t enemyDroneObsOffset = ENEMY_DRONE_OBS_OFFSET + processedDrones;
            scalarObs[enemyDroneObsOffset] = enemyDrone->weaponInfo->type + 1;

            scalarObsOffset = ENEMY_DRONE_OBS_OFFSET + (e->numDrones - 1) + (processedDrones * (ENEMY_DRONE_OBS_SIZE - 1));
            scalarObs[scalarObsOffset++] = enemyDrone->team == agentDrone->team;
            scalarObs[scalarObsOffset++] = scaleValue(enemyDroneRelPos.x, MAX_X_POS, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDroneRelPos.y, MAX_Y_POS, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDroneDistance, MAX_DISTANCE, true);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDrone->velocity.x, MAX_SPEED, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDrone->velocity.y, MAX_SPEED, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDroneAccel.x, MAX_ACCEL, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDroneAccel.y, MAX_ACCEL, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDroneRelNormPos.x, 1.0f, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDroneRelNormPos.y, 1.0f, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDrone->lastAim.x, 1.0f, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDrone->lastAim.y, 1.0f, false);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDroneAimAngle, PI, false);
            scalarObs[scalarObsOffset++] = scaleAmmo(e, enemyDrone);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDrone->weaponCooldown, enemyDrone->weaponInfo->coolDown, true);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDrone->weaponCharge, enemyDrone->weaponInfo->charge, true);
            scalarObs[scalarObsOffset++] = scaleValue(enemyDrone->energyLeft, DRONE_ENERGY_MAX, true);
            scalarObs[scalarObsOffset++] = (float)enemyDrone->energyFullyDepleted;
            scalarObs[scalarObsOffset++] = enemyDroneBraking;
            scalarObs[scalarObsOffset++] = scaleValue(enemyDrone->burstCooldown, DRONE_BURST_COOLDOWN, true);
            scalarObs[scalarObsOffset++] = (float)enemyDrone->chargingBurst;
            scalarObs[scalarObsOffset++] = scaleValue(enemyDrone->burstCharge, DRONE_ENERGY_MAX, true);
            scalarObs[scalarObsOffset++] = 1; // is drone alive

            processedDrones++;
            ASSERTF(scalarObsOffset == ENEMY_DRONE_OBS_OFFSET + (e->numDrones - 1) + (processedDrones * (ENEMY_DRONE_OBS_SIZE - 1)), "offset: %d", scalarObsOffset);
        }

        // compute active drone observations
        scalarObsOffset = ENEMY_DRONE_OBS_OFFSET + ((e->numDrones - 1) * ENEMY_DRONE_OBS_SIZE);
        const b2Vec2 agentDroneAccel = b2Sub(agentDrone->velocity, agentDrone->lastVelocity);
        float agentDroneBraking = 0.0f;
        if (agentDrone->braking) {
            agentDroneBraking = 1.0f;
        }

        scalarObs[scalarObsOffset++] = agentDrone->weaponInfo->type + 1;
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->pos.x, MAX_X_POS, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->pos.y, MAX_Y_POS, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->velocity.x, MAX_SPEED, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->velocity.y, MAX_SPEED, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDroneAccel.x, MAX_ACCEL, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDroneAccel.y, MAX_ACCEL, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->lastAim.x, 1.0f, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->lastAim.y, 1.0f, false);
        scalarObs[scalarObsOffset++] = scaleAmmo(e, agentDrone);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->weaponCooldown, agentDrone->weaponInfo->coolDown, true);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->weaponCharge, agentDrone->weaponInfo->charge, true);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->energyLeft, DRONE_ENERGY_MAX, true);
        scalarObs[scalarObsOffset++] = (float)agentDrone->energyFullyDepleted;
        scalarObs[scalarObsOffset++] = agentDroneBraking;
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->burstCooldown, DRONE_BURST_COOLDOWN, true);
        scalarObs[scalarObsOffset++] = (float)agentDrone->chargingBurst;
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->burstCharge, DRONE_ENERGY_MAX, true);
        scalarObs[scalarObsOffset++] = hitShot;
        scalarObs[scalarObsOffset++] = tookShot;
        scalarObs[scalarObsOffset++] = agentDrone->stepInfo.ownShotTaken;

        ASSERTF(scalarObsOffset == ENEMY_DRONE_OBS_OFFSET + ((e->numDrones - 1) * ENEMY_DRONE_OBS_SIZE) + DRONE_OBS_SIZE, "offset: %d", scalarObsOffset);
        scalarObs[scalarObsOffset] = scaleValue(e->stepsLeft, e->totalSteps, true);
    }
}

void setupEnv(env *e) {
    e->needsReset = false;

    e->stepsLeft = e->totalSteps;
    e->suddenDeathSteps = e->totalSuddenDeathSteps;
    e->suddenDeathWallCounter = 0;

    e->lastSpawnQuad = -1;

    int8_t mapIdx = e->pinnedMapIdx;
    if (e->pinnedMapIdx == -1) {
        uint8_t firstMap = 0;
        // don't evaluate on the boring empty map
        if (!e->isTraining) {
            firstMap = 1;
        }
        mapIdx = randInt(&e->randState, firstMap, NUM_MAPS - 1);
    }
    DEBUG_LOGF("setting up map %d", mapIdx);
    setupMap(e, mapIdx);

    DEBUG_LOG("creating drones");
    for (uint8_t i = 0; i < e->numDrones; i++) {
        createDrone(e, i);
    }

    DEBUG_LOG("placing floating walls");
    placeRandFloatingWalls(e, mapIdx);

    DEBUG_LOG("creating weapon pickups");
    // start spawning pickups in a random quadrant
    e->lastSpawnQuad = randInt(&e->randState, 0, 3);
    for (uint8_t i = 0; i < maps[mapIdx]->weaponPickups; i++) {
        createWeaponPickup(e);
    }

    if (e->client != NULL) {
        setEnvRenderScale(e);
        renderEnv(e, true, false, -1, -1);
    }

    computeObs(e);
}

// sets the timing related variables for the environment depending on
// the frame rate
void setEnvFrameRate(env *e, uint8_t frameRate) {
    e->frameRate = frameRate;
    e->deltaTime = 1.0f / (float)frameRate;
    e->frameSkip = frameRate / TRAINING_ACTIONS_PER_SECOND;

    e->totalSteps = ROUND_STEPS * frameRate;
    e->totalSuddenDeathSteps = SUDDEN_DEATH_STEPS * frameRate;
}

env *initEnv(env *e, uint8_t numDrones, uint8_t numAgents, uint8_t *obs, bool discretizeActions, float *contActions, int32_t *discActions, float *rewards, uint8_t *masks, uint8_t *terminals, uint8_t *truncations, logBuffer *logs, int8_t mapIdx, uint64_t seed, bool enableTeams, bool sittingDuck, bool isTraining) {
    e->numDrones = numDrones;
    e->numAgents = numAgents;
    e->teamsEnabled = enableTeams;
    e->numTeams = numDrones;
    if (e->teamsEnabled) {
        e->numTeams = 2;
    }
    e->sittingDuck = sittingDuck;
    e->isTraining = isTraining;

    e->obsBytes = obsBytes(e->numDrones);
    e->mapObsBytes = alignedSize(MAP_OBS_SIZE * sizeof(uint8_t), sizeof(float));

    e->obs = obs;
    e->discretizeActions = discretizeActions;
    e->contActions = contActions;
    e->discActions = discActions;

    e->rewards = rewards;
    e->masks = masks;
    e->terminals = terminals;
    e->truncations = truncations;

    float frameRate = TRAINING_FRAME_RATE;
    e->box2dSubSteps = TRAINING_BOX2D_SUBSTEPS;
    // set a higher frame rate and physics substeps when evaluating
    // to make it more enjoyable to play
    if (!isTraining) {
        frameRate = EVAL_FRAME_RATE;
        e->box2dSubSteps = EVAL_BOX2D_SUBSTEPS;
    }
    setEnvFrameRate(e, frameRate);
    e->randState = seed;
    e->needsReset = false;

    e->logs = logs;

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){.x = 0.0f, .y = 0.0f};
    e->worldID = b2CreateWorld(&worldDef);
    e->pinnedMapIdx = mapIdx;
    e->mapIdx = -1;

    cc_array_new(&e->cells);
    cc_array_new(&e->walls);
    cc_array_new(&e->floatingWalls);
    cc_array_new(&e->drones);
    cc_array_new(&e->pickups);
    cc_array_new(&e->projectiles);
    cc_array_new(&e->brakeTrailPoints);
    cc_array_new(&e->explosions);
    cc_array_new(&e->explodingProjectiles);

    e->mapPathing = fastCalloc(NUM_MAPS, sizeof(pathingInfo));
    for (uint8_t i = 0; i < NUM_MAPS; i++) {
        const mapEntry *map = maps[i];
        pathingInfo *info = &e->mapPathing[i];
        info->paths = fastMalloc(map->rows * map->columns * map->rows * map->columns * sizeof(uint8_t));
        memset(info->paths, UINT8_MAX, map->rows * map->columns * map->rows * map->columns * sizeof(uint8_t));
        info->pathBuffer = fastCalloc(3 * 8 * map->rows * map->columns, sizeof(int8_t));
    }

    e->humanInput = false;
    e->humanDroneInput = 0;
    e->connectedControllers = 0;

    return e;
}

void clearEnv(env *e) {
    // rewards get cleared in stepEnv every step
    memset(e->masks, 1, e->numAgents * sizeof(uint8_t));
    memset(e->terminals, 0x0, e->numAgents * sizeof(uint8_t));
    memset(e->truncations, 0x0, e->numAgents * sizeof(uint8_t));

    e->episodeLength = 0;
    memset(e->stats, 0x0, sizeof(e->stats));

    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        destroyDrone(drone);
    }

    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
        wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        destroyWall(e, wall, false);
    }

    for (size_t i = 0; i < cc_array_size(e->pickups); i++) {
        weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
        destroyWeaponPickup(e, pickup);
    }

    for (size_t i = 0; i < cc_array_size(e->projectiles); i++) {
        projectileEntity *p = safe_array_get_at(e->projectiles, i);
        destroyProjectile(e, p, false, false);
    }

    for (size_t i = 0; i < cc_array_size(e->brakeTrailPoints); i++) {
        brakeTrailPoint *trailPoint = safe_array_get_at(e->brakeTrailPoints, i);
        fastFree(trailPoint);
    }

    for (size_t i = 0; i < cc_array_size(e->explosions); i++) {
        explosionInfo *explosion = safe_array_get_at(e->explosions, i);
        fastFree(explosion);
    }

    cc_array_remove_all(e->drones);
    cc_array_remove_all(e->floatingWalls);
    cc_array_remove_all(e->pickups);
    cc_array_remove_all(e->projectiles);
    cc_array_remove_all(e->explodingProjectiles);
    cc_array_remove_all(e->brakeTrailPoints);
    cc_array_remove_all(e->explosions);
}

void destroyEnv(env *e) {
    clearEnv(e);

    for (uint8_t i = 0; i < NUM_MAPS; i++) {
        pathingInfo *info = &e->mapPathing[i];
        fastFree(info->paths);
        fastFree(info->pathBuffer);
    }
    fastFree(e->mapPathing);

    for (size_t i = 0; i < cc_array_size(e->walls); i++) {
        wallEntity *wall = safe_array_get_at(e->walls, i);
        destroyWall(e, wall, false);
    }

    for (size_t i = 0; i < cc_array_size(e->cells); i++) {
        mapCell *cell = safe_array_get_at(e->cells, i);
        fastFree(cell);
    }

    cc_array_destroy(e->cells);
    cc_array_destroy(e->walls);
    cc_array_destroy(e->drones);
    cc_array_destroy(e->floatingWalls);
    cc_array_destroy(e->pickups);
    cc_array_destroy(e->projectiles);
    cc_array_destroy(e->brakeTrailPoints);
    cc_array_destroy(e->explosions);
    cc_array_destroy(e->explodingProjectiles);

    b2DestroyWorld(e->worldID);
}

void resetEnv(env *e) {
    clearEnv(e);
    setupEnv(e);
}

float computeShotReward(const droneEntity *drone, const weaponInformation *weaponInfo) {
    const float weaponForce = weaponInfo->fireMagnitude * weaponInfo->invMass;
    const float scaledForce = (weaponForce * (weaponForce * SHOT_HIT_REWARD_COEF)) + 0.25f;
    return scaledForce + computeHitStrength(drone);
}

float computeExplosionReward(const droneEntity *drone) {
    return computeHitStrength(drone) * EXPLOSION_HIT_REWARD_COEF;
}

float computeReward(env *e, droneEntity *drone) {
    float reward = 0.0f;

    if (drone->energyFullyDepleted && drone->energyRefillWait == DRONE_ENERGY_REFILL_EMPTY_WAIT) {
        reward += ENERGY_EMPTY_PUNISHMENT;
    }

    // only reward picking up a weapon if the standard weapon was
    // previously held; every weapon is better than the standard
    // weapon, but other weapons are situational better so don't
    // reward switching a non-standard weapon
    if (drone->stepInfo.pickedUpWeapon && drone->stepInfo.prevWeapon == STANDARD_WEAPON) {
        reward += WEAPON_PICKUP_REWARD;
    }

    for (uint8_t i = 0; i < e->numDrones; i++) {
        if (i == drone->idx) {
            continue;
        }
        droneEntity *enemyDrone = safe_array_get_at(e->drones, i);
        const bool onTeam = drone->team == enemyDrone->team;

        if (drone->stepInfo.shotHit[i] != 0 && !onTeam) {
            // subtract 1 from the weapon type because 1 is added so we
            // can use 0 as no shot was hit
            const weaponInformation *weaponInfo = weaponInfos[drone->stepInfo.shotHit[i] - 1];
            reward += computeShotReward(enemyDrone, weaponInfo);
        }
        if (drone->stepInfo.explosionHit[i] && !onTeam) {
            reward += computeExplosionReward(enemyDrone);
        }

        if (e->numAgents == e->numDrones) {
            if (drone->stepInfo.shotTaken[i] != 0 && !onTeam) {
                const weaponInformation *weaponInfo = weaponInfos[drone->stepInfo.shotTaken[i] - 1];
                reward -= computeShotReward(drone, weaponInfo) * 0.5f;
            }
            if (drone->stepInfo.explosionTaken[i] && !onTeam) {
                reward -= computeExplosionReward(drone) * 0.5f;
            }
        }

        if (enemyDrone->dead && enemyDrone->diedThisStep) {
            if (!onTeam) {
                reward += ENEMY_DEATH_REWARD;
            } else {
                reward += TEAMMATE_DEATH_PUNISHMENT;
            }
            continue;
        }

        const b2Vec2 enemyDirection = b2Normalize(b2Sub(enemyDrone->pos, drone->pos));
        const float velocityToEnemy = b2Dot(drone->lastVelocity, enemyDirection);
        const float enemyDistance = b2Distance(enemyDrone->pos, drone->pos);
        // stop rewarding approaching an enemy if they're very close
        // to avoid constant clashing; always reward approaching when
        // the current weapon is the shotgun, it greatly benefits from
        // being close to enemies
        if (velocityToEnemy > 0.1f && (drone->weaponInfo->type == SHOTGUN_WEAPON || enemyDistance > DISTANCE_CUTOFF)) {
            reward += APPROACH_REWARD;
        }
    }

    return reward;
}

const float REWARD_EPS = 1.0e-6f;

void computeRewards(env *e, const bool roundOver, const int8_t winner, const int8_t winningTeam) {
    if (roundOver && winner != -1 && winner < e->numAgents) {
        e->rewards[winner] += WIN_REWARD;
    }

    for (uint8_t i = 0; i < e->numDrones; i++) {
        float reward = 0.0f;
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (!drone->dead) {
            reward = computeReward(e, drone);
            if (roundOver && winningTeam == drone->team) {
                reward += WIN_REWARD;
            }
        } else if (drone->diedThisStep) {
            reward = DEATH_PUNISHMENT;
        }
        if (i < e->numAgents) {
            e->rewards[i] += reward;
        }
        e->stats[i].reward += reward;
    }
}

static inline bool isActionNoop(const b2Vec2 action) {
    return b2Length(action) < ACTION_NOOP_MAGNITUDE;
}

agentActions _computeActions(env *e, droneEntity *drone, const agentActions *manualActions) {
    agentActions actions = {0};

    if (e->discretizeActions && manualActions == NULL) {
        const uint8_t offset = drone->idx * DISCRETE_ACTION_SIZE;
        uint8_t move = e->discActions[offset + 0];
        // 0 is no-op for both move and aim
        ASSERT(move <= 8);
        if (move != 0) {
            move--;
            actions.move.x = discMoveToContMoveMap[0][move];
            actions.move.y = discMoveToContMoveMap[1][move];
        }
        uint8_t aim = e->discActions[offset + 1];
        ASSERT(move <= 16);
        if (aim != 0) {
            aim--;
            actions.aim.x = discAimToContAimMap[0][aim];
            actions.aim.y = discAimToContAimMap[1][aim];
        }
        const uint8_t shoot = e->discActions[offset + 2];
        ASSERT(shoot <= 1);
        actions.chargingWeapon = (bool)shoot;
        actions.shoot = actions.chargingWeapon;
        if (!actions.chargingWeapon && drone->chargingWeapon) {
            actions.shoot = true;
        }
        const uint8_t brake = e->discActions[offset + 3];
        ASSERT(brake <= 1);
        if (brake == 1) {
            actions.brake = true;
        }
        const uint8_t burst = e->discActions[offset + 4];
        ASSERT(burst <= 1);
        actions.chargingBurst = (bool)burst;
        if (!actions.chargingBurst && drone->chargingBurst) {
            actions.burst = true;
        }
        return actions;
    }

    const uint8_t offset = drone->idx * CONTINUOUS_ACTION_SIZE;
    if (manualActions == NULL) {
        actions.move = (b2Vec2){.x = tanhf(e->contActions[offset + 0]), .y = tanhf(e->contActions[offset + 1])};
        actions.aim = (b2Vec2){.x = tanhf(e->contActions[offset + 2]), .y = tanhf(e->contActions[offset + 3])};
        actions.chargingWeapon = e->contActions[offset + 4] > 0.0f;
        actions.shoot = actions.chargingWeapon;
        if (!actions.chargingWeapon && drone->chargingWeapon) {
            actions.shoot = true;
        }
        actions.brake = e->contActions[offset + 5] > 0.0f;
        actions.chargingBurst = e->contActions[offset + 6] > 0.0f;
        if (!actions.chargingBurst && drone->chargingBurst) {
            actions.burst = true;
        }
    } else {
        actions.move = manualActions->move;
        actions.aim = manualActions->aim;
        actions.chargingWeapon = manualActions->chargingWeapon;
        actions.shoot = manualActions->shoot;
        actions.brake = manualActions->brake;
        actions.chargingBurst = manualActions->chargingBurst;
        actions.burst = manualActions->burst;
        actions.discardWeapon = manualActions->discardWeapon;
    }

    // cap movement magnitude to 1.0
    if (b2Length(actions.move) > 1.0f) {
        actions.move = b2Normalize(actions.move);
    } else if (isActionNoop(actions.move)) {
        actions.move = b2Vec2_zero;
    }

    if (isActionNoop(actions.aim)) {
        actions.aim = b2Vec2_zero;
    } else {
        actions.aim = b2Normalize(actions.aim);
    }

    return actions;
}

agentActions computeActions(env *e, droneEntity *drone, const agentActions *manualActions) {
    const agentActions actions = _computeActions(e, drone, manualActions);
    drone->lastMove = actions.move;
    if (!b2VecEqual(actions.aim, b2Vec2_zero)) {
        drone->lastAim = actions.aim;
    }
    return actions;
}

void updateConnectedControllers(env *e) {
    for (uint8_t i = 0; i < e->numDrones; i++) {
        if (IsGamepadAvailable(i)) {
            e->connectedControllers++;
        }
    }
}

void updateHumanInputToggle(env *e) {
    if (IsKeyPressed(KEY_LEFT_CONTROL)) {
        e->humanInput = !e->humanInput;
        if (!e->humanInput) {
            e->connectedControllers = 0;
        }
    }
    if (e->humanInput && e->connectedControllers == 0) {
        updateConnectedControllers(e);
    }
    if (e->connectedControllers > 1) {
        e->humanDroneInput = e->numDrones - e->connectedControllers;
        return;
    }

    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
        e->humanDroneInput = 0;
    }
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
        e->humanDroneInput = 1;
    }
    if (e->numDrones >= 3 && (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3))) {
        e->humanDroneInput = 2;
    }
    if (e->numDrones >= 4 && (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4))) {
        e->humanDroneInput = 3;
    }
}

agentActions getPlayerInputs(env *e, droneEntity *drone, uint8_t gamepadIdx) {
    agentActions actions = {0};

    bool controllerConnected = false;
    if (IsGamepadAvailable(gamepadIdx)) {
        controllerConnected = true;
    }
    if (controllerConnected) {
        float lStickX = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_LEFT_X);
        float lStickY = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_LEFT_Y);
        float rStickX = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_RIGHT_X);
        float rStickY = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_RIGHT_Y);

        if (IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
            actions.chargingWeapon = true;
            actions.shoot = true;
        } else if (drone->chargingWeapon && IsGamepadButtonUp(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
            actions.shoot = true;
        }

        if (IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_LEFT_TRIGGER_2)) {
            actions.brake = true;
        }

        if (IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) || IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
            actions.chargingBurst = true;
        } else if (drone->chargingBurst && (IsGamepadButtonUp(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) || IsGamepadButtonUp(gamepadIdx, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
            actions.burst = true;
        }

        if (IsGamepadButtonPressed(gamepadIdx, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
            actions.discardWeapon = true;
        }

        actions.move = (b2Vec2){.x = lStickX, .y = lStickY};
        actions.aim = (b2Vec2){.x = rStickX, .y = rStickY};
        return computeActions(e, drone, &actions);
    }
    if (!controllerConnected && drone->idx != e->humanDroneInput) {
        return actions;
    }

    b2Vec2 move = b2Vec2_zero;
    if (IsKeyDown(KEY_W)) {
        move.y += -1.0f;
    }
    if (IsKeyDown(KEY_S)) {
        move.y += 1.0f;
    }
    if (IsKeyDown(KEY_A)) {
        move.x += -1.0f;
    }
    if (IsKeyDown(KEY_D)) {
        move.x += 1.0f;
    }
    actions.move = b2Normalize(move);

    Vector2 mousePos = (Vector2){.x = (float)GetMouseX(), .y = (float)GetMouseY()};
    actions.aim = b2Normalize(b2Sub(rayVecToB2Vec(e, mousePos), drone->pos));

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        actions.chargingWeapon = true;
        actions.shoot = true;
    } else if (drone->chargingWeapon && IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
        actions.shoot = true;
    }
    if (IsKeyDown(KEY_SPACE)) {
        actions.brake = true;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        actions.chargingBurst = true;
    } else if (drone->chargingBurst && IsMouseButtonUp(MOUSE_BUTTON_RIGHT)) {
        actions.burst = true;
    }

    return computeActions(e, drone, &actions);
}

bool droneControlledByHuman(const env *e, uint8_t i) {
    if (!e->humanInput) {
        return false;
    }
    return (e->connectedControllers > 1 && i >= e->humanDroneInput) || (e->connectedControllers <= 1 && i == e->humanDroneInput);
}

void stepEnv(env *e) {
    if (e->needsReset) {
        DEBUG_LOG("Resetting environment");
        resetEnv(e);
    }

    agentActions stepActions[e->numDrones];
    memset(stepActions, 0x0, e->numDrones * sizeof(agentActions));

    // preprocess agent actions for the next frameSkip steps
    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead || droneControlledByHuman(e, i)) {
            continue;
        }

        if (i < e->numAgents) {
            stepActions[i] = computeActions(e, drone, NULL);
        } else {
            const agentActions scriptedActions = scriptedAgentActions(e, drone);
            stepActions[i] = computeActions(e, drone, &scriptedActions);
        }
    }

    // reset reward buffer
    memset(e->rewards, 0x0, e->numAgents * sizeof(float));

    for (int i = 0; i < e->frameSkip; i++) {
        e->episodeLength++;

        // handle actions
        if (e->client != NULL) {
            updateHumanInputToggle(e);
        }

        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            memset(&drone->stepInfo, 0x0, sizeof(droneStepInfo));
            if (drone->dead) {
                drone->diedThisStep = false;
            }
        }

        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            if (drone->dead) {
                continue;
            }

            agentActions actions;
            // take inputs from humans every frame
            if (droneControlledByHuman(e, i)) {
                actions = getPlayerInputs(e, drone, i - e->humanDroneInput);
            } else {
                actions = stepActions[i];
            }

            if (actions.chargingBurst) {
                droneChargeBurst(e, drone);
            }
            if (actions.burst) {
                droneBurst(e, drone);
            }
            if (!b2VecEqual(actions.move, b2Vec2_zero)) {
                droneMove(drone, actions.move);
            }
            droneBrake(e, drone, actions.brake);
            if (actions.shoot) {
                droneShoot(e, drone, actions.aim, actions.chargingWeapon);
            }
            if (actions.discardWeapon) {
                droneDiscardWeapon(e, drone);
            }
        }

        b2World_Step(e->worldID, e->deltaTime, e->box2dSubSteps);

        // update dynamic body positions and velocities
        handleBodyMoveEvents(e);

        // handle collisions
        handleContactEvents(e);
        handleSensorEvents(e);

        // handle sudden death
        e->stepsLeft = fmaxf(e->stepsLeft - 1, 0.0f);
        if (e->numDrones == e->numAgents && e->stepsLeft == 0) {
            e->suddenDeathSteps = fmaxf(e->suddenDeathSteps - 1, 0.0f);
            if (e->suddenDeathSteps == 0) {
                DEBUG_LOG("placing sudden death walls");
                handleSuddenDeath(e);
                e->suddenDeathSteps = e->totalSuddenDeathSteps;
            }
        }

        projectilesStep(e);

        int8_t lastAlive = -1;
        int8_t lastAliveTeam = -1;
        bool allAliveOnSameTeam = false;
        uint8_t deadDrones = 0;
        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            if (!drone->dead) {
                droneStep(e, drone);
                lastAlive = i;

                if (e->teamsEnabled) {
                    if (lastAliveTeam == -1) {
                        lastAliveTeam = drone->team;
                        allAliveOnSameTeam = true;
                    } else if (drone->team != lastAliveTeam) {
                        allAliveOnSameTeam = false;
                    }
                }
            } else {
                deadDrones++;
                if (i < e->numAgents) {
                    e->masks[i] = 0;
                    e->terminals[i] = 1;
                }
            }
        }

        weaponPickupsStep(e);

        bool roundOver = deadDrones >= e->numDrones - 1;
        if (e->teamsEnabled && allAliveOnSameTeam) {
            roundOver = true;
            lastAlive = -1;
        }
        // if the enemy drone(s) are scripted don't enable sudden death
        // so that the agent has to work for victories
        if (e->numDrones != e->numAgents && e->stepsLeft == 0) {
            roundOver = true;
        }
        if (roundOver && deadDrones < e->numDrones - 1) {
            lastAlive = -1;
        }
        computeRewards(e, roundOver, lastAlive, lastAliveTeam);

        if (e->client != NULL) {
            renderEnv(e, false, roundOver, lastAlive, lastAliveTeam);
        }

        if (roundOver) {
            if (e->numDrones != e->numAgents && e->stepsLeft == 0) {
                DEBUG_LOG("truncating episode");
                memset(e->truncations, 1, e->numAgents * sizeof(uint8_t));
            } else {
                DEBUG_LOG("terminating episode");
                memset(e->terminals, 1, e->numAgents * sizeof(uint8_t));
            }

            logEntry log = {0};
            log.length = e->episodeLength;
            if (lastAlive != -1) {
                e->stats[lastAlive].wins = 1.0f;
            } else if (!e->teamsEnabled || (e->teamsEnabled && lastAliveTeam == -1)) {
                log.ties = 1.0f;
            }

            for (uint8_t i = 0; i < e->numDrones; i++) {
                const droneEntity *drone = safe_array_get_at(e->drones, i);
                if (!drone->dead && e->teamsEnabled && drone->team == lastAliveTeam) {
                    e->stats[i].wins = 1.0f;
                }
                // set absolute distance traveled of agent drones
                e->stats[i].absDistanceTraveled = b2Distance(drone->initalPos, drone->pos);
            }

            memcpy(log.stats, e->stats, sizeof(e->stats));
            addLogEntry(e->logs, &log);

            e->needsReset = true;
            break;
        }
    }

#ifndef NDEBUG
    bool gotReward = false;
    for (uint8_t i = 0; i < e->numDrones; i++) {
        if (e->rewards[i] > REWARD_EPS || e->rewards[i] < -REWARD_EPS) {
            gotReward = true;
            break;
        }
    }
    if (gotReward) {
        DEBUG_RAW_LOG("rewards: [");
        for (uint8_t i = 0; i < e->numDrones; i++) {
            const float reward = e->rewards[i];
            DEBUG_RAW_LOGF("%f", reward);
            if (i < e->numDrones - 1) {
                DEBUG_RAW_LOG(", ");
            }
        }
        DEBUG_RAW_LOGF("] step %d\n", e->totalSteps - e->stepsLeft);
    }
#endif

    computeObs(e);
}

#endif
