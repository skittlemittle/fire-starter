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
    World(int width, int height) : gridSize(width, height)
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
        roman = 3,
        apartments = 4,
        cylinder = 5,
        tower = 6,
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
/* coppers */
class Police {
  public:
    Police() {} // shut
    Police(olc::vi2d gridSize, int x, int y, const int8_t *grid) : mapSize(gridSize), position(x, y)
    {
        int width = mapSize.x;
        int height = mapSize.y;
        // init the nodes with the map
        nodes = new Node[width * height];
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pos = y * width + x;
                nodes[pos].x = x;
                nodes[pos].y = y;
                nodes[pos].isObstacle = (grid[pos] == World::TileStates::road) ? false : true;
                nodes[pos].parent = nullptr;
                nodes[pos].visited = false;
            }
        }
        // connect em up
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (y > 0)
                    nodes[y * width + x].neighbors.push_back(&nodes[(y - 1) * width + x]);
                if (y < height - 1)
                    nodes[y * width + x].neighbors.push_back(&nodes[(y + 1) * width + x]);
                if (x > 0)
                    nodes[y * width + x].neighbors.push_back(&nodes[y * width + (x - 1)]);
                if (x < width - 1)
                    nodes[y * width + x].neighbors.push_back(&nodes[y * width + (x + 1)]);
            }
        }
        startNode = &nodes[y * mapSize.x + x];
    }

  private:
    int moveStep = 0; // cant have em moving every frame
    olc::vi2d mapSize;
    olc::vi2d position;
    struct Node {
        bool isObstacle = false;
        bool visited = false;
        float globalGoal;
        float localGoal;
        int x;
        int y;
        std::vector<Node *> neighbors;
        Node *parent;
    };
    Node *nodes = nullptr;
    Node *targetNode = nullptr;
    Node *startNode = nullptr;
    std::vector<Node *> path;
    void search();

  public:
    olc::vi2d getPos() { return position; }
    bool move();
    void setTarget(const olc::vi2d &targetPos)
    {
        targetNode = &nodes[targetPos.y * mapSize.x + targetPos.x];
        search();
    };
};

/* chase the player, returns true if it hit the player*/
bool Police::move()
{
    if (++moveStep % 180 == 0) {
        startNode = path[path.size() - 1];
        path.pop_back();
        position.x = startNode->x;
        position.y = startNode->y;
    }

    if (startNode == targetNode)
        return true;
    return false;
}
/* A* */
void Police::search()
{
    // reset
    for (int y = 0; y < mapSize.y; y++) {
        for (int x = 0; x < mapSize.x; x++) {
            int pos = y * mapSize.x + x;
            nodes[pos].visited = false;
            nodes[pos].localGoal = INFINITY;
            nodes[pos].globalGoal = INFINITY;
            nodes[pos].parent = nullptr;
        }
    }
    // helpers
    auto distance = [](Node *a, Node *b) {
        return sqrt((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y));
    };
    auto heuristic = [distance](Node *a, Node *b) { return distance(a, b); };

    Node *currentNode = startNode;
    startNode->localGoal = 0.0f;
    startNode->globalGoal = heuristic(startNode, targetNode);
    // list of nodes to test
    std::list<Node *> nodesToTest;
    nodesToTest.push_back(startNode);

    while (!nodesToTest.empty()) {
        // sort by the global goal to bias the search
        nodesToTest.sort(
            [](const Node *a, const Node *b) { return a->globalGoal < b->globalGoal; });
        // dont revisit nodes
        while (!nodesToTest.empty() && nodesToTest.front()->visited)
            nodesToTest.pop_front();
        // may have emptied it there
        if (nodesToTest.empty())
            break;

        currentNode = nodesToTest.front();
        currentNode->visited = true;
        // update da neighbors
        for (auto neighbor : currentNode->neighbors) {
            if (!neighbor->visited && neighbor->isObstacle == 0)
                nodesToTest.push_back(neighbor);

            float possibleLowerGoal = currentNode->localGoal + distance(currentNode, neighbor);
            if (possibleLowerGoal < neighbor->localGoal) {
                neighbor->parent = currentNode;
                neighbor->localGoal = possibleLowerGoal;
                neighbor->globalGoal = neighbor->localGoal + heuristic(neighbor, targetNode);
            }
        }
    }

    // make the path
    if (targetNode != nullptr) {
        Node *p = targetNode;
        while (p->parent != nullptr) {
            path.push_back(p);
            p = p->parent;
            // std::cout << p->x << " " << p->y << "\n";
        }
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
    Police *coppers = nullptr;

  public:
    bool OnUserCreate() override
    {
        spriteSheet = new olc::Sprite("assets/tileset.png");
        map = world.getGrid();
        // >:( manually placing coppers!!!! BAD
        coppers = new Police[2];
        coppers[0] = Police(WorldSize, 1, 0, map);
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

        // make cops chase
        coppers[0].setTarget(vSelected);
        if (coppers[0].move())
            player.decrementLives();

        SetPixelMode(olc::Pixel::MASK);
        map = world.getGrid(); // get the new world before each draw
        for (int y = 0; y < WorldSize.y; y++) {
            for (int x = 0; x < WorldSize.x; x++) {
                olc::vi2d vWorld = ToScreen(x, y);
                int8_t tileState = map[y * WorldSize.x + x];

                if (tileState < 0) {
                    // burning
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y * 2, spriteSheet,
                                      3 * TileSize.x, 3 * TileSize.y, TileSize.x, TileSize.y * 3);
                    world.stepBurn(x, y, fElapsedTime);
                }

                switch (tileState) {
                case World::burnt:
                    DrawPartialSprite(vWorld.x, vWorld.y, spriteSheet, 2 * TileSize.x, 0,
                                      TileSize.x, TileSize.y);
                    break;
                case World::road:
                    DrawPartialSprite(vWorld.x, vWorld.y, spriteSheet, 1 * TileSize.x, 0,
                                      TileSize.x, TileSize.y);
                    break;
                case World::hoose:
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y, spriteSheet, 0 * TileSize.x,
                                      1 * TileSize.y, TileSize.x, TileSize.y * 2);
                    break;
                case World::roman:
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y, spriteSheet, 1 * TileSize.x,
                                      1 * TileSize.y, TileSize.x, TileSize.y * 2);
                    break;
                case World::apartments:
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y * 2, spriteSheet,
                                      0 * TileSize.x, 3 * TileSize.y, TileSize.x, TileSize.y * 3);
                    break;
                case World::cylinder:
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y * 3, spriteSheet,
                                      0 * TileSize.x, 6 * TileSize.y, TileSize.x, TileSize.y * 4);
                    break;
                case World::tower:
                    DrawPartialSprite(vWorld.x, vWorld.y - TileSize.y * 3, spriteSheet,
                                      1 * TileSize.x, 6 * TileSize.y, TileSize.x, TileSize.y * 4);
                    break;
                }
                // draw coppers
                olc::vi2d pPos = ToScreen(coppers[0].getPos().x, coppers[0].getPos().y);
                DrawPartialSprite(pPos.x, pPos.y, spriteSheet, 3 * TileSize.x, 0, TileSize.x,
                                  TileSize.y);
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