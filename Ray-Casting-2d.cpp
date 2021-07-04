#include <windows.h>
#include <stdio.h>
#include <cmath>

#define ABS(a) ( (a) < 0 ? -(a) : (a) )

typedef unsigned char BYTE;
typedef struct
{
    BYTE R;
    BYTE G;
    BYTE B;
}RGB;
typedef struct
{
    BYTE B;
    BYTE G;
    BYTE R;
    BYTE A;
}RGB4;
typedef struct
{
    float x;
    float y;
    float angle;
}PLAYER;
typedef struct
{
    int x;
    int y;
}DOT_2D;

// Global vars
PLAYER player = { 25 ,25, 0 };

DOT_2D* borders = nullptr;
bool show_borders = true;
int num_borders = 0;

char keyCode = -1;

float FOV = 1.0472;  // Field of view
int NUM_RAYS = 100;  // Number of casting rays
int MAX_DEPTH = 300; // Maximum ray depth

// Global funclions
bool is_outside(int x, int y)
{
    for (int i = 0; i < num_borders; i++)
    {
        if (x >= borders[i].x && y >= borders[i].y && x <= borders[i].x + 50 && y <= borders[i].y + 50)
        {
            return false;
        }
    }
    if (x < 0 || y < 0 || x >= 500 || y >= 500)
    {
        return false;
    }
    return true;
}
void read_map_from_file(const char* fname)
{
    FILE* fp;

    fopen_s(&fp, fname, "rt");
    if (fp)
    {
        int i, j;
        char chr;

        // Count borders and get player position
        for (i = j = 0; (chr = getc(fp)) != EOF; j++)
        {
            if (chr == '#') num_borders++;
            else if (chr == 'p') 
            {
                player.x = (j + 0.5) * 50;
                player.y = (i + 0.5) * 50;
            }
            else if (chr == '\n') { j = -1; i++; }
        }

        // Read borders
        if (num_borders)
        {
            borders = new DOT_2D[num_borders];
            int k = 0;

            rewind(fp);

            for (i = j = 0; (chr = getc(fp)) != EOF; j++)
            {
                if (chr == '#' && k < num_borders) borders[k++] = { j * 50, i * 50 };
                if (chr == '\n') { j = -1; i++; }
            }
        }
    }
    else printf("%s read error.", fname);
}

// FRAME class
class FRAME
{
private:
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HDC tmpDc = nullptr;
    HBITMAP hbm = nullptr;
public:
    int width;
    int height;
    RGB4* buffer = nullptr;
    RGB pen_color = {};

    FRAME(int frameWidth, int frameHeight, HWND frameHwnd)
    {
        width = frameWidth;
        height = frameHeight;
        int size = width * height;
        buffer = new RGB4[size];

        hwnd = frameHwnd;
        hdc = GetDC(hwnd);
        tmpDc = CreateCompatibleDC(hdc);
    }
    ~FRAME()
    {
        delete[] buffer;
        DeleteDC(tmpDc);
        ReleaseDC(hwnd, hdc);
    }

    void clear(RGB color = { 255,255,255 })
    {
        int i;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                i = y * width + x;
                buffer[i].R = color.R;
                buffer[i].G = color.G;
                buffer[i].B = color.B;
            }
        }
    }
    void set_pixel(int x, int y)
    {
        if (x > -1 && y > -1 && x < width && y < height)
        {
            int i = y * width + x;
            buffer[i].R = pen_color.R;
            buffer[i].G = pen_color.G;
            buffer[i].B = pen_color.B;
        }
    }
    void set_circle(int x0, int y0, int R)
    {
        for (int x = x0 - R; x < x0 + R; x++)
        {
            for (int y = y0 - R; y < y0 + R; y++)
            {
                if (((x - x0) * (x - x0) + (y - y0) * (y - y0) - R * R) < 0) set_pixel(x, y);
            }
        }
    }
    void set_line(int x1, int y1, int x2, int y2)
    {
        int dx = ABS(x2 - x1);
        int dy = ABS(y2 - y1);
        int sx = (x2 >= x1) ? 1 : -1;
        int sy = (y2 >= y1) ? 1 : -1;

        if (dx > dy)
        {
            int d = (dy << 1) - dx, d1 = dy << 1, d2 = (dy - dx) << 1;

            for (int x = x1 + sx, y = y1, i = 1; i < dx; i++, x += sx)
            {
                if (d > 0)
                {
                    d += d2;
                    y += sy;
                }
                else d += d1;

                set_pixel(x, y);
            }
        }
        else
        {
            int d = (dx << 1) - dy, d1 = dx << 1, d2 = (dx - dy) << 1;

            for (int x = x1, y = y1 + sy, i = 1; i < dy; i++, y += sy)
            {
                if (d > 0)
                {
                    d += d2;
                    x += sx;
                }
                else d += d1;

                set_pixel(x, y);
            }
        }
        set_pixel(x1, y1);
        set_pixel(x2, y2);
    }
    void print()
    {
        hbm = CreateBitmap(width, height, 1, 8 * 4, buffer);

        SelectObject(tmpDc, hbm);
        BitBlt(hdc, 0, 0, width, height, tmpDc, 0, 0, SRCCOPY);

        DeleteObject(hbm);
    }
};

