#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

#include "camera.c"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define TILE_WIDTH  10
#define TILE_HEIGHT 10
#define PADDING     5

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define TILE_MOD 50
#define TILES_WIDE (SCREEN_WIDTH / TILE_WIDTH) * TILE_MOD
#define TILES_HIGH (SCREEN_HEIGHT / TILE_HEIGHT) * TILE_MOD
#define NUM_TILES TILES_WIDE * TILES_HIGH

#define CAMERA_PADDING 5

// Arbitrary
#define MAX_GRASS_SIZE    25
#define NUM_GRASS_PATCHES NUM_TILES / 5

#define LOGGING 1
#define LOG(...) if (LOGGING) printf(__VA_ARGS__)

typedef struct {
    int x;
    int y;
} Coordinate;

typedef enum {
    DIRT,
    GRASS
} TileType;

typedef struct {
    TileType type;
    bool     walked_on;
} Tile;

typedef struct {
    Coordinate pos;
    Color      color;
} Player;

typedef struct {
    Player    player;
    // x = starting tile in x axis
    // y = starting tile in y axis
    // width = width in tiles
    // height = height in tiles
    GameCamera camera;
    Tile *tiles;
} State;

size_t coord_to_index(int x, int y, int row_len)
{
    return x + (row_len * y);
}

Coordinate index_to_coord(int i, int row_len)
{
    int y = i / row_len;
    int x = i % row_len;
    return (Coordinate) {.x=x, .y=y};
}


Tile *tiles_generate(int num_grass_patches)
{
    Tile *tiles = malloc(sizeof(Tile) * NUM_TILES);
    memset(tiles, 0, sizeof(Tile) * NUM_TILES);
    for (int grass_patch = 0; grass_patch < num_grass_patches; grass_patch++) {
        int patch_center = rand() % NUM_TILES;
        int patch_size = (rand() % MAX_GRASS_SIZE) + 1;
        Coordinate coord = index_to_coord(patch_center, TILES_WIDE);
        for (int x = coord.x - patch_size; x < coord.x + patch_size; x++) {
            if (x < 0) continue;
            if (TILES_WIDE <= x) continue;

            for (int y = coord.y - patch_size; y < coord.y + patch_size; y++) {
                if (y < 0) continue;
                if (TILES_HIGH <= y) continue;

                int i = coord_to_index(x, y, TILES_WIDE);
                tiles[i].type = GRASS;
            }
        }
    }
    LOG("Generated %d tiles!\n", NUM_TILES);
    return tiles;
}

int main(void)
{
    srand(time(NULL));
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Explore");

    State state = {0};
    state.player = (Player){
        .pos = (Coordinate) {
            .x = 110,
            .y = 110,
        },
        .color = GREEN,
    };
    state.camera = (GameCamera) {
        .area =(Rectangle){
            .x      = 100,
            .y      = 100,
            .width  = SCREEN_WIDTH / 10,
            .height = SCREEN_HEIGHT / 10,
        },
        .screen_width = SCREEN_WIDTH,
        .screen_height = SCREEN_HEIGHT
    };
    state.tiles = tiles_generate(NUM_GRASS_PATCHES);

    SetTargetFPS(60);

    // int camera_extra_tiles = 0;
    while (!WindowShouldClose()) {
        // Handle input
        if (IsKeyDown(KEY_W)) state.player.pos.y -= 1;
        if (IsKeyDown(KEY_S)) state.player.pos.y += 1;
        if (IsKeyDown(KEY_A)) state.player.pos.x -= 1;
        if (IsKeyDown(KEY_D)) state.player.pos.x += 1;
        if (IsKeyPressed(KEY_R)) {
            free(state.tiles);
            state.tiles = tiles_generate(NUM_GRASS_PATCHES);
        }
        if (IsKeyPressed(KEY_Z)) {
            state.camera.area.width  -= state.camera.area.width / 10;
            state.camera.area.height -= state.camera.area.height / 10;
        }
        if (IsKeyPressed(KEY_Y)) {
            state.camera.area.width += state.camera.area.width / 10;
            state.camera.area.height += state.camera.area.height / 10;
        }

        // END handle input
        // Update state
        GameCamera *camera = &state.camera;
        uint32_t padding_x = camera_padding_wide(state.camera);
        uint32_t padding_y = camera_padding_high(state.camera);
        Coordinate pos = state.player.pos;
        int player_i = coord_to_index(pos.x, pos.y, TILES_WIDE);
        state.tiles[player_i].walked_on = true;

        if (pos.x < camera->area.x + padding_x) {
            camera->area.x -= padding_x * 2;
        }
        if (camera->area.x + camera->area.width < pos.x + padding_x) {
            camera->area.x += padding_x * 2;
        }
        if (pos.y < camera->area.y + padding_y) {
            camera->area.y -= padding_y * 2;
        }
        if (camera->area.y + camera->area.height < pos.y + padding_y) {
            camera->area.y += padding_y * 2;
        }
        // End update state

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        int ending_tile_y      = state.camera.area.y + state.camera.area.height + camera_extra_tiles_high(state.camera);
        int ending_tile_x      = state.camera.area.x + state.camera.area.width  + camera_extra_tiles_wide(state.camera);

        int tile_width  = camera_tile_width(state.camera);
        int tile_height = camera_tile_height(state.camera);

        LOG("state.camera.area.x: %f\n", state.camera.area.x);
        LOG("state.camera.area.width: %f\n", state.camera.area.width);
        LOG("camera_tile_width: %d\n", tile_width);

        for (int y = state.camera.area.y; y < ending_tile_y; y++) {
            for (int x = state.camera.area.x; x < ending_tile_x; x++) {
                int i = coord_to_index(x, y, TILES_WIDE);

                Tile tile = state.tiles[i];
                Color c;
                switch(tile.type) {
                case DIRT: {
                    c = BROWN;
                } break;
                case GRASS: {
                    c = DARKGREEN;
                } break;
                }
                if (tile.walked_on) {
                    c.a -= 20;
                }

                int pixel_x = (x - state.camera.area.x) * tile_width;
                int pixel_y = (y - state.camera.area.y) * tile_height;

                DrawRectangle(pixel_x,
                              pixel_y,
                              tile_width,
                              tile_height,
                              c);
            }
        }

        LOG("Player position: x:%d y:%d\n", pos.x, pos.y);
        DrawRectangle((state.player.pos.x - state.camera.area.x) * tile_width,
                      (state.player.pos.y - state.camera.area.y) * tile_height,
                      tile_width,
                      tile_height,
                      state.player.color);


        DrawFPS(10, 10);

        EndDrawing();
        // End draw
    }

    return 0;
}
