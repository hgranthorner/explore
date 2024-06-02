#include <inttypes.h>

#define MAX_GRASS_SIZE    15
#define NUM_GRASS_PATCHES NUM_TILES / 200
#define TREE_CHANCE       3
#define TREE_SIZE         4
#define TREE_TRUNK_SIZE   TREE_SIZE / 2

typedef enum {
    DIRT,
    GRASS,
    TREE,
    TREE_TRUNK
} TileType;

typedef struct {
    TileType type;
    bool     walked_on;
} Tile;

typedef struct {
    Tile *arr;
    uint64_t wide;
    uint64_t high;
} Tiles;

void set_tiles(Tiles tiles, Coordinate coord, int size, int value)
{
    for (int x = coord.x - size; x < coord.x + size; x++) {
        if (x < 0) continue;
        if (tiles.wide <= x) continue;

        for (int y = coord.y - size; y < coord.y + size; y++) {
            if (y < 0) continue;
            if (tiles.high <= y) continue;

            int i = coord_to_index(x, y, tiles.wide);
            tiles.arr[i].type = value;
        }
    }
}

Tiles tiles_generate(int num_grass_patches, uint64_t tiles_wide, uint64_t tiles_high)
{
    uint64_t num_tiles = tiles_wide * tiles_high;
    uint64_t max_grass_size = MAX_GRASS_SIZE;
    Tiles tiles = {0};
    tiles.wide = tiles_wide;
    tiles.high = tiles_high;
    tiles.arr = malloc(sizeof(Tile) * num_tiles);
    memset(tiles.arr, 0, sizeof(Tile) * num_tiles);

    uint64_t *trees = NULL;
    for (int grass_patch = 0; grass_patch < num_grass_patches; grass_patch++) {
        uint64_t patch_center = rand() % num_tiles;
        uint64_t patch_size = (rand() % max_grass_size) + 1;

        Coordinate coord = index_to_coord(patch_center, tiles.wide);
        LOG("Creating patch at x:%d y:%d\n", coord.x, coord.y);

        set_tiles(tiles, coord, patch_size, GRASS);

        bool has_tree = (rand() % TREE_CHANCE) == 0;
        if (has_tree) {
            arrput(trees, patch_center);
        }
    }

    for (size_t i = 0; i < arrlenu(trees); i++) {
        uint64_t patch_center = trees[i];
        Coordinate coord = index_to_coord(patch_center, tiles.wide);
        bool should_skip = false;

        for (int x = coord.x - (TREE_SIZE + 1); x < coord.x + (TREE_SIZE + 1); x++) {
            if (x < 0) continue;
            if (tiles.wide <= x) continue;
            for (int y = coord.y - (TREE_SIZE + 1); y < coord.y + (TREE_SIZE + 1); y++)  {
                if (y < 0) continue;
                if (tiles.high <= y) continue;
                int j = coord_to_index(x, y, tiles.wide);
                TileType type = tiles.arr[j].type;
                if (type == TREE || type == TREE_TRUNK) {
                    should_skip = true;
                }
            }
        }

        if (should_skip) continue;
        set_tiles(tiles, coord, TREE_SIZE, TREE);
        set_tiles(tiles, coord, TREE_TRUNK_SIZE, TREE_TRUNK);
    }

    LOG("Generated %" PRIu64 " tiles!\n", num_tiles);
    arrfree(trees);

    return tiles;
}
