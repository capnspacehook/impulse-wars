#ifndef IMPULSE_WARS_RENDER_H
#define IMPULSE_WARS_RENDER_H

#include "raylib.h"

#include "env.h"
#include "helpers.h"

const float DEFAULT_SCALE = 11.0f;
const uint16_t DEFAULT_WIDTH = 1250;
const uint16_t DEFAULT_HEIGHT = 1000;
const uint16_t HEIGHT_LEEWAY = 75;

const float START_READY_TIME = 1.0f;
const float END_WAIT_TIME = 2.0f;
const float EXPLOSION_TIME = 0.5f;

const Color barolo = {.r = 165, .g = 37, .b = 8, .a = 255};
const Color bambooBrown = {.r = 204, .g = 129, .b = 0, .a = 255};

const float halfDroneRadius = DRONE_RADIUS / 2.0f;
const float aimGuideHeight = 0.3f * DRONE_RADIUS;
const float chargedAimGuideHeight = DRONE_RADIUS;

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

    rayClient *client = fastCalloc(1, sizeof(rayClient));

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
    const float BASE_ROWS = 21.0f;
    const float scale = e->client->scale * (BASE_ROWS / e->map->rows);
    e->renderScale = scale;
}

Color getDroneColor(const uint8_t droneIdx) {
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

char *getWeaponAbreviation(const enum weaponType type) {
    char *name = "";
    switch (type) {
    case MACHINEGUN_WEAPON:
        name = "MCGN";
        break;
    case SNIPER_WEAPON:
        // TODO: rename to railgun everywhere
        name = "RAIL";
        break;
    case SHOTGUN_WEAPON:
        name = "SHGN";
        break;
    case IMPLODER_WEAPON:
        name = "IMPL";
        break;
    case ACCELERATOR_WEAPON:
        name = "ACCL";
        break;
    case FLAK_CANNON_WEAPON:
        name = "FLAK";
        break;
    case MINE_LAUNCHER_WEAPON:
        name = "MINE";
        break;
    default:
        ERRORF("unknown weapon pickup type %d", type);
    }
    return name;
}

float getWeaponAimGuideWidth(const enum weaponType type) {
    switch (type) {
    case STANDARD_WEAPON:
    case IMPLODER_WEAPON:
    case ACCELERATOR_WEAPON:
        return 5.0f;
    case FLAK_CANNON_WEAPON:
        return 7.5f;
    case MACHINEGUN_WEAPON:
    case MINE_LAUNCHER_WEAPON:
        return 10.0f;
    case SNIPER_WEAPON:
        return 150.0f;
    case SHOTGUN_WEAPON:
        return 3.0f;
    default:
        ERRORF("unknown weapon when getting aim guide width %d", type);
    }
}

Color getProjectileColor(const enum weaponType type) {
    switch (type) {
    case STANDARD_WEAPON:
        return PURPLE;
    case IMPLODER_WEAPON:
    case ACCELERATOR_WEAPON:
        return DARKBLUE;
    case FLAK_CANNON_WEAPON:
        return VIOLET;
    case MACHINEGUN_WEAPON:
    case SNIPER_WEAPON:
    case SHOTGUN_WEAPON:
        return ORANGE;
    case MINE_LAUNCHER_WEAPON:
        return BROWN;
    default:
        ERRORF("unknown weapon when getting projectile color %d", type);
    }
}

void renderTimer(const env *e, const char *timerStr, const Color color) {
    int fontSize = 2 * e->client->scale;
    int textWidth = MeasureText(timerStr, fontSize);
    int posX = (e->client->width - textWidth) / 2;
    DrawText(timerStr, posX, e->client->scale, fontSize, color);
}

void renderUI(const env *e, const bool starting, const bool ending, const int8_t winner, const int8_t winningTeam) {
    // render human control message
    if (e->humanInput) {
        const char *msg = "Human input: drone %d";
        char buffer[strlen(msg) + 2];
        snprintf(buffer, sizeof(buffer), msg, e->humanDroneInput);
        DrawText(buffer, e->client->scale, 3 * e->client->scale, 20, LIME);
    }

    // render timer
    if (starting) {
        renderTimer(e, "READY", WHITE);
        return;
    } else if (e->stepsLeft > (ROUND_STEPS - 1) * e->frameRate) {
        renderTimer(e, "GO!", WHITE);
        return;
    } else if (ending) {
        if (winner == -1 && winningTeam == -1) {
            renderTimer(e, "Tie", WHITE);
            return;
        }
        const int bufferSize = 16;
        char winStr[bufferSize];
        Color color;
        if (e->teamsEnabled) {
            snprintf(winStr, bufferSize, "Team %d wins!", winningTeam + 1);
            color = RAYWHITE;
        } else {
            snprintf(winStr, bufferSize, "Player %d wins!", winner + 1);
            color = getDroneColor(winner);
        }
        renderTimer(e, winStr, color);
        return;
    } else if (e->stepsLeft == 0) {
        renderTimer(e, "SUDDEN DEATH", WHITE);
        return;
    }

    const int bufferSize = 3;
    char timerStr[bufferSize];
    if (e->stepsLeft >= 10 * e->frameRate) {
        snprintf(timerStr, bufferSize, "%d", (uint16_t)(e->stepsLeft / e->frameRate));
    } else {
        snprintf(timerStr, bufferSize, "0%d", (uint16_t)(e->stepsLeft / e->frameRate));
    }
    renderTimer(e, timerStr, WHITE);
}

void renderBrakeTrails(const env *e, const bool ending) {
    CC_ArrayIter brakeTrailIter;
    cc_array_iter_init(&brakeTrailIter, e->brakeTrailPoints);
    brakeTrailPoint *trailPoint;
    while (cc_array_iter_next(&brakeTrailIter, (void **)&trailPoint) != CC_ITER_END) {
        if (trailPoint->lifetime == UINT16_MAX) {
            trailPoint->lifetime = 3.0f * e->frameRate;
        } else if (trailPoint->lifetime == 0) {
            fastFree(trailPoint);
            cc_array_iter_remove(&brakeTrailIter, NULL);
            continue;
        }

        Color trailColor = GRAY;
        trailColor.a = 32.0f * (trailPoint->lifetime / (3.0f * e->frameRate));
        float radius = 0.3f * e->renderScale;
        DrawCircleV(b2VecToRayVec(e, trailPoint->pos), radius, trailColor);
        if (!ending) {
            trailPoint->lifetime--;
        }
    }
}

void renderExplosions(const env *e, const bool ending) {
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
        Color falloffColor = GRAY;
        falloffColor.a = alpha;
        Color explosionColor = RAYWHITE;
        explosionColor.a = alpha;

        DrawCircleV(b2VecToRayVec(e, explosion->def.position), (explosion->def.radius + explosion->def.falloff) * e->renderScale, falloffColor);
        DrawCircleV(b2VecToRayVec(e, explosion->def.position), explosion->def.radius * e->renderScale, explosionColor);
        // don't fade out explosions on the win screen
        if (!ending) {
            explosion->renderSteps = fmaxf(explosion->renderSteps - 1, 0);
        }
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

    Vector2 pos = b2VecToRayVec(e, wall->pos);
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

    Vector2 pos = b2VecToRayVec(e, pickup->pos);
    Rectangle rec = {
        .x = pos.x,
        .y = pos.y,
        .width = PICKUP_THICKNESS * e->renderScale,
        .height = PICKUP_THICKNESS * e->renderScale,
    };
    Vector2 origin = {.x = (PICKUP_THICKNESS / 2.0f) * e->renderScale, .y = (PICKUP_THICKNESS / 2.0f) * e->renderScale};
    DrawRectanglePro(rec, origin, 0.0f, LIME);

    const char *weaponName = getWeaponAbreviation(pickup->weapon);
    DrawText(weaponName, rec.x - origin.x + (0.1f * e->renderScale), rec.y - origin.y + e->renderScale, e->renderScale, BLACK);
}

b2RayResult droneAimingAt(const env *e, droneEntity *drone) {
    const b2Vec2 rayEnd = b2MulAdd(drone->pos, 150.0f, drone->lastAim);
    const b2Vec2 translation = b2Sub(rayEnd, drone->pos);
    const b2QueryFilter filter = {.categoryBits = PROJECTILE_SHAPE, .maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | DRONE_SHAPE};
    return b2World_CastRayClosest(e->worldID, drone->pos, translation, filter);
}

void renderDroneGuides(const env *e, droneEntity *drone, const uint8_t droneIdx) {
    const float rayX = b2XToRayX(e, drone->pos.x);
    const float rayY = b2YToRayY(e, drone->pos.y);
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
    const entity *ent = b2Shape_GetUserData(rayRes.shapeId);

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
    const b2ShapeProxy proxyB = makeDistanceProxy(ent, &shapeIsCircle);
    const b2DistanceInput input = {
        .proxyA = proxyA,
        .proxyB = proxyB,
        .transformA = {.p = drone->pos, .q = b2Rot_identity},
        .transformB = {.p = rayRes.point, .q = b2Rot_identity},
        .useRadii = shapeIsCircle,
    };
    const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);

    float aimGuideWidth = getWeaponAimGuideWidth(drone->weaponInfo->type);
    aimGuideWidth = fminf(aimGuideWidth, output.distance + 0.1f) + (DRONE_RADIUS * 2.0f);

    // render laser aim guide
    const float aimAngle = RAD2DEG * b2Rot_GetAngle(b2MakeRot(b2Atan2(drone->lastAim.y, drone->lastAim.x)));
    // subtly light up the laser aim guide if the drone's weapon is fully charged
    if (drone->weaponInfo->charge != 0.0f && drone->weaponCharge == drone->weaponInfo->charge) {
        Rectangle chargedAimGuide = {
            .x = rayX,
            .y = rayY,
            .width = aimGuideWidth * e->renderScale,
            .height = chargedAimGuideHeight * e->renderScale,
        };
        Color chargedAimGuideColor = droneColor;
        chargedAimGuideColor.a = 100;
        DrawRectanglePro(chargedAimGuide, (Vector2){.x = 0.0f, .y = (chargedAimGuideHeight / 2.0f) * e->renderScale}, aimAngle, chargedAimGuideColor);
    }

    Rectangle aimGuide = {
        .x = rayX,
        .y = rayY,
        .width = aimGuideWidth * e->renderScale,
        .height = aimGuideHeight * e->renderScale,
    };
    DrawRectanglePro(aimGuide, (Vector2){.x = 0.0f, .y = (aimGuideHeight / 2.0f) * e->renderScale}, aimAngle, droneColor);
}

