#include "env.h"
#include "render.h"

int main(void) {
    const int NUM_DRONES = 2;

    env *e = fastCalloc(1, sizeof(env));

    uint8_t *obs = NULL;
    posix_memalign((void **)&obs, sizeof(void *), alignedSize(NUM_DRONES * obsBytes(NUM_DRONES), sizeof(float)));

    float *rewards = fastCalloc(NUM_DRONES, sizeof(float));
    float *contActions = fastCalloc(NUM_DRONES * CONTINUOUS_ACTION_SIZE, sizeof(float));
    int32_t *discActions = fastCalloc(NUM_DRONES * DISCRETE_ACTION_SIZE, sizeof(int32_t));
    uint8_t *terminals = fastCalloc(NUM_DRONES, sizeof(uint8_t));
    uint8_t *truncations = fastCalloc(NUM_DRONES, sizeof(uint8_t));
    logBuffer *logs = createLogBuffer(LOG_BUFFER_SIZE);

    initEnv(e, NUM_DRONES, NUM_DRONES, obs, true, contActions, discActions, rewards, terminals, truncations, logs, time(NULL), false, false);
    initMaps(e);
    setupEnv(e);
    e->humanInput = true;

    rayClient *client = createRayClient();
    e->client = client;
    resetEnv(e);

    while (true) {

        if (WindowShouldClose()) {
            destroyEnv(e);
            destroyMaps();
            fastFree(obs);
            fastFree(contActions);
            fastFree(discActions);
            fastFree(rewards);
            fastFree(terminals);
            fastFree(truncations);
            destroyLogBuffer(logs);
            fastFree(e);
            destroyRayClient(client);
            return 0;
        }

        stepEnv(e);
    }
}
