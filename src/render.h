#ifndef IMPULSE_WARS_RENDER_H
#define IMPULSE_WARS_RENDER_H

#include "raymath.h"
#include "rlgl.h"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION 100

#endif
#define RLIGHTS_IMPLEMENTATION

#include "rlights.h"

#define LETTER_BOUNDRY_SIZE 0.25f
#define TEXT_MAX_LAYERS 32
#define LETTER_BOUNDRY_COLOR VIOLET

bool SHOW_LETTER_BOUNDRY = false;
bool SHOW_TEXT_BOUNDRY = false;

void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color) {
    float x = position.x;
    float y = position.y;
    float z = position.z;

    // Set desired texture to be enabled while drawing following vertex data
    rlSetTexture(texture.id);

    // Vertex data transformation can be defined with the commented lines,
    // but in this example we calculate the transformed vertex data directly when calling rlVertex3f()
    // rlPushMatrix();
    // NOTE: Transformation is applied in inverse order (scale -> rotate -> translate)
    // rlTranslatef(2.0f, 0.0f, 0.0f);
    // rlRotatef(45, 0, 1, 0);
    // rlScalef(2.0f, 2.0f, 2.0f);

    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);
    // Front Face
    rlNormal3f(0.0f, 0.0f, 1.0f); // Normal Pointing Towards Viewer
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2); // Bottom Left Of The Texture and Quad
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2); // Bottom Right Of The Texture and Quad
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2); // Top Right Of The Texture and Quad
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2); // Top Left Of The Texture and Quad
    // Back Face
    rlNormal3f(0.0f, 0.0f, -1.0f); // Normal Pointing Away From Viewer
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2); // Bottom Right Of The Texture and Quad
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2); // Top Right Of The Texture and Quad
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2); // Top Left Of The Texture and Quad
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2); // Bottom Left Of The Texture and Quad
    // Top Face
    rlNormal3f(0.0f, 1.0f, 0.0f); // Normal Pointing Up
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2); // Top Left Of The Texture and Quad
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2); // Bottom Left Of The Texture and Quad
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2); // Bottom Right Of The Texture and Quad
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2); // Top Right Of The Texture and Quad
    // Bottom Face
    rlNormal3f(0.0f, -1.0f, 0.0f); // Normal Pointing Down
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2); // Top Right Of The Texture and Quad
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2); // Top Left Of The Texture and Quad
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2); // Bottom Left Of The Texture and Quad
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2); // Bottom Right Of The Texture and Quad
    // Right face
    rlNormal3f(1.0f, 0.0f, 0.0f); // Normal Pointing Right
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2); // Bottom Right Of The Texture and Quad
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2); // Top Right Of The Texture and Quad
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2); // Top Left Of The Texture and Quad
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2); // Bottom Left Of The Texture and Quad
    // Left Face
    rlNormal3f(-1.0f, 0.0f, 0.0f); // Normal Pointing Left
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2); // Bottom Left Of The Texture and Quad
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2); // Bottom Right Of The Texture and Quad
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2); // Top Right Of The Texture and Quad
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2); // Top Left Of The Texture and Quad
    rlEnd();
    // rlPopMatrix();

    rlSetTexture(0);
}

// Draw cube with texture piece applied to all faces
void DrawCubeTextureRec(Texture2D texture, Rectangle source, Vector3 position, float width, float height, float length, Color color) {
    float x = position.x;
    float y = position.y;
    float z = position.z;
    float texWidth = (float)texture.width;
    float texHeight = (float)texture.height;

    // Set desired texture to be enabled while drawing following vertex data
    rlSetTexture(texture.id);

    // We calculate the normalized texture coordinates for the desired texture-source-rectangle
    // It means converting from (tex.width, tex.height) coordinates to [0.0f, 1.0f] equivalent
    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);

    // Front face
    rlNormal3f(0.0f, 0.0f, 1.0f);
    rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f((source.x + source.width) / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(source.x / texWidth, source.y / texHeight);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);

    // Back face
    rlNormal3f(0.0f, 0.0f, -1.0f);
    rlTexCoord2f((source.x + source.width) / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(source.x / texWidth, source.y / texHeight);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);

    // Top face
    rlNormal3f(0.0f, 1.0f, 0.0f);
    rlTexCoord2f(source.x / texWidth, source.y / texHeight);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f((source.x + source.width) / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);

    // Bottom face
    rlNormal3f(0.0f, -1.0f, 0.0f);
    rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(source.x / texWidth, source.y / texHeight);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f((source.x + source.width) / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);

    // Right face
    rlNormal3f(1.0f, 0.0f, 0.0f);
    rlTexCoord2f((source.x + source.width) / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x + width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
    rlVertex3f(x + width / 2, y + height / 2, z - length / 2);
    rlTexCoord2f(source.x / texWidth, source.y / texHeight);
    rlVertex3f(x + width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x + width / 2, y - height / 2, z + length / 2);

    // Left face
    rlNormal3f(-1.0f, 0.0f, 0.0f);
    rlTexCoord2f(source.x / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x - width / 2, y - height / 2, z - length / 2);
    rlTexCoord2f((source.x + source.width) / texWidth, (source.y + source.height) / texHeight);
    rlVertex3f(x - width / 2, y - height / 2, z + length / 2);
    rlTexCoord2f((source.x + source.width) / texWidth, source.y / texHeight);
    rlVertex3f(x - width / 2, y + height / 2, z + length / 2);
    rlTexCoord2f(source.x / texWidth, source.y / texHeight);
    rlVertex3f(x - width / 2, y + height / 2, z - length / 2);

    rlEnd();

    rlSetTexture(0);
}

static void DrawTextCodepoint3D(Font font, int codepoint, Vector3 position, float fontSize, bool backface, Color tint) {
    // Character index position in sprite font
    // NOTE: In case a codepoint is not available in the font, index returned points to '?'
    int index = GetGlyphIndex(font, codepoint);
    float scale = fontSize / (float)font.baseSize;

    // Character destination rectangle on screen
    // NOTE: We consider charsPadding on drawing
    position.x += (float)(font.glyphs[index].offsetX - font.glyphPadding) / (float)font.baseSize * scale;
    position.z += (float)(font.glyphs[index].offsetY - font.glyphPadding) / (float)font.baseSize * scale;

    // Character source rectangle from font texture atlas
    // NOTE: We consider chars padding when drawing, it could be required for outline/glow shader effects
    Rectangle srcRec = {font.recs[index].x - (float)font.glyphPadding, font.recs[index].y - (float)font.glyphPadding,
                        font.recs[index].width + 2.0f * font.glyphPadding, font.recs[index].height + 2.0f * font.glyphPadding};

    float width = (float)(font.recs[index].width + 2.0f * font.glyphPadding) / (float)font.baseSize * scale;
    float height = (float)(font.recs[index].height + 2.0f * font.glyphPadding) / (float)font.baseSize * scale;

    if (font.texture.id > 0) {
        const float x = 0.0f;
        const float y = 0.0f;
        const float z = 0.0f;

        // normalized texture coordinates of the glyph inside the font texture (0.0f -> 1.0f)
        const float tx = srcRec.x / font.texture.width;
        const float ty = srcRec.y / font.texture.height;
        const float tw = (srcRec.x + srcRec.width) / font.texture.width;
        const float th = (srcRec.y + srcRec.height) / font.texture.height;

        if (SHOW_LETTER_BOUNDRY) {
            DrawCubeWiresV((Vector3){position.x + width / 2, position.y, position.z + height / 2}, (Vector3){width, LETTER_BOUNDRY_SIZE, height}, LETTER_BOUNDRY_COLOR);
        }

        rlCheckRenderBatchLimit(4 + 4 * backface);
        rlSetTexture(font.texture.id);

        rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);

        rlBegin(RL_QUADS);
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);

        // Front Face
        rlNormal3f(0.0f, 1.0f, 0.0f); // Normal Pointing Up
        rlTexCoord2f(tx, ty);
        rlVertex3f(x, y, z); // Top Left Of The Texture and Quad
        rlTexCoord2f(tx, th);
        rlVertex3f(x, y, z + height); // Bottom Left Of The Texture and Quad
        rlTexCoord2f(tw, th);
        rlVertex3f(x + width, y, z + height); // Bottom Right Of The Texture and Quad
        rlTexCoord2f(tw, ty);
        rlVertex3f(x + width, y, z); // Top Right Of The Texture and Quad

        if (backface) {
            // Back Face
            rlNormal3f(0.0f, -1.0f, 0.0f); // Normal Pointing Down
            rlTexCoord2f(tx, ty);
            rlVertex3f(x, y, z); // Top Right Of The Texture and Quad
            rlTexCoord2f(tw, ty);
            rlVertex3f(x + width, y, z); // Top Left Of The Texture and Quad
            rlTexCoord2f(tw, th);
            rlVertex3f(x + width, y, z + height); // Bottom Left Of The Texture and Quad
            rlTexCoord2f(tx, th);
            rlVertex3f(x, y, z + height); // Bottom Right Of The Texture and Quad
        }
        rlEnd();
        rlPopMatrix();

        rlSetTexture(0);
    }
}