void renderDrone(const env *e, const droneEntity *drone, const int droneIdx) {
    const Vector2 raylibPos = b2VecToRayVec(e, drone->pos);
    DrawCircleV(raylibPos, DRONE_RADIUS * e->renderScale, getDroneColor(droneIdx));
    DrawCircleV(raylibPos, DRONE_RADIUS * 0.8f * e->renderScale, BLACK);
}

void renderDroneUI(const env *e, const droneEntity *drone) {
    // draw energy meter
    const float energyMeterInnerRadius = 0.6f * e->renderScale;
    const float energyMeterOuterRadius = 0.3f * e->renderScale;
    const Vector2 energyMeterOrigin = {.x = b2XToRayX(e, drone->pos.x), .y = b2YToRayY(e, drone->pos.y)};
    float energyMeterEndAngle = 360.f * drone->energyLeft;
    Color energyMeterColor = RAYWHITE;
    if (drone->energyFullyDepleted && drone->energyRefillWait != 0.0f) {
        energyMeterColor = bambooBrown;
        energyMeterEndAngle = 360.0f * (1.0f - (drone->energyRefillWait / (DRONE_ENERGY_REFILL_EMPTY_WAIT)));
    } else if (drone->energyFullyDepleted) {
        energyMeterColor = GRAY;
    }
    DrawRing(energyMeterOrigin, energyMeterInnerRadius, energyMeterOuterRadius, 0.0f, energyMeterEndAngle, 1, energyMeterColor);

    // draw burst charge indicator
    if (drone->chargingBurst) {
        Color burstChargeColor = RAYWHITE;
        burstChargeColor.a = fminf((255.0f * drone->burstCharge) + 50.0f, 255.0f);
        const float burstChargeOuterRadius = ((DRONE_BURST_RADIUS_BASE * drone->burstCharge) + DRONE_BURST_RADIUS_MIN) * e->renderScale;
        const float burstChargeInnerRadius = burstChargeOuterRadius - (0.1f * e->renderScale);
        DrawRing(energyMeterOrigin, burstChargeInnerRadius, burstChargeOuterRadius, 0.0f, 360.0f, 1, burstChargeColor);
    }

    // draw ammo count
    const int bufferSize = 5;
    char ammoStr[bufferSize];
    snprintf(ammoStr, bufferSize, "%d", drone->ammo);
    float posX = drone->pos.x - 0.25;
    if (drone->ammo >= 10 || drone->ammo == INFINITE) {
        posX -= 0.25f;
    }
    DrawText(ammoStr, b2XToRayX(e, posX), b2YToRayY(e, drone->pos.y + 1.5f), e->renderScale, WHITE);

    const float maxCharge = drone->weaponInfo->charge;
    if (maxCharge == 0) {
        return;
    }

    // draw charge meter
    const float chargeMeterWidth = 2.0f;
    const float chargeMeterHeight = 1.0f;
    Rectangle outlineRec = {
        .x = b2XToRayX(e, drone->pos.x - (chargeMeterWidth / 2.0f)),
        .y = b2YToRayY(e, drone->pos.y - (chargeMeterHeight / 2.0f) + 3.0f),
        .width = chargeMeterWidth * e->renderScale,
        .height = chargeMeterHeight * e->renderScale,
    };
    DrawRectangleLinesEx(outlineRec, e->renderScale / 20.0f, RAYWHITE);

    const float fillRecWidth = (drone->weaponCharge / maxCharge) * chargeMeterWidth;
    Rectangle fillRec = {
        .x = b2XToRayX(e, drone->pos.x - 1.0f),
        .y = b2YToRayY(e, drone->pos.y - (chargeMeterHeight / 2.0f) + 3.0f),
        .width = fillRecWidth * e->renderScale,
        .height = chargeMeterHeight * e->renderScale,
    };
    const Vector2 origin = {.x = 0.0f, .y = 0.0f};
    DrawRectanglePro(fillRec, origin, 0.0f, RAYWHITE);
}

