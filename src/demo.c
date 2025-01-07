#include "env.h"
#include "render.h"

int main(void) {
    const int NUM_DRONES = 2;

    env *e = (env *)fastCalloc(1, sizeof(env));
    uint8_t *obs = (uint8_t *)aligned_alloc(sizeof(float), alignedSize(NUM_DRONES * obsBytes(), sizeof(float)));
    float *rewards = (float *)fastCalloc(NUM_DRONES, sizeof(float));
    float *actions = (float *)fastCalloc(NUM_DRONES * CONTINUOUS_ACTION_SIZE, sizeof(float));
    uint8_t *terminals = (uint8_t *)fastCalloc(NUM_DRONES, sizeof(uint8_t));
    uint8_t *truncations = (uint8_t *)fastCalloc(NUM_DRONES, sizeof(uint8_t));
    logBuffer *logs = createLogBuffer(LOG_BUFFER_SIZE);

    initEnv(e, NUM_DRONES, 1, obs, false, actions, NULL, rewards, terminals, truncations, logs, time(NULL), false);
    e->humanInput = true;

    rayClient *client = createRayClient();
    e->client = client;

    while (true) {
        if (WindowShouldClose()) {
            destroyEnv(e);
            fastFree(obs);
            fastFree(actions);
            fastFree(rewards);
            fastFree(terminals);
            destroyLogBuffer(logs);
            fastFree(e);
            destroyRayClient(client);
            return 0;
        }

        stepEnv(e);
    }
}
