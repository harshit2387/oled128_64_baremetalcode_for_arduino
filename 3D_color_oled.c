#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define TFT_CS   PB2
#define TFT_RST  PB1
#define TFT_DC   PB0

#define CS_LOW()   PORTB &= ~(1<<TFT_CS)
#define CS_HIGH()  PORTB |=  (1<<TFT_CS)

#define DC_LOW()   PORTB &= ~(1<<TFT_DC)
#define DC_HIGH()  PORTB |=  (1<<TFT_DC)

#define RST_LOW()  PORTB &= ~(1<<TFT_RST)
#define RST_HIGH() PORTB |=  (1<<TFT_RST)

/* RGB565 */
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define DARKGRAY 0x1082
#define ORANGE  0xFC00
#define VIOLET  0x801F
#define EDGE    0xFFFF

typedef struct
{
    float x;
    float y;
    float z;
} Vec3;

typedef struct
{
    int16_t x;
    int16_t y;
} Vec2;

typedef struct
{
    uint8_t face;
    float depth;
    uint16_t color;
} FaceInfo;

/* ---------------- SPI ---------------- */

void SPI_init()
{
    DDRB |=
        (1<<PB3) |
        (1<<PB5) |
        (1<<TFT_CS) |
        (1<<TFT_DC) |
        (1<<TFT_RST);

    SPCR =
        (1<<SPE) |
        (1<<MSTR);

    SPSR |= (1<<SPI2X);
}

void SPI_write(uint8_t d)
{
    SPDR = d;

    while(!(SPSR & (1<<SPIF)));
}

/* -------------- TFT ---------------- */

void cmd(uint8_t c)
{
    DC_LOW();
    CS_LOW();

    SPI_write(c);

    CS_HIGH();
}

void data(uint8_t d)
{
    DC_HIGH();
    CS_LOW();

    SPI_write(d);

    CS_HIGH();
}

void data16(uint16_t d)
{
    DC_HIGH();
    CS_LOW();

    SPI_write(d >> 8);
    SPI_write(d & 0xFF);

    CS_HIGH();
}

void setAddrWindow(
    uint8_t x0,
    uint8_t y0,
    uint8_t x1,
    uint8_t y1)
{
    cmd(0x2A);
    data(0);
    data(x0);
    data(0);
    data(x1);

    cmd(0x2B);
    data(0);
    data(y0);
    data(0);
    data(y1);

    cmd(0x2C);
}

void tft_init()
{
    RST_LOW();
    _delay_ms(100);

    RST_HIGH();
    _delay_ms(100);

    cmd(0x01);
    _delay_ms(150);

    cmd(0x11);
    _delay_ms(150);

    cmd(0x3A);
    data(0x05);

    cmd(0x29);
}

void drawPixel(
    int x,
    int y,
    uint16_t color)
{
    if(x < 0 || x > 127 || y < 0 || y > 127)
        return;

    setAddrWindow(
        (uint8_t)x,
        (uint8_t)y,
        (uint8_t)x,
        (uint8_t)y
    );

    DC_HIGH();
    CS_LOW();

    SPI_write(color >> 8);
    SPI_write(color & 0xFF);

    CS_HIGH();
}

void fillRect(
    int x,
    int y,
    int w,
    int h,
    uint16_t color)
{
    if(x < 0)
    {
        w += x;
        x = 0;
    }

    if(y < 0)
    {
        h += y;
        y = 0;
    }

    if(x + w > 128)
        w = 128 - x;

    if(y + h > 128)
        h = 128 - y;

    if(w <= 0 || h <= 0)
        return;

    setAddrWindow(
        (uint8_t)x,
        (uint8_t)y,
        (uint8_t)(x + w - 1),
        (uint8_t)(y + h - 1)
    );

    DC_HIGH();
    CS_LOW();

    for(int32_t i=0; i<(int32_t)w*h; i++)
    {
        SPI_write(color >> 8);
        SPI_write(color & 0xFF);
    }

    CS_HIGH();
}

