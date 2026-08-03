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

#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include <chrono>
#include <functional>
#include <print>
#include <vector>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

Camera2D camera{};

long long get_time_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

void DrawArrowedLine(const Vector2 from, const Vector2 to, Color color,
                     double size = 11, double angle = PI / 6) {
  const Vector2 local{to - from};
  const Vector2 negative{Vector2Normalize(Vector2Negate(local)) * size};
  const Vector2 a{Vector2Rotate(negative, angle)};
  const Vector2 b{Vector2Rotate(negative, -angle)};
  DrawTriangle(to + a, to + b, to, color);
  DrawLineEx(from, to, 2, color);
}

class GameObject {
public:
  GameObject() : pos{} {};
  virtual ~GameObject() = default;
  Vector2 pos;
  Vector2 size;
  virtual void Update() = 0;
  virtual void Draw() = 0;
  void DrawGizmos() {
    DrawRectangleV(pos, Vector2(20, 20), Color(22, 22, 100, 82));
    DrawArrowedLine(pos, pos + Vector2(100, 0), RED);
    DrawArrowedLine(pos, pos + Vector2(0, 100), BLUE);
  }
};

class DNDSystem {
  bool isEnabled;
  bool isDrag;
  Vector2 dragMouseStart;
  Vector2 dragMouseOffset;
  GameObject *curEntity;

public:
  Vector2 size;

  DNDSystem(GameObject *_curEntity)
      : isEnabled(true), isDrag{}, dragMouseStart{}, dragMouseOffset{},
        curEntity(_curEntity), size{} {};

  void Update() {
    if (!isEnabled)
      return;
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
  DNDSystem dndSystem;

public:
  PacketsInput()
      : GameObject(), lastTimeGen(get_time_ms()), curPackets{},
        dndSystem(this) {
    size = {200, 200};
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
    const Vector2 textPadding{0, 20};
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

    DrawText(std::format("Incoming requests: {}", curPackets).c_str(), pos.x,
             pos.y + size.y + textPadding.y, 20, GRAY);
  }
};

class Processor : public GameObject {
private:
  DNDSystem dndSystem;

public:
  Processor() : GameObject(), dndSystem(this) {
    size = {150, 150};
    dndSystem.size = size;
  };

  void Update() { dndSystem.Update(); }

  void Draw() {
    const double padding{20};
    DrawRectangleV(pos, size, GREEN);
    DrawText("Processor", pos.x, pos.y + size.y + padding, 20, GRAY);
  };
};

class InventoryPanel : public GameObject {
private:
public:
  struct ItemConfig {
    GameObject *item;
    int price;
  };
  std::vector<ItemConfig> items;
  float gap{20};
  int padding{20};

  InventoryPanel() : GameObject(), items{} {};

  void Update() {
    int totalW{};
    float maxH{};
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    for (const auto &item : items) {
      totalW += item.item->size.x;
      maxH = std::max(maxH, item.item->size.y);
    }

    size = {static_cast<float>(totalW) + (items.size() - 1) * gap + padding * 2,
            maxH + padding * 2 + 30};
    pos = {screenW / 2.0f - size.x / 2, screenH - size.y - 20};

    float lastX{pos.x + padding};
    for (size_t i{}; i < items.size(); ++i) {
      items[i].item->pos = {lastX, pos.y + padding};
      lastX += gap + items[i].item->size.x;
    }
  }

  void Draw() { DrawRectangleLines(pos.x, pos.y, size.x, size.y, GREEN); };
};

class GameState {
public:
  static GameState &Get() {
    static GameState instance;
    return instance;
  }
  int playerMoney = 100;

private:
  GameState() = default;
};

class HUD : public GameObject {
private:
  Vector2 padding{20, 20};
  int fontSize{20};

public:
  HUD() : GameObject() {};

  void Update() {}

