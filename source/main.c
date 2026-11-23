#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_W 400
#define SCREEN_H 240
#define TILE_SIZE 20
#define COLS (SCREEN_W / TILE_SIZE)
#define ROWS (SCREEN_H / TILE_SIZE)

int level[ROWS][COLS];
float playerX = 24.0f, playerY = 160.0f;
float velX = 0.0f, velY = 0.0f;
const float playerSize = 14.0f;
bool onGround = false;

void generateLevel() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            level[r][c] = 0;

    for (int c = 0; c < COLS; c++) {
        if (c < 3 || c > COLS - 3 || (rand() % 5 != 0)) {
            level[ROWS - 1][c] = 1;
            if (c > 3 && c < COLS - 3 && (rand() % 6 == 0))
                level[ROWS - 2][c] = 2;
        }
    }

    for (int r = 4; r < ROWS - 2; r += 3) {
        for (int c = 3; c < COLS - 3; c++) {
            if (rand() % 4 == 0) {
                level[r][c] = 1;
                if (rand() % 5 == 0 && level[r - 1][c] == 0)
                    level[r - 1][c] = 2;
            }
        }
    }

    playerX = 24.0f;
    playerY = (ROWS - 2) * TILE_SIZE - playerSize;
    velX = velY = 0.0f;
}

bool checkSolidCollision(float x, float y) {
    int left   = (int)(x / TILE_SIZE);
    int right  = (int)((x + playerSize - 0.1f) / TILE_SIZE);
    int top    = (int)(y / TILE_SIZE);
    int bottom = (int)((y + playerSize - 0.1f) / TILE_SIZE);

    for (int r = top; r <= bottom; r++) {
        for (int c = left; c <= right; c++) {
            if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
                if (level[r][c] == 1) return true;
            }
        }
    }
    return false;
}

// Accurate triangle hitbox matching the drawn red spike
bool checkSpikeCollision(float x, float y) {
    float pLeft   = x + 1.5f;
    float pRight  = x + playerSize - 1.5f;
    float pTop    = y + 1.5f;
    float pBottom = y + playerSize - 1.0f;

    int left   = (int)(pLeft / TILE_SIZE);
    int right  = (int)(pRight / TILE_SIZE);
    int top    = (int)(pTop / TILE_SIZE);
    int bottom = (int)(pBottom / TILE_SIZE);

    for (int r = top; r <= bottom; r++) {
        for (int c = left; c <= right; c++) {
            if (r >= 0 && r < ROWS && c >= 0 && c < COLS && level[r][c] == 2) {
                float tx = c * TILE_SIZE;
                float ty = r * TILE_SIZE;

                // Drawn triangle bounds: apex (tx + 10, ty + 2), base (tx + 2..tx + 18, ty + 20)
                float sTop = ty + 3.0f;
                float sBottom = ty + TILE_SIZE;
                if (pBottom <= sTop || pTop >= sBottom) continue;

                // Lowest player Y overlapping the spike determines widest horizontal span
                float testY = (pBottom < sBottom) ? pBottom : sBottom;
                float progress = (testY - (ty + 2.0f)) / (TILE_SIZE - 2.0f); // 0 at tip, 1 at base
                if (progress < 0.0f) progress = 0.0f;
                if (progress > 1.0f) progress = 1.0f;

                float halfWidth = progress * ((TILE_SIZE / 2.0f) - 2.5f);
                float centerX = tx + (TILE_SIZE / 2.0f);
                float sLeft = centerX - halfWidth;
                float sRight = centerX + halfWidth;

                if (pRight > sLeft && pLeft < sRight) {
                    return true;
                }
            }
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    consoleInit(GFX_BOTTOM, NULL);
    printf("3DS Random Platformer\nMove: D-Pad / Circle Pad\nJump: A | New Level: Y\nExit: START\n");

    srand(time(NULL));
    generateLevel();

    u32 clrWhite  = C2D_Color32(255, 255, 255, 255);
    u32 clrBlack  = C2D_Color32(0, 0, 0, 255);
    u32 clrRed    = C2D_Color32(230, 30, 30, 255);
    u32 clrPlayer = C2D_Color32(30, 120, 255, 255);

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;
        if (kDown & KEY_Y) generateLevel();

        velX = 0.0f;
        if (kHeld & KEY_LEFT)  velX = -2.5f;
        if (kHeld & KEY_RIGHT) velX =  2.5f;

        playerX += velX;
        if (playerX < 0) playerX = 0;
        if (checkSolidCollision(playerX, playerY)) playerX -= velX;

        if (playerX + playerSize >= SCREEN_W) generateLevel();

        if ((kDown & KEY_A) && onGround) {
            velY = -6.2f;
            onGround = false;
        }
        velY += 0.35f;
        if (velY > 7.0f) velY = 7.0f;

        playerY += velY;
        onGround = false;
        if (checkSolidCollision(playerX, playerY)) {
            if (velY > 0) onGround = true;
            playerY -= velY;
            velY = 0.0f;
        }

        if (playerY > SCREEN_H || checkSpikeCollision(playerX, playerY)) {
            playerX = 24.0f;
            playerY = (ROWS - 2) * TILE_SIZE - playerSize;
            velY = 0.0f;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, clrWhite);
        C2D_SceneBegin(top);

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                float tx = c * TILE_SIZE;
                float ty = r * TILE_SIZE;
                if (level[r][c] == 1) {
                    C2D_DrawRectSolid(tx, ty, 0.5f, TILE_SIZE, TILE_SIZE, clrBlack);
                } else if (level[r][c] == 2) {
                    C2D_DrawTriangle(
                        tx + TILE_SIZE / 2.0f, ty + 2.0f, clrRed,
                        tx + 2.0f, ty + TILE_SIZE, clrRed,
                        tx + TILE_SIZE - 2.0f, ty + TILE_SIZE, clrRed,
                        0.5f
                    );
                }
            }
        }

        C2D_DrawRectSolid(playerX, playerY, 0.6f, playerSize, playerSize, clrPlayer);
        C3D_FrameEnd(0);
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
