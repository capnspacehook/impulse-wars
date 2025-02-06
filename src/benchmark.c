#include "env.h"

void perfTest(const uint32_t numSteps) {
    const uint8_t NUM_DRONES = 2;

    env *e = fastCalloc(1, sizeof(env));

    uint8_t *obs = NULL;
    posix_memalign((void **)&obs, sizeof(void *), alignedSize(NUM_DRONES * obsBytes(NUM_DRONES), sizeof(float)));

    float *rewards = fastCalloc(NUM_DRONES, sizeof(float));
    float *actions = fastCalloc(NUM_DRONES * CONTINUOUS_ACTION_SIZE, sizeof(float));
    uint8_t *terminals = (unsigned char *)fastCalloc(NUM_DRONES, sizeof(uint8_t));
    uint8_t *truncations = fastCalloc(NUM_DRONES, sizeof(uint8_t));
    logBuffer *logs = createLogBuffer(1);

    initEnv(e, NUM_DRONES, NUM_DRONES, obs, false, actions, NULL, rewards, terminals, truncations, logs, time(NULL), false, true);
    initMaps(e);
    setupEnv(e);

    uint32_t steps = 0;
    while (steps != numSteps) {
        uint8_t actionOffset = 0;
        for (uint8_t i = 0; i < e->numDrones; i++) {
            e->contActions[actionOffset + 0] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 1] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 2] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 3] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 4] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 5] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 6] = randFloat(&e->randState, -1.0f, 1.0f);

            actionOffset += CONTINUOUS_ACTION_SIZE;
        }

        stepEnv(e);
        steps++;
    }

    destroyEnv(e);
    destroyMaps();

    fastFree(obs);
    fastFree(actions);
    fastFree(rewards);
    fastFree(terminals);
    fastFree(truncations);
    destroyLogBuffer(logs);
    fastFree(e);
}

int main(void) {
    perfTest(2500000);
    return 0;
}
