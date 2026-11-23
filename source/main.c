#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_W 400
#define SCREEN_H 240
#define TILE_SIZE 20
#define COLS 120
#define ROWS 12

int level[ROWS][COLS];
float playerX = 40.0f, playerY = 160.0f, velX = 0.0f, velY = 0.0f;
const float playerSize = 14.0f;
bool onGround = false, touchL = false, touchR = false;
int levelNum = 1, deaths = 0;

void updateHUD() {
    consoleClear();
    printf("=== 3DS RANDOM PLATFORMER ===\nLevel: %d | Deaths: %d\n\n", levelNum, deaths);
    printf("Move: D-Pad / Circle Pad\nJump: A (Infinite Wall-Bounce!)\nNew Map: Y | Exit: START\n\nReach the GREEN portal to warp!\n");
}

void generateLevel() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            level[r][c] = 0;

    for (int r = 0; r < ROWS; r++) {
        level[r][0] = 1;
        level[r][COLS - 1] = 1;
    }

    int gap = 0;
    for (int c = 1; c < COLS - 1; c++) {
        if (c < 6 || c > COLS - 7) {
            level[ROWS - 1][c] = 1;
            gap = 0;
        } else if (gap < 3 && (rand() % 5 == 0)) {
            level[ROWS - 1][c] = 0;
            gap++;
        } else {
            level[ROWS - 1][c] = 1;
            gap = 0;
            if (c > 7 && c < COLS - 8 && level[ROWS - 2][c - 1] != 2 && (rand() % 6 == 0))
                level[ROWS - 2][c] = 2;
        }
    }

    for (int c = 10; c < COLS - 10; c += 7 + (rand() % 6)) {
        int h = 3 + (rand() % 5);
        int sr = ROWS - 2 - (rand() % 2) * 2;
        for (int i = 0; i < h; i++)
            if (sr - i >= 2 && sr - i < ROWS - 1) level[sr - i][c] = 1;
    }

    for (int r = 3; r <= 8; r += 3) {
        for (int c = 6; c < COLS - 8; c++) {
            if (level[r][c] == 0 && (rand() % 5 == 0)) {
                int len = 2 + (rand() % 3);
                for (int k = 0; k < len && (c + k) < COLS - 7; k++) {
                    if (level[r][c + k] == 0) {
                        level[r][c + k] = 1;
                        if (k == 1 && (rand() % 4 == 0) && level[r - 1][c + k] == 0)
                            level[r - 1][c + k] = 2;
                    }
                }
                c += len + 2;
            }
        }
    }

    for (int r = ROWS - 4; r < ROWS - 1; r++)
        for (int c = 1; c <= 4; c++) level[r][c] = 0;

    int gc = COLS - 4, gr = ROWS - 2 - (rand() % 4);
    for (int r = gr - 1; r <= gr + 1; r++)
        for (int c = gc - 1; c <= gc + 1; c++)
            if (r >= 1 && r < ROWS - 1 && c >= 1 && c < COLS - 1) level[r][c] = 0;
    level[gr][gc] = 3;
    if (gr < ROWS - 2) level[gr + 1][gc] = 1;

    playerX = 36.0f;
    playerY = (ROWS - 2) * TILE_SIZE - playerSize;
    velX = velY = 0.0f;
    updateHUD();
}

bool checkTile(float x, float y, int t) {
    int l = (int)(x / TILE_SIZE), r = (int)((x + playerSize - 0.1f) / TILE_SIZE);
    int tp = (int)(y / TILE_SIZE), b = (int)((y + playerSize - 0.1f) / TILE_SIZE);
    for (int row = tp; row <= b; row++)
        for (int col = l; col <= r; col++)
            if (row >= 0 && row < ROWS && col >= 0 && col < COLS && level[row][col] == t) return true;
    return false;
}

