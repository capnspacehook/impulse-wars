#ifndef IMPULSE_WARS_RENDER_H
#define IMPULSE_WARS_RENDER_H

#include "raylib.h"

#include "env.h"
#include "helpers.h"

const float DEFAULT_SCALE = 11.0f;
const int DEFAULT_WIDTH = 1250;
const int DEFAULT_HEIGHT = 1000;
const int HEIGHT_LEEWAY = 100;

const Color barolo = {.r = 165, .b = 8, .g = 37, .a = 255};

const float halfDroneRadius = DRONE_RADIUS / 2.0f;
const float aimGuideHeight = 0.3f * DRONE_RADIUS;

static inline float b2XToRayX(const rayClient *c, const float x) {
    return c->halfWidth + (x * c->scale);
}

static inline float b2YToRayY(const rayClient *c, const float y) {
    return (c->halfHeight + (y * c->scale)) + (2 * c->scale);
}

static inline Vector2 b2VecToRayVec(const rayClient *c, const b2Vec2 v) {
    return (Vector2){.x = b2XToRayX(c, v.x), .y = b2YToRayY(c, v.y)};
}

static inline b2Vec2 rayVecToB2Vec(const rayClient *c, const Vector2 v) {
    return (b2Vec2){.x = (v.x - c->halfWidth) / c->scale, .y = ((v.y - c->halfHeight - (2 * c->scale)) / c->scale)};
}

rayClient *createRayClient() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Impulse Wars");
    const int monitor = GetCurrentMonitor();

    rayClient *client = (rayClient *)fastCalloc(1, sizeof(rayClient));

    if (client->height == 0) {
        client->height = GetMonitorHeight(monitor) - HEIGHT_LEEWAY;
    }
    if (client->width == 0) {
        client->width = ((float)client->height * ((float)DEFAULT_WIDTH / (float)DEFAULT_HEIGHT));
    }
    client->scale = (float)client->height * (float)(DEFAULT_SCALE / DEFAULT_HEIGHT);

    client->halfWidth = client->width / 2.0f;
    client->halfHeight = client->height / 2.0f;

    SetWindowSize(client->width, client->height);

    SetTargetFPS(FRAME_RATE);

    return client;
}

void destroyRayClient(rayClient *client) {
    CloseWindow();
    fastFree(client);
}

void renderUI(const env *e) {
    if (e->stepsLeft == 0) {
        DrawText("SUDDEN DEATH", (e->client->width / 2) - (8 * e->client->scale), e->client->scale, 2 * e->client->scale, WHITE);
        return;
    }

    const int bufferSize = 3;
    char timerStr[bufferSize];
    if (e->stepsLeft >= 10 * FRAME_RATE) {
        snprintf(timerStr, bufferSize, "%d", (uint16_t)(e->stepsLeft / FRAME_RATE));
    } else {
        snprintf(timerStr, bufferSize, "0%d", (uint16_t)(e->stepsLeft / FRAME_RATE));
    }
    DrawText(timerStr, (e->client->width / 2) - e->client->scale, e->client->scale, 2 * e->client->scale, WHITE);
}

void renderEmptyCell(const env *e, const b2Vec2 emptyCell, const size_t idx) {
    Rectangle rec = {
        .x = b2XToRayX(e->client, emptyCell.x - (WALL_THICKNESS / 2.0f)),
        .y = b2YToRayY(e->client, emptyCell.y - (WALL_THICKNESS / 2.0f)),
        .width = WALL_THICKNESS * e->client->scale,
        .height = WALL_THICKNESS * e->client->scale,
    };
    DrawRectangleLinesEx(rec, e->client->scale / 20.0f, RAYWHITE);

    const int bufferSize = 4;
    char idxStr[bufferSize];
    snprintf(idxStr, bufferSize, "%zu", idx);
    DrawText(idxStr, rec.x, rec.y, 1.5f * e->client->scale, WHITE);
}

void renderWall(const env *e, const wallEntity *wall) {
    Color color = {0};
    switch (wall->type) {
    case STANDARD_WALL_ENTITY:
        color = BLUE;
        break;
    case BOUNCY_WALL_ENTITY:
        color = YELLOW;
        break;
    case DEATH_WALL_ENTITY:
        color = RED;
        break;
    default:
        ERRORF("unknown wall type %d", wall->type);
    }

    b2Vec2 wallPos = b2Body_GetPosition(wall->bodyID);
    Vector2 pos = b2VecToRayVec(e->client, wallPos);
    Rectangle rec = {
        .x = pos.x,
        .y = pos.y,
        .width = wall->extent.x * e->client->scale * 2.0f,
        .height = wall->extent.y * e->client->scale * 2.0f,
    };

    Vector2 origin = {.x = wall->extent.x * e->client->scale, .y = wall->extent.y * e->client->scale};
    float angle = 0.0f;
    if (wall->isFloating) {
        angle = b2Rot_GetAngle(b2Body_GetRotation(wall->bodyID));
        angle *= RAD2DEG;
    }

    DrawRectanglePro(rec, origin, angle, color);
    // rec.x -= wall->extent.x * scale;
    // rec.y -= wall->extent.y * scale;
    // DrawRectangleLinesEx(rec, scale / 20.0f, WHITE);
}

