#ifndef IMPULSE_WARS_ENV_H
#define IMPULSE_WARS_ENV_H

#include "game.h"
#include "map.h"
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
#endif

const uint8_t TWO_BIT_MASK = 0x3;
const uint8_t THREE_BIT_MASK = 0x7;
const uint8_t FOUR_BIT_MASK = 0xf;

logBuffer *createLogBuffer(uint16_t capacity) {
    logBuffer *logs = (logBuffer *)fastCalloc(1, sizeof(logBuffer));
    logs->logs = (logEntry *)fastCalloc(capacity, sizeof(logEntry));
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
            }
        }
    }

    logs->size = 0;
    return log;
}

// computes a grid of the types and locations of static walls in the map;
// since walls never move, this only needs to be computed once per episode
void computeWallGridObs(env *e) {
    memset(e->wallObs, 0x0, MAP_OBS_SIZE * sizeof(uint8_t));

    // TODO: needs to be padded for smaller maps then max size
    uint16_t offset = 0;
    for (size_t i = 0; i < cc_array_size(e->cells); i++) {
        const mapCell *cell = safe_array_get_at(e->cells, i);
        uint8_t wallType = 0;
        if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
            wallType = cell->ent->type + 1;
        }
        e->wallObs[offset++] = (wallType & TWO_BIT_MASK) << 4;

        ASSERT(i <= MAX_MAP_COLUMNS * MAX_MAP_ROWS);
        ASSERT(offset <= MAP_OBS_SIZE);
    }
}

uint16_t findNearestCell(env *e, const b2Vec2 pos, const uint16_t cellIdx) {
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
    const uint8_t cellRow = cellIdx / e->columns;
    const uint8_t cellCol = cellIdx % e->columns;
    for (uint8_t i = 0; i < 8; i++) {
        const uint16_t newCellIdx = ((cellRow + cellOffsets[i][0]) * e->columns) + (cellCol + cellOffsets[i][1]);
        const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        if (minDistance != fminf(minDistance, b2Distance(pos, cell->pos))) {
            closestCell = newCellIdx;
        }
    }

    return closestCell;
}

