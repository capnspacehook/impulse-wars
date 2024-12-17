from libc.stdint cimport uint64_t
from libc.stdlib cimport calloc, free

import pufferlib

from impulse_wars cimport (
    NUM_DRONES,
    OBS_SIZE,
    ACTION_SIZE,
    OBS_HIGH,
    NUM_WEAPONS,
    NUM_WALL_TYPES,
    NUM_ENTITY_TYPES,
    DRONE_OBS_SIZE,
    NUM_PROJECTILE_OBS,
    PROJECTILE_OBS_SIZE,
    NUM_FLOATING_WALL_OBS,
    FLOATING_WALL_OBS_SIZE,
    MAX_MAP_COLUMNS,
    MAX_MAP_ROWS,
    env,
    initEnv,
    resetEnv,
    stepEnv,
    destroyEnv,
)


# doesn't seem like you an directly import C or Cython constants 
# from Python so we have to create wrapper functions

def numDrones() -> int:
    return NUM_DRONES

def obsSize() -> int:
    return OBS_SIZE

def actionsSize() -> int:
    return ACTION_SIZE

def obsHigh() -> float:
    return OBS_HIGH

def obsConstants() -> pufferlib.Namespace:
    weaponTypes = NUM_WEAPONS
    wallTypes = NUM_WALL_TYPES
    mapCellTypes = NUM_ENTITY_TYPES + NUM_WEAPONS
    droneObsSize = DRONE_OBS_SIZE
    numProjectileObs = NUM_PROJECTILE_OBS
    projectileObsSize = PROJECTILE_OBS_SIZE
    numFloatingWallObs = NUM_FLOATING_WALL_OBS
    floatingWallObsSize = FLOATING_WALL_OBS_SIZE
    maxMapColumns = MAX_MAP_COLUMNS
    maxMapRows = MAX_MAP_ROWS
    return pufferlib.Namespace(
        weaponTypes=NUM_WEAPONS,
        wallTypes=NUM_WALL_TYPES,
        mapCellTypes=NUM_ENTITY_TYPES + NUM_WEAPONS + 1,
        droneObsSize=DRONE_OBS_SIZE,
        numProjectileObs=NUM_PROJECTILE_OBS,
        projectileObsSize=PROJECTILE_OBS_SIZE,
        numFloatingWallObs=NUM_FLOATING_WALL_OBS,
        floatingWallObsSize=FLOATING_WALL_OBS_SIZE,
        maxMapColumns=MAX_MAP_COLUMNS,
        maxMapRows=MAX_MAP_ROWS,
    )


cdef class CyImpulseWars:
    cdef:
        env* envs
        int numEnvs

    def __init__(self, float[:, :] observations, float[:, :] actions, float[:] rewards, unsigned char[:] terminals, unsigned int numEnvs, uint64_t seed):
        self.envs = <env*>calloc(numEnvs, sizeof(env))
        self.numEnvs = numEnvs

        cdef int inc = NUM_DRONES
        cdef int i
        for i in range(self.numEnvs):
            initEnv(
                &self.envs[i],
                &observations[i * inc, 0],
                &actions[i * inc, 0],
                &rewards[i * inc],
                &terminals[i * inc],
                seed + i,
            )

    def reset(self):
        cdef int i
        for i in range(self.numEnvs):
            resetEnv(&self.envs[i])

    def step(self):
        cdef int i
        for i in range(self.numEnvs):
            stepEnv(&self.envs[i])

    def close(self):
        cdef int i
        for i in range(self.numEnvs):
            destroyEnv(&self.envs[i])

        free(self.envs)