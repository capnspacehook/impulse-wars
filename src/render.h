#ifndef IMPULSE_WARS_RENDER_H
#define IMPULSE_WARS_RENDER_H

#include "raylib.h"

#include "env.h"
#include "helpers.h"

const float DEFAULT_SCALE = 11.0f;
const uint16_t DEFAULT_WIDTH = 1250;
const uint16_t DEFAULT_HEIGHT = 1000;
const uint16_t HEIGHT_LEEWAY = 100;

const float EXPLOSION_TIME = 0.5f;

const Color barolo = {.r = 165, .b = 8, .g = 37, .a = 255};

const float halfDroneRadius = DRONE_RADIUS / 2.0f;
const float aimGuideHeight = 0.3f * DRONE_RADIUS;

static inline float b2XToRayX(const env *e, const float x) {
    return e->client->halfWidth + (x * e->renderScale);
}

static inline float b2YToRayY(const env *e, const float y) {
    return (e->client->halfHeight + (y * e->renderScale)) + (2 * e->renderScale);
}

static inline Vector2 b2VecToRayVec(const env *e, const b2Vec2 v) {
    return (Vector2){.x = b2XToRayX(e, v.x), .y = b2YToRayY(e, v.y)};
}

static inline b2Vec2 rayVecToB2Vec(const env *e, const Vector2 v) {
    return (b2Vec2){.x = (v.x - e->client->halfWidth) / e->renderScale, .y = ((v.y - e->client->halfHeight - (2 * e->renderScale)) / e->renderScale)};
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

    SetTargetFPS(EVAL_FRAME_RATE);

    return client;
}

void destroyRayClient(rayClient *client) {
    CloseWindow();
    fastFree(client);
}

void setEnvRenderScale(env *e) {
    float scale = e->client->scale;
    switch (e->columns) {
    case 20:
        scale *= 1.05f;
        break;
    case 21:
        break;
    case 23:
        scale *= 0.925f;
        break;
    case 24:
        scale *= 0.88f;
        break;
    default:
        ERRORF("unsupported number of map columns: %d", e->columns);
    }
    DEBUG_LOGF("setting render scale to %f before %f columns %d", scale, e->client->scale, e->columns);
    e->renderScale = scale;
}

void renderUI(const env *e) {
    // render human control message
    if (e->humanInput) {
        const char *msg = "Human input: drone %d";
        char buffer[strlen(msg) + 2];
        snprintf(buffer, sizeof(buffer), msg, e->humanDroneInput);
        DrawText(buffer, e->client->scale, 3 * e->client->scale, 20, LIME);
    }

    // render timer
    if (e->stepsLeft == 0) {
        DrawText("SUDDEN DEATH", (e->client->width / 2) - (8 * e->client->scale), e->client->scale, 2 * e->client->scale, WHITE);
        return;
    }

    const int bufferSize = 3;
    char timerStr[bufferSize];
    if (e->stepsLeft >= 10 * e->frameRate) {
        snprintf(timerStr, bufferSize, "%d", (uint16_t)(e->stepsLeft / e->frameRate));
    } else {
        snprintf(timerStr, bufferSize, "0%d", (uint16_t)(e->stepsLeft / e->frameRate));
    }
    DrawText(timerStr, (e->client->width / 2) - e->client->scale, e->client->scale, 2 * e->client->scale, WHITE);
}

void renderExplosions(const env *e) {
    CC_ArrayIter iter;
    cc_array_iter_init(&iter, e->explosions);
    explosionInfo *explosion;
    while (cc_array_iter_next(&iter, (void **)&explosion) != CC_ITER_END) {
        if (explosion->renderSteps == UINT16_MAX) {
            explosion->renderSteps = EXPLOSION_TIME * e->frameRate;
        } else if (explosion->renderSteps == 0) {
            fastFree(explosion);
            cc_array_iter_remove(&iter, NULL);
            continue;
        }

        const uint8_t alpha = (uint8_t)(255.0f * ((float)explosion->renderSteps / (EXPLOSION_TIME * e->frameRate)));
        DEBUG_LOGF("alpha: %d", alpha);
        Color falloffColor = GRAY;
        falloffColor.a = alpha;
        Color explosionColor = RAYWHITE;
        explosionColor.a = alpha;

        DrawCircleV(b2VecToRayVec(e, explosion->def.position), (explosion->def.radius + explosion->def.falloff) * e->renderScale, falloffColor);
        DrawCircleV(b2VecToRayVec(e, explosion->def.position), explosion->def.radius * e->renderScale, explosionColor);
        explosion->renderSteps = fmaxf(explosion->renderSteps - 1, 0);
    }
}

