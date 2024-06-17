#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

#define LOGGING 1
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
//#define TILE_MOD 50
#define TILE_MOD 5
#define TILES_WIDE (SCREEN_WIDTH / TILE_WIDTH) * TILE_MOD
#define TILES_HIGH (SCREEN_HEIGHT / TILE_HEIGHT) * TILE_MOD
#define NUM_TILES TILES_WIDE * TILES_HIGH

typedef struct {
    Coordinate pos;
    Coordinate cluster_id;
    Color      color;
} Player;

typedef struct {
    // NOTE(grant): the player operates in clustered tile coordinates.
    Player    player;
    // NOTE(grant): the camera operates in absolute tile coordinates. It doesn't care about clusters.
    GameCamera camera;
    Tiles tiles;
    bool debug_mode;
    bool speedup;
} State;

int main(void)
{
    srand(time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Explore");

    State state = {0};
    state.player = (Player){
        .pos = (Coordinate) {
            .x = 110,
            .y = 110,
        },
        .cluster_id = (Coordinate) {
            .x = 0,
            .y = 0,
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

    {
        // In case the player spawns in unpassable terrain, we need to move it.
        int x = state.player.pos.x;
        int *y = &state.player.pos.y;
        while (state.tiles.clusters[0].tiles[coord_to_index(x, *y, state.tiles.wide)].type == TREE_TRUNK) {
            state.player.pos.y -= 1;
        }
    }

    SetTargetFPS(60);
    Tiles *tiles = &state.tiles;
    Player *player = &state.player;

    while (!WindowShouldClose()) {
        GameCamera *camera = &state.camera;

        // Handle input
        state.camera.screen_width = GetScreenWidth();
        state.camera.screen_height = GetScreenHeight();

        bool toggled_debug_mode = false;
        if (IsKeyPressed(KEY_D) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
            state.debug_mode = !state.debug_mode;
            toggled_debug_mode = true;
        }
        bool toggled_speedup = false;
        if (IsKeyPressed(KEY_S) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
            state.speedup = !state.speedup;
            toggled_speedup = true;
        }

        Coordinate target_pos = player->pos;
        if (IsKeyDown(KEY_W))                        target_pos.y -= state.speedup ? 5 : 1;
        if (IsKeyDown(KEY_A))                        target_pos.x -= state.speedup ? 5 : 1;
        if (IsKeyDown(KEY_S) && !toggled_speedup)    target_pos.y += state.speedup ? 5 : 1;
        if (IsKeyDown(KEY_D) && !toggled_debug_mode) target_pos.x += state.speedup ? 5 : 1;

        if (IsKeyPressed(KEY_R)) {
            free_tiles(tiles);
            state.tiles = tiles_generate(NUM_GRASS_PATCHES, tiles->wide, tiles->wide);
            tiles = &state.tiles;
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
        Tile target_tile = tiles_get_tile(*tiles, player->cluster_id, target_pos);
        if (target_tile.type != TREE_TRUNK) {
            player->pos = target_pos;
        }

        if (player->pos.x < 0) {
            player->pos.x += tiles->wide;
            player->cluster_id.x--;
            LOG("New player cluster id x:%d y:%d\n", player->cluster_id.x, player->cluster_id.y);
        }
        if (tiles->wide <= player->pos.x) {
            player->pos.x -= tiles->wide;
            player->cluster_id.x++;
            LOG("New player cluster id x:%d y:%d\n", player->cluster_id.x, player->cluster_id.y);
        }
        if (player->pos.y < 0) {
            player->pos.y += tiles->high;
            player->cluster_id.y--;
            LOG("New player cluster id x:%d y:%d\n", player->cluster_id.x, player->cluster_id.y);
        }
        if (tiles->high <= player->pos.y) {
            player->pos.y -= tiles->high;
            player->cluster_id.y++;
            LOG("New player cluster id x:%d y:%d\n", player->cluster_id.x, player->cluster_id.y);
        }

        uint32_t padding_x = camera_padding_wide(state.camera);
        uint32_t padding_y = camera_padding_high(state.camera);
        Coordinate p_cluster_id = player->cluster_id;
        int player_i = coord_to_index(player->pos.x, player->pos.y, tiles->wide);
        Cluster *p_cluster = tiles_lookup_cluster(*tiles, p_cluster_id);
        Coordinate player_abs_pos = tiles_cluster_to_abs(*tiles, player->cluster_id, player->pos);

        if (!p_cluster->tiles[player_i].walked_on) {
            //LOG("Walking on tile i:%d x:%d y:%d\n", player_i, player_abs_pos.x, player_abs_pos.y);
            p_cluster->tiles[player_i].walked_on = true;
        }

        int camera_x_with_padding = camera->area.x + padding_x;
        if (player_abs_pos.x < camera_x_with_padding) {
            /* LOG("Moving left! camera x:%d padding %d | player x:%d y:%d (abs x:%d y:%d)\n", (int)camera->area.x, padding_x, player->pos.x, player->pos.y, player_abs_pos.x, player_abs_pos.y); */
            /* LOG("if (player_abs_pos.x[%d] < camera->area.x[%f] + padding_x[%d])\n", */
            /*     player_abs_pos.x, camera->area.x, padding_x); */

            camera->area.x -= padding_x * 2;
        }

        // NOTE(grant): If I don't break this out, I get weird bugs. Idk why.
        //  Gonna do it for all the rest of them, just in case.
        int right_edge = (int)camera->area.x + (int)camera->area.width;
        int player_plus_padding = player_abs_pos.x + padding_x;
        if (right_edge < player_plus_padding) {
            /* LOG("Moving right! camera x:%d padding %d | player x:%d y:%d (abs x:%d y:%d)\n", (int)camera->area.x, padding_x, player->pos.x, player->pos.y, player_abs_pos.x, player_abs_pos.y); */
            /* LOG("if ((right_edge[%d]) camera->area.x[%f] + camera->area.width[%f] < (player_plus_padding[%d]) player_abs_pos.x[%d] + padding_x[%d])\n", */
            /*     right_edge, camera->area.x, camera->area.width, player_plus_padding, player_abs_pos.x, padding_x); */

            camera->area.x += padding_x * 2;
        }

        int camera_y_with_padding = camera->area.y + padding_y;
        if (player_abs_pos.y < camera_y_with_padding) {
            // LOG("Moving up!\n");
            camera->area.y -= padding_y * 2;
        }

        int top_edge = camera->area.y + camera->area.height;
        int player_y_with_padding = player_abs_pos.y + padding_y;
        if (top_edge < player_y_with_padding) {
            // LOG("Moving down!\n");
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

        int y_i = 0;
        for (int y = camera->area.y; y < ending_tile_y; y++) {
            int x_i = 0;
            for (int x = camera->area.x; x < ending_tile_x; x++) {
                // TODO(grant): Figure out how to properly draw across clusters
                ClusterCoordinate info = tiles_abs_to_cluster(*tiles, (Coordinate) {.x=x, .y=y});
                Tile tile = tiles_get_tile(*tiles, info.cluster_id, info.coord);

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

                int pixel_x = x_i * tile_width;
                int pixel_y = y_i * tile_height;

                DrawRectangle(pixel_x, pixel_y, tile_width, tile_height, c);
                x_i++;
            }
            y_i++;
        }

        Coordinate abs_player = tiles_cluster_to_abs(*tiles, p_cluster_id, player->pos);
        int player_pixel_x = (abs_player.x - state.camera.area.x) * tile_width;
        int player_pixel_y = (abs_player.y - state.camera.area.y) * tile_height;

        DrawRectangle(player_pixel_x, player_pixel_y, tile_width, tile_height, player->color);

        if (state.debug_mode) {
            char player_text[1000] = {0};
            snprintf(player_text, 1000, "Player pos x:%d y:%d - cluster x:%d y:%d - px x:%d y:%d", player->pos.x, player->pos.y, player->cluster_id.x, player->cluster_id.y, player_pixel_x, player_pixel_y);
            DrawText(player_text, 10, 30, 20, WHITE);

            char camera_text[1000] = {0};
            snprintf(camera_text, 1000, "Camera x:%d->%d y:%d->%d", (int)state.camera.area.x, (int)(state.camera.area.x + state.camera.area.width), (int)state.camera.area.y, (int)(state.camera.area.y + state.camera.area.height));
            DrawText(camera_text, 10, 50, 20, WHITE);

            DrawFPS(10, 10);
        }

        EndDrawing();
        // End draw
    }

    return 0;
}
