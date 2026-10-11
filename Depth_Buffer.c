
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "I2C.h"
#include "OLED.h"

#define W 128
#define H 64

typedef struct
{
    int16_t x;
    int16_t y;
} V2;

typedef struct
{
    V2 a;
    V2 b;
    V2 c;
    int16_t z;
    uint8_t pattern;
} Triangle;

void swap_vertex(V2 *a, V2 *b)
{
    V2 t = *a;
    *a = *b;
    *b = t;
}

void draw_line(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    int sx = dx < 0 ? -1 : 1;
    int sy = dy < 0 ? -1 : 1;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    int err = dx - dy;

    while (1)
    {
        if (x0 >= 0 && x0 < W && y0 >= 0 && y0 < H)
            oled_pixel(x0, y0);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

int edge_x(V2 p, V2 q, int y)
{
    if (p.y == q.y)
        return p.x;

    return p.x +
           (long)(q.x - p.x) * (y - p.y) /
           (q.y - p.y);
}

uint8_t pixel_pattern(int x, int y, uint8_t pattern)
{
    switch (pattern)
    {
        case 0:
            return 1;

        case 1:
            return ((x + y) & 1) == 0;

        case 2:
            return (y & 1) == 0;

        default:
            return ((x & 3) == 0) || ((y & 3) == 0);
    }
}

void fill_triangle(V2 a, V2 b, V2 c, uint8_t pattern)
{
    if (a.y > b.y) swap_vertex(&a, &b);
    if (a.y > c.y) swap_vertex(&a, &c);
    if (b.y > c.y) swap_vertex(&b, &c);

    if (a.y == c.y)
    {
        int xmin = a.x;
        int xmax = a.x;

        if (b.x < xmin) xmin = b.x;
        if (c.x < xmin) xmin = c.x;

        if (b.x > xmax) xmax = b.x;
        if (c.x > xmax) xmax = c.x;

        if (a.y >= 0 && a.y < H)
        {
            if (xmin < 0) xmin = 0;
            if (xmax >= W) xmax = W - 1;

            for (int x = xmin; x <= xmax; x++)
            {
                if (pixel_pattern(x, a.y, pattern))
                    oled_pixel(x, a.y);
            }
        }

        return;
    }

    int y_start = a.y < 0 ? 0 : a.y;
    int y_end = c.y >= H ? H - 1 : c.y;

    for (int y = y_start; y <= y_end; y++)
    {
        int x1;
        int x2 = edge_x(a, c, y);

        if (y < b.y)
            x1 = edge_x(a, b, y);
        else
            x1 = edge_x(b, c, y);

        if (x1 > x2)
        {
            int t = x1;
            x1 = x2;
            x2 = t;
        }

        if (x1 < 0) x1 = 0;
        if (x2 >= W) x2 = W - 1;

        for (int x = x1; x <= x2; x++)
        {
            if (pixel_pattern(x, y, pattern))
                oled_pixel(x, y);
        }
    }
}

void draw_triangle(Triangle t)
{
    fill_triangle(t.a, t.b, t.c, t.pattern);

    draw_line(t.a.x, t.a.y, t.b.x, t.b.y);
    draw_line(t.b.x, t.b.y, t.c.x, t.c.y);
    draw_line(t.c.x, t.c.y, t.a.x, t.a.y);
}

void sort_by_depth(Triangle triangles[], int count)
{
    for (int i = 0; i < count - 1; i++)
    {
        for (int j = 0; j < count - 1 - i; j++)
        {
            if (triangles[j].z > triangles[j + 1].z)
            {
                Triangle temp = triangles[j];
                triangles[j] = triangles[j + 1];
                triangles[j + 1] = temp;
            }
        }
    }
}

int main(void)
{
    i2c_init();
    oled_init();

    Triangle triangles[3] =
    {
        {
            {18, 46}, {70, 5}, {112, 25},
            10, 2
        },

        {
            {5, 31}, {119, 50}, {112, 31},
            20, 1
        },

        {
            {70, 18}, {91, 27}, {95, 59},
            30, 0
        }
    };

    sort_by_depth(triangles, 3);

    while (1)
    {
        oled_clear();

        for (int i = 0; i < 3; i++)
        {
            draw_triangle(triangles[i]);
        }

        oled_update();
        _delay_ms(100);
    }
}
