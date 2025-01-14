#ifndef IMPULSE_WARS_SCRIPTED_BOT_H
#define IMPULSE_WARS_SCRIPTED_BOT_H

#include "game.h"
#include "types.h"

const float WALL_DANGER_DISTANCE = 10.0f;

float recoilDistance(const env *e, const droneEntity *drone, const b2Vec2 vel, const float steps) {
    float initialSpeed = drone->weaponInfo->recoilMagnitude;
    if (!b2VecEqual(vel, b2Vec2_zero)) {
        initialSpeed = b2Length(b2MulSV(drone->weaponInfo->recoilMagnitude, vel));
    }
    initialSpeed += DRONE_MOVE_MAGNITUDE;

    const float damping = 1.0f + DRONE_LINEAR_DAMPING * e->deltaTime;
    return initialSpeed * (damping / DRONE_LINEAR_DAMPING) * (1.0f - powf(1.0f / (damping), steps));
}

// TODO: pathfind to agent
// TODO: switch between personalities when episode begins:
// - aggressive
// - defensive
// - lazy
agentActions scriptedBotActions(const env *e, droneEntity *drone) {
    agentActions actions = {0};
    if (e->sittingDuck) {
        return actions;
    }

    const b2Vec2 pos = getCachedPos(drone->bodyID, &drone->pos);

    // TODO: use cells to find nearest walls
    kdres *res = kd_nearest(e->wallTree, pos.x, pos.y);
    b2Vec2 nearestWallPos;
    kd_res_item(res, &nearestWallPos.x, &nearestWallPos.y);
    kd_res_free(res);
    float nearestWallDistance = b2Distance(nearestWallPos, pos);

    for (uint8_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
        wallEntity *floatingWall = safe_array_get_at(e->floatingWalls, i);
        if (floatingWall->type != DEATH_WALL_ENTITY) {
            continue;
        }
        const b2Vec2 floatingWallPos = getCachedPos(floatingWall->bodyID, &floatingWall->pos);
        const float floatingWallDistance = b2Distance(floatingWallPos, pos);
        if (floatingWallDistance < nearestWallDistance) {
            nearestWallPos = floatingWallPos;
            nearestWallDistance = floatingWallDistance;
        }
    }

    const b2Vec2 vel = b2Body_GetLinearVelocity(drone->bodyID);
    const b2Vec2 wallDirection = b2Normalize(b2Sub(nearestWallPos, pos));
    const b2Vec2 invWallDirection = b2MulSV(-1.0f, wallDirection);

    const float shotWait = (drone->weaponInfo->coolDown + (float)weaponCharge(e, drone->weaponInfo->type)) * e->frameRate;
    const float shotDistance = recoilDistance(e, drone, vel, shotWait + 3);

    if (nearestWallDistance <= WALL_DANGER_DISTANCE) {
        actions.move = invWallDirection;

        const b2Vec2 rayEnd = b2MulAdd(pos, shotDistance, invWallDirection);
        const b2Vec2 translation = b2Sub(rayEnd, pos);
        const b2QueryFilter filter = {.categoryBits = DRONE_SHAPE, .maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | DRONE_SHAPE};
        const b2RayResult res = b2World_CastRayClosest(e->worldID, pos, translation, filter);
        if (!res.hit) {
            actions.shoot = true;
        } else {
            ASSERT(b2Shape_IsValid(res.shapeId));
            const entity *ent = b2Shape_GetUserData(res.shapeId);
            if (ent->type == DRONE_ENTITY || ent->type == STANDARD_WALL_ENTITY) {
                actions.shoot = true;
            }
        }
        if (actions.shoot) {
            actions.aim = wallDirection;
        }
        return actions;
    }

    // TODO: only fire if recoil won't kill
    if (drone->inLineOfSight[0]) {
        droneEntity *agentDrone = (droneEntity *)safe_array_get_at(e->drones, 0);
        const b2Vec2 agentDronePos = getCachedPos(agentDrone->bodyID, &agentDrone->pos);

        const b2Vec2 agentDroneRelNormPos = b2Normalize(b2Sub(agentDronePos, pos));
        actions.move = agentDroneRelNormPos;
        actions.aim = agentDroneRelNormPos;
        actions.shoot = true;
    }

    return actions;
}

#endif