static void DrawText3D(Font font, const char *text, Vector3 position, float fontSize, float fontSpacing, float lineSpacing, bool backface, Color tint) {
    int length = TextLength(text); // Total length in bytes of the text, scanned by codepoints in loop

    float textOffsetY = 0.0f; // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f; // Offset X to next character to draw

    float scale = fontSize / (float)font.baseSize;

    for (int i = 0; i < length;) {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) {
            codepointByteCount = 1;
        }

        if (codepoint == '\n') {
            // NOTE: Fixed line spacing of 1.5 line-height
            // TODO: Support custom line spacing defined by user
            textOffsetY += scale + lineSpacing / (float)font.baseSize * scale;
            textOffsetX = 0.0f;
        } else {
            if ((codepoint != ' ') && (codepoint != '\t')) {
                DrawTextCodepoint3D(font, codepoint, (Vector3){position.x + textOffsetX, position.y, position.z + textOffsetY}, fontSize, backface, tint);
            }

            if (font.glyphs[index].advanceX == 0) {
                textOffsetX += (float)(font.recs[index].width + fontSpacing) / (float)font.baseSize * scale;
            } else {
                textOffsetX += (float)(font.glyphs[index].advanceX + fontSpacing) / (float)font.baseSize * scale;
            }
        }

        i += codepointByteCount; // Move text bytes counter to next codepoint
    }
}

#include "helpers.h"

const Color STONE_GRAY = (Color){80, 80, 80, 255};
const Color PUFF_RED = (Color){187, 0, 0, 255};
const Color PUFF_GREEN = (Color){0, 187, 0, 255};
const Color PUFF_YELLOW = (Color){160, 160, 0, 255};
const Color PUFF_CYAN = (Color){0, 187, 187, 255};
const Color PUFF_WHITE = (Color){241, 241, 241, 241};
const Color PUFF_BACKGROUND = (Color){6, 24, 24, 255};
const Color PUFF_BACKGROUND2 = (Color){18, 72, 72, 255};

bool droneControlledByHuman(const env *e, uint8_t i);

const float DEFAULT_SCALE = 11.0f;
const uint16_t DEFAULT_WIDTH = 1500;
const uint16_t DEFAULT_HEIGHT = 1000;
const uint16_t HEIGHT_LEEWAY = 75;

const float START_READY_TIME = 1.0f;
const float END_WAIT_TIME = 2.0f;

const float EXPLOSION_TIME = 0.5f;

const float DRONE_RESPAWN_GUIDE_SHRINK_TIME = 0.75f;
const float DRONE_RESPAWN_GUIDE_HOLD_TIME = 0.75f;
const float DRONE_RESPAWN_GUIDE_MAX_RADIUS = DRONE_RADIUS * 5.5f;
const float DRONE_RESPAWN_GUIDE_MIN_RADIUS = DRONE_RADIUS * 2.5f;

const float DRONE_PIECE_LIFETIME = 2.0f;

const Color barolo = {.r = 165, .g = 37, .b = 8, .a = 255};
const Color bambooBrown = {.r = 204, .g = 129, .b = 0, .a = 255};

const float halfDroneRadius = DRONE_RADIUS / 2.0f;
const float droneThrusterRadius = DRONE_RADIUS * 0.66;
const float aimGuideHeight = 0.3f * DRONE_RADIUS;
const float chargedAimGuideHeight = DRONE_RADIUS;

const float wallLinePercent = 0.9f;

const float sqrt2 = 1.414213562f;

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

void updateTrailPoints(const env *e, trailPoints *tp, const uint8_t maxLen, const b2Vec2 pos) {
    const Vector2 v = (Vector2){.x = pos.x, .y = pos.y};
    if (tp->length < maxLen) {
        tp->points[tp->length++] = v;
        return;
    }

    for (uint8_t i = 0; i < maxLen - 1; i++) {
        tp->points[i] = tp->points[i + 1];
    }
    tp->points[maxLen - 1] = v;
}

rayClient *createRayClient() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Impulse Wars");

    rayClient *client = fastCalloc(1, sizeof(rayClient));

    if (client->height == 0) {
#ifndef __EMSCRIPTEN__
        const int monitor = GetCurrentMonitor();
        client->height = GetMonitorHeight(monitor) - HEIGHT_LEEWAY;
#else
        client->height = DEFAULT_HEIGHT;
#endif
    }
    if (client->width == 0) {
        client->width = ((float)client->height * ((float)DEFAULT_WIDTH / (float)DEFAULT_HEIGHT));
    }
    client->scale = (float)client->height * (float)(DEFAULT_SCALE / DEFAULT_HEIGHT);

    client->halfWidth = client->width / 2.0f;
    client->halfHeight = client->height / 2.0f;

    SetWindowSize(client->width, client->height);

#ifndef __EMSCRIPTEN__
    SetTargetFPS(EVAL_FRAME_RATE);
#endif

    client->texture = LoadTexture("resources/cubicmap_atlas.png");

    char *vsPath = TextFormat("shaders/gls%i/lighting.vs", GLSL_VERSION);
    char *fsPath = TextFormat("shaders/gls%i/lighting.fs", GLSL_VERSION);
    client->shader = LoadShader(vsPath, fsPath);
    client->shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(client->shader, "viewPos");
    int ambientLoc = GetShaderLocation(client->shader, "ambient");
    float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    SetShaderValue(client->shader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);

    for (int i = 0; i < MAX_LIGHTS; i++) {
        client->lights[i] = CreateLight(LIGHT_POINT,
                                        (Vector3){0.0f, 20.0f, 0.0f},
                                        Vector3Zero(),
                                        (Color){40, 40, 40, 255},
                                        client->shader);
    }

    vsPath = TextFormat("shaders/gls%i/bloom.vs", GLSL_VERSION);
    fsPath = TextFormat("shaders/gls%i/bloom.fs", GLSL_VERSION);
    client->bloom = LoadShader(vsPath, fsPath);
    client->bloomPosLoc = GetShaderLocation(client->bloom, "pos");

    vsPath = TextFormat("shaders/gls%i/grid.vs", GLSL_VERSION);
    fsPath = TextFormat("shaders/gls%i/grid.fs", GLSL_VERSION);
    client->grid = LoadShader(vsPath, fsPath);
    for (int i = 0; i < 4; i++) {
        client->gridPosLoc[i] = GetShaderLocation(client->grid, TextFormat("pos[%i]", i));
        client->gridColorLoc[i] = GetShaderLocation(client->grid, TextFormat("color[%i]", i));
    }
    return client;
}