bool checkSpike(float x, float y) {
    float pL = x + 1.5f, pR = x + playerSize - 1.5f, pT = y + 1.5f, pB = y + playerSize - 1.0f;
    for (int r = (int)(pT / TILE_SIZE); r <= (int)(pB / TILE_SIZE); r++) {
        for (int c = (int)(pL / TILE_SIZE); c <= (int)(pR / TILE_SIZE); c++) {
            if (r >= 0 && r < ROWS && c >= 0 && c < COLS && level[r][c] == 2) {
                float tx = c * TILE_SIZE, ty = r * TILE_SIZE;
                if (pB <= ty + 3.0f || pT >= ty + TILE_SIZE) continue;
                float testY = (pB < ty + TILE_SIZE) ? pB : (ty + TILE_SIZE);
                float prog = (testY - (ty + 2.0f)) / (TILE_SIZE - 2.0f);
                if (prog < 0.0f) prog = 0.0f;
                if (prog > 1.0f) prog = 1.0f;
                float hw = prog * ((TILE_SIZE / 2.0f) - 2.5f);
                float cx = tx + (TILE_SIZE / 2.0f);
                if (pR > cx - hw && pL < cx + hw) return true;
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
    srand(time(NULL));
    generateLevel();

    u32 cW = C2D_Color32(255,255,255,255), cB = C2D_Color32(0,0,0,255), cR = C2D_Color32(230,30,30,255);
    u32 cG = C2D_Color32(20,210,90,255), cG2 = C2D_Color32(180,255,200,255), cP = C2D_Color32(30,120,255,255);

    while (aptMainLoop()) {
        hidScanInput();
        u32 kD = hidKeysDown(), kH = hidKeysHeld();
        if (kD & KEY_START) break;
        if (kD & KEY_Y) generateLevel();

        float tVX = ((kH & KEY_LEFT) ? -2.8f : 0.0f) + ((kH & KEY_RIGHT) ? 2.8f : 0.0f);
        velX += (velX < tVX) ? 0.45f : (velX > tVX ? -0.45f : 0.0f);
        if ((tVX == 0.0f && velX > -0.45f && velX < 0.45f)) velX = 0.0f;

        playerX += velX;
        if (checkTile(playerX, playerY, 1)) { playerX -= velX; velX = 0.0f; }

        touchL = checkTile(playerX - 1.0f, playerY, 1);
        touchR = checkTile(playerX + 1.0f, playerY, 1);

        if (kD & KEY_A) {
            if (onGround) { velY = -6.4f; onGround = false; }
            else if (touchL) { velY = -6.0f; velX = 3.6f; }
            else if (touchR) { velY = -6.0f; velX = -3.6f; }
        }

        velY += 0.35f;
        bool slide = !onGround && velY > 0 && ((touchL && (kH & KEY_LEFT)) || (touchR && (kH & KEY_RIGHT)));
        if (velY > (slide ? 1.8f : 7.0f)) velY = (slide ? 1.8f : 7.0f);

        playerY += velY;
        onGround = false;
        if (checkTile(playerX, playerY, 1)) {
            if (velY > 0) onGround = true;
            playerY -= velY;
            velY = 0.0f;
        }

        if (checkTile(playerX, playerY, 3)) { levelNum++; generateLevel(); }
        if (playerY > SCREEN_H || checkSpike(playerX, playerY)) {
            deaths++; playerX = 36.0f; playerY = (ROWS - 2) * TILE_SIZE - playerSize; velX = velY = 0.0f; updateHUD();
        }

        float camX = playerX + (playerSize / 2.0f) - (SCREEN_W / 2.0f);
        if (camX < 0.0f) camX = 0.0f;
        if (camX > (COLS * TILE_SIZE) - SCREEN_W) camX = (COLS * TILE_SIZE) - SCREEN_W;

        int sC = (int)(camX / TILE_SIZE), eC = sC + (SCREEN_W / TILE_SIZE) + 1;
        if (eC >= COLS) eC = COLS - 1;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, cW);
        C2D_SceneBegin(top);
        for (int r = 0; r < ROWS; r++) {
            for (int c = sC; c <= eC; c++) {
                int t = level[r][c];
                if (!t) continue;
                float tx = (c * TILE_SIZE) - camX, ty = r * TILE_SIZE;
                if (t == 1) C2D_DrawRectSolid(tx, ty, 0.5f, TILE_SIZE, TILE_SIZE, cB);
                else if (t == 2) C2D_DrawTriangle(tx + 10.0f, ty + 2.0f, cR, tx + 2.0f, ty + 20.0f, cR, tx + 18.0f, ty + 20.0f, cR, 0.5f);
                else if (t == 3) {
                    C2D_DrawRectSolid(tx + 1.0f, ty + 1.0f, 0.5f, 18.0f, 18.0f, cG);
                    C2D_DrawRectSolid(tx + 5.0f, ty + 5.0f, 0.55f, 10.0f, 10.0f, cG2);
                }
            }
        }
        C2D_DrawRectSolid(playerX - camX, playerY, 0.6f, playerSize, playerSize, cP);
        C3D_FrameEnd(0);
    }
    C2D_Fini(); C3D_Fini(); gfxExit();
    return 0;
}
