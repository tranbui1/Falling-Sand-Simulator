#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // For Uint8 data type
#include <time.h>   // For random seeds
#include <stdbool.h>

// Define the size of the window
#define WIDTH 640
#define HEIGHT 480

// Define the scaling factor of the window
# define SCALING_FACTOR 10

// Define the size of 2D array to hold drawn points
#define COLS WIDTH / SCALING_FACTOR
#define ROWS HEIGHT / SCALING_FACTOR

// Initialize a struct to store RGB of colors
typedef struct {
    Uint8 r;
    Uint8 g;
    Uint8 b;
} Color;

// Initialize a struct to store point data
typedef struct {
    int exists;
    Color color;
} Point;

// Prints out error message and quits the program
int error_checking(const char *format, const char *error) {
    printf(format, error);
    SDL_Quit();
    return 1;
}

Color varyColor() {
    Color color;
    // Cartoonish darker sand base
    color.r = 200 + (rand() % 20); // 200–219
    color.g = 170 + (rand() % 20); // 170–189
    color.b = 60 + (rand() % 10);  // 60–69 
    return color;
}

// Adds new point into the array to be drawn
int addNewPoint(Point points[ROWS][COLS], int col, int row, Color color) {
    // Bounds checking
    if (col < 0 || col >= COLS || row < 0 || row >= ROWS) {
        return -1;
    }

    points[row][col].exists = 1;  
    points[row][col].color = (Color){ .r = color.r, .g = color.g, .b = color.b};

    return 0;
}

int sandGravity(Point points[ROWS][COLS], int col, int row) {
    int falling = 1;
    Color color = points[row][col].color;
    
    while (falling == 1) {
        // If there is no bottom pixel, move down
        if (points[row][col + 1].exists == 0) {
            points[row][col].exists = -1;
            addNewPoint(points, col + 1, row, color);
        }
        
        // If there is a bottom pixel, try moving downward left
        else if (points[row + 1][col - 1].exists == 0) {
            points[row][col].exists = -1;
            addNewPoint(points, col - 1, row + 1, color);
        }

        // If there is a pixel downward left, try moving downward right
        else if (points[row + 1][col + 1].exists == 0) {
            points[row][col].exists = -1;
            addNewPoint(points, col - 1, row + 1, color);
        }

        // Else, stay
        else {
            falling = 0;
        }
    }

    return 0;
}

// Draw all points
int drawAllPoints(SDL_Renderer *renderer, Point points[ROWS][COLS]) {
    // Set the background to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw all points, checking all points in grid
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            if (points[row][col].exists == 1) { 
                SDL_SetRenderDrawColor(renderer, 
                                    points[row][col].color.r,  
                                    points[row][col].color.g,  
                                    points[row][col].color.b, 
                                    255);
                int error = SDL_RenderDrawPoint(renderer, col, row); 
                if (error) error_checking("Encountered error while drawing point on renderer: %s\n", SDL_GetError());

            }
        }
    }
    
    SDL_RenderPresent(renderer);
    return 0;
}

// Draw line between two points to prevent skipping (using Bresenham's line algorithm)
int drawLine(SDL_Renderer *renderer, Point points[ROWS][COLS], int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1, y = y1;
    
    while (1) {
        // Add point at current position
        if (x >= 0 && x < COLS && y >= 0 && y < ROWS) {
            if (points[y][x].exists == 0) { // Only add if point doesn't exist
                addNewPoint(points, x, y, varyColor()); // x=col, y=row
            }
        }
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
    
    return 0;
}

int main() {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    // Set pseudorandom seed
    srand(time(NULL));

    printf("Initializing SDL.\n");

    // Initialize SDL and its default subsystems
    if ((SDL_Init(SDL_INIT_VIDEO)) < 0) error_checking("Encountered error while initializing SDL: %s\n", SDL_GetError());

    printf("SDL successfully initialized!\n");

    // Create window
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);

    // Window and renderer error checking
    if (window == NULL) error_checking("Encountered error while initializing window: %s\n", SDL_GetError());
    if (renderer == NULL) error_checking("Encountered error while initializing renderer: %s\n", SDL_GetError());

    // Set the render scale
    SDL_RenderSetScale(renderer, SCALING_FACTOR, SCALING_FACTOR);

    // Initialize 2D array to store points for easy collision checks
    Point points[ROWS][COLS]; 

    // Initialize the array:
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            points[i][j].exists = 0;
        }
    }

    SDL_Event event;
    int quit = 0;
    bool mouseHeld = false; 
    int prevX = -1;
    int prevY = -1;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                mouseHeld = true;
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                mouseHeld = false;
                prevX = -1;  // Reset so new strokes don't connect
                prevY = -1;
            }
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }

        if (mouseHeld) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            mouseX /= SCALING_FACTOR;
            mouseY /= SCALING_FACTOR;

            if ((prevX >= 0) && (prevY >= 0)) {
                drawLine(renderer, points, prevX, prevY, mouseX, mouseY);
            } else {
                int result = addNewPoint(points, mouseX, mouseY, varyColor()); 
                if (result != 0) {
                    printf("Mouse coordinates out of bounds: (%d, %d)\n", mouseX, mouseY);
                }
            }

            prevX = mouseX;
            prevY = mouseY;
        }


        drawAllPoints(renderer, points); 

        SDL_Delay(16); // For ~60 FPS
    }

    // Shut down all systems
    SDL_Quit();

    return 0;
}