void computeObs(env *e) {
    for (uint8_t agent = 0; agent < e->numAgents; agent++) {
        //
        // compute discrete map observations
        //
        uint16_t mapObsOffset = e->obsBytes * agent;
        const uint16_t mapObsStart = mapObsOffset;

        memcpy(e->obs + mapObsOffset, e->wallObs, MAP_OBS_SIZE * sizeof(uint8_t));

        // compute discretized location and type of weapon pickups on grid
        for (size_t i = 0; i < cc_array_size(e->pickups); i++) {
            const weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);

            mapObsOffset = mapObsStart + pickup->mapCellIdx;
            ASSERTF(mapObsOffset <= mapObsStart + MAP_OBS_SIZE, "offset: %d", mapObsOffset);
            e->obs[mapObsOffset] |= 1 << 3;
        }

        // compute discretized location and index of drones on grid
        uint8_t newDroneIdx = 1;
        uint16_t droneCells[e->numDrones];
        memset(droneCells, 0x0, sizeof(droneCells));
        for (size_t i = 0; i < cc_array_size(e->drones); i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            const b2Vec2 pos = getCachedPos(drone->bodyID, &drone->pos);

            // ensure drones do not share cells in the observation
            int16_t cellIdx = entityPosToCellIdx(e, pos);
            if (cellIdx == -1) {
                ERRORF("drone %zu out of bounds at position %f %f", i, pos.x, pos.y);
            }
            if (i != 0) {
                for (uint8_t j = 0; j < i; j++) {
                    if (droneCells[j] == cellIdx) {
                        cellIdx = findNearestCell(e, pos, cellIdx);
                        break;
                    }
                }
            }
            droneCells[i] = cellIdx;

            // set the agent's drone to be drone 0
            uint8_t droneIdx = 0;
            if (i != agent) {
                droneIdx = newDroneIdx++;
            }
            mapObsOffset = mapObsStart + cellIdx;
            ASSERTF(mapObsOffset <= mapObsStart + MAP_OBS_SIZE, "offset: %d", mapObsOffset);
            e->obs[mapObsOffset] |= ((droneIdx + 1) & THREE_BIT_MASK);
        }

        //
        // compute continuous scalar observations
        //
        uint16_t scalarObsOffset = 0;
        const uint16_t scalarObsStart = mapObsStart + e->mapObsBytes;
        float *scalarObs = (float *)(e->obs + scalarObsStart);
        memset(scalarObs, 0x0, SCALAR_OBS_SIZE * sizeof(float));

        const droneEntity *agentDrone = safe_array_get_at(e->drones, agent);
        const b2Vec2 agentDronePos = agentDrone->pos.pos;

        // compute type and location of N nearest weapon pickups
        // TODO: use KD tree here
        for (uint8_t i = 0; i < NUM_WEAPON_PICKUP_OBS; i++) {
            const weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);

            scalarObsOffset = WEAPON_PICKUP_TYPES_OBS_OFFSET + i;
            ASSERTF(scalarObsOffset <= WEAPON_PICKUP_POS_OBS_OFFSET, "offset: %d", scalarObsOffset);
            scalarObs[scalarObsOffset] = pickup->weapon + 1;

            scalarObsOffset = WEAPON_PICKUP_POS_OBS_OFFSET + (i * 2);
            ASSERTF(scalarObsOffset <= PROJECTILE_TYPES_OBS_OFFSET, "offset: %d", scalarObsOffset);
            const b2Vec2 pickupPos = b2Sub(pickup->pos, agentDronePos);
            scalarObs[scalarObsOffset++] = scaleValue(pickupPos.x, MAX_X_POS, false);
            scalarObs[scalarObsOffset] = scaleValue(pickupPos.y, MAX_Y_POS, false);
        }

        // compute type and location of N projectiles
        uint8_t projIdx = 0;
        for (SNode *cur = e->projectiles->head; cur != NULL; cur = cur->next) {
            // TODO: handle better
            if (projIdx == NUM_PROJECTILE_OBS) {
                break;
            }
            const projectileEntity *projectile = (projectileEntity *)cur->data;

            scalarObsOffset = PROJECTILE_TYPES_OBS_OFFSET + projIdx;
            ASSERTF(scalarObsOffset <= PROJECTILE_POS_OBS_OFFSET, "offset: %d", scalarObsOffset);
            scalarObs[scalarObsOffset] = projectile->weaponInfo->type + 1;

            scalarObsOffset = PROJECTILE_POS_OBS_OFFSET + (projIdx * 2);
            ASSERTF(scalarObsOffset <= ENEMY_DRONE_OBS_OFFSET, "offset: %d", scalarObsOffset);
            const b2Vec2 projectilePos = b2Sub(projectile->lastPos, agentDronePos);
            scalarObs[scalarObsOffset++] = scaleValue(projectilePos.x, MAX_X_POS, false);
            scalarObs[scalarObsOffset] = scaleValue(projectilePos.y, MAX_Y_POS, false);

            projIdx++;
        }

        // compute enemy drone observations
        // TODO: handle multiple enemy drones
        const droneEntity *enemyDrone = safe_array_get_at(e->drones, 1);
        const b2Vec2 enemyDronePos = b2Sub(enemyDrone->pos.pos, agentDronePos);
        const b2Vec2 enemyDroneVel = b2Body_GetLinearVelocity(enemyDrone->bodyID);
        scalarObsOffset = ENEMY_DRONE_OBS_OFFSET;
        scalarObs[scalarObsOffset++] = scaleValue(enemyDronePos.x, MAX_X_POS, false);
        scalarObs[scalarObsOffset++] = scaleValue(enemyDronePos.y, MAX_Y_POS, false);
        scalarObs[scalarObsOffset++] = scaleValue(enemyDroneVel.x, MAX_SPEED, false);
        scalarObs[scalarObsOffset++] = scaleValue(enemyDroneVel.y, MAX_SPEED, false);

        // compute active drone observations
        ASSERTF(scalarObsOffset == DRONE_OBS_OFFSET, "offset: %d", scalarObsOffset);
        const b2Vec2 agentDroneVel = b2Body_GetLinearVelocity(agentDrone->bodyID);
        int8_t maxAmmo = weaponAmmo(e->defaultWeapon->type, agentDrone->weaponInfo->type);
        uint8_t scaledAmmo = 0;
        if (agentDrone->ammo != INFINITE) {
            scaledAmmo = scaleValue(agentDrone->ammo, maxAmmo, true);
        }

        scalarObs[scalarObsOffset++] = scaleValue(e->stepsLeft, ROUND_STEPS, true);
        scalarObs[scalarObsOffset++] = scaleValue(agentDronePos.x, MAX_X_POS, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDronePos.y, MAX_Y_POS, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDroneVel.x, MAX_SPEED, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDroneVel.y, MAX_SPEED, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->lastAim.x, 1.0f, false);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->lastAim.y, 1.0f, false);
        scalarObs[scalarObsOffset++] = scaledAmmo;
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->weaponCooldown, agentDrone->weaponInfo->coolDown, true);
        scalarObs[scalarObsOffset++] = scaleValue(agentDrone->charge, weaponCharge(agentDrone->weaponInfo->type), true);
        scalarObs[scalarObsOffset] = agentDrone->weaponInfo->type + 1;
    }
}