// TODO: render projectile trails, different colors for different weapons
void renderProjectiles(env *e) {
    for (size_t i = 0; i < cc_array_size(e->projectiles); i++) {
        projectileEntity *projectile = safe_array_get_at(e->projectiles, i);
        const Color projectileColor = getProjectileColor(projectile->weaponInfo->type);
        DrawCircleV(b2VecToRayVec(e, projectile->pos), e->renderScale * projectile->weaponInfo->radius, projectileColor);
    }
}

void _renderEnv(env *e, const bool starting, const bool ending, const int8_t winner, const int8_t winningTeam) {
    BeginDrawing();

    ClearBackground(BLACK);
    DrawFPS(e->renderScale, e->renderScale);

    renderUI(e, starting, ending, winner, winningTeam);

    renderExplosions(e, ending);

    for (size_t i = 0; i < cc_array_size(e->pickups); i++) {
        const weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
        renderWeaponPickup(e, pickup);
    }

    renderBrakeTrails(e, ending);

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
        renderDroneUI(e, drone);
    }

    // for (size_t i = 0; i < cc_array_size(e->cells); i++) {
    //     mapCell *cell;
    //     cc_array_get_at(e->cells, i, (void **)&cell);
    //     renderEmptyCell(e, cell->pos, i);
    // }

    if (!b2VecEqual(e->debugPoint, b2Vec2_zero)) {
        DrawCircleV(b2VecToRayVec(e, e->debugPoint), DRONE_RADIUS * 0.5f * e->renderScale, WHITE);
    }

    EndDrawing();
}

void renderEnv(env *e, const bool starting, const bool ending, const int8_t winner, const int8_t winningTeam) {
    if (starting) {
        for (uint16_t i = 0; i < (uint16_t)(START_READY_TIME * e->frameRate); i++) {
            _renderEnv(e, starting, ending, winner, winningTeam);
        }
    } else if (ending) {
        for (uint16_t i = 0; i < (uint16_t)(END_WAIT_TIME * e->frameRate); i++) {
            _renderEnv(e, starting, ending, winner, winningTeam);
        }
    } else {
        _renderEnv(e, starting, ending, winner, winningTeam);
    }
}

#endif
