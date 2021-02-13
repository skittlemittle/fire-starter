#define OLC_PGE_APPLICATION
#include "./include/olcPixelGameEngine.h"
#include <iostream>
#include <unordered_map>

class Player {
  public:
    /*default, 3 lives, matchstick*/
    Player() : SelectedWeapon(Player::Weapons::match), lives(3) {}

  private:
    enum Weapons { match = 1, molotov = 2, lighter = 3 };
    Player::Weapons SelectedWeapon;
    int8_t lives;
    int32_t points = 0;

  public:
    int32_t getScore() { return points; };
    void incrementScore(int8_t points) { this->points += points; };
    int8_t getLives() { return lives; }
    void decrementLives()
    {
        if (lives > 0)
            lives--;
    }
    Player::Weapons currentWeapon() { return this->SelectedWeapon; }
    void selectWeapon(Player::Weapons weapon) { SelectedWeapon = weapon; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/* endsville */
class World {
  public:
    World(int width = 14, int height = 10) : gridSize(width, height)
    {
        // map "generation"
        grid = new int8_t[width * height]{
            0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, //
            0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, //
            0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, //
            0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, //
            0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, //
            0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, //
            1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, //
            0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, //
            0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, //
            0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, //
        };

        srand(time(NULL));
        for (int i = 0; i < gridSize.x * gridSize.y; i++) {
            if (grid[i] != 1) {
                grid[i] = 2 + (rand() % 5); // magic number >:(
            }
        }
    }

  private:
    olc::vi2d gridSize;
    int8_t *grid = nullptr;
    std::unordered_map<int, float> burningTiles;

  public:
    enum TileStates {
        burning = -1,
        burnt = 0,
        road = 1,
        hoose = 2,
        tower = 3,
        roman = 4,
        cylinder = 5,
        building = 6,
    };
    int8_t burnBuilding(const olc::vi2d &vTarget);
    void rebuild(); // there is no final victory
    const int8_t *getGrid() { return grid; }
    void stepBurn(int8_t x, int8_t y, float time);
};

/* burn tile if there is a building on it */
int8_t World::burnBuilding(const olc::vi2d &Target)
{
    int target = Target.y * gridSize.x + Target.x;
    if (grid[target] > World::TileStates::road) {
        int8_t i = grid[target];
        grid[target] = World::TileStates::burning;
        burningTiles[target] = 1;
        return i;
    }
    return 0;
}
/* randomly rebuild a few sites */
void World::rebuild()
{
    int glen = gridSize.x * gridSize.y;
    for (int i = 0; i < 5; i++) { // magic numbers >:(
        int target = rand() % glen;
        if (grid[target] == TileStates::burnt)
            grid[target] = 2 + (rand() % 5);
    }
}
/* increment burning "counter" */
void World::stepBurn(int8_t x, int8_t y, float time)
{
    int target = y * gridSize.x + x;
    if (burningTiles[target] <= 0) {
        grid[target] = World::TileStates::burnt;
        burningTiles.erase(target);
    }
    else {
        burningTiles[target] -= time;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/* main */
class FireStarter : public olc::PixelGameEngine {
  public:
    FireStarter() { sAppName = "Firestarter"; }

  private:
    olc::vi2d WorldSize = {14, 10};
    olc::vi2d TileSize = {40, 20};
    olc::vi2d Origin = {5, 1};
    olc::vi2d vSelected = olc::vi2d(Origin);
    olc::Sprite *spriteSheet = nullptr;
    const int8_t *map = nullptr;
    World world = World(WorldSize.x, WorldSize.y);
    Player player = Player();

  public:
    bool OnUserCreate() override
    {
        spriteSheet = new olc::Sprite("assets/isometric_demo.png");
        map = world.getGrid();
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        Clear(olc::WHITE);

        auto ToScreen = [&](int x, int y) {
            return olc::vi2d{(Origin.x * TileSize.x) + (x - y) * (TileSize.x / 2),
                             (Origin.y * TileSize.y) + (x + y) * (TileSize.y / 2)};
        };

        // event handleing
        if (GetKey(olc::Key::LEFT).bPressed)
            vSelected += {-1, +0};
        if (GetKey(olc::Key::RIGHT).bPressed)
            vSelected += {+1, +0};
        if (GetKey(olc::Key::UP).bPressed)
            vSelected += {0, -1};
        if (GetKey(olc::Key::DOWN).bPressed)
            vSelected += {0, +1};
        if (GetKey(olc::Key::ENTER).bPressed)
            player.incrementScore(world.burnBuilding(vSelected));

        SetPixelMode(olc::Pixel::MASK);

        map = world.getGrid(); // get the new world before each draw
        for (int y = 0; y < WorldSize.y; y++) {
            for (int x = 0; x < WorldSize.x; x++) {
                olc::vi2d vWorld = ToScreen(x, y);
                int8_t tileState = map[y * WorldSize.x + x];

                if (tileState < 0) {
                    // burning
                    DrawPartialSprite(vWorld.x, vWorld.y, spriteSheet, 3 * TileSize.x, 0,
                                      TileSize.x, TileSize.y);
                    world.stepBurn(x, y, fElapsedTime);
                }

                switch (tileState) {
                case 0:
                    // Invisble Tile
                    DrawPartialSprite(vWorld.x, vWorld.y, spriteSheet, 1 * TileSize.x, 0,
                                      TileSize.x, TileSize.y);
                    break;
                case 1:
                    // Visible Tile
                    DrawPartialSprite(vWorld.x, vWorld.y, spriteSheet, 2 * TileSize.x, 0,
                                      TileSize.x, TileSize.y);
                    break;
                case 2:
                    // Tree
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y, spriteSheet, 0 * TileSize.x,
                                      1 * TileSize.y, TileSize.x, TileSize.y * 2);
                    break;
                case 3:
                    // Spooky Tree
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y, spriteSheet, 1 * TileSize.x,
                                      1 * TileSize.y, TileSize.x, TileSize.y * 2);
                    break;
                case 4:
                    // Beach
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y, spriteSheet, 2 * TileSize.x,
                                      1 * TileSize.y, TileSize.x, TileSize.y * 2);
                    break;
                case 5:
                    // Water
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y, spriteSheet, 3 * TileSize.x,
                                      1 * TileSize.y, TileSize.x, TileSize.y * 2);
                    break;
                }
            }
        }

        // draw the selector highlight
        SetPixelMode(olc::Pixel::ALPHA);
        olc::vi2d vSelectedWorld = ToScreen(vSelected.x, vSelected.y);
        DrawPartialSprite(vSelectedWorld.x, vSelectedWorld.y, spriteSheet, 0 * TileSize.x, 0,
                          TileSize.x, TileSize.y);

        // regen buildings
        if ((rand() % 256) > 254)
            world.rebuild();

        // cring text
        SetPixelMode(olc::Pixel::NORMAL);
        DrawString(4, 4, "Lives: " + std::to_string(player.getLives()), olc::BLACK);
        DrawString(4, 14, "Score: " + std::to_string(player.getScore()), olc::BLACK);
        return true;
    }
};

int main()
{
    FireStarter fs;
    if (fs.Construct(512, 480, 2, 2))
        fs.Start();

    return 0;
}