// Window processing function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if (uMessage == WM_DESTROY) PostQuitMessage(0);
    else if (uMessage == WM_KEYDOWN) keyCode = wParam;
    else if (uMessage == WM_KEYUP) keyCode = 0;
    return DefWindowProc(hwnd, uMessage, wParam, lParam);
}

int main()
{
    // Register the window class
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASS wc = {};
    const wchar_t CLASS_NAME[] = L"myWindow";

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create the window
    const int windowWidth = 500;
    const int windowHeight = 500;
    HWND hwnd = CreateWindow(CLASS_NAME, L"Ray Casting 2d", WS_OVERLAPPEDWINDOW, 0, 0, windowWidth + 16, windowHeight + 39, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    //ShowWindow(GetConsoleWindow(), SW_SHOWNORMAL); // SW_HIDE or SW_SHOWNORMAL - Hide or Show console
    MSG msg = {};

    // Game part
    read_map_from_file("map.txt");

    FRAME frame(windowWidth, windowHeight, hwnd);

    float dx, dy, sin_a, cos_a;
    int rx, ry;

    // -+-+-+-+-+-+-+-+-+-+- MAIL LOOP -+-+-+-+-+-+-+-+-+-+-
    while (GetKeyState(VK_ESCAPE) >= 0)
    {
        // Processing window messages
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) break;
        }

        // Processing key event
        if (keyCode)
        {
            switch (keyCode)
            {
            case 38:
                dx = 10 * cos(player.angle);
                dy = 10 * sin(player.angle);

                if (is_outside(player.x + dx, player.y + dy))
                {
                    player.x += dx / 10;
                    player.y += dy / 10;
                }
            break;
            case 40:
                dx = 10 * cos(player.angle);
                dy = 10 * sin(player.angle);

                if (is_outside(player.x - dx, player.y - dy))
                {
                    player.x -= dx / 10;
                    player.y -= dy / 10;
                }
            break;
            case 37: player.angle -= FOV / 100; break;
            case 39: player.angle += FOV / 100; break;
            case 49: show_borders = true; break;
            case 50: show_borders = false; break;
            }
        
            // Draw background
            frame.clear({ 0,0,0 });

            // Draw borders
            frame.pen_color = { 150,150,150 };

            if (show_borders)
            {
                for (int i = 0; i < num_borders; i++)
                {
                    for (int l = 0; l < 50; l++)
                    {
                        frame.set_pixel(borders[i].x + l, borders[i].y);
                        frame.set_pixel(borders[i].x, borders[i].y + l);
                        frame.set_pixel(borders[i].x + 50, borders[i].y + l);
                        frame.set_pixel(borders[i].x + l, borders[i].y + 50);
                    }
                }
            }

            // Cast rays
            frame.pen_color = { 150,150,150 };

            for (float angle = -FOV/2; angle <= FOV/2; angle += FOV/NUM_RAYS)
            {
                sin_a = sin(player.angle + angle);
                cos_a = cos(player.angle + angle);
                    
                for (int depth = 0; depth < MAX_DEPTH; depth++)
                {
                    rx = int(player.x + cos_a * depth);
                    ry = int(player.y + sin_a * depth);

                    if (rx % 50 == 0 || ry % 50 == 0)
                    {
                        if (!is_outside(rx, ry)) break;
                    }
                    frame.set_pixel(rx, ry);
                }
            }

            // Draw player
            frame.pen_color = { 0,255,0 };

            frame.set_line(player.x, player.y, player.x + MAX_DEPTH * cos(player.angle), player.y + MAX_DEPTH * sin(player.angle));
            frame.set_line(player.x, player.y, player.x + MAX_DEPTH * cos(player.angle - FOV/2), player.y + MAX_DEPTH * sin(player.angle - FOV/2));
            frame.set_line(player.x, player.y, player.x + MAX_DEPTH * cos(player.angle + FOV/2), player.y + MAX_DEPTH * sin(player.angle + FOV/2));
            frame.set_circle(player.x, player.y, 9);

            // Print buffer
            frame.print();
        }
    }

    delete[] borders;

    return 0;
}