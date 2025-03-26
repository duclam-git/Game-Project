#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>
#include <algorithm>

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

struct Entity {
    SDL_Rect rect;
    int speed;
};

struct Enemy : public Entity {
    int health;
};

struct Bullet {
    SDL_Rect rect;
    float dx, dy;
    float speed;
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
    Game() : running(false), wave(1), playerSpeed(5), playerHealth(100), score(0) {
        player.rect = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 40, 40};
        player.speed = playerSpeed;
        srand(static_cast<unsigned int>(time(nullptr)));
    }

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
        if (TTF_Init() < 0) return false;
        window = SDL_CreateWindow("Wave Survival", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        font = TTF_OpenFont("arial.ttf", 24);
        Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096);
        hitSound = Mix_LoadWAV("hit.wav");
        pickupSound = Mix_LoadWAV("pickup.wav");
        return window && renderer && font && hitSound && pickupSound;
    }

    void run() {
        running = true;
        spawnWave();
        lastFireTime = SDL_GetTicks();
        while (running) {
            handleEvents();
            update();
            render();
            SDL_Delay(16);
        }
    }

    void cleanup() {
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
    Entity player;
    vector<Enemy> enemies;
    vector<PowerUp> powerUps;
    vector<Bullet> bullets;
    bool running;
    int wave;
    int playerSpeed;
    int playerHealth;
    int score;
    Uint32 lastFireTime;
    const Uint32 fireCooldown = 300;

    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }

        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        if (keystates[SDL_SCANCODE_W]) player.rect.y -= player.speed;
        if (keystates[SDL_SCANCODE_S]) player.rect.y += player.speed;
        if (keystates[SDL_SCANCODE_A]) player.rect.x -= player.speed;
        if (keystates[SDL_SCANCODE_D]) player.rect.x += player.speed;

        Wall::keepInside(player.rect);
    }

    void spawnWave() {
        enemies.clear();
        for (int i = 0; i < wave * 5; i++) {
            Enemy e;
            e.rect = {rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT, 30, 30};
            e.speed = 1 + wave / 3;
            e.health = 10 + wave * 2;
            enemies.push_back(e);
        }
        if (rand() % 3 == 0) {
            PowerUp p;
            p.rect = {rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT, 20, 20};
            p.type = (rand() % 2 == 0) ? PowerUp::HEALTH : PowerUp::SPEED;
            p.speed = 0;
            powerUps.push_back(p);
        }
    }

    void update() {
        Uint32 currentTime = SDL_GetTicks();

        for (auto& e : enemies) {
            int dx = player.rect.x - e.rect.x;
            int dy = player.rect.y - e.rect.y;
            double dist = sqrt((double)(dx * dx + dy * dy));
            e.rect.x += int(e.speed * dx / dist);
            e.rect.y += int(e.speed * dy / dist);

            if (SDL_HasIntersection(&player.rect, &e.rect)) {
                playerHealth -= 1;
                Mix_PlayChannel(-1, hitSound, 0);
                if (playerHealth <= 0) running = false;
            }
        }

        if (currentTime - lastFireTime > fireCooldown && !enemies.empty()) {
            Enemy* closestEnemy = nullptr;
            float minDist = 1e9;
            for (auto& en : enemies) {
                float dist = sqrt(pow(player.rect.x - en.rect.x, 2) + pow(player.rect.y - en.rect.y, 2));
                if (dist < minDist) {
                    minDist = dist;
                    closestEnemy = &en;
                }
            }

            if (closestEnemy) {
                Bullet bullet;
                bullet.rect = {player.rect.x + player.rect.w / 2 - 5, player.rect.y + player.rect.h / 2 - 5, 10, 10};
                float dx = closestEnemy->rect.x - bullet.rect.x;
                float dy = closestEnemy->rect.y - bullet.rect.y;
                float length = sqrt(dx * dx + dy * dy);
                bullet.dx = dx / length;
                bullet.dy = dy / length;
                bullet.speed = 8.0f;

                bullets.push_back(bullet);
                lastFireTime = currentTime;
            }
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
                    eIt->health -= 5;
                    if (eIt->health <= 0) eIt = enemies.erase(eIt);
                    else ++eIt;
                    hit = true;
                    break;
                } else ++eIt;
            }

            if (hit) bIt = bullets.erase(bIt);
            else ++bIt;
        }

        for (size_t i = 0; i < powerUps.size();) {
            if (SDL_HasIntersection(&player.rect, &powerUps[i].rect)) {
                Mix_PlayChannel(-1, pickupSound, 0);
                if (powerUps[i].type == PowerUp::HEALTH) playerHealth += 20;
                else if (powerUps[i].type == PowerUp::SPEED) player.speed += 2;
                powerUps.erase(powerUps.begin() + i);
            } else i++;
        }

        if (enemies.empty()) {
            wave++;
            player.speed = playerSpeed;
            score += 100 * wave;
            spawnWave();
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

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &player.rect);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (auto& e : enemies) SDL_RenderFillRect(renderer, &e.rect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        for (auto& b : bullets) SDL_RenderFillRect(renderer, &b.rect);

        for (auto& p : powerUps) {
            if (p.type == PowerUp::HEALTH) SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            else SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderFillRect(renderer, &p.rect);
        }

        renderText("Health: " + to_string(playerHealth), 10, 10);
        renderText("Wave: " + to_string(wave), 10, 40);
        renderText("Score: " + to_string(score), 10, 70);

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
