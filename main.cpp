#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <unordered_map>
#include <list>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace std;

const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 10;

struct Room {
    string name;
    list<string> items;
    vector<Room*> neighbors;
    int x, y; // For ASCII map
};

struct Monster {
    string name;
    Room* location;
    int hp;
};

//Graph Dungeon Map
class Dungeon {
public:
    unordered_map<string, Room*> rooms;

    void addRoom(const string &name, int x, int y) {
        rooms[name] = new Room{name, {}, {}, x, y};
    }

    void connectRooms(const string &a, const string &b) {
        rooms[a]->neighbors.push_back(rooms[b]);
        rooms[b]->neighbors.push_back(rooms[a]);
    }

    void printMap(Room* playerRoom, const queue<Monster*> &monsterQueue) 
    {
        int innerWidth = MAP_WIDTH;
        int innerHeight = MAP_HEIGHT;
        int width = innerWidth + 2;
        int height = innerHeight + 2;
        vector<vector<char>> map(height, vector<char>(width, ' '));

        // Draw borders with corners
        for (int x = 0; x < width; ++x) {
            map[0][x] = (x == 0 || x == width - 1) ? '+' : '-';
            map[height - 1][x] = (x == 0 || x == width - 1) ? '+' : '-';
        }
        for (int y = 1; y < height - 1; ++y) {
            map[y][0] = '|';
            map[y][width - 1] = '|';
        }

        auto inBounds = [&](int x, int y){
            return x >= 1 && x < width - 1 && y >= 1 && y < height - 1;
        };

        // Draw rooms and connections within bounds
        for (auto it = rooms.begin(); it != rooms.end(); ++it) {
            Room* room = it->second;
            int rx = room->x + 1;
            int ry = room->y + 1;
            if (inBounds(rx, ry)) map[ry][rx] = room->items.empty() ? 'R' : 'I';

            for (Room* nbr : room->neighbors) {
                int nx = nbr->x + 1;
                int ny = nbr->y + 1;
                if (ny == ry) {
                    int minX = min(nx, rx);
                    int maxX = max(nx, rx);
                    for (int x = minX + 1; x < maxX; x++) if (inBounds(x, ry)) map[ry][x] = '-';
                }
                if (nx == rx) {
                    int minY = min(ny, ry);
                    int maxY = max(ny, ry);
                    for (int y = minY + 1; y < maxY; y++) if (inBounds(rx, y)) map[y][rx] = '|';
                }
            }
        }

        // Player marker
        int px = playerRoom->x + 1;
        int py = playerRoom->y + 1;
        if (inBounds(px, py)) map[py][px] = 'P';

        // Monsters
        queue<Monster*> temp = monsterQueue;
        while (!temp.empty()) {
            Monster* m = temp.front(); temp.pop();
            int mx = m->location->x + 1;
            int my = m->location->y + 1;
            if (!inBounds(mx, my)) continue;
            if (map[my][mx] == 'P') continue;
            map[my][mx] = 'M';
        }

        // Print map
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) cout << map[y][x];
            cout << '\n';
        }
        // Legend for the current room
        cout << "== Room: " << playerRoom->name << " ==\n";
        if (!playerRoom->items.empty()) {
            cout << "Items: ";
            bool first = true;
            for (const auto &it : playerRoom->items) {
                if (!first) cout << ", ";
                cout << it;
                first = false;
            }
            cout << '\n';
        } else {
            cout << "Items: (none)\n";
        }
        // List monsters in the same room
        queue<Monster*> temp2 = monsterQueue;
        bool anyMon = false;
        cout << "Monsters: ";
        while (!temp2.empty()) {
            Monster* m = temp2.front(); temp2.pop();
            if (m->location == playerRoom) {
                if (anyMon) cout << ", ";
                cout << m->name << " (HP=" << m->hp << ")";
                anyMon = true;
            }
        }
        if (!anyMon) cout << "(none)";
        cout << '\n';
    }
};

// --- Tree: Skill Tree ---
struct SkillNode {
    string skillName;
    vector<SkillNode*> children;
};