void setupEnv(env *e) {
    e->needsReset = false;

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){.x = 0.0f, .y = 0.0f};
    e->worldID = b2CreateWorld(&worldDef);

    e->stepsLeft = ROUND_STEPS;
    e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
    e->suddenDeathWallCounter = 0;

    DEBUG_LOG("creating map");
    const int mapIdx = 0; // randInt(&e->randState, 0, NUM_MAPS - 1);
    createMap(e, mapIdx);

    mapBounds bounds = {.min = {.x = FLT_MAX, .y = FLT_MAX}, .max = {.x = FLT_MIN, .y = FLT_MIN}};
    for (size_t i = 0; i < cc_array_size(e->walls); i++) {
        const wallEntity *wall = safe_array_get_at(e->walls, i);
        bounds.min.x = fminf(wall->pos.pos.x - wall->extent.x + WALL_THICKNESS, bounds.min.x);
        bounds.min.y = fminf(wall->pos.pos.y - wall->extent.y + WALL_THICKNESS, bounds.min.y);
        bounds.max.x = fmaxf(wall->pos.pos.x + wall->extent.x - WALL_THICKNESS, bounds.max.x);
        bounds.max.y = fmaxf(wall->pos.pos.y + wall->extent.y - WALL_THICKNESS, bounds.max.y);
    }
    e->bounds = bounds;

    DEBUG_LOG("creating drones");
    for (int i = 0; i < e->numDrones; i++) {
        createDrone(e, i);
    }

    DEBUG_LOG("placing floating walls");
    placeRandFloatingWalls(e, mapIdx);

    DEBUG_LOG("creating weapon pickups");
    for (int i = 0; i < maps[mapIdx]->weaponPickups; i++) {
        createWeaponPickup(e);
    }

    computeWallGridObs(e);
    computeObs(e);
}

env *initEnv(env *e, uint8_t numDrones, uint8_t numAgents, uint8_t *obs, bool discretizeActions, float *contActions, int32_t *discActions, float *rewards, uint8_t *terminals, logBuffer *logs, uint64_t seed) {
    e->numDrones = numDrones;
    e->numAgents = numAgents;

    e->obsBytes = obsBytes();
    e->mapObsBytes = alignedSize(MAP_OBS_SIZE * sizeof(uint8_t), sizeof(float));

    e->obs = obs;
    e->discretizeActions = discretizeActions;
    e->contActions = contActions;
    e->discActions = discActions;
    e->rewards = rewards;
    e->terminals = terminals;

    e->randState = seed;
    e->needsReset = false;

    e->logs = logs;

    e->wallObs = fastCalloc(MAP_OBS_SIZE, sizeof(uint8_t));
    cc_array_new(&e->cells);
    cc_array_new(&e->walls);
    // e->wallTree = kd_create(2);
    cc_array_new(&e->floatingWalls);
    cc_array_new(&e->drones);
    cc_array_new(&e->pickups);
    // e->pickupTree = kd_create(2);
    cc_slist_new(&e->projectiles);

    setupEnv(e);

    return e;
}

void clearEnv(env *e) {
    memset(e->wallObs, 0x0, MAP_OBS_SIZE * sizeof(uint8_t));

    // rewards get cleared in stepEnv every step
    memset(e->terminals, 0x0, e->numAgents * sizeof(uint8_t));

    e->episodeLength = 0;
    memset(e->stats, 0x0, sizeof(e->stats));

    for (size_t i = 0; i < cc_array_size(e->pickups); i++) {
        weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
        destroyWeaponPickup(e, pickup, false);
    }

    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        destroyDrone(drone);
    }

    destroyAllProjectiles(e);

    for (size_t i = 0; i < cc_array_size(e->walls); i++) {
        wallEntity *wall = safe_array_get_at(e->walls, i);
        destroyWall(wall);
    }

    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
        wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        destroyWall(wall);
    }

    for (size_t i = 0; i < cc_array_size(e->cells); i++) {
        mapCell *cell = safe_array_get_at(e->cells, i);
        fastFree(cell);
    }

    cc_array_remove_all(e->cells);
    cc_array_remove_all(e->walls);
    // kd_clear(e->wallTree);
    cc_array_remove_all(e->floatingWalls);
    cc_array_remove_all(e->drones);
    cc_array_remove_all(e->pickups);
    // kd_clear(e->pickupTree);
    cc_slist_remove_all(e->projectiles);

    b2DestroyWorld(e->worldID);
}

