#include <windows.h>
#include <stdio.h>
#include <math.h>
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
    int x;
    int y;
    double angle;
}PLAYER;

typedef struct
{
    int x;
    int y;
} DOT_2D;

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

    bool cmp_pixel(int x, int y, RGB color)
    {
        int i = y * width + x;

        if (buffer[i].R == color.R && buffer[i].G == color.G && buffer[i].B == color.B) return true;
        else return false;
    }

    void set_rect(int x1, int y1, int x2, int y2)
    {
        for (int y = y1; y <= y2; y++)
        {
            for (int x = x1; x <= x2; x++)
            {
                set_pixel(x, y);
            }
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

    void set_ray(int x1, int y1, int x2, int y2)
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

                if (cmp_pixel(x, y, {}))
                {
                    return;
                }

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

                if (cmp_pixel(x, y, {}))
                {
                    return;
                }

                set_pixel(x, y);
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

PLAYER player = {};
DOT_2D* borders;
int bn = 0, bh = 40;
char KeyCode = -1;

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
            if (chr == '#') bn++;
            else if (chr == 'p') player = { j * 1000 + 500, i * 1000 + 500 };
            else if (chr == '\n') { j = -1; i++; }
        }

        // Read borders
        if (bn)
        {
            borders = new DOT_2D[bn];
            int k = 0;

            rewind(fp);

            for (i = j = 0; (chr = getc(fp)) != EOF; j++)
            {
                if (chr == '#' && k < bn) borders[k++] = { j * 1000, i * 1000 };
                if (chr == '\n') { j = -1; i++; }
            }
        }
    }
}

// Window processing function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_DESTROY) PostQuitMessage(0);
    else if (uMsg == WM_KEYDOWN) KeyCode = wParam;
    else if (uMsg == WM_KEYUP) KeyCode = 0;
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
    // Register the window class
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASS wc = {};
    const wchar_t CLASS_NAME[] = L"MyWindow";

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create the window
    const int windowWidth = 400;
    const int windowHeight = 400;
    HWND hwnd = CreateWindow(CLASS_NAME, L"My Window", WS_OVERLAPPEDWINDOW, 0, 0, windowWidth+16, windowHeight+39, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    //ShowWindow(GetConsoleWindow(), SW_SHOWNORMAL); // SW_HIDE or SW_SHOWNORMAL - Hide or Show console
    MSG msg = {};
    
    /** MAIN LOOP **/
    FRAME frame(windowWidth, windowHeight, hwnd);
    read_map_from_file("map.txt");

    int dx, dy;
    bool outside;
    
    while (GetKeyState(VK_ESCAPE) >= 0)
    {
        // Processing window messages
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) break;
        }

        // Processing frames after key event
        if (KeyCode)
        {
            // Logic
            if (KeyCode == 38)
            {
                dx = int(80 * cos(player.angle));
                dy = int(80 * sin(player.angle));

                outside = true;
                for (int i = 0; i < bn; i++)
                {
                    if (player.x + dx > borders[i].x && player.y + dy > borders[i].y && player.x + dx < borders[i].x + 1000 && player.y + dy < borders[i].y + 1000)
                    {
                        outside = false;
                        break;
                    }
                }

                if (outside)
                {
                    player.x += dx/4;
                    player.y += dy/4;
                }
            }
            else if (KeyCode == 40)
            {
                dx = int(80 * cos(player.angle));
                dy = int(80 * sin(player.angle));

                outside = true;
                for (int i = 0; i < bn; i++)
                {
                    if (player.x - dx > borders[i].x && player.y - dy > borders[i].y && player.x - dx < borders[i].x + 1000 && player.y - dy < borders[i].y + 1000)
                    {
                        outside = false;
                        break;
                    }
                }

                if (outside)
                {
                    player.x -= dx/4;
                    player.y -= dy/4;
                }
            }
            else if (KeyCode == 37)
            {
                player.angle -= 0.01;
            }
            else if (KeyCode == 39)
            {
                player.angle += 0.01;
            }

            // Clear buffer
            frame.clear();

            // Set borders
            frame.pen_color = {};
            for (int i = 0; i < bn; i++)
            {
                frame.set_rect(borders[i].x / 25, borders[i].y / 25, borders[i].x / 25 + bh, borders[i].y / 25 + bh);
            }

            // Set rays
            for (double angle = -0.6; angle <= 0.6; angle += 0.01)
            {
                frame.pen_color = { 0,255,255 };
                frame.set_ray(player.x / 25, player.y / 25, player.x / 25 + int(bh * 8 * cos(player.angle + angle)), player.y / 25 + int(bh * 8 * sin(player.angle + angle)));
            }

            // Set rlayer
            frame.pen_color = { 255,0,0 };
            frame.set_line(player.x / 25, player.y / 25, player.x / 25 + int(bh * 8 * cos(player.angle - 0.6)), player.y / 25 + int(bh * 8 * sin(player.angle - 0.6)));
            frame.set_line(player.x / 25, player.y / 25, player.x / 25 + int(bh * 8 * cos(player.angle)), player.y / 25 + int(bh * 8 * sin(player.angle)));
            frame.set_line(player.x / 25, player.y / 25, player.x / 25 + int(bh * 8 * cos(player.angle+0.6)), player.y / 25 + int(bh * 8 * sin(player.angle + 0.6)));
            frame.set_circle(player.x / 25, player.y / 25, 5);

            // Print buffer
            frame.print();
        }
    }

    delete[] borders;

    return 0;
}