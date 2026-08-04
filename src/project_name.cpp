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
#include <memory>
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

class Component {
public:
  bool destroyed;
  bool enabled{true};
  class GameObject *owner = nullptr;
  virtual ~Component() = default;
  virtual void OnAttach() {}
  virtual void OnDetach() {}
  virtual void Update() {}
  virtual void Draw() {}
};

class GameObject {
public:
  GameObject() : pos{} {};
  virtual ~GameObject() = default;
  Vector2 pos;
  Vector2 size;

  void DrawGizmos() {
    DrawRectangleV(pos, Vector2(20, 20), Color(22, 22, 100, 82));
    DrawArrowedLine(pos, pos + Vector2(100, 0), RED);
    DrawArrowedLine(pos, pos + Vector2(0, 100), BLUE);
  }
  std::vector<std::unique_ptr<Component>> components;

  template <typename T, typename... Args> T *AddComponent(Args &&...args) {
    auto c = std::make_unique<T>(std::forward<Args>(args)...);
    c->owner = this;
    c->OnAttach();
    T *ptr = c.get();
    components.push_back(std::move(c));
    return ptr;
  }

  template <typename T> T *GetComponent() {
    for (auto &c : components)
      if (auto t = dynamic_cast<T *>(c.get()))
        return t;
    return nullptr;
  }

  template <typename T> void RemoveComponent() {
    for (auto &c : components) {
      if (dynamic_cast<T *>(c.get())) {
        c->destroyed = true;
        return;
      }
    }
  }

  void RemoveComponent(Component *target) {
    if (target)
      target->destroyed = true;
  }

  void Update() {
    for (auto &c : components)
      if (c->enabled && !c->destroyed)
        c->Update();

    SweepDestroyed();
  }

  void Draw() {
    for (auto &c : components)
      if (c->enabled && !c->destroyed)
        c->Draw();
  }

private:
  void SweepDestroyed() {
    for (auto &c : components) {
      if (c->destroyed)
        c->OnDetach();
    }
    components.erase(std::remove_if(components.begin(), components.end(),
                                    [](const std::unique_ptr<Component> &c) {
                                      return c->destroyed;
                                    }),
                     components.end());
  }
};

class DNDComponent : public Component {
  bool isDrag;
  Vector2 dragMouseStart;
  Vector2 dragMouseOffset;

public:
  DNDComponent() : isDrag{}, dragMouseStart{}, dragMouseOffset{} {};

  void Update() override {
    const Vector2 pos{owner->pos};
    const Vector2 mousePos{GetScreenToWorld2D(GetMousePosition(), camera)};
    const bool isHover =
        mousePos.x > pos.x && mousePos.x < pos.x + owner->size.x &&
        mousePos.y > pos.y && mousePos.y < pos.y + owner->size.y;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isDrag && isHover) {
      isDrag = true;
      dragMouseStart = mousePos;
      dragMouseOffset = mousePos - pos;
    }

    if (isDrag) {
      owner->pos = mousePos - dragMouseOffset;
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

class PacketsInputComponent : public Component {
private:
  const int rows{7};
  const int cols{3};
  const Vector2 textPadding{0, 20};
  const Vector2 padding{20, 20};
  const int gap{20};
  int curPackets;
  int maxPackets{rows * cols};
  long long lastTimeGen;
  long long packetGenTimeout{1000};

public:
  PacketsInputComponent() : curPackets{}, lastTimeGen(get_time_ms()) {};

  void OnAttach() override { owner->size = {200, 200}; }

  void Update() override {
    if (curPackets < maxPackets &&
        get_time_ms() - lastTimeGen >= packetGenTimeout) {
      lastTimeGen = get_time_ms();
      curPackets += 1;
    }
  }

  void Draw() override {
    const auto &size = owner->size;
    const auto &pos = owner->pos;
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

class ProcessorComponent : public Component {
private:
public:
  ProcessorComponent() = default;

  void OnAttach() override { owner->size = {150, 150}; }

  void Draw() override {
    const double padding{20};
    DrawRectangleV(owner->pos, owner->size, GREEN);
    DrawText("Processor", owner->pos.x, owner->pos.y + owner->size.y + padding,
             20, GRAY);
  };
};

std::vector<std::unique_ptr<GameObject>> entities;
std::vector<std::unique_ptr<GameObject>> guiEntities;

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

struct ShopItem {
  std::string id;
  int price;
};

class InventoryPanelComponent : public Component {
public:
  std::vector<GameObject *> items;
  std::vector<ShopItem> configs;
  float gap{20};
  int padding{20};

  InventoryPanelComponent() : items{} {};

  void AddShopItem(ShopItem config) {
    // auto previewItem = ItemFactory::Instance().Create(config.id);
    // auto preview = std::make_unique<PreviewItem>(previewItem.get());
    // previewItem->size = {150, 150};
    // items.push_back(previewItem.get());
    // configs.push_back(config);
    // guiEntities.push_back(std::move(previewItem));
    // guiEntities.push_back(std::move(preview));
  };

  void Update() {
    int totalW{};
    float maxH{};
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    for (const auto &item : items) {
      totalW += item->size.x;
      maxH = std::max(maxH, item->size.y);
    }

    owner->size = {static_cast<float>(totalW) + (items.size() - 1) * gap +
                       padding * 2,
                   maxH + padding * 2 + 30};
    owner->pos = {screenW / 2.0f - owner->size.x / 2,
                  screenH - owner->size.y - 20};

    float lastX{owner->pos.x + padding};
    for (size_t i{}; i < items.size(); ++i) {
      items[i]->pos = {lastX, owner->pos.y + padding};
      lastX += gap + items[i]->size.x;
    }
  }

  void Draw() {
    DrawRectangleLines(owner->pos.x, owner->pos.y, owner->size.x, owner->size.y,
                       GREEN);
  };
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

class HUDComponent : public Component {
private:
  Vector2 padding{20, 20};
  int fontSize{20};

public:
  void Update() override {};

  void Draw() override {
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

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main() {
  // ItemFactory::Instance().Register(
  //     "processor", [] { return std::make_unique<Processor>(); });
  // std::vector<ShopItem> shopItems{
  //     {"processor", 20},
  // };

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

  auto packetsInput = std::make_unique<GameObject>();
  packetsInput->AddComponent<PacketsInputComponent>();
  packetsInput->AddComponent<DNDComponent>();

  auto processor = std::make_unique<GameObject>();
  processor->AddComponent<ProcessorComponent>();
  processor->AddComponent<DNDComponent>();

  packetsInput->pos = {-300, 0};
  processor->pos = {100, 100};

  auto hud = std::make_unique<GameObject>();
  hud->AddComponent<HUDComponent>();

  auto inventoryPanel = std::make_unique<GameObject>();
  inventoryPanel->AddComponent<InventoryPanelComponent>();

  // for (const auto &shopItem : shopItems)
  //   inventoryPanel->AddShopItem(shopItem);

  entities.push_back(std::move(packetsInput));
  entities.push_back(std::move(processor));

  guiEntities.push_back(std::move(hud));
  guiEntities.push_back(std::move(inventoryPanel));

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
