#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

#define LOGGING 0
#define LOG(...) if (LOGGING) printf(__VA_ARGS__)

#include "camera.c"
#include "coordinate.c"
#include "stb.c"
#include "tiles.c"

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

typedef struct {
    Coordinate pos;
    Color      color;
} Player;

typedef struct {
    Player    player;
    GameCamera camera;
    Tiles tiles;
} State;

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

    LOG("NUM_TILES: %d\n", NUM_TILES);
    state.tiles = tiles_generate(NUM_GRASS_PATCHES, TILES_WIDE, TILES_HIGH);


    while (state.tiles.arr[coord_to_index(state.player.pos.x, state.player.pos.y, state.tiles.wide)].type == TREE_TRUNK) {
        state.player.pos.y -= 1;
    }

    SetTargetFPS(60);

    // int camera_extra_tiles = 0;
    while (!WindowShouldClose()) {
        GameCamera *camera = &state.camera;

        // Handle input
        Coordinate target_pos = state.player.pos;
        if (IsKeyDown(KEY_W)) target_pos.y -= 1;
        if (IsKeyDown(KEY_S)) target_pos.y += 1;
        if (IsKeyDown(KEY_A)) target_pos.x -= 1;
        if (IsKeyDown(KEY_D)) target_pos.x += 1;
        if (IsKeyPressed(KEY_R)) {
            free(state.tiles.arr);
            state.tiles = tiles_generate(NUM_GRASS_PATCHES, state.tiles.wide, state.tiles.wide);
        }
        if (IsKeyPressed(KEY_Z)) {
            camera->area.width  -= camera->area.width / 10;
            camera->area.height -= camera->area.height / 10;
        }
        if (IsKeyPressed(KEY_Y)) {
            camera->area.width += camera->area.width / 10;
            camera->area.height += camera->area.height / 10;
        }

        // END handle input
        // Update state
        int target_i = coord_to_index(target_pos.x, target_pos.y, state.tiles.wide);
        if (state.tiles.arr[target_i].type != TREE_TRUNK) {
            state.player.pos = target_pos;
        }

        uint32_t padding_x = camera_padding_wide(state.camera);
        uint32_t padding_y = camera_padding_high(state.camera);
        Coordinate pos = state.player.pos;
        int player_i = coord_to_index(pos.x, pos.y, state.tiles.wide);
        state.tiles.arr[player_i].walked_on = true;

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

        int ending_tile_y = camera->area.y + camera->area.height + camera_extra_tiles_high(state.camera);
        int ending_tile_x = camera->area.x + camera->area.width  + camera_extra_tiles_wide(state.camera);

        int tile_width  = camera_tile_width(state.camera);
        int tile_height = camera_tile_height(state.camera);

        LOG("state.camera.area.x: %f\n", state.camera.area.x);
        LOG("state.camera.area.width: %f\n", state.camera.area.width);
        LOG("camera_tile_width: %d\n", tile_width);

        for (int y = camera->area.y; y < ending_tile_y; y++) {
            for (int x = camera->area.x; x < ending_tile_x; x++) {
                int i = coord_to_index(x, y, state.tiles.wide);

                Tile tile = state.tiles.arr[i];
                Color c;
                switch(tile.type) {
                case DIRT: {
                    c = BROWN;
                } break;
                case GRASS: {
                    c = DARKGREEN;
                } break;
                case TREE: {
                    c = DARKGREEN;
                    c.a -= 40;
                } break;
                case TREE_TRUNK: {
                    c = DARKBROWN;
                } break;
                }

                if (tile.walked_on) {
                    c.a -= 20;
                }

                int pixel_x = (x - camera->area.x) * tile_width;
                int pixel_y = (y - camera->area.y) * tile_height;

                DrawRectangle(pixel_x,
                              pixel_y,
                              tile_width,
                              tile_height,
                              c);
            }
        }

        LOG("Player position: x:%d y:%d\n", pos.x, pos.y);
        DrawRectangle((state.player.pos.x - camera->area.x) * tile_width,
                      (state.player.pos.y - camera->area.y) * tile_height,
                      tile_width,
                      tile_height,
                      state.player.color);


        DrawFPS(10, 10);

        EndDrawing();
        // End draw
    }

    return 0;
}