void renderWeaponPickup(const env *e, const weaponPickupEntity *pickup) {
    if (pickup->respawnWait != 0.0f || pickup->floatingWallsTouching != 0) {
        return;
    }

    b2Vec2 pickupPos = b2Body_GetPosition(pickup->bodyID);
    Vector2 pos = b2VecToRayVec(e->client, pickupPos);
    Rectangle rec = {
        .x = pos.x,
        .y = pos.y,
        .width = PICKUP_THICKNESS * e->client->scale,
        .height = PICKUP_THICKNESS * e->client->scale,
    };
    Vector2 origin = {.x = (PICKUP_THICKNESS / 2.0f) * e->client->scale, .y = (PICKUP_THICKNESS / 2.0f) * e->client->scale};
    DrawRectanglePro(rec, origin, 0.0f, LIME);

    char *name = "";
    switch (pickup->weapon) {
    case MACHINEGUN_WEAPON:
        name = "MCGN";
        break;
    case SNIPER_WEAPON:
        name = "SNPR";
        break;
    case SHOTGUN_WEAPON:
        name = "SHGN";
        break;
    case IMPLODER_WEAPON:
        name = "IMPL";
        break;
    default:
        ERRORF("unknown weapon pickup type %d", pickup->weapon);
    }

    DrawText(name, rec.x - origin.x + (0.1f * e->client->scale), rec.y - origin.y + e->client->scale, e->client->scale, BLACK);
}

Color getDroneColor(const int droneIdx) {
    switch (droneIdx) {
    case 0:
        return barolo;
    case 1:
        return GREEN;
    case 2:
        return BLUE;
    case 3:
        return YELLOW;
    default:
        ERRORF("unsupported number of drones %d", droneIdx + 1);
        return WHITE;
    }
}

void renderDroneGuides(const env *e, droneEntity *drone, const int droneIdx) {
    b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
    const float rayX = b2XToRayX(e->client, pos.x);
    const float rayY = b2YToRayY(e->client, pos.y);
    const Color droneColor = getDroneColor(droneIdx);

    // render thruster move guide
    if (!b2VecEqual(drone->lastMove, b2Vec2_zero)) {
        float moveMagnitude = b2Length(drone->lastMove);
        float moveRot = RAD2DEG * b2Rot_GetAngle(b2MakeRot(b2Atan2(-drone->lastMove.y, -drone->lastMove.x)));
        Rectangle moveGuide = {
            .x = rayX,
            .y = rayY,
            .width = ((halfDroneRadius * moveMagnitude) + halfDroneRadius) * e->client->scale * 2.0f,
            .height = halfDroneRadius * e->client->scale * 2.0f,
        };
        DrawRectanglePro(moveGuide, (Vector2){.x = 0.0f, .y = 0.5f * e->client->scale}, moveRot, droneColor);
    }

    // find length of laser aiming guide by where it touches the nearest shape
    const b2RayResult rayRes = droneAimingAt(e, drone);
    ASSERT(b2Shape_IsValid(rayRes.shapeId));
    const entity *ent = (entity *)b2Shape_GetUserData(rayRes.shapeId);

    b2SimplexCache cache = {0};
    bool shapeIsCircle = false;
    b2ShapeProxy proxyA = {0};
    if (ent->type == DRONE_ENTITY) {
        proxyA.radius = DRONE_RADIUS;
        shapeIsCircle = true;
    } else {
        proxyA.count = 1;
        proxyA.points[0] = (b2Vec2){.x = 0.0f, .y = 0.0f};
    }
    const b2ShapeProxy proxyB = makeDistanceProxy(ent->type, &shapeIsCircle);
    const b2DistanceInput input = {
        .proxyA = proxyA,
        .proxyB = proxyB,
        .transformA = {.p = pos, .q = b2Rot_identity},
        .transformB = {.p = rayRes.point, .q = b2Rot_identity},
        .useRadii = shapeIsCircle,
    };
    const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);

    float aimGuideWidth = 0.0f;
    switch (drone->weaponInfo->type) {
    case STANDARD_WEAPON:
        aimGuideWidth = 5.0f;
        break;
    case MACHINEGUN_WEAPON:
        aimGuideWidth = 10.0f;
        break;
    case SNIPER_WEAPON:
        aimGuideWidth = 100.0f;
        break;
    case SHOTGUN_WEAPON:
        aimGuideWidth = 3.0f;
        break;
    case IMPLODER_WEAPON:
        aimGuideWidth = 5.0f;
        break;
    default:
        ERRORF("unknown weapon when getting aim guide width %d", drone->weaponInfo->type);
    }
    aimGuideWidth = fminf(aimGuideWidth, output.distance) + (DRONE_RADIUS * 2.0f);

    // render laser aim guide
    Rectangle aimGuide = {
        .x = rayX,
        .y = rayY,
        .width = aimGuideWidth * e->client->scale,
        .height = aimGuideHeight * e->client->scale,
    };
    const float aimAngle = RAD2DEG * b2Rot_GetAngle(b2MakeRot(b2Atan2(drone->lastAim.y, drone->lastAim.x)));
    DrawRectanglePro(aimGuide, (Vector2){.x = 0.0f, .y = (aimGuideHeight / 2.0f) * e->client->scale}, aimAngle, droneColor);
}

