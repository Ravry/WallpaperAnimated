#include <windows.h>

#include <GL/glew.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

namespace fs = std::filesystem;

void Log(const char* message)
{
    MessageBox(NULL, message, "Message", MB_OK);
}

GLuint texID;
GLuint* texturesArray;
int frame = 0;
int framesNum = 0;
int frameDelay = 0;
std::vector<std::string> wallpaperfiles;

unsigned char* loadFileToMemory(const char* filename, size_t* outSize) {
    FILE* file = NULL;
    errno_t err = fopen_s(&file, filename, "rb");

    if (err != 0 || file == NULL) {
        // Handle error, file could not be opened
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the file content
    unsigned char* buffer = (unsigned char*)malloc(fileSize);
    if (buffer) {
        fread(buffer, 1, fileSize, file);
    }

    fclose(file);

    if (outSize) {
        *outSize = fileSize;
    }

    return buffer;
}

void loadGIF(const char* filename) {
    size_t gifSize;
    unsigned char* gifData = loadFileToMemory(filename, &gifSize);

    if (!gifData) {
        fprintf(stderr, "Failed to load GIF file.\n");
        return;
    }
    int* delays;
    int width, height, channels, num_frames;
    unsigned char* gifFrames = stbi_load_gif_from_memory(gifData, gifSize, &delays, &width, &height, &num_frames, &channels, 0);

    if (!gifFrames) {
        fprintf(stderr, "Failed to decode GIF data.\n");
        free(gifData);
        return;
    }

    // Allocate memory for texture handles
    GLuint* textures = (GLuint*)malloc(num_frames * sizeof(GLuint));
    if (!textures) {
        fprintf(stderr, "Failed to allocate memory for textures.\n");
        stbi_image_free(gifFrames);
        free(gifData);
        return;
    }

    glGenTextures(num_frames, textures);

    for (int i = 0; i < num_frames; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, gifFrames + i * width * height * 4);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Cleanup
    stbi_image_free(gifFrames);
    free(gifData);

    // Here, you can store the texture handles in a global or static variable if needed
    // For example:
    texturesArray = textures;
    framesNum = num_frames;
    frameDelay = delays[0];
}

void LoadTexture(const char* filename) {
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Set texture wrapping/filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load texture data
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else {
            std::cerr << "Unsupported number of channels" << std::endl;
            stbi_image_free(data);
            return;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load image" << std::endl;
    }
}

void GetAllFilesInDirectory(std::string path)
{
    try {
        if (fs::exists(path) && fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (fs::is_regular_file(entry)) {
                    std::string file = entry.path().filename().string();
                    wallpaperfiles.push_back(path + file);
                }
            }
        }
        else {
            std::cerr << "Path does not exist or is not a directory" << std::endl;
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
}



LRESULT CALLBACK BGWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
        {
            SetTimer(hwnd, 1, frameDelay, NULL);
            return 0;
        }
        case WM_TIMER:
        {
            if (wParam == 1)
            {
                SetTimer(hwnd, 1, frameDelay, NULL);
                frame = (frame + 1) % framesNum;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Set up the OpenGL scene
            glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (texturesArray)
                glBindTexture(GL_TEXTURE_2D, texturesArray[frame]);

            glBegin(GL_QUADS);

            glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.f, -1.f);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(0.f, -1.f);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(0.f, 1.f);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.f, 1.f);


            glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.f + 1.f, -1.f);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(0.f + 1.f, -1.f);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(0.f + 1.f, 1.f);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.f + 1.f, 1.f);
            
            glEnd();

            SwapBuffers(hdc);  // Swap the buffers to display the scene
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
        {
            delete texturesArray;
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK SelectionWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg) {
        case WM_CREATE: 
        {
            GetAllFilesInDirectory("C:/Wallpaper/");

            for (int i = 0; i < wallpaperfiles.size(); i++)
            { 
                HWND HWNDButton = CreateWindow(
                    "BUTTON",          // Predefined class name for button
                    wallpaperfiles.at(i).c_str(),        // Button text
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // Styles
                    2 + i % 4 * (188 + 10),                // x position
                    2 + i / 4 * (30 + 5),              // y position
                    188,               // Button width
                    30,                // Button height
                    hwnd,              // Parent window handle
                    (HMENU)i,          // ID of the button
                    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                    NULL               // Pointer to additional data
                );
            }

            return 0;
        }
        case WM_COMMAND:
        {
            int wmID = LOWORD(wParam);
            HWND hButton = GetDlgItem(hwnd, wmID);

            if (hButton)
            {
                int length = GetWindowTextLength(hButton) + 1; // +1 for null terminator

                // Allocate buffer for the text
                wchar_t* textBuffer = new wchar_t[length];

                // Retrieve the button text
                GetWindowText(hButton, (LPSTR)textBuffer, length);

                // Display the button text in a message box
                std::wstring buttonText(textBuffer);
                MessageBox(hwnd, (LPCSTR)buttonText.c_str(), "Button Clicked", MB_OK);

                loadGIF((const char*)buttonText.c_str());

                // Clean up
                delete[] textBuffer;
            }
            
            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            HBRUSH greenBrush = CreateSolidBrush(0xFF0000);

            FillRect(hdc, &ps.rcPaint, greenBrush);
            EndPaint(hwnd, &ps);
        }
        return 0;
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


  
struct ScreenSize {
    int width;
    int height;
};

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    ScreenSize* screenSize = reinterpret_cast<ScreenSize*>(dwData);

    int monitorWidth = lprcMonitor->right;
    int monitorHeight = lprcMonitor->bottom;

    // Update total width and height considering all monitors
    screenSize->width = max(screenSize->width, monitorWidth);
    screenSize->height = max(screenSize->height, monitorHeight);

    return TRUE; // Continue enumeration
}

