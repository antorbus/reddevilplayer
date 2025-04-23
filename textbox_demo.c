#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define SPOTLIGHT_WIDTH 600
#define SPOTLIGHT_HEIGHT 70
#define RESULTS_HEIGHT 400
#define CORNER_RADIUS 10

int main(void)
{
    // Initialize window
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_MSAA_4X_HINT);
    InitWindow(SPOTLIGHT_WIDTH, SPOTLIGHT_HEIGHT + RESULTS_HEIGHT, "Spotlight");
    
    // Set window position
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    SetWindowPosition((screenWidth - SPOTLIGHT_WIDTH) / 2, screenHeight / 4);
    
    // Load SF-Pro font with optimal settings for macOS-like rendering
    // We're using a larger base size and custom parameters for better quality
    int fontBaseSize = 96; // Much larger base size for high quality
    Font sfPro = { 0 };
    
    // Generate font with improved quality settings
    int codepointCount = 95; // Basic Latin character set
    int* codepoints = (int*)malloc(codepointCount * sizeof(int));
    for (int i = 0; i < codepointCount; i++) codepoints[i] = 32 + i; // Generate array of codepoints (32-126)
    
    // Enhanced font loading with proper parameters
    sfPro = LoadFontEx("SF-Pro.ttf", fontBaseSize, codepoints, codepointCount);
    free(codepoints);
    
    // Apply high-quality filtering
    SetTextureFilter(sfPro.texture, TEXTURE_FILTER_BILINEAR);
    
    // THE ONE AND ONLY GREY COLOR
    Color singleGrey = (Color){ 180, 180, 180, 200 };
    Color textColor = BLACK;
    
    // Adjust font metrics for macOS-like appearance
    float fontSpacing = 0.5f; // Slight adjustment to character spacing
    
    char searchText[40] = { 0 };
    bool showResults = false;
    
    SetTargetFPS(60);
    
    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        
        showResults = (searchText[0] != '\0');
        SetWindowSize(SPOTLIGHT_WIDTH, SPOTLIGHT_HEIGHT + (showResults ? RESULTS_HEIGHT : 0));
        
        // Handle keyboard input
        if (IsKeyPressed(KEY_BACKSPACE) && strlen(searchText) > 0) {
            searchText[strlen(searchText)-1] = '\0';
        }
        
        // Handle character input
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (strlen(searchText) < 40)) {
                int len = strlen(searchText);
                searchText[len] = (char)key;
                searchText[len + 1] = '\0';
            }
            key = GetCharPressed();
        }
        
        BeginDrawing();
            ClearBackground(BLANK);
            
            // SINGLE BACKGROUND - NO INNER COLORS
            DrawRectangleRounded(
                (Rectangle){ 0, 0, SPOTLIGHT_WIDTH, SPOTLIGHT_HEIGHT + (showResults ? RESULTS_HEIGHT : 0) }, 
                0.1f, 10, singleGrey);
            
            
            // Draw text directly - NO TEXTBOX BACKGROUND
            // Using specific parameters for high-quality text rendering
            DrawTextEx(sfPro, searchText, (Vector2){ 30, 15 }, 38, fontSpacing, textColor);
            
            // Cursor
            if ((int)(GetTime()*2) % 2 == 0) {
                int textWidth = MeasureTextEx(sfPro, searchText, 38, fontSpacing).x;
                DrawRectangle(30 + textWidth, 15, 2, 40, textColor);
            }
            
            // Results with high-quality rendering
            if (showResults) {
                // DrawLine(20, SPOTLIGHT_HEIGHT, SPOTLIGHT_WIDTH - 20, SPOTLIGHT_HEIGHT, textColor);
                
                const char* demoResults[] = {
                    "Applications",
                    "System Preferences",
                    "Documents",
                    "Folders",
                    "Websites"
                };
                
                for (int i = 0; i < 5; i++) {
                    // High-quality text rendering with adjusted positioning and spacing
                    DrawTextEx(sfPro, demoResults[i], 
                              (Vector2){ 30, SPOTLIGHT_HEIGHT + 30 + i*70 },
                              38, fontSpacing, textColor);
                }
            }
            
        EndDrawing();
    }
    
    UnloadFont(sfPro);
    CloseWindow();
    
    return 0;
}