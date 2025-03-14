#include "env.h"
#include "render.h"

#ifdef __EMSCRIPTEN__
void emscriptenStep(void *e) {
    stepEnv((env *)e);
    return;
}
#endif

int main(void) {
    const int NUM_DRONES = 4;

    env *e = fastCalloc(1, sizeof(env));

    uint8_t *obs = NULL;
    posix_memalign((void **)&obs, sizeof(void *), alignedSize(NUM_DRONES * obsBytes(NUM_DRONES), sizeof(float)));

    float *rewards = fastCalloc(NUM_DRONES, sizeof(float));
    float *contActions = fastCalloc(NUM_DRONES * CONTINUOUS_ACTION_SIZE, sizeof(float));
    int32_t *discActions = fastCalloc(NUM_DRONES * DISCRETE_ACTION_SIZE, sizeof(int32_t));
    uint8_t *masks = fastCalloc(NUM_DRONES, sizeof(uint8_t));
    uint8_t *terminals = fastCalloc(NUM_DRONES, sizeof(uint8_t));
    uint8_t *truncations = fastCalloc(NUM_DRONES, sizeof(uint8_t));
    logBuffer *logs = createLogBuffer(LOG_BUFFER_SIZE);

    rayClient *client = createRayClient();
    e->client = client;

    initEnv(e, NUM_DRONES, 0, obs, true, contActions, discActions, rewards, masks, terminals, truncations, logs, -1, time(NULL), true, false, false);
    initMaps(e);
    setupEnv(e);
    e->humanInput = true;

    // Hack hack hack
    Vector2 rayCenter = b2VecToRayVec(e, b2Vec2_zero);
    rayCenter.x = 0.0f;
    rayCenter.y = 0.0f;
    client->camera = (Camera3D){
        .position = (Vector3){.x = 75.0f, .y = 150.0f, .z = 75.0f},
        .target = (Vector3){.x = rayCenter.x, .y = 0.0f, .z = rayCenter.y},
        .up = (Vector3){.x = 0.0f, .y = 1.0f, .z = 0.0f},
        .fovy = 135.0f,
        .projection = CAMERA_ORTHOGRAPHIC,
    };


#ifdef __EMSCRIPTEN__
    lastFrameTime = emscripten_get_now();
    emscripten_set_main_loop_arg(emscriptenStep, e, 0, true);
#else
    while (true) {
        if (WindowShouldClose()) {
            destroyEnv(e);
            destroyMaps();
            free(obs);
            fastFree(contActions);
            fastFree(discActions);
            fastFree(rewards);
            fastFree(masks);
            fastFree(terminals);
            fastFree(truncations);
            destroyLogBuffer(logs);
            fastFree(e);
            destroyRayClient(client);
            return 0;
        }

        stepEnv(e);
    }
#endif
}