class SkillTree {
public:
    SkillNode* root;
    SkillTree(string rootSkill) { root = new SkillNode{rootSkill}; }
};

// --- Player ---
struct Player {
    Room* currentRoom;
    list<string> inventory;
    stack<Room*> moveHistory;
    int hp;
    bool dodgeActive;
    int xp = 0;
    int level = 1;
    int baseDamage = 5;
    int maxHp = 30;
    int healthUpgrades = 0;
    int damageUpgrades = 0;
    int money = 0;
};

unordered_map<string, int> itemValues = { {"Gold", 5}, {"Diamond", 20}, {"Coal", 2} };

queue<Monster*> monsterQueue;

// --- Game Functions ---
void movePlayer(Player &player, Room* nextRoom) {
    player.moveHistory.push(player.currentRoom);
    player.currentRoom = nextRoom;
    cout << "Moved to " << nextRoom->name << endl;
}

void undoMove(Player &player) {
    if (!player.moveHistory.empty()) {
        player.currentRoom = player.moveHistory.top();
        player.moveHistory.pop();
        cout << "Rewound to " << player.currentRoom->name << endl;
    } else cout << "No moves to undo!" << endl;
}

// BFS Example
Room* bfsFindItem(Room* start, const string &item) {
    queue<Room*> q;
    unordered_map<Room*, bool> visited;
    q.push(start);
    visited[start] = true;

    while (!q.empty()) {
        Room* r = q.front(); q.pop();
        if (find(r->items.begin(), r->items.end(), item) != r->items.end())
            return r;
        for (Room* neighbor : r->neighbors) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                q.push(neighbor);
            }
        }
    }
    return nullptr;
}

// Inventory Sorting
void sortInventory(Player &player) { player.inventory.sort(); }

void printSkillTree(const Player &player) {
    cout << "== Skill Tree ==\n";
    cout << "Health Path: +5 HP per tier (0-3). Current: " << player.healthUpgrades << "/3\n";
    cout << "Damage Path: +5 Damage per tier (0-3). Current: " << player.damageUpgrades << "/3\n";
}

void applyUpgrade(Player &player, const string &choice) {
    if (choice == "health") {
        if (player.healthUpgrades < 3) {
            player.healthUpgrades++;
            player.maxHp += 5;
            player.hp = min(player.hp + 5, player.maxHp);
            cout << "You feel sturdier. Max HP is now " << player.maxHp << "\n";
        } else {
            cout << "Health path already maxed.\n";
        }
    } else if (choice == "damage") {
        if (player.damageUpgrades < 3) {
            player.damageUpgrades++;
            player.baseDamage += 5;
            cout << "Your strikes hit harder. Base damage is now " << player.baseDamage << "\n";
        } else {
            cout << "Damage path already maxed.\n";
        }
    } else {
        cout << "Unknown upgrade. Choose 'health' or 'damage'.\n";
    }
}

int xpForNextLevel(int level) {
    return 10 + (level - 1) * 10;
}

void grantXPAndHandleLevel(Player &player, int xp) {
    player.xp += xp;
    while (player.xp >= xpForNextLevel(player.level)) {
        player.xp -= xpForNextLevel(player.level);
        player.level++;
        cout << "== Level Up! You are now level " << player.level << " ==\n";
        printSkillTree(player);
        cout << "Choose an upgrade (health/damage): ";
        string choice; getline(cin, choice);
        applyUpgrade(player, choice);
    }
}

// Removed monsterTurn function entirely