HWND CreateBackgroundWindow(HINSTANCE hInstance) {
    ScreenSize screenSize = { 0, 0 };
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&screenSize));
    
    const char BACKGROUND_CLASS_NAME[] = "Background Window Class";
    WNDCLASS wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = BGWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = BACKGROUND_CLASS_NAME;
    RegisterClass(&wc);

    HWND hwndBG = CreateWindowEx(
        WS_EX_TOOLWINDOW, // Use WS_EX_TOOLWINDOW to create a window that does not show in the taskbar
        BACKGROUND_CLASS_NAME,
        "Behind Desktop Icons",
        WS_POPUP, // No title bar or border
        CW_USEDEFAULT, CW_USEDEFAULT, screenSize.width, screenSize.height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwndBG == NULL) {
        return 0;
    }
}

void SetWindowBehindDektopIcons(HWND hwndBG)
{
    // Get the handle of the desktop window
    HWND hwndDesktop = FindWindow("Progman", NULL);
    if (hwndDesktop) {
        // Send a special message to the desktop window to refresh its children
        SendMessageTimeout(hwndDesktop, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL);

        // Get the handle of the WorkerW window, which contains the icons
        HWND hwndWorkerW = NULL;
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            HWND p = FindWindowEx(hwnd, NULL, "SHELLDLL_DefView", NULL);
        if (p) {
            *(HWND*)lParam = FindWindowEx(NULL, hwnd, "WorkerW", NULL);
        }
        return TRUE;
            }, (LPARAM)&hwndWorkerW);

        if (hwndWorkerW) {
            // Place the background window behind the WorkerW window (which is behind the icons)
            SetParent(hwndBG, hwndWorkerW);
            SetWindowPos(hwndBG, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
}

HWND CreateSelectionWindow(HINSTANCE hInstance) {
    const char SELECTION_CLASS_NAME[] = "Selection Window Class";

    WNDCLASS swc = {};
    swc.lpfnWndProc = SelectionWindowProc;
    swc.hInstance = hInstance;
    swc.lpszClassName = SELECTION_CLASS_NAME;
    RegisterClass(&swc);


    HWND hwndSelection = CreateWindowEx(
        0, // No extended styles
        SELECTION_CLASS_NAME,
        "Selection Window",
        WS_OVERLAPPEDWINDOW, // Normal window with a title bar
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwndSelection == NULL) {
        return 0;
    }
}

void EnableOpenGL(HWND hwnd, HDC *hdc, HGLRC *hrc) {
    PIXELFORMATDESCRIPTOR pfd;
    int format;

    *hdc = GetDC(hwnd);

    // Set the pixel format for the DC
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    format = ChoosePixelFormat(*hdc, &pfd);
    SetPixelFormat(*hdc, format, &pfd);

    // Create and enable the render context (RC)
    *hrc = wglCreateContext(*hdc);
    wglMakeCurrent(*hdc, *hrc);
}

void DisableOpenGL(HWND hwnd, HDC hdc, HGLRC hrc) {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hrc);
    ReleaseDC(hwnd, hdc);
}

void ShowAndUpdateWindows(HWND hwndBG, HWND hwndSelection, int nShowCmd)
{
    ShowWindow(hwndBG, nShowCmd);
    UpdateWindow(hwndBG);

    ShowWindow(hwndSelection, nShowCmd);
    UpdateWindow(hwndSelection);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    // Print to the console to test
    std::cout << "Console initialized for debugging." << std::endl;


    
    ScreenSize screenSize = { 0, 0 };
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&screenSize));

    HDC hdc;
    HGLRC hrc;

    const char BACKGROUND_CLASS_NAME[] = "Background Window Class";
    WNDCLASS wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = BGWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = BACKGROUND_CLASS_NAME;
    RegisterClass(&wc);

    HWND hwndBG = CreateWindowEx(
        WS_EX_TOOLWINDOW, // Use WS_EX_TOOLWINDOW to create a window that does not show in the taskbar
        BACKGROUND_CLASS_NAME,
        "Behind Desktop Icons",
        WS_POPUP, // No title bar or border
        CW_USEDEFAULT, CW_USEDEFAULT, screenSize.width, screenSize.height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwndBG == NULL) {
        return 0;
    }
    
    HWND hwndSelection = CreateSelectionWindow(hInstance);

    SetWindowBehindDektopIcons(hwndBG);
    ShowAndUpdateWindows(hwndBG, hwndSelection, nShowCmd);


    stbi_set_flip_vertically_on_load(true);
    EnableOpenGL(hwndBG, &hdc, &hrc);

    glEnable(GL_TEXTURE_2D);

    if (glewInit() != GLEW_OK)
    {
        Log("glew init failed");
    }

    //LoadTexture("C:\\Users\\paull\\Pictures\\Wallpaper.png");
    loadGIF("C:\\Users\\paull\\Pictures\\Wallpaper.gif");

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hwndBG)
        DestroyWindow(hwndBG);
    if (hwndSelection)
        DestroyWindow(hwndSelection);

    FreeConsole();

    return 0;
}