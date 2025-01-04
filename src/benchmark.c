#include "env.h"

void perfTest(const float testTime) {
    const uint8_t NUM_DRONES = 2;

    env *e = (env *)fastCalloc(1, sizeof(env));
    uint8_t *obs = (uint8_t *)aligned_alloc(sizeof(float), alignedSize(NUM_DRONES * obsBytes(), sizeof(float)));
    float *rewards = (float *)fastCalloc(NUM_DRONES, sizeof(float));
    float *actions = (float *)fastCalloc(NUM_DRONES * CONTINUOUS_ACTION_SIZE, sizeof(float));
    unsigned char *terminals = (unsigned char *)fastCalloc(NUM_DRONES, sizeof(bool));
    logBuffer *logs = createLogBuffer(1);

    initEnv(e, NUM_DRONES, NUM_DRONES, obs, false, actions, NULL, rewards, terminals, logs, 0);

    const time_t start = time(NULL);
    int steps = 0;
    while (time(NULL) - start < testTime) {
        uint8_t actionOffset = 0;
        for (uint8_t i = 0; i < e->numDrones; i++) {
            e->contActions[actionOffset + 0] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 1] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 2] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 3] = randFloat(&e->randState, -1.0f, 1.0f);
            e->contActions[actionOffset + 4] = randFloat(&e->randState, -1.0f, 1.0f);

            actionOffset += CONTINUOUS_ACTION_SIZE;
        }

        stepEnv(e);
        steps++;
    }

    const time_t end = time(NULL);
    printf("SPS: %f\n", (float)NUM_DRONES * ((float)steps / (float)(end - start)));
    printf("Steps: %d\n", NUM_DRONES * steps);

    destroyEnv(e);

    fastFree(obs);
    fastFree(actions);
    fastFree(rewards);
    fastFree(terminals);
    destroyLogBuffer(logs);
    fastFree(e);
}

int main(void) {
    perfTest(5.0f);
    return 0;
}