void destroyRayClient(rayClient *client) {
    CloseWindow();
    fastFree(client);
}

void handle_camera_controls(rayClient *client) {
    static Vector2 prev_mouse_pos = {0};
    static bool is_dragging = false;
    float camera_move_speed = 0.5f;

    // Handle mouse drag for camera movement
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        prev_mouse_pos = GetMousePosition();
        is_dragging = true;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        is_dragging = false;
    }

    if (is_dragging) {
        Vector2 current_mouse_pos = GetMousePosition();
        Vector2 delta = {
            (current_mouse_pos.x - prev_mouse_pos.x) * camera_move_speed,
            -(current_mouse_pos.y - prev_mouse_pos.y) * camera_move_speed};

        // Update camera position (only X and Y)
        client->camera.position.x += delta.x;
        client->camera.position.y += delta.y;

        // Update camera target (only X and Y)
        client->camera.target.x += delta.x;
        client->camera.target.y += delta.y;

        prev_mouse_pos = current_mouse_pos;
    }

    // Handle mouse wheel for zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        float zoom_factor = 1.0f - (wheel * 0.1f);
        // Adjust camera position for zoom while maintaining height
        client->camera.position.x = client->camera.target.x +
                                    (client->camera.position.x - client->camera.target.x) * zoom_factor;
        client->camera.position.y = client->camera.target.y +
                                    (client->camera.position.y - client->camera.target.y) * zoom_factor;
    }
}

void setEnvRenderScale(env *e) {
    const float BASE_ROWS = 21.0f;
    const float scale = e->client->scale * (BASE_ROWS / e->map->rows);
    e->renderScale = scale;
}

Color getDroneColor(const uint8_t droneIdx) {
    switch (droneIdx) {
    case 0:
        return PUFF_RED;
    case 1:
        return PUFF_GREEN;
    case 2:
        return PUFF_CYAN;
    case 3:
        return PUFF_YELLOW;
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
    case BLACK_HOLE_WEAPON:
        name = "BLKH";
        break;
    default:
        ERRORF("unknown weapon pickup type %d", type);
    }
    return name;
}

