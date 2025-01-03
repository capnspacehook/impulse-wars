#include "env.h"
#include "render.h"

const float lStickDeadzone = 0.1f;
const float rStickDeadzone = 0.1f;

void getPlayerInputs(const env *e, const droneEntity *drone, const int gamepadIdx) {
    uint8_t offset = gamepadIdx * CONTINUOUS_ACTION_SIZE;
    if (IsGamepadAvailable(gamepadIdx)) {
        float lStickX = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_LEFT_X);
        float lStickY = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_LEFT_Y);
        float rStickX = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_RIGHT_X);
        float rStickY = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_RIGHT_Y);

        bool shoot = IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
        if (!shoot) {
            shoot = IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        }

        e->contActions[offset++] = lStickX;
        e->contActions[offset++] = lStickY;
        e->contActions[offset++] = rStickX;
        e->contActions[offset++] = rStickY;
        e->contActions[offset++] = shoot;

        return;
    }
    if (gamepadIdx != 0) {
        return;
    }

    b2Vec2 move = b2Vec2_zero;
    if (IsKeyDown(KEY_W)) {
        move.y += -1.0f;
    }
    if (IsKeyDown(KEY_S)) {
        move.y += 1.0f;
    }
    if (IsKeyDown(KEY_A)) {
        move.x += -1.0f;
    }
    if (IsKeyDown(KEY_D)) {
        move.x += 1.0f;
    }
    move = b2Normalize(move);

    Vector2 mousePos = (Vector2){.x = (float)GetMouseX(), .y = (float)GetMouseY()};
    b2Vec2 dronePos = b2Body_GetPosition(drone->bodyID);
    const b2Vec2 aim = b2Normalize(b2Sub(rayVecToB2Vec(e->client, mousePos), dronePos));

    bool shoot = false;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        shoot = true;
    }

    e->contActions[offset++] = move.x;
    e->contActions[offset++] = move.y;
    e->contActions[offset++] = aim.x;
    e->contActions[offset++] = aim.y;
    e->contActions[offset++] = shoot;
}

int main(void) {
    const int NUM_DRONES = 2;

    env *e = (env *)fastCalloc(1, sizeof(env));
    uint8_t *obs = (uint8_t *)aligned_alloc(sizeof(float), alignedSize(NUM_DRONES * obsBytes(), sizeof(float)));
    float *rewards = (float *)fastCalloc(NUM_DRONES, sizeof(float));
    float *actions = (float *)fastCalloc(NUM_DRONES * CONTINUOUS_ACTION_SIZE, sizeof(float));
    uint8_t *terminals = (uint8_t *)fastCalloc(NUM_DRONES, sizeof(uint8_t));
    logBuffer *logs = createLogBuffer(LOG_BUFFER_SIZE);

    initEnv(e, NUM_DRONES, NUM_DRONES, obs, false, actions, NULL, rewards, terminals, logs, time(NULL));

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

        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone;
            cc_array_get_at(e->drones, i, (void **)&drone);
            getPlayerInputs(e, drone, i);
        }

        stepEnv(e);
    }
}