void renderDrone(const env *e, const droneEntity *drone, const int droneIdx) {
    const b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
    const Vector2 raylibPos = b2VecToRayVec(e->client, pos);
    DrawCircleV(raylibPos, DRONE_RADIUS * e->client->scale, BLACK);
    DrawCircleLinesV(raylibPos, DRONE_RADIUS * e->client->scale, getDroneColor(droneIdx));
}

void renderDroneLabels(const env *e, const droneEntity *drone) {
    const b2Vec2 pos = b2Body_GetPosition(drone->bodyID);

    const int bufferSize = 5;
    char ammoStr[bufferSize];
    snprintf(ammoStr, bufferSize, "%d", drone->ammo);
    float posX = pos.x - 0.25;
    if (drone->ammo >= 10 || drone->ammo == INFINITE) {
        posX -= 0.25f;
    }
    DrawText(ammoStr, b2XToRayX(e->client, posX), b2YToRayY(e->client, pos.y + 1.5f), e->client->scale, WHITE);

    const float maxCharge = weaponCharge(drone->weaponInfo->type);
    if (maxCharge == 0.0f) {
        return;
    }

    const float chargeMeterWidth = 2.0f;
    const float chargeMeterHeight = 1.0f;
    Rectangle outlineRec = {
        .x = b2XToRayX(e->client, pos.x - (chargeMeterWidth / 2.0f)),
        .y = b2YToRayY(e->client, pos.y - (chargeMeterHeight / 2.0f) + 3.0f),
        .width = chargeMeterWidth * e->client->scale,
        .height = chargeMeterHeight * e->client->scale,
    };
    DrawRectangleLinesEx(outlineRec, e->client->scale / 20.0f, RAYWHITE);

    const float fillRecWidth = (drone->charge / maxCharge) * chargeMeterWidth;
    Rectangle fillRec = {
        .x = b2XToRayX(e->client, pos.x - 1.0f),
        .y = b2YToRayY(e->client, pos.y - (chargeMeterHeight / 2.0f) + 3.0f),
        .width = fillRecWidth * e->client->scale,
        .height = chargeMeterHeight * e->client->scale,
    };
    const Vector2 origin = {.x = 0.0f, .y = 0.0f};
    DrawRectanglePro(fillRec, origin, 0.0f, RAYWHITE);
}

void renderProjectiles(env *e) {
    if (e->explosionSteps != 0) {
        DrawCircleV(b2VecToRayVec(e->client, e->explosion.position), e->explosion.falloff * 2.0f * e->client->scale, GRAY);
        DrawCircleV(b2VecToRayVec(e->client, e->explosion.position), e->explosion.radius * e->client->scale, RAYWHITE);
        e->explosionSteps = fmaxf(e->explosionSteps - 1, 0);
    }

    for (SNode *cur = e->projectiles->head; cur != NULL; cur = cur->next) {
        projectileEntity *projectile = (projectileEntity *)cur->data;
        b2Vec2 pos = b2Body_GetPosition(projectile->bodyID);
        DrawCircleV(b2VecToRayVec(e->client, pos), e->client->scale * projectile->weaponInfo->radius, PURPLE);
    }
}

void renderEnv(env *e) {
    BeginDrawing();

    ClearBackground(BLACK);
    DrawFPS(e->client->scale, e->client->scale);

    renderUI(e);

    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            continue;
        }
        renderDroneGuides(e, drone, i);
    }
    for (uint8_t i = 0; i < e->numDrones; i++) {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            continue;
        }
        renderDrone(e, drone, i);
    }

    for (size_t i = 0; i < cc_array_size(e->walls); i++) {
        const wallEntity *wall = safe_array_get_at(e->walls, i);
        renderWall(e, wall);
    }

    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
        const wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        renderWall(e, wall);
    }

    for (size_t i = 0; i < cc_array_size(e->pickups); i++) {
        const weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
        renderWeaponPickup(e, pickup);
    }

    renderProjectiles(e);

    for (uint8_t i = 0; i < e->numDrones; i++) {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            continue;
        }
        renderDroneLabels(e, drone);
    }

    // for (size_t i = 0; i < cc_array_size(e->cells); i++) {
    //     mapCell *cell;
    //     cc_array_get_at(e->cells, i, (void **)&cell);
    //     if (cell->ent == NULL) {
    //         renderEmptyCell(e, cell->pos, i);
    //     }
    // }

    EndDrawing();
}

#endif