char *getWeaponName(const enum weaponType type) {
    char *name = "";
    switch (type) {
    case STANDARD_WEAPON:
        name = "Standard";
        break;
    case MACHINEGUN_WEAPON:
        name = "Machine Gun";
        break;
    case SNIPER_WEAPON:
        name = "Railgun";
        break;
    case SHOTGUN_WEAPON:
        name = "Shotgun";
        break;
    case IMPLODER_WEAPON:
        name = "Imploder";
        break;
    case ACCELERATOR_WEAPON:
        name = "Accelerator";
        break;
    case FLAK_CANNON_WEAPON:
        name = "Flak Cannon";
        break;
    case MINE_LAUNCHER_WEAPON:
        name = "Mine Launcher";
        break;
    case BLACK_HOLE_WEAPON:
        name = "Black Hole";
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
        return 1.0f;
    case FLAK_CANNON_WEAPON:
    case BLACK_HOLE_WEAPON:
        return 7.5f;
    case MACHINEGUN_WEAPON:
    case MINE_LAUNCHER_WEAPON:
        return 2.0f;
    case SNIPER_WEAPON:
        return 50.0f;
    case SHOTGUN_WEAPON:
        return 0.6f;
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
        return MAROON;
    case MACHINEGUN_WEAPON:
    case SNIPER_WEAPON:
    case SHOTGUN_WEAPON:
        return ORANGE;
    case MINE_LAUNCHER_WEAPON:
        return BROWN;
    case BLACK_HOLE_WEAPON:
        return GRAY;
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

void renderUI(const env *e, const bool starting) {
    // render drone info
    const uint8_t droneInfoStrSize = 64;
    char droneInfoStr[droneInfoStrSize];
    memset(droneInfoStr, 0x0, droneInfoStrSize);
    uint8_t fontSize = 2 * e->client->scale;
    uint8_t xMargin = 5 * e->client->scale;
    uint8_t yMargin = 12 * e->client->scale;

    for (int i = 0; i < e->numDrones; i++) {
        const droneEntity *drone = safe_array_get_at(e->drones, i);

        // REVIEW: Raylib has a TextFormat function
        snprintf(droneInfoStr, droneInfoStrSize, "Drone %d", drone->idx + 1);
        const Vector2 textSize = MeasureTextEx(GetFontDefault(), droneInfoStr, fontSize, fontSize / 10);
        const uint16_t lineWidth = textSize.x + (3 * (e->renderScale * 2.5f));

        uint16_t x = 0;
        uint16_t y = 0;
        switch (drone->idx) {
        case 0:
            x = xMargin;
            y = yMargin;
            break;
        case 1:
            x = e->client->width - lineWidth - xMargin;
            y = yMargin;
            break;
        case 2:
            x = xMargin;
            y = e->client->height - yMargin - (6 * e->client->scale);
            break;
        case 3:
            x = e->client->width - lineWidth - xMargin;
            y = e->client->height - yMargin - (6 * e->client->scale);
            break;
        }

        Color textColor = PUFF_WHITE;
        if (drone->livesLeft == 0) {
            textColor = Fade(PUFF_WHITE, 0.5f);
        }
        DrawText(droneInfoStr, x, y, fontSize, textColor);

        uint16_t lifeX = x + textSize.x;
        uint16_t lifeY = y + (textSize.y / 2);
        const Color droneColor = getDroneColor(drone->idx);
        for (uint8_t i = 0; i < drone->livesLeft; i++) {
            lifeX += e->renderScale * 2.5f;
            DrawCircleLines(lifeX, lifeY, e->renderScale, droneColor);
        }

        y += textSize.y + e->client->scale;
        DrawLine(x, y, x + textSize.x, y, droneColor);

        y += e->client->scale;
        const char *weaponName = getWeaponName(drone->weaponInfo->type);
        DrawText(weaponName, x, y, fontSize, textColor);

        y += textSize.y + e->client->scale;
        DrawLine(x, y, x + MeasureText(weaponName, fontSize), y, droneColor);

        y += e->client->scale;
        char *playerType = "";
        if (droneControlledByHuman(e, drone->idx)) {
            playerType = "Human";
        } else if (drone->idx < e->numAgents) {
            playerType = "NN";
        } else {
            playerType = "Scripted";
        }
        if (e->teamsEnabled) {
            snprintf(droneInfoStr, droneInfoStrSize, "%s | Team %d", playerType, drone->team + 1);
        } else {
            memset(droneInfoStr, 0x0, droneInfoStrSize);
            strncpy(droneInfoStr, playerType, strlen(playerType));
        }

        DrawText(droneInfoStr, x, y, fontSize, textColor);
    }

    // render timer
    if (starting) {
        renderTimer(e, "READY", PUFF_WHITE);
        return;
    } else if (e->stepsLeft > (ROUND_STEPS - 1) * e->frameRate) {
        renderTimer(e, "GO!", PUFF_WHITE);
        return;
    } else if (e->stepsLeft == 0) {
        renderTimer(e, "SUDDEN DEATH", PUFF_WHITE);
        return;
    }

    const uint8_t bufferSize = 3;
    char timerStr[bufferSize];
    if (e->stepsLeft >= 10 * e->frameRate) {
        snprintf(timerStr, bufferSize, "%d", (uint16_t)(e->stepsLeft / e->frameRate));
    } else {
        snprintf(timerStr, bufferSize, "0%d", (uint16_t)(e->stepsLeft / e->frameRate));
    }
    renderTimer(e, timerStr, PUFF_WHITE);
}

void renderBrakeTrails(const env *e) {
    const float maxLifetime = 3.0f * e->frameRate;
    const float radius = 0.3f * e->renderScale;

    CC_ArrayIter brakeTrailIter;
    cc_array_iter_init(&brakeTrailIter, e->brakeTrailPoints);
    brakeTrailPoint *trailPoint;
    while (cc_array_iter_next(&brakeTrailIter, (void **)&trailPoint) != CC_ITER_END) {
        if (trailPoint->lifetime == UINT16_MAX) {
            trailPoint->lifetime = maxLifetime;
        } else if (trailPoint->lifetime == 0) {
            fastFree(trailPoint);
            cc_array_iter_remove(&brakeTrailIter, NULL);
            continue;
        }

        Color trailColor = Fade(GRAY, 0.133f * (trailPoint->lifetime / maxLifetime));
        DrawCircleV(b2VecToRayVec(e, trailPoint->pos), radius, trailColor);
        trailPoint->lifetime--;
    }
}

void renderExplosions(const env *e) {
    const uint16_t maxRenderSteps = EXPLOSION_TIME * e->frameRate;

    CC_ArrayIter iter;
    cc_array_iter_init(&iter, e->explosions);
    explosionInfo *explosion;

    while (cc_array_iter_next(&iter, (void **)&explosion) != CC_ITER_END) {
        if (explosion->renderSteps == UINT16_MAX) {
            explosion->renderSteps = maxRenderSteps;
        } else if (explosion->renderSteps == 0) {
            fastFree(explosion);
            cc_array_iter_remove(&iter, NULL);
            continue;
        }

        const Vector2 explosionPos = b2VecToRayVec(e, explosion->def.position);
        const float alpha = (float)explosion->renderSteps / maxRenderSteps;

        // color bursts with a bit of the parent drone's color
        if (explosion->isBurst) {
            const Color falloffColor = Fade(PUFF_WHITE, alpha);
            const Color explosionColor = Fade(GRAY, alpha);
            const Color droneColor = Fade(getDroneColor(explosion->droneIdx), alpha);

            BeginBlendMode(BLEND_ADDITIVE);
            // DrawCircleV(explosionPos, (explosion->def.radius + explosion->def.falloff) * e->renderScale, falloffColor);
            // DrawCircleV(explosionPos, (explosion->def.radius + explosion->def.falloff) * e->renderScale, droneColor);
            // DrawCircleV(explosionPos, explosion->def.radius * e->renderScale, explosionColor);
            // DrawCircleV(explosionPos, explosion->def.radius * e->renderScale, droneColor);
            DrawSphereEx(
                (Vector3){.x = explosion->def.position.x, .y = 0.5f, .z = explosion->def.position.y},
                explosion->def.radius + explosion->def.falloff,
                20,
                50,
                droneColor);
            DrawSphereEx(
                (Vector3){.x = explosion->def.position.x, .y = 0.5f, .z = explosion->def.position.y},
                explosion->def.radius,
                20,
                50,
                droneColor);
            EndBlendMode();
        } else {
            const Color falloffColor = Fade(GRAY, alpha);
            const Color explosionColor = Fade(GRAY, alpha);

            // DrawCircleV(explosionPos, (explosion->def.radius + explosion->def.falloff) * e->renderScale, falloffColor);
            // DrawCircleV(explosionPos, explosion->def.radius * e->renderScale, explosionColor);
            BeginBlendMode(BLEND_ADDITIVE);
            DrawSphereEx(
                (Vector3){.x = explosion->def.position.x, .y = 0.5f, .z = explosion->def.position.y},
                explosion->def.radius + explosion->def.falloff,
                20,
                50,
                falloffColor);
            DrawSphereEx(
                (Vector3){.x = explosion->def.position.x, .y = 0.5f, .z = explosion->def.position.y},
                explosion->def.radius,
                20,
                50,
                explosionColor);
            EndBlendMode();
        }

        explosion->renderSteps = max(explosion->renderSteps - 1, 0);
    }
}

void renderEmptyCell(const env *e, const b2Vec2 emptyCell, const size_t idx) {
    Rectangle rec = {
        .x = b2XToRayX(e, emptyCell.x - (WALL_THICKNESS / 2.0f)),
        .y = b2YToRayY(e, emptyCell.y - (WALL_THICKNESS / 2.0f)),
        .width = WALL_THICKNESS * e->renderScale,
        .height = WALL_THICKNESS * e->renderScale,
    };
    // DrawRectangleLinesEx(rec, e->renderScale / 20.0f, Fade(GRAY, 0.25f));
    DrawCubeWiresV(
        (Vector3){.x = rec.x, .y = 0.5f, .z = rec.y},
        (Vector3){.x = WALL_THICKNESS, .y = 0.0f, .z = WALL_THICKNESS},
        PUFF_WHITE);

    MAYBE_UNUSED(idx);
    // used for debugging
    //
    // const int bufferSize = 4;
    // char idxStr[bufferSize];
    // snprintf(idxStr, bufferSize, "%zu", idx);
    // DrawText(idxStr, rec.x, rec.y, 1.5f * e->renderScale, WHITE);
}

void renderWall(const env *e, const wallEntity *wall) {
    Color color = {0};
    Rectangle texture_rect;
    switch (wall->type) {
    case STANDARD_WALL_ENTITY:
        color = PUFF_CYAN;
        texture_rect = (Rectangle){
            0.0f,
            0.0f,
            e->client->texture.width / 2.0f,
            e->client->texture.height / 2.0f};
        break;
    case BOUNCY_WALL_ENTITY:
        color = PUFF_YELLOW;
        // TODO: Get a texture
        texture_rect = (Rectangle){
            0.0f,
            e->client->texture.height / 2.0f,
            e->client->texture.width / 2.0f,
            e->client->texture.height / 2.0f};
        break;
    case DEATH_WALL_ENTITY:
        color = PUFF_RED;
        texture_rect = (Rectangle){
            e->client->texture.width / 2.0f,
            0.0f,
            e->client->texture.width / 2.0f,
            e->client->texture.height / 2.0f};
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
        angle = b2Rot_GetAngle(wall->rot);
        angle *= RAD2DEG;
    }

    // DrawRectanglePro(rec, origin, angle, color);
    origin.x *= wallLinePercent;
    origin.y *= wallLinePercent;
    rec.width *= wallLinePercent;
    rec.height *= wallLinePercent;
    // DrawRectanglePro(rec, origin, angle, BLACK);

    float y_start, y_size;
    if (wall->isFloating) {
        y_start = 0.0f;
        y_size = FLOATING_WALL_THICKNESS;
    } else {
        y_start = 0.0f;
        y_size = 2.0f * WALL_THICKNESS;
    }

    Color faded = (Color){.r = 0.2 * color.r, .g = 0.2 * color.g, .b = 0.2 * color.b, .a = color.a};

    rlPushMatrix();
    rlTranslatef(wall->pos.x, 0.0f, wall->pos.y);
    rlRotatef(-angle, 0.0f, 1.0f, 0.0f);

    float x = 2.0f * wall->extent.x;
    float z = 2.0f * wall->extent.y;
    /*
    DrawCubeV(
        (Vector3){.x = 0.0f, .y = y_start, .z = 0.0f},
        (Vector3){.x = x, .y = y_size, .z = z},
        faded
    );
    */

    DrawCubeTextureRec(
        e->client->texture,
        texture_rect,
        (Vector3){.x = 0.0f, .y = y_start, .z = 0.0f},
        x, y_size, z, WHITE);
    // Draw cube with an applied texture, but only a defined rectangle piece of the texture
    // DrawCubeTextureRec(texture, (Rectangle){ 0.0f, texture.height/2.0f, texture.width/2.0f, texture.height/2.0f },
    //    (Vector3){ 2.0f, 1.0f, 0.0f }, 2.0f, 2.0f, 2.0f, WHITE);

    /*
    DrawCubeWiresV(
        (Vector3){.x = 0.0f, .y = y_start, .z = 0.0f},
        (Vector3){.x = x, .y = y_size, .z = z},
        color
    );
    */
    rlPopMatrix();
}

void renderWeaponPickup(const env *e, const weaponPickupEntity *pickup) {
    if (pickup->respawnWait != 0.0f || pickup->floatingWallsTouching != 0) {
        return;
    }
    Rectangle texture_rect = (Rectangle){
        e->client->texture.width / 2.0f,
        e->client->texture.height / 2.0f,
        e->client->texture.width / 2.0f,
        e->client->texture.height / 2.0f};

    Vector3 start = (Vector3){
        .x = pickup->pos.x - PICKUP_THICKNESS / 2.0f,
        .y = 1.5f,
        .z = pickup->pos.y - PICKUP_THICKNESS / 2.0f};
    Vector3 size = (Vector3){.x = PICKUP_THICKNESS, .y = 0.0f, .z = PICKUP_THICKNESS};

    // DrawCubeWiresV(start, size, PUFF_GREEN);
    // DrawCubeV(start, size, Fade(PUFF_GREEN, 0.2f));

    DrawCubeTextureRec(
        e->client->texture,
        texture_rect,
        (Vector3){.x = pickup->pos.x, .y = 0.5f, .z = pickup->pos.y},
        PICKUP_THICKNESS, 1.0f, PICKUP_THICKNESS, WHITE);

    const char *weaponName = getWeaponAbreviation(pickup->weapon);
    Font font = GetFontDefault();
    DrawText3D(font, weaponName, start, 12, 0.5f, -1.0f, false, PUFF_WHITE);
}

b2Vec2 b2RotatedPolygonVec(const float cosA, const float sinA, const b2Vec2 pos, const b2Vec2 vertice) {
    return (b2Vec2){
        .x = cosA * (vertice.x - pos.x) - sinA * (vertice.y - pos.y) + pos.x,
        .y = sinA * (vertice.x - pos.x) + cosA * (vertice.y - pos.y) + pos.y,
    };
}

void renderDronePieces(env *e) {
    const float maxLifetime = e->frameRate * DRONE_PIECE_LIFETIME;

    CC_ArrayIter iter;
    cc_array_iter_init(&iter, e->dronePieces);
    dronePieceEntity *piece;

    while (cc_array_iter_next(&iter, (void **)&piece) != CC_ITER_END) {
        if (piece->lifetime == UINT16_MAX) {
            piece->lifetime = maxLifetime;
        }

        float baseAlpha = 1.0f;
        if (piece->isShieldPiece) {
            baseAlpha = 0.5f;
        }
        const float alpha = baseAlpha * ((float)piece->lifetime / maxLifetime);
        const Color color = Fade(getDroneColor(piece->droneIdx), alpha);
        const float cosA = cosf(b2Rot_GetAngle(piece->rot));
        const float sinA = sinf(b2Rot_GetAngle(piece->rot));

        b2Vec2 v1 = b2Add(piece->pos, piece->vertices[0]);
        v1 = b2RotatedPolygonVec(cosA, sinA, piece->pos, v1);
        b2Vec2 v2 = b2Add(piece->pos, piece->vertices[1]);
        v2 = b2RotatedPolygonVec(cosA, sinA, piece->pos, v2);
        b2Vec2 v3 = b2Add(piece->pos, piece->vertices[2]);
        v3 = b2RotatedPolygonVec(cosA, sinA, piece->pos, v3);

        // DrawTriangleLines(
        //     b2VecToRayVec(e, v1),
        //     b2VecToRayVec(e, v2),
        //     b2VecToRayVec(e, v3),
        //     color);

        float adj = 2.0 - piece->lifetime / maxLifetime;
        if (adj < 1.0f) {
            adj = 1.0f;
        }
        float r = fminf(adj * (float)color.r, 255.0f);
        float g = fminf(adj * (float)color.g, 255.0f);
        float b = fminf(adj * (float)color.b, 255.0f);
        float a = color.a;
        Color adjustedColor = (Color){.r = r, .g = g, .b = b, .a = a};

        DrawTriangle3D(
            (Vector3){.x = v1.x, .y = 0.5f, .z = v1.y},
            (Vector3){.x = v2.x, .y = 0.5f, .z = v2.y},
            (Vector3){.x = v3.x, .y = 0.5f, .z = v3.y},
            adjustedColor);

        piece->lifetime--;
        if (piece->lifetime == 0) {
            destroyDronePiece(e, piece);
            cc_array_iter_remove_fast(&iter, NULL);
        }
    }
}

void renderDroneRespawnGuides(const env *e, droneEntity *drone) {
    if (drone->respawnGuideLifetime == 0) {
        return;
    }
    const float maxLifetime = e->frameRate * (DRONE_RESPAWN_GUIDE_SHRINK_TIME + DRONE_RESPAWN_GUIDE_HOLD_TIME);
    const uint16_t shrinkTime = e->frameRate * DRONE_RESPAWN_GUIDE_SHRINK_TIME;
    if (drone->respawnGuideLifetime == UINT16_MAX) {
        drone->respawnGuideLifetime = maxLifetime;
    }

    float radius = DRONE_RESPAWN_GUIDE_MIN_RADIUS;
    if (drone->respawnGuideLifetime >= maxLifetime - shrinkTime) {
        radius += DRONE_RESPAWN_GUIDE_MAX_RADIUS * ((drone->respawnGuideLifetime - (e->frameRate * DRONE_RESPAWN_GUIDE_HOLD_TIME)) / shrinkTime);
    }

    Color color = Fade(getDroneColor(drone->idx), 0.5f);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawSphere(
        (Vector3){.x = drone->pos.x, .y = 0.0f, .z = drone->pos.y},
        radius,
        color);
    EndBlendMode();

    drone->respawnGuideLifetime--;
}

b2RayResult droneAimingAt(const env *e, droneEntity *drone) {
    const b2Vec2 rayEnd = b2MulAdd(drone->pos, 150.0f, drone->lastAim);
    const b2Vec2 translation = b2Sub(rayEnd, drone->pos);
    const b2QueryFilter filter = {.categoryBits = PROJECTILE_SHAPE, .maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | DRONE_SHAPE};
    return b2World_CastRayClosest(e->worldID, drone->pos, translation, filter);
}

void renderDroneGuides(env *e, droneEntity *drone, const bool ending) {
    const float rayX = b2XToRayX(e, drone->pos.x);
    const float rayY = b2YToRayY(e, drone->pos.y);
    const Color droneColor = getDroneColor(drone->idx);

    // render thruster move guide
    if (!b2VecEqual(drone->lastMove, b2Vec2_zero)) {
        const float moveMagnitude = b2Length(drone->lastMove);
        const float moveRot = RAD2DEG * b2Rot_GetAngle(b2MakeRot(b2Atan2(-drone->lastMove.y, -drone->lastMove.x)));
        float flickerWidth = 0.0f;
        if (!ending) {
            flickerWidth = randFloat(&e->randState, -0.05f, 0.05f);
        }
        Rectangle moveGuide = {
            .x = rayX,
            .y = rayY,
            .width = ((halfDroneRadius * moveMagnitude) + halfDroneRadius + flickerWidth) * e->renderScale * 2.0f,
            .height = droneThrusterRadius * e->renderScale * 2.0f,
        };
        const Color thrusterColor = Fade(droneColor, 0.9f);
        // DrawRectanglePro(moveGuide, (Vector2){.x = 0.0f, .y = droneThrusterRadius * e->renderScale}, moveRot, thrusterColor);

        /*
        DrawCubeV(
            (Vector3){.x = drone->pos.x, .y = 0.5f, .z = drone->pos.y},
            (Vector3){.x = ((halfDroneRadius * moveMagnitude) + halfDroneRadius + flickerWidth) * 2.0f, .y = droneThrusterRadius * 2.0f, .z = moveRot},
            thrusterColor
        );
        */
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
    aimGuideWidth = min(aimGuideWidth, output.distance + 0.1f) + (DRONE_RADIUS * 2.0f);

    if (drone->weaponInfo->type == SNIPER_WEAPON) {
        aimGuideWidth = 6.0f;
    }

    // render laser aim guide
    Vector3 aimStart = (Vector3){.x = drone->pos.x, .y = 0.5, .z = drone->pos.y};
    Vector3 aimEnd = (Vector3){
        .x = drone->pos.x + aimGuideWidth * drone->lastAim.x,
        .y = 0.5,
        .z = drone->pos.y + aimGuideWidth * drone->lastAim.y};

    if (drone->weaponInfo->type == SNIPER_WEAPON) {
        DrawLine3D(
            aimStart,
            aimEnd,
            droneColor);
    }

    // subtly light up the laser aim guide if the drone's weapon is fully charged
    if (drone->weaponInfo->charge != 0.0f && drone->weaponCharge == drone->weaponInfo->charge) {
        Rectangle chargedAimGuide = {
            .x = drone->pos.x,
            .y = drone->pos.y,
            .width = aimGuideWidth * e->renderScale,
            .height = chargedAimGuideHeight * e->renderScale,
        };
        const Color chargedAimGuideColor = Fade(droneColor, 100.0f / 255.0f);
        // DrawRectanglePro(chargedAimGuide, (Vector2){.x = 0.0f, .y = (chargedAimGuideHeight / 2.0f) * e->renderScale}, aimAngle, chargedAimGuideColor);
        DrawCylinderWiresEx(
            aimStart,
            aimEnd,
            chargedAimGuideHeight,
            chargedAimGuideHeight,
            10,
            chargedAimGuideColor);
    }

    Rectangle aimGuide = {
        .x = rayX,
        .y = rayY,
        .width = aimGuideWidth * e->renderScale,
        .height = aimGuideHeight * e->renderScale,
    };
    // DrawRectanglePro(aimGuide, (Vector2){.x = 0.0f, .y = (aimGuideHeight / 2.0f) * e->renderScale}, aimAngle, droneColor);
    DrawCylinderWiresEx(
        aimStart,
        aimEnd,
        aimGuideHeight,
        aimGuideHeight,
        10,
        droneColor);
}

void renderDroneTrail(const env *e, const droneEntity *drone, Color droneColor) {
    if (drone->trailPoints.length < 2) {
        return;
    }

    const float trailWidth = DRONE_RADIUS;
    const float numPoints = drone->trailPoints.length;

    for (uint8_t i = 0; i < drone->trailPoints.length - 1; i++) {
        const Vector2 p0 = drone->trailPoints.points[i];
        const Vector2 p1 = drone->trailPoints.points[i + 1];

        // compute direction and a perpendicular vector
        Vector2 segment = Vector2Subtract(p1, p0);
        if (Vector2Length(segment) == 0) {
            continue;
        }
        segment = Vector2Normalize(segment);
        const Vector2 perp = {-segment.y, segment.x};

        // compute four vertices for the quad segment
        const Vector2 v0 = Vector2Add(p0, Vector2Scale(perp, trailWidth));
        const Vector2 v1 = Vector2Subtract(p0, Vector2Scale(perp, trailWidth));
        const Vector2 v2 = Vector2Add(p1, Vector2Scale(perp, trailWidth));
        const Vector2 v3 = Vector2Subtract(p1, Vector2Scale(perp, trailWidth));

        // draw the quad as two triangles
        const float alpha0 = 0.5f * ((float)(i + 1) / numPoints);
        const float alpha1 = 0.5f * ((float)(i + 2) / numPoints);
        // DrawTriangle(v0, v2, v1, Fade(droneColor, alpha0));
        // DrawTriangle(v1, v2, v3, Fade(droneColor, alpha1));
        DrawTriangle3D(
            (Vector3){.x = v0.x, .y = 0.5f, .z = v0.y},
            (Vector3){.x = v2.x, .y = 0.5f, .z = v2.y},
            (Vector3){.x = v1.x, .y = 0.5f, .z = v1.y},
            Fade(droneColor, alpha0));
        DrawTriangle3D(
            (Vector3){.x = v1.x, .y = 0.5f, .z = v1.y},
            (Vector3){.x = v2.x, .y = 0.5f, .z = v2.y},
            (Vector3){.x = v3.x, .y = 0.5f, .z = v3.y},
            Fade(droneColor, alpha1));
    }
}

void renderDrone(const env *e, const droneEntity *drone) {
    const Vector2 raylibPos = b2VecToRayVec(e, drone->pos);
    const Color droneColor = getDroneColor(drone->idx);
    renderDroneTrail(e, drone, droneColor);

    DrawSphereWires(
        (Vector3){.x = drone->pos.x, .y = 0.5f, .z = drone->pos.y},
        DRONE_RADIUS,
        8,
        8,
        droneColor);
    // DrawCircleV(raylibPos, DRONE_RADIUS * e->renderScale, droneColor);
    // DrawCircleV(raylibPos, DRONE_RADIUS * 0.8f * e->renderScale, BLACK);

    if (drone->shield != NULL) {
        // DrawCircleV(b2VecToRayVec(e, drone->shield->pos), DRONE_SHIELD_RADIUS * e->renderScale, Fade(droneColor, 0.5f));
        DrawSphereWires(
            (Vector3){.x = drone->shield->pos.x, .y = 0.5f, .z = drone->shield->pos.y},
            DRONE_SHIELD_RADIUS,
            6,
            8,
            Fade(droneColor, 0.5f));
    }
}

void renderDroneUI(const env *e, const droneEntity *drone) {
    // draw energy meter
    const float energyMeterInnerRadius = 0.6f * e->renderScale;
    const float energyMeterOuterRadius = 0.3f * e->renderScale;
    const Vector2 energyMeterOrigin = {.x = b2XToRayX(e, drone->pos.x), .y = b2YToRayY(e, drone->pos.y)};
    float energyMeterEndAngle = 360.f * drone->energyLeft;
    Color energyMeterColor = PUFF_WHITE;
    if (drone->shield != NULL) {
        energyMeterColor = bambooBrown;
    } else if (drone->energyFullyDepleted && drone->energyRefillWait != 0.0f) {
        energyMeterColor = bambooBrown;
        energyMeterEndAngle = 360.0f * (1.0f - (drone->energyRefillWait / (DRONE_ENERGY_REFILL_EMPTY_WAIT)));
    } else if (drone->energyFullyDepleted) {
        energyMeterColor = GRAY;
    }
    // TODO: Figure out how to draw energy meters
    // DrawRing(energyMeterOrigin, energyMeterInnerRadius, energyMeterOuterRadius, 0.0f, energyMeterEndAngle, 1, energyMeterColor);

    // draw burst charge indicator
    if (drone->chargingBurst) {
        const float alpha = min(drone->burstCharge + (50.0f / 255.0f), 1.0f);
        const Color burstChargeColor = Fade(PUFF_WHITE, alpha);
        const float burstChargeOuterRadius = ((DRONE_BURST_RADIUS_BASE * drone->burstCharge) + DRONE_BURST_RADIUS_MIN) * e->renderScale;
        const float burstChargeInnerRadius = burstChargeOuterRadius - (0.1f * e->renderScale);
        // TODO: Figure out how to draw energy meters
        // DrawRing(energyMeterOrigin, burstChargeInnerRadius, burstChargeOuterRadius, 0.0f, 360.0f, 1, burstChargeColor);
    }

    // draw ammo count
    const int bufferSize = 5;
    char ammoStr[bufferSize];
    snprintf(ammoStr, bufferSize, "%d", drone->ammo);
    float posX = drone->pos.x - 0.25;
    if (drone->ammo >= 10 || drone->ammo == INFINITE) {
        posX -= 0.25f;
    }
    // DrawText(ammoStr, b2XToRayX(e, posX), b2YToRayY(e, drone->pos.y + 1.5f), e->renderScale, PUFF_WHITE);

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
    // DrawRectangleLinesEx(outlineRec, e->renderScale / 20.0f, RAYWHITE);

    const float fillRecWidth = (drone->weaponCharge / maxCharge) * chargeMeterWidth;
    Rectangle fillRec = {
        .x = b2XToRayX(e, drone->pos.x - 1.0f),
        .y = b2YToRayY(e, drone->pos.y - (chargeMeterHeight / 2.0f) + 3.0f),
        .width = fillRecWidth * e->renderScale,
        .height = chargeMeterHeight * e->renderScale,
    };
    const Vector2 origin = {.x = 0.0f, .y = 0.0f};
    // DrawRectanglePro(fillRec, origin, 0.0f, RAYWHITE);
}

void renderProjectileTrail(const env *e, const projectileEntity *proj, const Color color) {
    if (proj->trailPoints.length < 2) {
        return; // need at least two points
    }

    const float maxWidth = proj->weaponInfo->radius;
    const float numPoints = proj->trailPoints.length;

    for (uint8_t i = 0; i < proj->trailPoints.length - 1; i++) {
        const Vector2 p0 = proj->trailPoints.points[i];
        const Vector2 p1 = proj->trailPoints.points[i + 1];

        // Compute a perpendicular vector for the segment
        Vector2 dir = Vector2Subtract(p1, p0);
        if (Vector2Length(dir) == 0) {
            continue; // Avoid division by zero
        }
        dir = Vector2Normalize(dir);
        const Vector2 perp = {-dir.y, dir.x};

        // Compute widths for the start and end of the segment.
        // Taper so that older segments are narrower.
        const float taper0 = (float)(i + 1) / numPoints;
        const float taper1 = (float)(i + 2) / numPoints;
        const float width0 = maxWidth * taper0;
        const float width1 = maxWidth * taper1;

        // Calculate two vertices on each side of the segment.
        const Vector2 v0 = Vector2Add(p0, Vector2Scale(perp, width0));
        const Vector2 v1 = Vector2Subtract(p0, Vector2Scale(perp, width0));
        const Vector2 v2 = Vector2Add(p1, Vector2Scale(perp, width1));
        const Vector2 v3 = Vector2Subtract(p1, Vector2Scale(perp, width1));

        // Draw two triangles for the quad and fade the color with distance so older parts are more transparent.
        // DrawTriangle(v0, v2, v1, Fade(color, taper0));
        // DrawTriangle(v1, v2, v3, Fade(color, taper1));
        DrawTriangle3D(
            (Vector3){.x = v0.x, .y = 0.5f, .z = v0.y},
            (Vector3){.x = v2.x, .y = 0.5f, .z = v2.y},
            (Vector3){.x = v1.x, .y = 0.5f, .z = v1.y},
            Fade(color, taper0));
        DrawTriangle3D(
            (Vector3){.x = v1.x, .y = 0.5f, .z = v1.y},
            (Vector3){.x = v2.x, .y = 0.5f, .z = v2.y},
            (Vector3){.x = v3.x, .y = 0.5f, .z = v3.y},
            Fade(color, taper1));
    }
}

void renderProjectiles(env *e) {
    for (size_t i = 0; i < cc_array_size(e->projectiles); i++) {
        projectileEntity *projectile = safe_array_get_at(e->projectiles, i);

        const Color color = getProjectileColor(projectile->weaponInfo->type);
        float adj = 2.0 - projectile->distance / 10.0f;
        if (adj < 1.0f) {
            adj = 1.0f;
        }
        float r = fminf(adj * (float)color.r, 255.0f);
        float g = fminf(adj * (float)color.g, 255.0f);
        float b = fminf(adj * (float)color.b, 255.0f);
        float a = color.a;

        Color adjustedColor = (Color){.r = r, .g = g, .b = b, .a = a};
        renderProjectileTrail(e, projectile, adjustedColor);
        // BeginShaderMode(e->client->bloom);
        // float pos[3] = {projectile->pos.x, 0.0f, projec1tile->pos.y};
        // SetShaderValue(e->client->bloom, e->client->bloomPosLoc, pos, SHADER_UNIFORM_VEC3);

        DrawSphere(
            (Vector3){.x = projectile->pos.x, .y = 0.5f, .z = projectile->pos.y},
            projectile->weaponInfo->radius,
            adjustedColor);
        // EndShaderMode();
    }
}

void renderBannerText(env *e, const bool starting, const int8_t winner, const int8_t winningTeam) {
    const int bufferSize = 16;
    char winStr[bufferSize];
    memset(winStr, 0x0, bufferSize);
    Color color = PUFF_WHITE;

    if (starting) {
        const char *text = "Ready?";
        strncpy(winStr, text, strlen(text));
    } else if (winner == -1 && winningTeam == -1) {
        const char *text = "Tie";
        strncpy(winStr, text, strlen(text));
    } else if (e->teamsEnabled) {
        snprintf(winStr, bufferSize, "Team %d wins!", winningTeam + 1);
    } else {
        snprintf(winStr, bufferSize, "Player %d wins!", winner + 1);
        color = getDroneColor(winner);
    }

    uint16_t fontSize = 5 * e->client->scale;
    uint16_t textWidth = MeasureText(winStr, fontSize);
    uint16_t posX = (e->client->halfWidth - (textWidth / 2));
    DrawText(winStr, posX, e->client->halfHeight, fontSize, color);
}

void minimalStepEnv(env *e) {
    for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead || drone->shield == NULL) {
            continue;
        }

        // update shield velocity if its active
        b2Body_SetLinearVelocity(drone->shield->bodyID, b2Body_GetLinearVelocity(drone->bodyID));
    };

    b2World_Step(e->worldID, e->deltaTime, e->box2dSubSteps);

    handleBodyMoveEvents(e);
    handleContactEvents(e);
    handleSensorEvents(e);

    projectilesStep(e);

    for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            continue;
        }
        droneStep(e, drone);
    }
}

