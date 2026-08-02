/*******************************************************************************************
 *
 *   project_name
 *
 *   This example has been created using raylib 1.0 (www.raylib.com)
 *   raylib is licensed under an unmodified zlib/libpng license (View raylib.h
 * for details)
 *
 *   Copyright (c) 2026 Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include <chrono>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

Camera2D camera{};

long long get_time_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

class GameObject {
public:
  GameObject() : pos{} {};
  Vector2 pos;
  void Update();
  void Draw();
};

class DNDSystem {
  bool isDrag;
  Vector2 dragMouseStart;
  Vector2 dragMouseOffset;
  GameObject *curEntity;

public:
  Vector2 size;

  DNDSystem(GameObject *_curEntity)
      : isDrag{}, dragMouseStart{}, dragMouseOffset{}, curEntity(_curEntity),
        size{} {};

  void Update() {
    const Vector2 pos{curEntity->pos};
    const Vector2 mousePos{GetScreenToWorld2D(GetMousePosition(), camera)};
    const bool isHover = mousePos.x > pos.x && mousePos.x < pos.x + size.x &&
                         mousePos.y > pos.y && mousePos.y < pos.y + size.y;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isDrag && isHover) {
      isDrag = true;
      dragMouseStart = mousePos;
      dragMouseOffset = mousePos - pos;
    }

    if (isDrag) {
      curEntity->pos = mousePos - dragMouseOffset;
    }

    if (isDrag && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      isDrag = false;
      dragMouseStart = {};
      dragMouseOffset = {};
    }

    if (isDrag) {
      SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
    } else if (isHover) {
      SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }
  }
};

class PacketsInput : public GameObject {
private:
  long long lastTimeGen;
  long long packetGenTimeout{1000};
  const int rows{7};
  const int cols{3};
  int curPackets;
  int maxPackets{rows * cols};
  Vector2 size{200, 200};
  DNDSystem dndSystem;

public:
  PacketsInput()
      : GameObject(), lastTimeGen(get_time_ms()), curPackets{},
        dndSystem(this) {
    dndSystem.size = size;
  };

  void Update() {
    dndSystem.Update();

    if (curPackets < maxPackets &&
        get_time_ms() - lastTimeGen >= packetGenTimeout) {
      lastTimeGen = get_time_ms();
      curPackets += 1;
    }
  }

  void Draw() {
    const Vector2 textPadding{0, -40};
    const Vector2 padding{20, 20};
    const int gap{20};
    const Vector2 itemSize{(size.x - 2 * padding.x - (cols - 1) * gap) / cols,
                           (size.y - 2 * padding.y - (rows - 1) * gap) / rows};
    DrawRectangleLines(pos.x, pos.y, size.x, size.y, GREEN);

    for (int row{}; row < rows; ++row) {
      for (int col{}; col < cols; ++col) {
        const int nrow{rows - 1 - row};
        double x{padding.x + col * itemSize.x + pos.x + gap * (col)};
        double y{padding.y + nrow * itemSize.y + pos.y + gap * (nrow)};
        DrawRectangleLines(x, y, itemSize.x, itemSize.y, LIGHTGRAY);

        if (static_cast<int>(cols * row + col + 1) > curPackets) {
          continue;
        }

        DrawRectangle(x, y, itemSize.x, itemSize.y, GREEN);
      }
    }

    DrawText(std::format("Incoming requests: {}", curPackets).c_str(),
             pos.x + textPadding.x, pos.y + textPadding.y, 20, BLACK);
  }
};

void DrawArrowedLine(const Vector2 from, const Vector2 to, Color color,
                     double size = 11, double angle = PI / 6) {
  const Vector2 local{to - from};
  const Vector2 negative{Vector2Normalize(Vector2Negate(local)) * size};
  const Vector2 a{Vector2Rotate(negative, angle)};
  const Vector2 b{Vector2Rotate(negative, -angle)};
  DrawTriangle(to + a, to + b, to, color);
  DrawLineEx(from, to, 2, color);
}

class Processor : public GameObject {
private:
  Vector2 size{200, 200};
  DNDSystem dndSystem;

public:
  Processor() : GameObject(), dndSystem(this) { dndSystem.size = size; };

  void Update() { dndSystem.Update(); }

  void Draw() {
    const double padding{20};
    DrawRectangleV(pos, size, GREEN);
    DrawText("Processor", pos.x + padding, pos.y + padding, 20, BLACK);
  };
};

//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
PacketsInput packetsInput{};
Processor processor{};

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void); // Update and draw one frame

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main() {
  // Initialization
  //--------------------------------------------------------------------------------------
  const int screenWidth = 1920;
  const int screenHeight = 1080;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(screenWidth, screenHeight, "raylib - project_name");

  camera.target = {0, 0};
  camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  packetsInput.pos = {-300, 0};
  processor.pos = {100, 100};

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    UpdateDrawFrame();
  }
#endif

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}

// Update and draw game frame
static void UpdateDrawFrame(void) {
  // Update
  //----------------------------------------------------------------------------------

  SetMouseCursor(MOUSE_CURSOR_ARROW);
  packetsInput.Update();
  processor.Update();
  //----------------------------------------------------------------------------------

  // Draw
  //----------------------------------------------------------------------------------
  BeginDrawing();

  ClearBackground(RAYWHITE);

  BeginMode2D(camera);

  DrawArrowedLine({0, 0}, {100, 0}, RED);
  DrawArrowedLine({0, 0}, {0, 100}, BLUE);

  packetsInput.Draw();
  processor.Draw();

  EndMode2D();

  DrawFPS(10, 10);

  EndDrawing();
  //----------------------------------------------------------------------------------
}