bool combat(Player &player, Monster *m) {
    cout << "== Combat: " << m->name << " (HP=" << m->hp << ") ==\n";
    bool inCombat = true;
    bool playerAlive = true;
    while (inCombat) {
        cout << "HP: " << player.hp << "/" << player.maxHp << "\n";
        cout << "Commands: swing, double swing, dodge, run\n> ";
        string cmd; getline(cin, cmd);
        if (cmd == "swing") {
            int hitRoll = rand() % 100;
            if (hitRoll < 80) {
                int dmg = player.baseDamage + (rand() % 5); // base + 0-4
                m->hp -= dmg;
                cout << "You swing for " << dmg << ". (" << m->name << " HP=" << max(0, m->hp) << ")\n";
            } else {
                cout << "You miss!\n";
            }
        } else if (cmd == "double swing") {
            int hitRoll = rand() % 100;
            if (hitRoll < 55) {
                int dmg1 = max(1, player.baseDamage - 1) + (rand() % 5);
                int dmg2 = max(1, player.baseDamage - 1) + (rand() % 5);
                int total = dmg1 + dmg2;
                m->hp -= total;
                cout << "Double swing hits for " << total << ". (" << m->name << " HP=" << max(0, m->hp) << ")\n";
            } else {
                cout << "You overextend and miss both swings!\n";
            }
        } else if (cmd == "dodge") {
            player.dodgeActive = true;
            cout << "You prepare to dodge.\n";
        } else if (cmd == "run") {
            cout << "You attempt to flee...\n";
            int fleeRoll = rand() % 100;
            if (fleeRoll < 60) { cout << "You escape!\n"; return true; }
            else cout << "You fail to escape!\n";
        } else if (cmd == "skills") {
            printSkillTree(player);
            continue;
        } else {
            cout << "Unknown command.\n";
            continue;
        }

        if (m->hp <= 0) {
            cout << m->name << " is defeated!\n";
            return true; // victory
        }

        // Monster's turn
        int hitRoll = rand() % 100;
        int hitChance = player.dodgeActive ? 35 : 85;
        player.dodgeActive = false;
        if (hitRoll < hitChance) {
            int dmg = 3 + (rand() % 5);
            player.hp -= dmg;
            cout << m->name << " hits you for " << dmg << "! (HP=" << max(0, player.hp) << ")\n";
            if (player.hp <= 0) { playerAlive = false; return false; }
        } else {
            cout << m->name << " misses!\n";
        }
    }
    return playerAlive;
}

