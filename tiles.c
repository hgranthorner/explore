#include <inttypes.h>

#define MAX_GRASS_SIZE    15
#define NUM_GRASS_PATCHES NUM_TILES / 200
#define TREE_CHANCE       3
#define TREE_SIZE         4
#define TREE_TRUNK_SIZE   TREE_SIZE / 2

#define Vec(type) type *

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
    Coordinate cluster_id;
    Tile *tiles;
} Cluster;

typedef struct {
    Coordinate coord;
    Coordinate cluster_id;
} ClusterCoordinate;

typedef struct {
    // stb_ds resizable array
    Vec(Cluster) clusters;
    uint64_t wide;
    uint64_t high;
    uint64_t num_grass_patches;
} Tiles;

void cluster_set_tiles(Cluster *cluster, Coordinate coord, int wide, int high, int size, int value)
{
    for (int x = coord.x - size; x < coord.x + size; x++) {
        if (x < 0) continue;
        if (wide <= x) continue;

        for (int y = coord.y - size; y < coord.y + size; y++) {
            if (y < 0) continue;
            if (high <= y) continue;

            int i = coord_to_index(x, y, wide);
            cluster->tiles[i].type = value;
        }
    }
}

Cluster generate_cluster(Coordinate cluster_id, int wide, int high, uint64_t num_grass_patches)
{
    uint64_t num_tiles = wide * high;
    uint64_t max_grass_size = MAX_GRASS_SIZE;
    Cluster cluster = {
        .cluster_id = cluster_id,
        .tiles = malloc(sizeof(Tile) * num_tiles),
    };
    memset(cluster.tiles, 0, sizeof(Tile) * num_tiles);
    uint64_t *trees = NULL;
    for (int grass_patch = 0; grass_patch < num_grass_patches; grass_patch++) {
        uint64_t patch_center = rand() % num_tiles;
        uint64_t patch_size = (rand() % max_grass_size) + 1;

        Coordinate coord = index_to_coord(patch_center, wide);

        cluster_set_tiles(&cluster, coord, wide, high, patch_size, GRASS);

        bool has_tree = (rand() % TREE_CHANCE) == 0;
        if (has_tree) {
            arrput(trees, patch_center);
        }
    }

    for (size_t i = 0; i < arrlenu(trees); i++) {
        uint64_t patch_center = trees[i];
        Coordinate coord = index_to_coord(patch_center, wide);
        bool should_skip = false;

        for (int x = coord.x - (TREE_SIZE + 1); x < coord.x + (TREE_SIZE + 1); x++) {
            if (x < 0) continue;
            if (wide <= x) continue;
            for (int y = coord.y - (TREE_SIZE + 1); y < coord.y + (TREE_SIZE + 1); y++)  {
                if (y < 0) continue;
                if (high <= y) continue;
                int j = coord_to_index(x, y, wide);
                TileType type = cluster.tiles[j].type;
                if (type == TREE || type == TREE_TRUNK) {
                    should_skip = true;
                }
            }
        }

        if (should_skip) continue;
        cluster_set_tiles(&cluster, coord, wide, high, TREE_SIZE, TREE);
        cluster_set_tiles(&cluster, coord, wide, high, TREE_TRUNK_SIZE, TREE_TRUNK);
    }

    LOG("Generated %" PRIu64 " tiles!\n", num_tiles);
    arrfree(trees);

    return cluster;
}

Tiles tiles_generate(int num_grass_patches, uint64_t tiles_wide, uint64_t tiles_high)
{
    Tiles tiles = {0};
    tiles.wide = tiles_wide;
    tiles.high = tiles_high;
    tiles.num_grass_patches = num_grass_patches;
    tiles.clusters = NULL;
    Cluster initial_cluster = generate_cluster((Coordinate) {.x = 0, .y = 0}, tiles_wide, tiles_high, num_grass_patches);
    arrput(tiles.clusters, initial_cluster);
    return tiles;
}

void free_tiles(Tiles *tiles)
{
    for (int i = 0; i < arrlenu(tiles->clusters); i++) {
        free(tiles->clusters[i].tiles);
    }
    arrfree(tiles->clusters);
}

bool cluster_equals(Coordinate c_id1, Coordinate c_id2)
{
    return c_id1.x == c_id2.x && c_id1.y == c_id2.y;
}

Cluster *tiles_lookup_cluster(Tiles tiles, Coordinate cluster_id)
{
    for (int i = 0; i < arrlenu(tiles.clusters); i++) {
        Cluster *cluster = &tiles.clusters[i];
        if (cluster_equals(cluster->cluster_id, cluster_id)) {
            return cluster;
        }
    }

    return NULL;
}

Tile tiles_get_tile(Tiles tiles, Coordinate current_cluster_id, Coordinate target)
{
    int x = target.x;
    int y = target.y;
    Coordinate c_id = current_cluster_id;

    while (x < 0) {
        c_id.x--;
        x = tiles.wide + x;
        // Top left cluster
        while (y < 0) {
            c_id.y--;
            y += tiles.high;
            // Bottom left cluster
        }
        while (tiles.high <= y) {
            c_id.y++;
            y -= tiles.high;
        }
    }
    while (y < 0) {
        c_id.y--;
        y += tiles.high;
        // Top right cluster
        while (tiles.wide <= x) {
            c_id.x++;
            x -= tiles.wide;
            // Top middle cluster
        }
    }
    while (tiles.wide <= x) {
        c_id.x++;
        x -= tiles.wide;
        // Bottom right cluster
        while (tiles.high <= y) {
            c_id.y++;
            y -= tiles.high;
            // Middle right cluster
        }
        // Bottom middle cluster
    }
    while (tiles.high <= y) {
        c_id.y++;
        y -= tiles.high;
        // Normal
    }

    Cluster *cluster = tiles_lookup_cluster(tiles, c_id);
    if (cluster == NULL) {
        arrput(tiles.clusters, generate_cluster(c_id, tiles.wide, tiles.high, tiles.num_grass_patches));
        cluster = tiles_lookup_cluster(tiles, c_id);
    }

    int i = coord_to_index(x, y, tiles.wide);
    return cluster->tiles[i];
}

Coordinate tiles_cluster_to_abs(Tiles tiles, Coordinate cluster_id, Coordinate position)
{
    while (cluster_id.x < 0) {
        position.x -= tiles.wide;
        cluster_id.x++;
    }
    while (0 < cluster_id.x) {
        position.x += tiles.wide;
        cluster_id.x--;
    }
    while (cluster_id.y < 0) {
        position.y -= tiles.high;
        cluster_id.y++;
    }
    while (0 < cluster_id.y) {
        position.y += tiles.high;
        cluster_id.y--;
    }

    return position;
}

ClusterCoordinate tiles_abs_to_cluster(Tiles tiles, Coordinate position)
{
    Coordinate cluster_id = {0};
    while (position.x < 0) {
        position.x += tiles.wide;
        cluster_id.x--;
    }
    while (tiles.wide < position.x) {
        position.x -= tiles.wide;
        cluster_id.x++;
    }
    while (position.y < 0) {
        position.y += tiles.high;
        cluster_id.y--;
    }
    while (tiles.high < position.y) {
        position.y -= tiles.high;
        cluster_id.y++;
    }

    return (ClusterCoordinate) {.cluster_id=cluster_id, .coord=position};
}
