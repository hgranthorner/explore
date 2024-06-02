typedef struct {
    int x;
    int y;
} Coordinate;

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