// TODO:
// - add drone death animations
// - add info UI on sides:
//   - teams, weapon type, ammo, cooldown, type of player
void _renderEnv(env *e, const bool starting, const bool ending, const int8_t winner, const int8_t winningTeam) {
    e->client->lightIdx = 0;
    if (ending) {
        minimalStepEnv(e);
    }

    // UpdateCamera(&e->client->camera, CAMERA_ORBITAL);

    Vector3 camVec = e->client->camera.position;
    float cameraPos[3] = {camVec.x, camVec.y, camVec.z};
    SetShaderValue(e->client->shader, e->client->shader.locs[SHADER_LOC_VECTOR_VIEW],
                   cameraPos, SHADER_UNIFORM_VEC3);
    for (int i = 0; i < MAX_LIGHTS; i++) {
        e->client->lights[i].enabled = false;
    }

    for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        Light *light = &e->client->lights[e->client->lightIdx];
        if (drone->dead) {
            light->enabled = false;
            float gridPos[2] = {-1000, -1000};
            SetShaderValue(e->client->grid, e->client->gridPosLoc[drone->idx], gridPos, SHADER_UNIFORM_VEC2);
            continue;
        }
        e->client->lightIdx++;
        light->enabled = false;
        light->position.x = drone->pos.x;
        light->position.y = 5.0f;
        light->position.z = drone->pos.y;
        const Color droneColor = getDroneColor(drone->idx);
        light->color = (Color){0.5 * droneColor.r, 0.5 * droneColor.g, 0.5 * droneColor.b, droneColor.a};

        float gridPos[2] = {drone->pos.x, drone->pos.y};
        SetShaderValue(e->client->grid, e->client->gridPosLoc[drone->idx], gridPos, SHADER_UNIFORM_VEC2);
        float gridColor[4] = {droneColor.r, droneColor.g, droneColor.b, droneColor.a};
        SetShaderValue(e->client->grid, e->client->gridColorLoc[drone->idx], gridColor, SHADER_UNIFORM_VEC4);
    }

    for (size_t i = 0; i < cc_array_size(e->projectiles); i++) {
        projectileEntity *projectile = safe_array_get_at(e->projectiles, i);
        const Color color = getProjectileColor(projectile->weaponInfo->type);
        Light *light = &e->client->lights[e->client->lightIdx];
        e->client->lightIdx++;
        light->enabled = true;
        light->position.x = projectile->pos.x;
        light->position.y = 0.0f;
        light->position.z = projectile->pos.y;
        light->color = (Color){0.05 * color.r, 0.05 * color.g, 0.05 * color.b, color.a};
    }

    for (int i = 0; i < MAX_LIGHTS; i++) {
        UpdateLightValues(e->client->shader, e->client->lights[i]);
    }

    BeginDrawing();
    ClearBackground(PUFF_BACKGROUND);
