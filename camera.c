#include "raylib.h"

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    Rectangle area;
    size_t screen_width;
    size_t screen_height;
} GameCamera;

uint32_t camera_tile_width(GameCamera camera)
{
    return ((float)camera.screen_width / (float)camera.area.width);
}

uint32_t camera_tile_height(GameCamera camera)
{
    return ((float)camera.screen_height / (float)camera.area.height);
}

uint32_t camera_extra_tiles_wide(GameCamera camera)
{
    int current_pixels_wide = camera_tile_width(camera) * camera.area.width;
    int total_width = camera.screen_width;
    uint32_t extra_tiles = 1;
    while (current_pixels_wide < total_width) {
        extra_tiles++;
        current_pixels_wide = current_pixels_wide + camera_tile_width(camera);
    }
    return extra_tiles;
}

uint32_t camera_extra_tiles_high(GameCamera camera)
{
    int current_pixels_high = camera_tile_height(camera) * camera.area.height;
    int total_height = camera.screen_height;
    uint32_t extra_tiles = 1;
    while (current_pixels_high < total_height) {
        extra_tiles++;
        current_pixels_high = current_pixels_high + camera_tile_height(camera);
    }
    return extra_tiles;
}

uint32_t camera_padding_wide(GameCamera camera)
{
    return camera.area.width / 10;
}

uint32_t camera_padding_high(GameCamera camera)
{
    return camera.area.height / 10;
}