int main() {
    srand(time(0));

    Dungeon dungeon;
    dungeon.addRoom("Entrance", 1, 1);
    dungeon.addRoom("Hall", 10, 1);
    dungeon.addRoom("Treasure", 18, 1);
    dungeon.addRoom("Side", 10, 8);

    dungeon.connectRooms("Entrance", "Hall");
    dungeon.connectRooms("Hall", "Treasure");
    dungeon.connectRooms("Hall", "Side");

    dungeon.rooms["Treasure"]->items.push_back("Gold");
    dungeon.rooms["Side"]->items.push_back("Potion");
    dungeon.rooms["Treasure"]->items.push_back("Diamond");
    dungeon.rooms["Side"]->items.push_back("Coal");

    Player player{dungeon.rooms["Entrance"], {}, {}, 30, false};
    player.maxHp = 30;
    player.hp = player.maxHp;
    player.baseDamage = 5;
    player.inventory.push_back("Sword");

    SkillTree skills("Basic Attack");
    skills.root->children.push_back(new SkillNode{"Fireball"});

    Monster* goblin = new Monster{"Goblin", dungeon.rooms["Hall"], 10};
    Monster* orc = new Monster{"Orc", dungeon.rooms["Side"], 15};
    monsterQueue.push(goblin);
    monsterQueue.push(orc);

    string command;
    while (true) {
        dungeon.printMap(player.currentRoom, monsterQueue);
        cout << "HP: " << player.hp << "  Money: $" << player.money << "\n";
        cout << "You are in: " << player.currentRoom->name << "\n";

        cout << "\nCommand (move, pick [item], sell [item], inventory, skills, quit): ";
        getline(cin, command);

        if (command == "quit") break;
        else if (command == "move") {
            if (player.currentRoom->neighbors.empty()) {
                cout << "There is nowhere to move from here." << endl;
            } else {
                // Move to the first neighbor; if multiple, cycle by picking a random neighbor for variety
                Room* next = player.currentRoom->neighbors[ rand() % player.currentRoom->neighbors.size() ];
                movePlayer(player, next);
            }
        }
        else if (command == "pick" || (command.size() >= 5 && command.substr(0, 5) == "pick ")) {
            auto &items = player.currentRoom->items;
            if (command == "pick") {
                if (items.empty()) {
                    cout << "There are no items here." << endl;
                } else {
                    // Pick up all items in the room
                    cout << "You pick up: ";
                    bool first = true;
                    bool potionPicked = false;
                    for (const auto &itName : items) {
                        if (!first) cout << ", ";
                        cout << itName;
                        player.inventory.push_back(itName);
                        if (itName == "Potion") {
                            int heal = 10;
                            player.hp = min(player.hp + heal, player.maxHp);
                            potionPicked = true;
                        }
                        first = false;
                    }
                    cout << "\n";
                    if (potionPicked) {
                        cout << "You feel refreshed. (HP=" << player.hp << ")\n";
                    }
                    items.clear();
                }
            } else {
                string item = command.substr(5);
                auto it = find(items.begin(), items.end(), item);
                if (it != items.end()) {
                    player.inventory.push_back(item);
                    items.erase(it);
                    cout << "Picked up " << item << endl;
                    if (item == "Potion") {
                        int heal = 10;
                        player.hp = min(player.hp + heal, player.maxHp);
                        cout << "You drink the potion and heal " << heal << ". (HP=" << player.hp << ")\n";
                    }
                } else cout << "Item not found!" << endl;
            }
        }
        else if (command == "undo") undoMove(player);
        else if (command == "inventory") {
            sortInventory(player);
            cout << "Inventory: ";
            for (auto &i : player.inventory) cout << i << " ";
            cout << endl;
        }
        else if (command == "skills") {
            printSkillTree(player);
        }
        else if (command == "sell" || (command.size() >= 5 && command.substr(0,5) == "sell ")) {
            if (command == "sell") {
                // sell all sellable items
                int earned = 0;
                for (auto it = player.inventory.begin(); it != player.inventory.end(); ) {
                    auto v = itemValues.find(*it);
                    if (v != itemValues.end()) {
                        earned += v->second;
                        it = player.inventory.erase(it);
                    } else {
                        ++it;
                    }
                }
                if (earned > 0) {
                    player.money += earned;
                    cout << "You sold your goods for $" << earned << ". (Money=$" << player.money << ")\n";
                } else {
                    cout << "You have nothing to sell." << "\n";
                }
            } else {
                string item = command.substr(5);
                auto it = find(player.inventory.begin(), player.inventory.end(), item);
                if (it != player.inventory.end()) {
                    auto v = itemValues.find(item);
                    if (v != itemValues.end()) {
                        player.money += v->second;
                        player.inventory.erase(it);
                        cout << "Sold " << item << " for $" << v->second << ". (Money=$" << player.money << ")\n";
                    } else {
                        cout << "You can't sell that." << "\n";
                    }
                } else {
                    cout << "Item not found in inventory!" << "\n";
                }
            }
        }

        // Auto-enter combat if a monster is here
        {
            bool found = false;
            int size = monsterQueue.size();
            Monster* target = nullptr;
            for (int i = 0; i < size; ++i) {
                Monster* m = monsterQueue.front(); monsterQueue.pop();
                if (!found && m->location == player.currentRoom) { target = m; found = true; }
                monsterQueue.push(m);
            }
            if (target) {
                cout << "(" << target->name << ") wants to fight!\n";
                bool victory = combat(player, target);
                if (victory) {
                    // Remove target from queue if defeated; if fled, leave as-is
                    int size2 = monsterQueue.size();
                    for (int i = 0; i < size2; ++i) {
                        Monster* m = monsterQueue.front(); monsterQueue.pop();
                        if (m == target) {
                            if (target->hp <= 0) {
                                player.currentRoom->items.push_back("Loot");
                                grantXPAndHandleLevel(player, 10);
                                delete m;
                                continue; // don't push back
                            }
                        }
                        monsterQueue.push(m);
                    }
                } else {
                    cout << "You have been defeated. Game Over.\n";
                    break;
                }
            }
        }
    }


    return 0;
}