void drawFastHLine(
    int x,
    int y,
    int w,
    uint16_t color)
{
    fillRect(
        x,
        y,
        w,
        1,
        color
    );
}

/* ---------- Line Drawing ----------- */

void drawLine(
    int x0,
    int y0,
    int x1,
    int y1,
    uint16_t color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while(1)
    {
        drawPixel(x0,y0,color);

        if(x0==x1 && y0==y1)
            break;

        int e2 = 2*err;

        if(e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if(e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void drawLineThick(
    int x0,
    int y0,
    int x1,
    int y1,
    uint16_t color)
{
    drawLine(x0, y0, x1, y1, color);
    drawLine(x0+1, y0, x1+1, y1, color);
    drawLine(x0, y0+1, x1, y1+1, color);
}

void fillEllipseFast(
    int cx,
    int cy,
    int rx,
    int ry,
    uint16_t color)
{
    long rx2 = (long)rx * rx;
    long ry2 = (long)ry * ry;
    long limit = rx2 * ry2;

    for(int y=-ry; y<=ry; y++)
    {
        for(int x=-rx; x<=rx; x++)
        {
            long v =
                ((long)x * x * ry2) +
                ((long)y * y * rx2);

            if(v <= limit)
            {
                int start = x;

                while(x <= rx)
                {
                    long v2 =
                        ((long)x * x * ry2) +
                        ((long)y * y * rx2);

                    if(v2 > limit)
                        break;

                    x++;
                }

                drawFastHLine(
                    cx + start,
                    cy + y,
                    x - start,
                    color
                );
            }
        }
    }
}

/* ----------- Cube Data ------------ */

Vec3 cube[8] =
{
    {-30,-30, 30},
    { 30,-30, 30},
    { 30, 30, 30},
    {-30, 30, 30},

    {-30,-30,-30},
    { 30,-30,-30},
    { 30, 30,-30},
    {-30, 30,-30}
};

uint8_t edges[12][2] =
{
    {0,1},
    {1,2},
    {2,3},
    {3,0},

    {4,5},
    {5,6},
    {6,7},
    {7,4},

    {0,4},
    {1,5},
    {2,6},
    {3,7}
};

uint16_t edgeColor[12] =
{
    RED,
    GREEN,
    BLUE,
    YELLOW,

    CYAN,
    MAGENTA,
    WHITE,
    RED,

    GREEN,
    BLUE,
    YELLOW,
    CYAN
};

uint8_t faces[6][4] =
{
    {0,1,2,3},
    {4,7,6,5},
    {0,3,7,4},
    {1,5,6,2},
    {3,2,6,7},
    {0,4,5,1}
};

uint16_t faceColor[6] =
{
    RED,
    GREEN,
    BLUE,
    YELLOW,
    CYAN,
    MAGENTA
};

/* -------- Projection and Rasterizing ---------- */

void rotateProject(
    Vec3 p,
    Vec2 *out,
    float ax,
    float ay)
{
    float x = p.x;
    float y = p.y;
    float z = p.z;

    float x1 =
        x*cos(ay) +
        z*sin(ay);

    float z1 =
        z*cos(ay) -
        x*sin(ay);

    float y1 =
        y*cos(ax) -
        z1*sin(ax);

    float z2 =
        z1*cos(ax) +
        y*sin(ax);

    float scale = 120.0f;
    float dist  = 100.0f;

    out->x =
        (int)((x1*scale)/(z2+dist))
        + 64;

    out->y =
        (int)((y1*scale)/(z2+dist))
        + 64;
}

uint16_t shadeColor(
    uint16_t color,
    float light)
{
    if(light < 0.22f)
        light = 0.22f;

    if(light > 1.0f)
        light = 1.0f;

    uint8_t r =
        (color >> 11) & 0x1F;

    uint8_t g =
        (color >> 5) & 0x3F;

    uint8_t b =
        color & 0x1F;

    r = (uint8_t)(r * light);
    g = (uint8_t)(g * light);
    b = (uint8_t)(b * light);

    return
        ((uint16_t)r << 11) |
        ((uint16_t)g << 5) |
        b;
}

void fillTriangle(
    int x0,
    int y0,
    int x1,
    int y1,
    int x2,
    int y2,
    uint16_t color)
{
    int miny = y0;
    int maxy = y0;

    if(y1 < miny) miny = y1;
    if(y2 < miny) miny = y2;

    if(y1 > maxy) maxy = y1;
    if(y2 > maxy) maxy = y2;

    if(miny < 0)
        miny = 0;

    if(maxy > 127)
        maxy = 127;

    for(int y=miny; y<=maxy; y++)
    {
        int nodes[3];
        uint8_t count = 0;

        if((y0 < y1 && y >= y0 && y < y1) ||
           (y1 < y0 && y >= y1 && y < y0))
        {
            nodes[count++] =
                x0 +
                (long)(y - y0) *
                (x1 - x0) /
                (y1 - y0);
        }

        if((y1 < y2 && y >= y1 && y < y2) ||
           (y2 < y1 && y >= y2 && y < y1))
        {
            nodes[count++] =
                x1 +
                (long)(y - y1) *
                (x2 - x1) /
                (y2 - y1);
        }

        if((y2 < y0 && y >= y2 && y < y0) ||
           (y0 < y2 && y >= y0 && y < y2))
        {
            nodes[count++] =
                x2 +
                (long)(y - y2) *
                (x0 - x2) /
                (y0 - y2);
        }

        if(count >= 2)
        {
            if(nodes[0] > nodes[1])
            {
                int t = nodes[0];
                nodes[0] = nodes[1];
                nodes[1] = t;
            }

            drawFastHLine(
                nodes[0],
                y,
                nodes[1] - nodes[0] + 1,
                color
            );
        }
    }
}

void drawFace(
    Vec2 *p,
    uint8_t a,
    uint8_t b,
    uint8_t c,
    uint8_t d,
    uint16_t color)
{
    fillTriangle(
        p[a].x,
        p[a].y,
        p[b].x,
        p[b].y,
        p[c].x,
        p[c].y,
        color
    );

    fillTriangle(
        p[a].x,
        p[a].y,
        p[c].x,
        p[c].y,
        p[d].x,
        p[d].y,
        color
    );
}

void drawFaceOutline(
    Vec2 *p,
    uint8_t a,
    uint8_t b,
    uint8_t c,
    uint8_t d)
{
    drawLine(p[a].x, p[a].y, p[b].x, p[b].y, EDGE);
    drawLine(p[b].x, p[b].y, p[c].x, p[c].y, EDGE);
    drawLine(p[c].x, p[c].y, p[d].x, p[d].y, EDGE);
    drawLine(p[d].x, p[d].y, p[a].x, p[a].y, EDGE);
}

void drawRoundCorners(
    Vec2 *p,
    FaceInfo *visible,
    uint8_t visibleCount)
{
    uint8_t used[8] =
    {
        0,0,0,0,0,0,0,0
    };

    for(uint8_t i=0; i<visibleCount; i++)
    {
        uint8_t f = visible[i].face;

        for(uint8_t j=0; j<4; j++)
        {
            uint8_t v = faces[f][j];

            if(!used[v])
            {
                used[v] = 1;

                fillRect(
                    p[v].x - 1,
                    p[v].y - 1,
                    3,
                    3,
                    EDGE
                );
            }
        }
    }
}

void drawCube(
    float ax,
    float ay,
    float az)
{
    Vec2 p[8];
    float rx[8];
    float ry[8];
    float rz[8];

    FaceInfo visible[6];
    uint8_t visibleCount = 0;

    float sinx = sin(ax);
    float cosx = cos(ax);
    float siny = sin(ay);
    float cosy = cos(ay);
    float sinz = sin(az);
    float cosz = cos(az);

    for(uint8_t i=0;i<8;i++)
    {
        float x = cube[i].x;
        float y = cube[i].y;
        float z = cube[i].z;

        float x1 =
            x*cosy +
            z*siny;

        float z1 =
            z*cosy -
            x*siny;

        float y1 =
            y*cosx -
            z1*sinx;

        float z2 =
            z1*cosx +
            y*sinx;

        float x2 =
            x1*cosz -
            y1*sinz;

        float y2 =
            x1*sinz +
            y1*cosz;

        rx[i] = x2;
        ry[i] = y2;
        rz[i] = z2;

        p[i].x =
            (int)((x2 * 82.0f) / (z2 + 100.0f))
            + 64;

        p[i].y =
            (int)((y2 * 82.0f) / (z2 + 100.0f))
            + 64;
    }

    for(uint8_t f=0; f<6; f++)
    {
        uint8_t a = faces[f][0];
        uint8_t b = faces[f][1];
        uint8_t c = faces[f][2];

        float ux = rx[b] - rx[a];
        float uy = ry[b] - ry[a];
        float uz = rz[b] - rz[a];

        float vx = rx[c] - rx[a];
        float vy = ry[c] - ry[a];
        float vz = rz[c] - rz[a];

        float nx = uy*vz - uz*vy;
        float ny = uz*vx - ux*vz;
        float nz = ux*vy - uy*vx;

        if(nz < 0.0f)
        {
            nx *= 0.00027778f;
            ny *= 0.00027778f;
            nz *= 0.00027778f;

            float light =
                (-0.35f * nx) +
                (-0.55f * ny) +
                (-0.75f * nz);

            light =
                0.30f +
                0.70f * light;

            visible[visibleCount].face = f;
            visible[visibleCount].color =
                shadeColor(
                    faceColor[f],
                    light
                );

            visible[visibleCount].depth =
                (
                    rz[faces[f][0]] +
                    rz[faces[f][1]] +
                    rz[faces[f][2]] +
                    rz[faces[f][3]]
                ) * 0.25f;

            visibleCount++;
        }
    }

    for(uint8_t i=0; i<visibleCount; i++)
    {
        for(uint8_t j=i+1; j<visibleCount; j++)
        {
            if(visible[i].depth < visible[j].depth)
            {
                FaceInfo temp = visible[i];
                visible[i] = visible[j];
                visible[j] = temp;
            }
        }
    }

    for(uint8_t i=0; i<visibleCount; i++)
    {
        uint8_t f = visible[i].face;

        drawFace(
            p,
            faces[f][0],
            faces[f][1],
            faces[f][2],
            faces[f][3],
            visible[i].color
        );
    }

    for(uint8_t i=0; i<visibleCount; i++)
    {
        uint8_t f = visible[i].face;

        drawFaceOutline(
            p,
            faces[f][0],
            faces[f][1],
            faces[f][2],
            faces[f][3]
        );
    }

    drawRoundCorners(
        p,
        visible,
        visibleCount
    );
}

/* -------------- MAIN --------------- */

int main()
{
    SPI_init();

    PORTB |=
        (1<<TFT_CS) |
        (1<<TFT_DC) |
        (1<<TFT_RST);

    tft_init();

    float ax = 0.55f;
    float ay = 0.75f;
    float az = 0.20f;

    fillRect(
        0,
        0,
        128,
        128,
        BLACK
    );

    while(1)
    {
        fillRect(
            16,
            10,
            96,
            104,
            BLACK
        );

        fillEllipseFast(
            64,
            98,
            25,
            6,
            DARKGRAY
        );

        drawCube(ax, ay, az);

        ax += 0.020f;
        ay += 0.032f;
        az += 0.014f;

        if(ax > 6.28f)
            ax = 0.0f;

        if(ay > 6.28f)
            ay = 0.0f;

        if(az > 6.28f)
            az = 0.0f;

        _delay_ms(1);
    }
}
