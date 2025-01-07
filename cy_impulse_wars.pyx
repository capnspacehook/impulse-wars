from libc.stdint cimport int32_t, uint8_t, uint16_t, uint64_t
from libc.stdlib cimport calloc, free

import pufferlib

from impulse_wars cimport (
    MAX_DRONES,
    CONTINUOUS_ACTION_SIZE,
    obsBytes,
    alignedSize,
    MAP_OBS_SIZE,
    SCALAR_OBS_SIZE,
    NUM_WALL_TYPES,
    NUM_WEAPONS,
    MAX_MAP_COLUMNS,
    MAX_MAP_ROWS,
    NUM_NEAR_WALL_OBS,
    NEAR_WALL_POS_OBS_SIZE,
    NEAR_WALL_OBS_SIZE,
    NEAR_WALL_TYPES_OBS_OFFSET,
    NEAR_WALL_POS_OBS_OFFSET,
    NUM_FLOATING_WALL_OBS,
    FLOATING_WALL_OBS_SIZE,
    FLOATING_WALL_INFO_OBS_SIZE,
    FLOATING_WALL_TYPES_OBS_OFFSET,
    FLOATING_WALL_INFO_OBS_OFFSET,
    NUM_WEAPON_PICKUP_OBS,
    WEAPON_PICKUP_POS_OBS_SIZE,
    WEAPON_PICKUP_OBS_SIZE,
    WEAPON_PICKUP_TYPES_OBS_OFFSET,
    WEAPON_PICKUP_POS_OBS_OFFSET,
    NUM_PROJECTILE_OBS,
    PROJECTILE_POS_OBS_SIZE,
    PROJECTILE_OBS_SIZE,
    PROJECTILE_TYPES_OBS_OFFSET,
    PROJECTILE_POS_OBS_OFFSET,
    ENEMY_DRONE_OBS_OFFSET,
    ENEMY_DRONE_OBS_SIZE,
    DRONE_OBS_OFFSET,
    DRONE_OBS_SIZE,
    env,
    initEnv,
    rayClient,
    createRayClient,
    destroyRayClient,
    resetEnv,
    stepEnv,
    destroyEnv,
    LOG_BUFFER_SIZE,
    logBuffer,
    logEntry,
    createLogBuffer,
    destroyLogBuffer,
    aggregateAndClearLogBuffer,
)


# doesn't seem like you can directly import C or Cython constants 
# from Python so we have to create wrapper functions

def maxDrones() -> int:
    return MAX_DRONES


def continuousActionsSize() -> int:
    return CONTINUOUS_ACTION_SIZE


def obsConstants(numDrones: int) -> pufferlib.Namespace:
    return pufferlib.Namespace(
        obsBytes=obsBytes(),
        mapObsSize=MAP_OBS_SIZE,
        scalarObsSize=SCALAR_OBS_SIZE,
        scalarObsBytes=SCALAR_OBS_SIZE * sizeof(float),
        wallTypes=NUM_WALL_TYPES,
        weaponTypes=NUM_WEAPONS + 1,
        maxMapColumns=MAX_MAP_COLUMNS,
        maxMapRows=MAX_MAP_ROWS,
        scalarObsOffset=alignedSize(MAP_OBS_SIZE, sizeof(float)),
        numNearWallObs=NUM_NEAR_WALL_OBS,
        nearWallPosObsSize=NEAR_WALL_POS_OBS_SIZE,
        nearWallObsSize=NEAR_WALL_OBS_SIZE,
        nearWallTypesObsOffset=NEAR_WALL_TYPES_OBS_OFFSET,
        nearWallPosObsOffset=NEAR_WALL_POS_OBS_OFFSET,
        numFloatingWallObs=NUM_FLOATING_WALL_OBS,
        floatingWallInfoObsSize=FLOATING_WALL_INFO_OBS_SIZE,
        floatingWallObsSize=FLOATING_WALL_OBS_SIZE,
        floatingWallTypesObsOffset=FLOATING_WALL_TYPES_OBS_OFFSET,
        floatingWallInfoObsOffset=FLOATING_WALL_INFO_OBS_OFFSET,
        numWeaponPickupObs=NUM_WEAPON_PICKUP_OBS,
        weaponPickupPosObsSize=WEAPON_PICKUP_POS_OBS_SIZE,
        weaponPickupObsSize=WEAPON_PICKUP_OBS_SIZE,
        weaponPickupTypesObsOffset=WEAPON_PICKUP_TYPES_OBS_OFFSET,
        weaponPickupPosObsOffset=WEAPON_PICKUP_POS_OBS_OFFSET,
        numProjectileObs=NUM_PROJECTILE_OBS,
        projectilePosObsSize=PROJECTILE_POS_OBS_SIZE,
        projectileObsSize=PROJECTILE_OBS_SIZE,
        projectileTypesObsOffset=PROJECTILE_TYPES_OBS_OFFSET,
        projectilePosObsOffset=PROJECTILE_POS_OBS_OFFSET,
        enemyDroneObsOffset=ENEMY_DRONE_OBS_OFFSET,
        enemyDroneObsSize=ENEMY_DRONE_OBS_SIZE,
        droneObsOffset=DRONE_OBS_OFFSET,
        droneObsSize=DRONE_OBS_SIZE,
    )


cdef class CyImpulseWars:
    cdef:
        uint16_t numEnvs
        uint8_t numDrones
        bint render
        env* envs
        logBuffer *logs
        rayClient* rayClient

    def __init__(self, uint16_t numEnvs, uint8_t numDrones, uint8_t numAgents, uint8_t[:, :] observations, bint discretizeActions, float[:, :] contActions, int32_t[:, :] discActions, float[:] rewards, uint8_t[:] terminals, uint8_t[:] truncations, uint64_t seed, bint render, bint isTraining):
        self.numEnvs = numEnvs
        self.numDrones = numDrones
        self.render = render
        self.envs = <env*>calloc(numEnvs, sizeof(env))
        self.logs = createLogBuffer(LOG_BUFFER_SIZE)

        cdef int inc = numAgents
        cdef int i
        for i in range(self.numEnvs):
            initEnv(
                &self.envs[i],
                numDrones,
                numAgents,
                &observations[i * inc, 0],
                discretizeActions,
                &contActions[i * inc, 0],
                &discActions[i * inc, 0],
                &rewards[i * inc],
                &terminals[i * inc],
                &truncations[i * inc],
                self.logs,
                seed + i,
                isTraining,
            )

    cdef _initRaylib(self):
        self.rayClient = createRayClient()
        cdef int i
        for i in range(self.numEnvs):
            self.envs[i].client = self.rayClient

    def reset(self):
        if self.render and self.rayClient == NULL:
            self._initRaylib()

        cdef int i
        for i in range(self.numEnvs):
            resetEnv(&self.envs[i])

    def step(self):
        cdef int i
        for i in range(self.numEnvs):
            stepEnv(&self.envs[i])

    def log(self):
        cdef logEntry log = aggregateAndClearLogBuffer(self.numDrones, self.logs)
        return log

    def close(self):
        cdef int i
        for i in range(self.numEnvs):
            destroyEnv(&self.envs[i])

        destroyLogBuffer(self.logs)
        free(self.envs)

        if self.rayClient != NULL:
            destroyRayClient(self.rayClient)