#ifndef __EMSCRIPTEN__
    DrawFPS(e->renderScale, e->renderScale);
#endif

    renderUI(e, starting);

    // float cameraPos[3] = {e->client->camera.position.x, e->client->camera.position.y, e->client->camera.position.z};
    // SetShaderValue(e->client->shader, e->client->shader.locs[SHADER_LOC_VECTOR_VIEW],
    //               cameraPos, SHADER_UNIFORM_VEC3);

    BeginMode3D(e->client->camera);
    BeginShaderMode(e->client->shader);

    /*
    for (size_t i = 0; i < cc_array_size(e->cells); i++) {
        const mapCell *cell = safe_array_get_at(e->cells, i);
        if (cell->ent != NULL) {
            continue;
        }

        renderEmptyCell(e, cell->pos, i);
    }
    */

    DrawLine3D(
        (Vector3){.x = 0.0f, .y = -4.0f, .z = 0.0f},
        (Vector3){.x = 0.0f, .y = 0.0f, .z = 0.0f},
        PUFF_WHITE);

    float y = -WALL_THICKNESS;
    Color color = (Color){.r = 0.8 * PUFF_CYAN.r, .g = 0.8 * PUFF_CYAN.g, .b = 0.8 * PUFF_CYAN.b, .a = PUFF_CYAN.a};

    DrawPlane(
        (Vector3){.x = 0.0f, .y = -WALL_THICKNESS - 1.0f, .z = 0.0f},
        (Vector2){.x = WALL_THICKNESS * e->map->columns, .y = WALL_THICKNESS * e->map->rows},
        PUFF_BACKGROUND);

    for (int i = 0; i < e->map->columns; i++) {
        float d = WALL_THICKNESS * e->map->columns;
        DrawLine3D(
            (Vector3){.x = -d / 2.0f, .y = y, .z = WALL_THICKNESS * i - d / 2.0f},
            (Vector3){.x = (d - WALL_THICKNESS) / 2.0f, .y = y, .z = WALL_THICKNESS * i - d / 2.0f},
            color);
    }
    for (int i = 0; i < e->map->rows; i++) {
        float d = WALL_THICKNESS * e->map->rows;
        DrawLine3D(
            (Vector3){.x = WALL_THICKNESS * i - d / 2.0f, .y = y, .z = -d / 2.0f},
            (Vector3){.x = WALL_THICKNESS * i - d / 2.0f, .y = y, .z = (d - WALL_THICKNESS) / 2.0f},
            color);
    }

    // Smaller higher grid
    EndShaderMode();
    BeginBlendMode(BLEND_ALPHA);
    BeginShaderMode(e->client->grid);
    y = 0.0f;
    color = PUFF_BACKGROUND;
    for (int i = 0; i < 2 * e->map->columns; i++) {
        float d = WALL_THICKNESS * e->map->columns;
        DrawLine3D(
            (Vector3){.x = -d / 2.0f, .y = y, .z = WALL_THICKNESS / 2.0f * i - d / 2.0f},
            (Vector3){.x = (d - WALL_THICKNESS / 2.0f) / 2.0f, .y = y, .z = WALL_THICKNESS / 2.0f * i - d / 2.0f},
            color);
    }
    for (int i = 0; i < 2 * e->map->rows; i++) {
        float d = WALL_THICKNESS * e->map->rows;
        DrawLine3D(
            (Vector3){.x = WALL_THICKNESS * i / 2.0f - d / 2.0f, .y = y, .z = -d / 2.0f},
            (Vector3){.x = WALL_THICKNESS * i / 2.0f - d / 2.0f, .y = y, .z = (d - WALL_THICKNESS / 2.0f) / 2.0f},
            color);
    }
    EndShaderMode();
    EndBlendMode();

    for (size_t i = 0; i < cc_array_size(e->pickups); i++) {
        const weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
        renderWeaponPickup(e, pickup);
    }

    renderBrakeTrails(e);
    renderDronePieces(e);

    for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            continue;
        }
        renderDroneGuides(e, drone, ending);
    }
    for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            continue;
        }
        renderDrone(e, drone);
    }

    for (size_t i = 0; i < cc_array_size(e->walls); i++) {
        const wallEntity *wall = safe_array_get_at(e->walls, i);
        renderWall(e, wall);
    }

    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
        const wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        renderWall(e, wall);
    }

    for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            continue;
        }
        renderDroneRespawnGuides(e, drone);
    }

    BeginBlendMode(BLEND_ALPHA);
    renderProjectiles(e);
    EndBlendMode();

    BeginShaderMode(e->client->shader);
    renderExplosions(e);

    EndShaderMode();
    EndMode3D();

    for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            continue;
        }
        renderDroneUI(e, drone);
    }

    if (starting || ending) {
        renderBannerText(e, starting, winner, winningTeam);
    }

    // if (!b2VecEqual(e->debugPoint, b2Vec2_zero)) {
    //     DrawCircleV(b2VecToRayVec(e, e->debugPoint), DRONE_RADIUS * 0.5f * e->renderScale, WHITE);
    // }

    EndDrawing();
}

void renderWait(env *e, const bool starting, const bool ending, const int8_t winner, const int8_t winningTeam, const float time) {
#ifdef __EMSCRIPTEN__
    const double startTime = emscripten_get_now();
    while (time > (emscripten_get_now() - startTime) / 1000.0) {
        _renderEnv(e, starting, ending, winner, winningTeam);
        emscripten_sleep(e->deltaTime * 1000.0);
    }
#else
    for (uint16_t i = 0; i < (uint16_t)(time * e->frameRate); i++) {
        _renderEnv(e, starting, ending, winner, winningTeam);
    }
#endif
}

void renderEnv(env *e, const bool starting, const bool ending, const int8_t winner, const int8_t winningTeam) {
    if (starting) {
        renderWait(e, starting, ending, winner, winningTeam, START_READY_TIME);
    } else if (ending) {
        renderWait(e, starting, ending, winner, winningTeam, END_WAIT_TIME);
    } else {
        _renderEnv(e, starting, ending, winner, winningTeam);
    }
}

#endif