  void Draw() {
    int screenW{GetScreenWidth()};
    std::string moneyText{std::format("${}", GameState::Get().playerMoney)};

    DrawText(moneyText.c_str(),
             screenW - MeasureText(moneyText.c_str(), fontSize) - padding.x,
             padding.y, fontSize, BLACK);
  };
};
//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------

std::vector<std::unique_ptr<GameObject>> entities;
std::vector<std::unique_ptr<GameObject>> guiEntities;
//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

bool showDemoWindow = false;

void DrawImGui() {
  rlImGuiBegin();

  // show ImGui Content
  if (IsKeyPressed(KEY_F1)) {
    showDemoWindow = !showDemoWindow;
  }

  if (showDemoWindow) {

    ImGui::ShowDemoWindow(&showDemoWindow);

    if (ImGui::Begin("Test Window", &showDemoWindow) && showDemoWindow) {
      ImGui::TextUnformatted(ICON_FA_JEDI);
    }
    ImGui::End();
  }

  // end ImGui Content
  rlImGuiEnd();
}

static void UpdateDrawFrame(void) {
  SetMouseCursor(MOUSE_CURSOR_ARROW);
  for (const auto &entity : entities)
    entity->Update();
  for (const auto &entity : guiEntities)
    entity->Update();
  //----------------------------------------------------------------------------------

  // Draw
  //----------------------------------------------------------------------------------
  BeginDrawing();

  ClearBackground(RAYWHITE);

  BeginMode2D(camera);

  for (const auto &entity : entities)
    entity->Draw();

  if (showDemoWindow) {
    for (const auto &entity : entities)
      entity->DrawGizmos();
  }

  EndMode2D();

  for (const auto &entity : guiEntities)
    entity->Draw();

  if (showDemoWindow) {
    for (const auto &entity : guiEntities)
      entity->DrawGizmos();
  }

  DrawFPS(10, 10);
  DrawImGui();

  EndDrawing();
  //----------------------------------------------------------------------------------
};

struct ShopItem {
  std::string id;
  int price;
};

class ItemFactory {
public:
  using Creator = std::function<std::unique_ptr<GameObject>()>;

  static ItemFactory &Instance() {
    static ItemFactory instance;
    return instance;
  };

  void Register(const std::string &id, Creator creator) {
    creators[id] = std::move(creator);
  };

  std::unique_ptr<GameObject> Create(const std::string &id) {
    auto it = creators.find(id);
    return it != creators.end() ? it->second() : nullptr;
  };

private:
  std::unordered_map<std::string, Creator> creators;
};

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main() {
  ItemFactory::Instance().Register(
      "processor", [] { return std::make_unique<Processor>(); });

  // Initialization
  //--------------------------------------------------------------------------------------
  const int screenWidth = 1920;
  const int screenHeight = 1080;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
  InitWindow(screenWidth, screenHeight, "raylib - project_name");
  rlImGuiSetup(true);

  ImGuiStyle &style = ImGui::GetStyle();
  style.FontSizeBase = 20.0f;

  camera.target = {0, 0};
  camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  auto packetsInput = std::make_unique<PacketsInput>();
  auto processor = std::make_unique<Processor>();

  packetsInput->pos = {-300, 0};
  processor->pos = {100, 100};

  entities.push_back(std::move(packetsInput));
  entities.push_back(std::move(processor));

  guiEntities.push_back(std::make_unique<HUD>());

  auto inventoryPanel = std::make_unique<InventoryPanel>();
  auto processorGUI = std::make_unique<Processor>();
  auto processorGUI2 = std::make_unique<Processor>();

  processorGUI->size.x = 100;
  processorGUI->size.y = 100;
  processorGUI2->size.x = 100;
  processorGUI2->size.y = 100;

  inventoryPanel->items.emplace_back(processorGUI.get(), 20);
  inventoryPanel->items.emplace_back(processorGUI2.get(), 40);

  guiEntities.push_back(std::move(inventoryPanel));
  guiEntities.push_back(std::move(processorGUI));
  guiEntities.push_back(std::move(processorGUI2));

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
  rlImGuiShutdown();
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
