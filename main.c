#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

#define TILE_WIDTH  10
#define TILE_HEIGHT 10
#define PADDING     5

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define TILES_WIDE SCREEN_WIDTH / TILE_WIDTH
#define TILES_HIGH SCREEN_HEIGHT / TILE_HEIGHT
#define NUM_TILES TILES_WIDE * TILES_HIGH

// Arbitrary
#define MAX_GRASS_SIZE    25
#define NUM_GRASS_PATCHES 10

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
    Rectangle camera;
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
    return tiles;
}

int main(void)
{
    srand(time(NULL));
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Explore");

    Tile *tiles = tiles_generate(NUM_GRASS_PATCHES);

    State state = {0};
    state.player = (Player){
        .pos = (Coordinate) {
            .x = 5,
            .y = 5,
        },
        .color = GREEN,
    };
    state.camera = (Rectangle){
        .x      = 0,
        .y      = 0,
        .width  = TILES_WIDE,
        .height = TILES_HIGH,
    };

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_W)) state.player.pos.y -= 1;
        if (IsKeyPressed(KEY_S)) state.player.pos.y += 1;
        if (IsKeyPressed(KEY_A)) state.player.pos.x -= 1;
        if (IsKeyPressed(KEY_D)) state.player.pos.x += 1;
        if (IsKeyPressed(KEY_R)) {
            free(tiles);
            tiles = tiles_generate(NUM_GRASS_PATCHES);
        }
        if (IsKeyPressed(KEY_Z)) {
            state.camera.width -= 1;
            state.camera.height -= 1;
        }
        if (IsKeyPressed(KEY_Y)) {
            state.camera.width += 1;
            state.camera.height += 1;
        }

        int i = coord_to_index(state.player.pos.x, state.player.pos.y, TILES_WIDE);
        tiles[i].walked_on = true;

        BeginDrawing();
        ClearBackground(BLACK);

        int starting_tile_y         = state.camera.y;
        int starting_tile_x         = state.camera.x;
        int ending_tile_y           = state.camera.y + state.camera.height;
        int ending_tile_x           = state.camera.x + state.camera.width;
        int camera_tile_width     = (float)SCREEN_WIDTH / (float)state.camera.width;
        int camera_tile_height    = (float)SCREEN_HEIGHT / (float)state.camera.height;

        printf("state.camera.width: %f\n", state.camera.width);
        printf("camera_tile_width: %d\n", camera_tile_width);

        for (int y = starting_tile_y; y < ending_tile_y; y++) {
            for (int x = starting_tile_x; x < ending_tile_x; x++) {
                int abs_y = y + starting_tile_y;
                int abs_x = x + starting_tile_x;
                int i = coord_to_index(abs_x, abs_y, TILES_WIDE);

                Tile tile = tiles[i];
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

                int pixel_x = x * camera_tile_width;
                int pixel_y = y * camera_tile_height;

                DrawRectangle(pixel_x,
                              pixel_y,
                              camera_tile_width,
                              camera_tile_height,
                              c);
            }
        }

        DrawRectangle(state.player.pos.x * camera_tile_width,
                      state.player.pos.y * camera_tile_height,
                      camera_tile_width,
                      camera_tile_height,
                      state.player.color);

        DrawFPS(10, 10);

        EndDrawing();
    }

    return 0;
}