void destroyEnv(env *e) {
    clearEnv(e);

    fastFree(e->wallObs);
    cc_array_destroy(e->cells);
    cc_array_destroy(e->walls);
    free(e->wallTree);
    cc_array_destroy(e->floatingWalls);
    cc_array_destroy(e->drones);
    cc_array_destroy(e->pickups);
    free(e->pickupTree);
    cc_slist_destroy(e->projectiles);
}

void resetEnv(env *e) {
    clearEnv(e);
    setupEnv(e);
}

float computeShotHitReward(env *e, const uint8_t enemyIdx) {
    // compute reward based off of how much the projectile(s) or explosion(s)
    // caused the enemy drone to change velocity
    const droneEntity *enemyDrone = safe_array_get_at(e->drones, enemyIdx);
    const float prevEnemySpeed = b2Length(enemyDrone->lastVelocity);
    const float curEnemySpeed = b2Length(b2Body_GetLinearVelocity(enemyDrone->bodyID));
    return scaleValue(fabsf(curEnemySpeed - prevEnemySpeed), MAX_SPEED, true) * SHOT_HIT_REWARD_COEF;
}

float computeReward(env *e, const droneEntity *drone) {
    float reward = 0.0f;
    if (drone->dead) {
        reward += DEATH_PUNISHMENT;
    }
    // TODO: compute kill reward
    for (uint8_t i = 0; i < e->numDrones; i++) {
        if (i == drone->idx) {
            continue;
        }
        if (drone->stepInfo.pickedUpWeapon) {
            reward += WEAPON_PICKUP_REWARD;
        }
        if (drone->stepInfo.shotHit[i] || drone->stepInfo.explosionHit[i]) {
            reward += computeShotHitReward(e, i);
        }

        const droneEntity *enemyDrone = safe_array_get_at(e->drones, i);
        const b2Vec2 directionVec = b2Normalize(b2Sub(enemyDrone->pos.pos, drone->pos.pos));
        const float aimDot = b2Dot(drone->lastAim, directionVec);
        const float distance = b2Distance(drone->pos.pos, enemyDrone->pos.pos);
        const float aimThreshold = cosf(atanf(AIM_TOLERANCE / distance));
        if (aimDot >= aimThreshold) {
            reward += AIM_REWARD;
            if (drone->stepInfo.firedShot) {
                reward += AIMED_SHOT_REWARD;
            }
        }
    }

    return reward;
}

void computeRewards(env *e, const bool roundOver, const uint8_t winner) {
    if (roundOver) {
        if (winner < e->numAgents) {
            e->rewards[winner] += WIN_REWARD;
        }
        e->stats[winner].reward += WIN_REWARD;
    }

    for (int i = 0; i < e->numDrones; i++) {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        const float reward = computeReward(e, drone);
        if (i < e->numAgents) {
            e->rewards[i] += reward;
        }
        e->stats[i].reward += reward;

        if (reward != 0.0f) {
            DEBUG_LOGF("step: %f drone: %d reward: %f", ROUND_STEPS - e->stepsLeft, i, reward);
        }
    }
}

typedef struct agentActions {
    b2Vec2 move;
    b2Vec2 aim;
    bool shoot;
} agentActions;

static inline bool isActionNoop(const b2Vec2 action) {
    return b2Length(action) < ACTION_NOOP_MAGNITUDE;
}

agentActions computeActions(env *e, const uint8_t droneIdx) {
    agentActions actions = {0};

    if (e->discretizeActions) {
        const uint8_t offset = droneIdx * DISCRETE_ACTION_SIZE;

        // 8 is no-op for both move and aim
        const uint8_t move = e->discActions[offset + 0];
        ASSERT(move <= 8);
        if (move != 8) {
            actions.move.x = discToContActionMap[0][move];
            actions.move.y = discToContActionMap[1][move];
        }
        const uint8_t aim = e->discActions[offset + 1];
        ASSERT(move <= 8);
        if (aim != 8) {
            actions.aim.x = discToContActionMap[0][aim];
            actions.aim.y = discToContActionMap[1][aim];
        }
        const uint8_t shoot = e->discActions[offset + 2];
        ASSERT(shoot <= 1);
        actions.shoot = (bool)shoot;
        return actions;
    }
    const uint8_t offset = droneIdx * CONTINUOUS_ACTION_SIZE;

    b2Vec2 move = (b2Vec2){.x = tanhf(e->contActions[offset + 0]), .y = tanhf(e->contActions[offset + 1])};
    ASSERT_VEC_BOUNDED(move);
    // cap movement magnitude to 1.0
    if (b2Length(move) > 1.0f) {
        move = b2Normalize(move);
    } else if (isActionNoop(move)) {
        move = b2Vec2_zero;
    }
    actions.move = move;

    const b2Vec2 rawAim = (b2Vec2){.x = tanhf(e->contActions[offset + 2]), .y = tanhf(e->contActions[offset + 3])};
    ASSERT_VEC_BOUNDED(rawAim);
    b2Vec2 aim;
    if (isActionNoop(rawAim)) {
        aim = b2Vec2_zero;
    } else {
        aim = b2Normalize(rawAim);
    }
    actions.aim = aim;
    actions.shoot = e->contActions[offset + 4] > 0.0f;

    return actions;
}