void renderEmptyCell(const env *e, const b2Vec2 emptyCell, const size_t idx) {
    Rectangle rec = {
        .x = b2XToRayX(e, emptyCell.x - (WALL_THICKNESS / 2.0f)),
        .y = b2YToRayY(e, emptyCell.y - (WALL_THICKNESS / 2.0f)),
        .width = WALL_THICKNESS * e->renderScale,
        .height = WALL_THICKNESS * e->renderScale,
    };
    DrawRectangleLinesEx(rec, e->renderScale / 20.0f, RAYWHITE);

    const int bufferSize = 4;
    char idxStr[bufferSize];
    snprintf(idxStr, bufferSize, "%zu", idx);
    DrawText(idxStr, rec.x, rec.y, 1.5f * e->renderScale, WHITE);
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
    Vector2 pos = b2VecToRayVec(e, wallPos);
    Rectangle rec = {
        .x = pos.x,
        .y = pos.y,
        .width = wall->extent.x * e->renderScale * 2.0f,
        .height = wall->extent.y * e->renderScale * 2.0f,
    };

    Vector2 origin = {.x = wall->extent.x * e->renderScale, .y = wall->extent.y * e->renderScale};
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
    Vector2 pos = b2VecToRayVec(e, pickupPos);
    Rectangle rec = {
        .x = pos.x,
        .y = pos.y,
        .width = PICKUP_THICKNESS * e->renderScale,
        .height = PICKUP_THICKNESS * e->renderScale,
    };
    Vector2 origin = {.x = (PICKUP_THICKNESS / 2.0f) * e->renderScale, .y = (PICKUP_THICKNESS / 2.0f) * e->renderScale};
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

    DrawText(name, rec.x - origin.x + (0.1f * e->renderScale), rec.y - origin.y + e->renderScale, e->renderScale, BLACK);
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

b2RayResult droneAimingAt(const env *e, droneEntity *drone) {
    const b2Vec2 pos = getCachedPos(drone->bodyID, &drone->pos);
    const b2Vec2 rayEnd = b2MulAdd(pos, 150.0f, drone->lastAim);
    const b2Vec2 translation = b2Sub(rayEnd, pos);
    const b2QueryFilter filter = {.categoryBits = PROJECTILE_SHAPE, .maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | DRONE_SHAPE};
    return b2World_CastRayClosest(e->worldID, pos, translation, filter);
}

void renderDroneGuides(const env *e, droneEntity *drone, const int droneIdx) {
    b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
    const float rayX = b2XToRayX(e, pos.x);
    const float rayY = b2YToRayY(e, pos.y);
    const Color droneColor = getDroneColor(droneIdx);

    // render thruster move guide
    if (!b2VecEqual(drone->lastMove, b2Vec2_zero)) {
        float moveMagnitude = b2Length(drone->lastMove);
        float moveRot = RAD2DEG * b2Rot_GetAngle(b2MakeRot(b2Atan2(-drone->lastMove.y, -drone->lastMove.x)));
        Rectangle moveGuide = {
            .x = rayX,
            .y = rayY,
            .width = ((halfDroneRadius * moveMagnitude) + halfDroneRadius) * e->renderScale * 2.0f,
            .height = halfDroneRadius * e->renderScale * 2.0f,
        };
        DrawRectanglePro(moveGuide, (Vector2){.x = 0.0f, .y = 0.5f * e->renderScale}, moveRot, droneColor);
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
        .width = aimGuideWidth * e->renderScale,
        .height = aimGuideHeight * e->renderScale,
    };
    const float aimAngle = RAD2DEG * b2Rot_GetAngle(b2MakeRot(b2Atan2(drone->lastAim.y, drone->lastAim.x)));
    DrawRectanglePro(aimGuide, (Vector2){.x = 0.0f, .y = (aimGuideHeight / 2.0f) * e->renderScale}, aimAngle, droneColor);
}

void renderDrone(const env *e, const droneEntity *drone, const int droneIdx) {
    const b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
    const Vector2 raylibPos = b2VecToRayVec(e, pos);
    DrawCircleV(raylibPos, DRONE_RADIUS * e->renderScale, getDroneColor(droneIdx));
    DrawCircleV(raylibPos, DRONE_RADIUS * 0.85f * e->renderScale, BLACK);
}

void renderDroneLabels(const env *e, const droneEntity *drone) {
    const b2Vec2 pos = b2Body_GetPosition(drone->bodyID);

    // draw brake meter
    const float brakeMeterInnerRadius = 10.0f;
    const float brakeMeterOuterRadius = 5.0f;
    const Vector2 brakeMeterOrigin = {.x = b2XToRayX(e, pos.x), .y = b2YToRayY(e, pos.y)};
    const float breakMeterEndAngle = 360.f * drone->brakeLeft;
    Color brakeMeterColor = RAYWHITE;
    if (drone->brakeFullyDepleted) {
        brakeMeterColor = DARKGRAY;
    }
    DrawRing(brakeMeterOrigin, brakeMeterInnerRadius, brakeMeterOuterRadius, 0.0f, breakMeterEndAngle, 1, brakeMeterColor);

    // draw ammo count
    const int bufferSize = 5;
    char ammoStr[bufferSize];
    snprintf(ammoStr, bufferSize, "%d", drone->ammo);
    float posX = pos.x - 0.25;
    if (drone->ammo >= 10 || drone->ammo == INFINITE) {
        posX -= 0.25f;
    }
    DrawText(ammoStr, b2XToRayX(e, posX), b2YToRayY(e, pos.y + 1.5f), e->renderScale, WHITE);

    const float maxCharge = weaponCharge(e, drone->weaponInfo->type);
    if (maxCharge == 0.0f) {
        return;
    }

    // draw charge meter
    const float chargeMeterWidth = 2.0f;
    const float chargeMeterHeight = 1.0f;
    Rectangle outlineRec = {
        .x = b2XToRayX(e, pos.x - (chargeMeterWidth / 2.0f)),
        .y = b2YToRayY(e, pos.y - (chargeMeterHeight / 2.0f) + 3.0f),
        .width = chargeMeterWidth * e->renderScale,
        .height = chargeMeterHeight * e->renderScale,
    };
    DrawRectangleLinesEx(outlineRec, e->renderScale / 20.0f, RAYWHITE);

    const float fillRecWidth = (drone->charge / maxCharge) * chargeMeterWidth;
    Rectangle fillRec = {
        .x = b2XToRayX(e, pos.x - 1.0f),
        .y = b2YToRayY(e, pos.y - (chargeMeterHeight / 2.0f) + 3.0f),
        .width = fillRecWidth * e->renderScale,
        .height = chargeMeterHeight * e->renderScale,
    };
    const Vector2 origin = {.x = 0.0f, .y = 0.0f};
    DrawRectanglePro(fillRec, origin, 0.0f, RAYWHITE);
}

void renderProjectiles(env *e) {
    for (SNode *cur = e->projectiles->head; cur != NULL; cur = cur->next) {
        projectileEntity *projectile = (projectileEntity *)cur->data;
        b2Vec2 pos = b2Body_GetPosition(projectile->bodyID);
        DrawCircleV(b2VecToRayVec(e, pos), e->renderScale * projectile->weaponInfo->radius, PURPLE);
    }
}

void renderEnv(env *e) {
    BeginDrawing();

    ClearBackground(BLACK);
    DrawFPS(e->renderScale, e->renderScale);

    renderUI(e);

    renderExplosions(e);

    for (size_t i = 0; i < cc_array_size(e->pickups); i++) {
        const weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
        renderWeaponPickup(e, pickup);
    }

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
