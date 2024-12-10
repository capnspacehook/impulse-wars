#pragma once

#include "game.h"
#include "map.h"
#include "types.h"

env *createEnv(void)
{
    env *e = calloc(1, sizeof(env));
    return e;
}

void setupEnv(env *e)
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){.x = 0.0f, .y = 0.0f};
    e->worldID = b2CreateWorld(&worldDef);

    cc_deque_new(&e->cells);
    cc_deque_new(&e->walls);
    cc_deque_new(&e->entities);
    cc_deque_new(&e->drones);
    cc_deque_new(&e->pickups);
    cc_slist_new(&e->projectiles);

    e->stepsLeft = ROUND_STEPS;
    e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
    e->suddenDeathWallCounter = 0;

    createMap(e, "prototype_arena.txt");

    mapBounds bounds = {.min = {.x = FLT_MAX, .y = FLT_MAX}, .max = {.x = FLT_MIN, .y = FLT_MIN}};
    for (size_t i = 0; i < cc_deque_size(e->walls); i++)
    {
        wallEntity *wall;
        cc_deque_get_at(e->walls, i, (void **)&wall);
        bounds.min.x = fminf(wall->position.x - wall->extent.x + WALL_THICKNESS, bounds.min.x);
        bounds.min.y = fminf(wall->position.y - wall->extent.y + WALL_THICKNESS, bounds.min.y);
        bounds.max.x = fmaxf(wall->position.x + wall->extent.x - WALL_THICKNESS, bounds.max.x);
        bounds.max.y = fmaxf(wall->position.y + wall->extent.y - WALL_THICKNESS, bounds.max.y);
    }
    e->bounds = bounds;

    for (int i = 0; i < NUM_DRONES; i++)
    {
        createDrone(e);
    }

    for (int i = 0; i < 2; i++)
    {
        createWeaponPickup(e, MACHINEGUN_WEAPON);
        createWeaponPickup(e, SNIPER_WEAPON);
        createWeaponPickup(e, SHOTGUN_WEAPON);
        createWeaponPickup(e, IMPLODER_WEAPON);
    }
}

void clearEnv(env *e)
{
    for (size_t i = 0; i < cc_deque_size(e->pickups); i++)
    {
        weaponPickupEntity *pickup;
        cc_deque_get_at(e->pickups, i, (void **)&pickup);
        destroyWeaponPickup(pickup);
    }

    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone;
        cc_deque_get_at(e->drones, i, (void **)&drone);
        destroyDrone(drone);
    }

    destroyAllProjectiles(e);

    for (size_t i = 0; i < cc_deque_size(e->walls); i++)
    {
        wallEntity *wall;
        cc_deque_get_at(e->walls, i, (void **)&wall);
        destroyWall(wall);
    }

    for (size_t i = 0; i < cc_deque_size(e->cells); i++)
    {
        mapCell *cell;
        cc_deque_get_at(e->cells, i, (void **)&cell);
        free(cell);
    }

    cc_deque_destroy(e->cells);
    cc_deque_destroy(e->walls);
    cc_deque_destroy(e->entities);
    cc_deque_destroy(e->drones);
    cc_deque_destroy(e->pickups);
    cc_slist_destroy(e->projectiles);

    b2DestroyWorld(e->worldID);
}

void destroyEnv(env *e)
{
    clearEnv(e);
    free(e);
}

void resetEnv(env *e)
{
    clearEnv(e);
    setupEnv(e);
}

void stepEnv(env *e, float deltaTime)
{
    e->stepsLeft = fmaxf(e->stepsLeft - 1, 0.0f);
    if (e->stepsLeft == 0)
    {
        e->suddenDeathSteps = fmaxf(e->suddenDeathSteps - 1, 0.0f);
        if (e->suddenDeathSteps == 0)
        {
            handleSuddenDeath(e);
            e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
        }
    }

    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone;
        cc_deque_get_at(e->drones, i, (void **)&drone);
        droneStep(drone, deltaTime);
    }

    projectilesStep(e);

    for (size_t i = 0; i < cc_deque_size(e->pickups); i++)
    {
        weaponPickupEntity *pickup;
        cc_deque_get_at(e->pickups, i, (void **)&pickup);
        weaponPickupStep(e, pickup, deltaTime);
    }

    b2World_Step(e->worldID, deltaTime, 8);

    handleContactEvents(e);
    handleSensorEvents(e);
}

bool envTerminated(env *e)
{
    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone;
        cc_deque_get_at(e->drones, i, (void **)&drone);
        if (drone->dead)
        {
            return true;
        }
    }
    return false;
}