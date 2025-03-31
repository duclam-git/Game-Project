#ifndef GAME_H
#define GAME_H

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
#include <fstream>
#include "constant.h"

using namespace std;

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

struct EnemyStats {
    int health;
    int speed;
    int size;
};

const EnemyStats ENEMY_BASIC  = { 10,  2, 30 };
const EnemyStats ENEMY_FAST   = {  5,  4, 30 };
const EnemyStats ENEMY_TANK   = { 30,  2, 40 };

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
enum GameState { TITLE_SCREEN, WEAPON_SELECTION, PLAYING, SHOP, UPGRADE_MENU, GAME_OVER };
    GameState gameState = TITLE_SCREEN;
    enum WeaponType { PISTOL, SHOTGUN };
    WeaponType selectedWeapon = PISTOL;
    Game() : running(false), wave(1), playerSpeed(PLAYER_START_SPEED), playerHealth(PLAYER_START_HEALTH), playerDamage(PLAYER_START_DAMAGE), score(-200), coins(0) {
        player.rect = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 40, 40};
        player.speed = playerSpeed;
        srand(static_cast<unsigned int>(time(nullptr)));
    }

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return false;
        if (TTF_Init() < 0) return false;
        window = SDL_CreateWindow("Dungeon Survival", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        font = TTF_OpenFont("assets/fonts/arial.ttf", 24);
        Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096);
        backgroundMusic = Mix_LoadMUS("assets/sounds/background.mp3");
        if (!backgroundMusic) {
        cout << "Failed to load background music: " << Mix_GetError() << endl;
        return false;
        }
        hitSound = Mix_LoadWAV("assets/sounds/hit.wav");
        pickupSound = Mix_LoadWAV("assets/sounds/pickup.wav");
        titlebgTexture = loadTexture("assets/images/titlebackground.png");
        startButtonTexture = loadTexture("assets/images/startbutton.png");
        quitButtonTexture = loadTexture("assets/images/quit.png");
        playerTexture = loadTexture("assets/images/player.png");
        enemyTexture = loadTexture("assets/images/enemy.png");
        coinTexture = loadTexture("assets/images/coin.png");
        powerUpTexture = loadTexture("assets/images/powerup.png");
        pistolTexture = loadTexture("assets/images/pistol.png");
        shotgunTexture = loadTexture("assets/images/shotgun.png");
        shopHealthTexture = loadTexture("assets/images/shophealth.jfif");
        shopDamageTexture = loadTexture("assets/images/shopdamage.png");
        upgradeSpeedTexture = loadTexture("assets/images/upgradespeed.png");
        upgradeDamageTexture = loadTexture("assets/images/upgradedamage.png");
        upgradeHealthTexture = loadTexture("assets/images/upgradehealth.png");
        backgroundTexture = loadTexture("assets/images/background.png");
        gameoverTexture = loadTexture("assets/images/gameover.png");

    if (!playerTexture || !enemyTexture || !coinTexture || !powerUpTexture || !pistolTexture || !shotgunTexture || !shopHealthTexture || !shopDamageTexture || !upgradeSpeedTexture || !upgradeDamageTexture || !upgradeHealthTexture || !backgroundTexture) return false;
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
        Mix_PlayMusic(backgroundMusic, -1);
        Mix_VolumeMusic(32);
        while (running) {
            handleEvents();
            
            if (gameState == TITLE_SCREEN) {
                renderTitleScreen();
            } else if (gameState == GAME_OVER) {
                renderGameOver();
            } else {
                update();
                render();
            }
    
            SDL_Delay(16);
        }
    }

    void cleanup() {
        Mix_FreeMusic(backgroundMusic);
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
    Mix_Music* backgroundMusic;
    Mix_Chunk* hitSound;
    Mix_Chunk* pickupSound;
    SDL_Texture* titlebgTexture;
    SDL_Texture* startButtonTexture;
    SDL_Texture* quitButtonTexture;
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
    SDL_Texture* backgroundTexture;
    SDL_Texture* gameoverTexture;
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

            if (gameState == TITLE_SCREEN && e.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;
    
                SDL_Rect playButton = {SCREEN_WIDTH / 3 + 50, 400, 200, 100};
                SDL_Rect quitButton = {SCREEN_WIDTH / 3 + 50, 500, 200, 100};
    
                if (mouseX >= playButton.x && mouseX <= playButton.x + playButton.w &&
                    mouseY >= playButton.y && mouseY <= playButton.y + playButton.h) {
                    gameState = WEAPON_SELECTION;
                }
    
                if (mouseX >= quitButton.x && mouseX <= quitButton.x + quitButton.w &&
                    mouseY >= quitButton.y && mouseY <= quitButton.y + quitButton.h) {
                    running = false;
                }
            }

            if (gameState == GAME_OVER && e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN) {
                    resetGame();
                    gameState = TITLE_SCREEN;
                }
            }

            
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
                            coins -= 20;
                            playerHealth += HEALTH_PACK_AMOUNT;
                        } else if(e.key.keysym.sym == SDLK_2 && coins >= 25){
                            coins -= 25;
                            playerDamage += DAMAGE_UPGRADE_AMOUNT;
                        } else if (e.key.keysym.sym == SDLK_RETURN) {
                            gameState = PLAYING;
                        }

                    } else if (gameState == UPGRADE_MENU) {
                        if (e.key.keysym.sym == SDLK_1) {
                            playerSpeed += SPEED_UPGRADE_AMOUNT;
                            gameState = PLAYING;
                        } else if (e.key.keysym.sym == SDLK_2) {
                            playerDamage += DAMAGE_UPGRADE_AMOUNT;
                            gameState = PLAYING;
                        } else if (e.key.keysym.sym == SDLK_3) {
                            playerHealth += HEALTH_PACK_AMOUNT;
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
                for (int i = - SHOTGUN_BULLET_COUNT / 2; i <= SHOTGUN_BULLET_COUNT / 2; i++) {
                    Bullet bullet;
                    bullet.rect = {player.rect.x + player.rect.w / 2 - 5, player.rect.y + player.rect.h / 2 - 5, 10, 10};
                    double angle = atan2(mouseY - bullet.rect.y, mouseX - bullet.rect.x);
                    angle += i * SHOTGUN_SPREAD_ANGLE / 100.0;
                    bullet.dx = cos(angle);
                    bullet.dy = sin(angle);
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
                e.health = ENEMY_BASIC.health + wave * 2;
                e.speed = ENEMY_BASIC.speed + wave / 5;
                e.rect.w = ENEMY_BASIC.size; e.rect.h = ENEMY_BASIC.size;
            } else if (e.type == Enemy::FAST) {
                e.speed = ENEMY_FAST.speed + wave / 3;
                e.health = ENEMY_FAST.health + wave;
                e.rect.w = ENEMY_FAST.size; e.rect.h = ENEMY_FAST.size;
            } else if (e.type == Enemy::TANK) {
                e.speed = ENEMY_TANK.speed + wave / 10;
                e.health = ENEMY_TANK.health + wave * 5;
                e.rect.w = ENEMY_TANK.size; e.rect.h = ENEMY_TANK.size;
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

    void resetGame() {
        playerHealth = 100;
        score = -200;
        wave = 1;
        enemies.clear();
        bullets.clear();
        coinsOnGround.clear();
        powerUps.clear();
    }

    void renderTitleScreen() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        renderImage(titlebgTexture, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        
        SDL_Rect playButton = {SCREEN_WIDTH / 3 + 50, 400, 200, 100};
        SDL_Rect quitButton = {SCREEN_WIDTH / 3 + 50, 500, 200, 100};
        
        renderImage(startButtonTexture, SCREEN_WIDTH / 3 + 50, 400, 200, 100);
        renderImage(quitButtonTexture, SCREEN_WIDTH / 3 + 50, 500, 200, 100);
    
        SDL_RenderPresent(renderer);
    }

    void renderGameOver() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    
        int highScore = loadHighScore();
        renderImage(gameoverTexture, SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        renderText("Your Score: " + to_string(score), SCREEN_WIDTH / 3, 150);
        renderText("High Score: " + to_string(highScore), SCREEN_WIDTH / 3, 200);
        renderText("Press Enter to return to title", SCREEN_WIDTH / 3, 250);
    
        SDL_RenderPresent(renderer);
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
        renderText("1.Increase Speed", SCREEN_WIDTH / 3 + 100, SCREEN_HEIGHT / 4 + 60 + 32);
        renderText("2.Increase Damage", SCREEN_WIDTH / 3 + 100, SCREEN_HEIGHT / 4 + 140 + 32);
        renderText("3.Increase Max Health", SCREEN_WIDTH / 3 + 100, SCREEN_HEIGHT / 4 + 220 + 32);
        renderImage(upgradeSpeedTexture, SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 60, 64, 64);
        renderImage(upgradeDamageTexture, SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 140, 64, 64);
        renderImage(upgradeHealthTexture, SCREEN_WIDTH / 3, SCREEN_HEIGHT / 4 + 220, 64, 64);

    SDL_RenderPresent(renderer);
    }

    int loadHighScore() {
        ifstream file("highscore.txt");
        int highScore = 0;
        if (file.is_open()) {
            file >> highScore;
            file.close();
        }
        return highScore;
    }
    
    void saveHighScore(int score) {
        int currentHighScore = loadHighScore();
        if (score > currentHighScore) {
            ofstream file("highscore.txt");
            file << score;
            file.close();
        }
    }

    void update() {
        Uint32 currentTime = SDL_GetTicks();

        if (playerHealth <= 0) {
            saveHighScore(score);
            gameState = GAME_OVER;
        }
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
                        score += 10;
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
                coins += COIN_VALUE;
                coinsOnGround.erase(coinsOnGround.begin() + i);
            } else {
                i++;
            }
        }

        if (enemies.empty()) {
            if (wave % 3 == 0) gameState = UPGRADE_MENU;
            if (wave % 5 == 0) {
                gameState = SHOP;
            }
            spawnWave();
            wave++;
            player.speed = playerSpeed;
            score += 100 * wave;
            
        }
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

        SDL_Rect bgRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, backgroundTexture, NULL, &bgRect);
        
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
        renderText("Wave: " + to_string(wave - 1), 10, 40);
        renderText("Score: " + to_string(score), 10, 70);
        renderText("Coins: " + to_string(coins), 10, 100);

        SDL_RenderPresent(renderer);
    }

};

#endif