void stepEnv(env *e) {
    if (e->needsReset) {
        DEBUG_LOG("Resetting environment");
        resetEnv(e);
    }

    agentActions stepActions[e->numAgents];
    memset(stepActions, 0x0, e->numAgents * sizeof(agentActions));

    // handle actions
    // TODO: don't use tanh on human actions
    for (uint8_t i = 0; i < e->numAgents; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);

        stepActions[i] = computeActions(e, i);
        drone->lastMove = stepActions[i].move;
        if (!b2VecEqual(stepActions[i].aim, b2Vec2_zero)) {
            drone->lastAim = stepActions[i].aim;
        }
    }

    // reset reward buffer
    memset(e->rewards, 0x0, e->numAgents * sizeof(float));

    for (int i = 0; i < FRAMESKIP; i++) {
        e->episodeLength++;

        // handle actions
        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            drone->lastVelocity = b2Body_GetLinearVelocity(drone->bodyID);
            memset(&drone->stepInfo, 0x0, sizeof(droneStepInfo));
            if (i >= e->numAgents) {
                break;
            }

            const agentActions actions = stepActions[i];
            if (!b2VecEqual(actions.move, b2Vec2_zero)) {
                droneMove(drone, actions.move);
            }
            if (actions.shoot) {
                droneShoot(e, drone, actions.aim);
            }
        }

        // update entity info, step physics, and handle events
        b2World_Step(e->worldID, DELTA_TIME, BOX2D_SUBSTEPS);

        // mark old positions as invalid now that physics has been stepped
        // projectiles will have their positions correctly updated in projectilesStep
        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            drone->pos.valid = false;
        }
        for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
            wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
            wall->pos.valid = false;
        }

        // TODO: fix
        // handle sudden death
        e->stepsLeft = fmaxf(e->stepsLeft - 1, 0.0f);
        // if (e->stepsLeft == 0) {
        //     e->suddenDeathSteps = fmaxf(e->suddenDeathSteps - 1, 0.0f);
        //     if (e->suddenDeathSteps == 0) {
        //         DEBUG_LOG("placing sudden death walls");
        //         handleSuddenDeath(e);
        //         e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
        //     }
        // }

        projectilesStep(e);

        handleContactEvents(e);
        handleSensorEvents(e);

        uint8_t lastAlive = 0;
        uint8_t deadDrones = 0;
        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            droneStep(e, drone, DELTA_TIME);
            if (drone->dead) {
                deadDrones++;
                e->terminals[i] = 1;
            } else {
                lastAlive = i;
            }
        }

        weaponPickupsStep(e, DELTA_TIME);

        const bool roundOver = deadDrones >= e->numDrones - 1 || e->stepsLeft == 0;
        computeRewards(e, roundOver, lastAlive);

        if (e->client != NULL) {
            renderEnv(e);
        }

        if (roundOver) {
            memset(e->terminals, 1, e->numAgents * sizeof(uint8_t));

            e->stats[lastAlive].wins = 1.0f;

            // set absolute distance traveled of agent drones
            for (uint8_t i = 0; i < e->numDrones; i++) {
                const droneEntity *drone = safe_array_get_at(e->drones, i);
                e->stats[i].absDistanceTraveled = b2Distance(drone->initalPos, drone->pos.pos);
            }

            // add existing projectile distances to stats
            for (SNode *cur = e->projectiles->head; cur != NULL; cur = cur->next) {
                const projectileEntity *projectile = (projectileEntity *)cur->data;
                e->stats[projectile->droneIdx].shotDistances[projectile->weaponInfo->type] += projectile->distance;
            }

            logEntry log = {0};
            log.length = e->episodeLength;
            memcpy(log.stats, e->stats, sizeof(e->stats));
            addLogEntry(e->logs, &log);

            e->needsReset = true;
            break;
        }
    }

    computeObs(e);
}

bool envTerminated(env *e) {
    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            return true;
        }
    }
    return false;
}

#endif
