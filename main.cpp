#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int SPAWN_SAFE_RADIUS = 100;

struct Entity {
    SDL_Rect rect;
    int speed;
};

struct Coin {
    SDL_Rect rect;
};

struct Enemy : public Entity {
    int health;
    enum EnemyType { BASIC, FAST, TANK } type;
};

struct Bullet {
    SDL_Rect rect;
    double dx, dy;
    double speed;
    enum BulletType { PISTOL, SHOTGUN } type;
};

struct PowerUp : public Entity {
    enum Type {HEALTH, SPEED} type;
};

class Wall {
public:
    static void keepInside(SDL_Rect &rect) {
        if (rect.x < 0) rect.x = 0;
        if (rect.y < 0) rect.y = 0;
        if (rect.x + rect.w > SCREEN_WIDTH) rect.x = SCREEN_WIDTH - rect.w;
        if (rect.y + rect.h > SCREEN_HEIGHT) rect.y = SCREEN_HEIGHT - rect.h;
    }
};

class Game {
public:
    enum GameState { WEAPON_SELECTION, PLAYING, SHOP, UPGRADE_MENU };
    GameState gameState = WEAPON_SELECTION;
    enum WeaponType { PISTOL, SHOTGUN };
    WeaponType selectedWeapon = PISTOL;
    Game() : running(false), wave(1), playerSpeed(5), playerHealth(100), score(0), coins(0) {
        player.rect = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 40, 40};
        player.speed = playerSpeed;
        srand(static_cast<unsigned int>(time(nullptr)));
    }

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return false;
        if (TTF_Init() < 0) return false;
        window = SDL_CreateWindow("Wave Survival", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        font = TTF_OpenFont("arial.ttf", 24);
        Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096);
        hitSound = Mix_LoadWAV("hit.wav");
        pickupSound = Mix_LoadWAV("pickup.wav");
        playerTexture = loadTexture("player.png");
        enemyTexture = loadTexture("enemy.png");
        coinTexture = loadTexture("coin.png");
        powerUpTexture = loadTexture("powerup.png");
        pistolTexture = loadTexture("pistol.png");
        shotgunTexture = loadTexture("shotgun.png");
        shopHealthTexture = loadTexture("shophealth.jfif");
        shopDamageTexture = loadTexture("shopdamage.png");
        upgradeSpeedTexture = loadTexture("upgradespeed.png");
        upgradeDamageTexture = loadTexture("upgradedamage.png");
        upgradeHealthTexture = loadTexture("upgradehealth.png");


    if (!playerTexture || !enemyTexture || !coinTexture || !powerUpTexture) return false;
        return window && renderer && font && hitSound && pickupSound;
    }

    SDL_Texture* loadTexture(const std::string& path) {
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            cout << "Failed to load image: " << IMG_GetError() << endl;
            return nullptr;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    }

    void run() {
        running = true;
        spawnWave();
        lastFireTime = SDL_GetTicks();
        while (running) {
            update();
            handleEvents();
            render();
            SDL_Delay(16);
        }
    }

    void cleanup() {
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(enemyTexture);
        SDL_DestroyTexture(coinTexture);
        SDL_DestroyTexture(powerUpTexture);
        SDL_DestroyTexture(pistolTexture);
        SDL_DestroyTexture(shotgunTexture);
        SDL_DestroyTexture(shopHealthTexture);
        SDL_DestroyTexture(shopDamageTexture);
        SDL_DestroyTexture(upgradeSpeedTexture);
        SDL_DestroyTexture(upgradeDamageTexture);
        SDL_DestroyTexture(upgradeHealthTexture);
        IMG_Quit();
        Mix_FreeChunk(hitSound);
        Mix_FreeChunk(pickupSound);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
    }

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    Mix_Chunk* hitSound;
    Mix_Chunk* pickupSound;
    SDL_Texture* playerTexture;
    SDL_Texture* enemyTexture;
    SDL_Texture* coinTexture;
    SDL_Texture* powerUpTexture;
    SDL_Texture* pistolTexture;
    SDL_Texture* shotgunTexture;
    SDL_Texture* shopHealthTexture;
    SDL_Texture* shopDamageTexture;
    SDL_Texture* upgradeSpeedTexture;
    SDL_Texture* upgradeDamageTexture;
    SDL_Texture* upgradeHealthTexture;
    Entity player;
    vector<Enemy> enemies;
    vector<Coin> coinsOnGround;
    vector<PowerUp> powerUps;
    vector<Bullet> bullets;
    bool running;
    int wave;
    int playerSpeed;
    int playerHealth;
    int playerDamage;
    int score;
    int coins;
    Uint32 lastFireTime;
    const Uint32 fireCooldown = 300;

    SDL_Point randomSafeSpawn() {
        SDL_Point point;
        do {
            point.x = rand() % (SCREEN_WIDTH - 40);
            point.y = rand() % (SCREEN_HEIGHT - 40);
        } while (sqrt(pow(player.rect.x - point.x, 2) + pow(player.rect.y - point.y, 2)) < SPAWN_SAFE_RADIUS);
        return point;
    }
    
    void renderEntity(SDL_Texture* texture, SDL_Rect& rect) {
        SDL_RenderCopy(renderer, texture, NULL, &rect);
    }

    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            
            if (gameState == WEAPON_SELECTION && e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_1) {
                    selectedWeapon = PISTOL;
                    gameState = PLAYING;
                }
                else if (e.key.keysym.sym == SDLK_2) {
                    selectedWeapon = SHOTGUN;
                    gameState = PLAYING;
                }
            }

            if (gameState == SHOP || gameState == UPGRADE_MENU) {
                if (e.type == SDL_KEYDOWN) {
                    if (gameState == SHOP) {
                        if (e.key.keysym.sym == SDLK_1 && coins >= 20) {
                            coins-=20;
                            playerHealth+=20;
                        } else if(e.key.keysym.sym == SDLK_2 && coins >= 25){
                            coins -= 25;
                            playerDamage += 2;
                        } else if (e.key.keysym.sym == SDLK_RETURN) {
                            gameState = PLAYING;
                        }

                    } else if (gameState == UPGRADE_MENU) {
                        if (e.key.keysym.sym == SDLK_1 && coins >= 30) {
                            playerSpeed += 1;
                            coins -= 30;
                        } else if (e.key.keysym.sym == SDLK_2 && coins >= 40) {
                            playerDamage += 5;
                            coins -= 40;
                        } else if (e.key.keysym.sym == SDLK_3 && coins >= 50) {
                            playerHealth += 25;
                            coins -= 50;
                        } else if (e.key.keysym.sym == SDLK_RETURN) {
                            gameState = PLAYING;
                        }
                    }
                }
            }
        }
    }

    void shootBullet() {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        
        switch (selectedWeapon) {
            case PISTOL: {
                Bullet bullet;
                bullet.rect = {player.rect.x + player.rect.w / 2 - 5, player.rect.y + player.rect.h / 2 - 5, 10, 10};
                bullet.dx = (mouseX - bullet.rect.x) / sqrt(pow(mouseX - bullet.rect.x, 2) + pow(mouseY - bullet.rect.y, 2));
                bullet.dy = (mouseY - bullet.rect.y) / sqrt(pow(mouseX - bullet.rect.x, 2) + pow(mouseY - bullet.rect.y, 2));
                bullet.speed = 8.0f;
                bullets.push_back(bullet);
                break;
            }
            case SHOTGUN: {
                for (int i = -1; i <= 1; i++) {
                    Bullet bullet;
                    bullet.rect = {player.rect.x + player.rect.w / 2 - 5, player.rect.y + player.rect.h / 2 - 5, 10, 10};
                    double angleOffset = i * 0.1;
                    bullet.dx = (mouseX - bullet.rect.x) / sqrt(pow(mouseX - bullet.rect.x, 2) + pow(mouseY - bullet.rect.y, 2)) + angleOffset;
                    bullet.dy = (mouseY - bullet.rect.y) / sqrt(pow(mouseX - bullet.rect.x, 2) + pow(mouseY - bullet.rect.y, 2)) + angleOffset;
                    bullet.speed = 7.0f;
                    bullets.push_back(bullet);
                }
                break;
            }
        }
        lastFireTime = SDL_GetTicks();
    }

    void spawnWave() {
        enemies.clear();
        for (int i = 0; i < wave * 5; i++) {
            Enemy e;
            SDL_Point spawn = randomSafeSpawn();
            e.rect = {spawn.x, spawn.y, 30, 30};

            int enemyTypeRand = rand() % 3;
            e.type = static_cast<Enemy::EnemyType>(enemyTypeRand);
            if (e.type == Enemy::BASIC) {
                e.speed = 1 + wave / 3;
                e.health = 10 + wave * 2;
            } else if (e.type == Enemy::FAST) {
                e.speed = 2 + wave / 2;
                e.health = 5 + wave;
            } else if (e.type == Enemy::TANK) {
                e.speed = 1;
                e.health = 30 + wave * 5;
                e.rect.w = 40; e.rect.h = 40;
            }
            enemies.push_back(e);
        }
        if (rand() % 5 == 0) {
            PowerUp p;
            SDL_Point spawn = randomSafeSpawn();
            p.rect = {spawn.x, spawn.y, 20, 20};
            p.type = (rand() % 2 == 0) ? PowerUp::HEALTH : PowerUp::SPEED;
            p.speed = 0;
            powerUps.push_back(p);
        }
    }

    void renderImage(SDL_Texture* texture, int x, int y, int w, int h) {
        if (!texture) return; // Avoid crashing if texture failed to load
        SDL_Rect destRect = {x, y, w, h};
        SDL_RenderCopy(renderer, texture, nullptr, &destRect);
    }

    void renderWeaponSelection() {
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

        renderText("Select Your Weapon", SCREEN_WIDTH / 3 - 50, 100);
        renderText("1. Pistol", SCREEN_WIDTH / 3 + 100, 150 + 32);
        renderText("2. Shotgun", SCREEN_WIDTH / 3 + 100, 200 + 64);
        renderImage(pistolTexture, SCREEN_WIDTH / 3, 150, 64, 64);
        renderImage(shotgunTexture, SCREEN_WIDTH / 3, 250, 64, 64);

        SDL_RenderPresent(renderer);
    }

    void renderShop() {
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        
        renderText("1. Health +20 (Cost: 20)", SCREEN_WIDTH / 3 + 100, SCREEN_HEIGHT / 4 + 60 + 32);
        renderText("2. Bullet Damage +2 (Cost: 25)", SCREEN_WIDTH / 3 + 100, SCREEN_HEIGHT / 4 + 140 + 32);
        renderText("Press Enter to Continue", SCREEN_WIDTH / 3 + 100, SCREEN_HEIGHT / 4 + 220);
        renderText("SHOP - Buy Upgrades", SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 20);
        renderImage(shopHealthTexture, SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 60, 64, 64);
        renderImage(shopDamageTexture, SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 140, 64, 64);

    SDL_RenderPresent(renderer);
    }

    void renderUpgradeMenu() {
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

        renderText("UPGRADE MENU", SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 20);
        renderText("1. Increase Speed (Cost: 30)", SCREEN_WIDTH / 3 + 100, SCREEN_HEIGHT / 4 + 60 + 32);
        renderText("2. Increase Damage (Cost: 40)", SCREEN_WIDTH / 3 + 100, SCREEN_HEIGHT / 4 + 140 + 32);
        renderText("3. Increase Max Health (Cost: 50)", SCREEN_WIDTH / 3 + 100, SCREEN_HEIGHT / 4 + 220 + 32);
        renderText("Press Enter to Continue", SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 280);
        renderImage(upgradeSpeedTexture, SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 60, 64, 64);
        renderImage(upgradeDamageTexture, SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 140, 64, 64);
        renderImage(upgradeHealthTexture, SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 220, 64, 64);

    SDL_RenderPresent(renderer);
    }

    void update() {
        Uint32 currentTime = SDL_GetTicks();

        if (gameState == WEAPON_SELECTION) return;
        if (gameState == SHOP) return;
        if (gameState == UPGRADE_MENU) return;

        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        if (keystates[SDL_SCANCODE_W]) player.rect.y -= player.speed;
        if (keystates[SDL_SCANCODE_S]) player.rect.y += player.speed;
        if (keystates[SDL_SCANCODE_A]) player.rect.x -= player.speed;
        if (keystates[SDL_SCANCODE_D]) player.rect.x += player.speed;

        Wall::keepInside(player.rect);
        for (auto& e : enemies) {
            int dx = player.rect.x - e.rect.x;
            int dy = player.rect.y - e.rect.y;
            double dist = sqrt((double)(dx * dx + dy * dy));
            e.rect.x += int(e.speed * dx / dist);
            e.rect.y += int(e.speed * dy / dist);

            if (SDL_HasIntersection(&player.rect, &e.rect)) {
                playerHealth -= (e.type == Enemy::TANK) ? 3 : 1;
                Mix_PlayChannel(-1, hitSound, 0);
                if (playerHealth <= 0) running = false;
            }
        }

        if (currentTime - lastFireTime > fireCooldown && SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            shootBullet();
        }

        for (auto& b : bullets) {
            b.rect.x += int(b.dx * b.speed);
            b.rect.y += int(b.dy * b.speed);
        }

        bullets.erase(remove_if(bullets.begin(), bullets.end(), [](Bullet& b) {
            return b.rect.x < -10 || b.rect.x > SCREEN_WIDTH || b.rect.y < -10 || b.rect.y > SCREEN_HEIGHT;
        }), bullets.end());

        for (auto bIt = bullets.begin(); bIt != bullets.end();) {
            bool hit = false;
            for (auto eIt = enemies.begin(); eIt != enemies.end();) {
                if (SDL_HasIntersection(&bIt->rect, &eIt->rect)) {
                    eIt->health -= playerDamage;
                    if (eIt->health <= 0) {
                        Coin c;
                        c.rect = {eIt->rect.x + eIt->rect.w / 2, eIt->rect.y + eIt->rect.h / 2, 15, 15};
                        coinsOnGround.push_back(c);
        
                        eIt = enemies.erase(eIt);
                    } else {
                        ++eIt;
                    }
                    hit = true;
                    break;
                } else {
                    ++eIt;
                }
            }
        
            if (hit) {
                bIt = bullets.erase(bIt);
            } else {
                ++bIt;
            }
        }

        for (size_t i = 0; i < powerUps.size();) {
            if (SDL_HasIntersection(&player.rect, &powerUps[i].rect)) {
                Mix_PlayChannel(-1, pickupSound, 0);
                if (powerUps[i].type == PowerUp::HEALTH) playerHealth += 20;
                else if (powerUps[i].type == PowerUp::SPEED) player.speed += 2;
                powerUps.erase(powerUps.begin() + i);
            } else i++;
        }

        for (size_t i = 0; i < coinsOnGround.size();) {
            if (SDL_HasIntersection(&player.rect, &coinsOnGround[i].rect)) {
                coins += 10;
                coinsOnGround.erase(coinsOnGround.begin() + i);
            } else {
                i++;
            }
        }

        if (enemies.empty()) {
            if (wave % 3 == 0 && wave % 5 != 0) {
                gameState = UPGRADE_MENU;
            }
            else if (wave % 5 == 0) {
                gameState = SHOP;
            }
            spawnWave();
            wave++;
            player.speed = playerSpeed;
            score += 100 * wave;
            
            
        }

        // Debug output
        cout << "Health: " << playerHealth << ", Wave: " << wave << ", Score: " << score << endl;
    }

    void renderText(const string& message, int x, int y) {
        SDL_Color color = {255, 255, 255};
        SDL_Surface* surface = TTF_RenderText_Solid(font, message.c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect dst = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        
        renderText("Coins: " + to_string(coins), 10, 10);
        
        if (gameState == WEAPON_SELECTION) {
            renderWeaponSelection();
            return;
        }
        if (gameState == SHOP) {
            renderShop();
            return;
        }
        if (gameState == UPGRADE_MENU) {
            renderUpgradeMenu();
            return;
        }
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        renderEntity(playerTexture, player.rect);
        
        for (auto& c : coinsOnGround) {
            renderEntity(coinTexture, c.rect);
        }

        for (auto& e : enemies) {
            renderEntity(enemyTexture, e.rect);
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (auto& b : bullets) SDL_RenderFillRect(renderer, &b.rect);

        for (auto& p : powerUps) {
            renderEntity(powerUpTexture, p.rect);
        }

        renderText("Health: " + to_string(playerHealth), 10, 10);
        renderText("Wave: " + to_string(wave), 10, 40);
        renderText("Score: " + to_string(score), 10, 70);
        renderText("Coins: " + to_string(coins), 10, 100);

        SDL_RenderPresent(renderer);
    }

};

int main(int argc, char* argv[]) {
    Game game;
    if (game.init()) {
        game.run();
    }
    game.cleanup();
    return 0;
}
