#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <math.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

#define PIN_OK     1
#define PIN_UP     2
#define PIN_LEFT   3
#define PIN_RIGHT  5
#define PIN_SDA    6
#define PIN_SCL    7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Preferences prefs;

enum AppState { STATE_MENU, STATE_DINO, STATE_DOOM, STATE_PONG, STATE_TETRIS };
AppState appState = STATE_MENU;

enum MenuItem { MENU_DINO = 0, MENU_DOOM = 1, MENU_PONG = 2, MENU_TETRIS = 3 };
const int MENU_COUNT = 4;
int menuSelection = MENU_DINO;

const unsigned long DEBOUNCE_MS   = 40;
const unsigned long LONG_PRESS_MS = 1500;

struct Btn {
  uint8_t pin;
  bool lastState;
  unsigned long lastDebounce;
  unsigned long pressStart;
};

Btn btnOk    = {PIN_OK,    LOW, 0, 0};
Btn btnUp    = {PIN_UP,    LOW, 0, 0};
Btn btnLeft  = {PIN_LEFT,  LOW, 0, 0};
Btn btnRight = {PIN_RIGHT, LOW, 0, 0};

bool readClick(Btn &b) {
  bool clicked = false;
  bool reading = digitalRead(b.pin);
  if (reading == HIGH && b.lastState == LOW) {
    if (millis() - b.lastDebounce > DEBOUNCE_MS) {
      clicked = true;
      b.lastDebounce = millis();
      b.pressStart = millis();
    }
  }
  if (reading == LOW) b.pressStart = 0;
  b.lastState = reading;
  return clicked;
}

bool isHeld(Btn &b) {
  return digitalRead(b.pin) == HIGH;
}

bool isLongPress(Btn &b) {
  if (isHeld(b) && b.pressStart > 0) {
    return (millis() - b.pressStart) >= LONG_PRESS_MS;
  }
  return false;
}

// ===================== DINO =====================
const int GROUND_Y      = 52;
const int DINO_X        = 18;
const int DINO_W        = 10;
const int DINO_H        = 14;
const int DINO_GROUND_Y = GROUND_Y - DINO_H;
const int JUMP_VELOCITY = -8;
const int GRAVITY       = 1;

const int OBS_W_MIN = 6,  OBS_W_MAX = 10;
const int OBS_H_MIN = 10, OBS_H_MAX = 20;
const int OBS_GAP_MIN = 20, OBS_GAP_MAX = 65;

const float SPEED_START = 3.2f;
const float SPEED_MAX   = 9.5f;
const float SPEED_STEP  = 0.05f;

const unsigned long SCORE_INTERVAL_MS = 100;
const long SCORE_PER_TICK = 5;
const unsigned long FRAME_DELAY_MS = 15;

int dinoY = DINO_GROUND_Y;
int dinoVy = 0;
bool isJumping = false;
int obstacleX = SCREEN_WIDTH;
int obstacleW = OBS_W_MIN;
int obstacleH = OBS_H_MIN;
long score = 0;
long highScore = 0;
float gameSpeed = SPEED_START;
bool dinoStarted = false;
bool dinoGameOver = false;
unsigned long lastScoreTime = 0;

void spawnObstacle() {
  obstacleX = SCREEN_WIDTH + random(OBS_GAP_MIN, OBS_GAP_MAX);
  obstacleW = random(OBS_W_MIN, OBS_W_MAX + 1);
  obstacleH = random(OBS_H_MIN, OBS_H_MAX + 1);
}

void resetDino() {
  score = 0;
  dinoY = DINO_GROUND_Y;
  dinoVy = 0;
  isJumping = false;
  gameSpeed = SPEED_START;
  obstacleX = SCREEN_WIDTH;
  spawnObstacle();
  dinoGameOver = false;
  lastScoreTime = millis();
}

void updateDino(bool jumpPressed) {
  if (jumpPressed && !isJumping) {
    dinoVy = JUMP_VELOCITY;
    isJumping = true;
  }
  dinoY += dinoVy;
  dinoVy += GRAVITY;
  if (dinoY > DINO_GROUND_Y) {
    dinoY = DINO_GROUND_Y;
    dinoVy = 0;
    isJumping = false;
  }
}

void updateObstacle() {
  obstacleX -= (int)gameSpeed;
  if (obstacleX < -obstacleW) {
    spawnObstacle();
    if (gameSpeed < SPEED_MAX) gameSpeed += SPEED_STEP;
  }
}

void updateScore() {
  if (millis() - lastScoreTime > SCORE_INTERVAL_MS) {
    score += SCORE_PER_TICK;
    lastScoreTime = millis();
  }
}

bool checkCollision() {
  bool xOverlap = (obstacleX < DINO_X + DINO_W) && (obstacleX + obstacleW > DINO_X);
  int obstacleTopY = GROUND_Y - obstacleH;
  bool yOverlap = (dinoY + DINO_H) > obstacleTopY;
  return xOverlap && yOverlap;
}

void endDino() {
  dinoGameOver = true;
  if (score > highScore) {
    highScore = score;
    prefs.putLong("highscore", highScore);
  }
}

void drawDinoStart() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 18);
  display.print(F("DINO GAME"));
  display.setCursor(10, 34);
  display.print(F("Hi-Score: "));
  display.print(highScore);
  display.setCursor(8, 50);
  display.print(F("> Press OK to Play <"));
}

void drawDinoHud() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(55, 2);
  display.print(F("SCORE:"));
  display.print(score);
  display.setCursor(55, 12);
  display.print(F("HI:"));
  display.print(highScore);
}

void drawGround() {
  display.drawLine(0, GROUND_Y, SCREEN_WIDTH - 1, GROUND_Y, SSD1306_WHITE);
}

void drawDinoSprite() {
  display.fillRect(DINO_X, dinoY, DINO_W, DINO_H, SSD1306_WHITE);
}

void drawObstacle() {
  display.fillRect(obstacleX, GROUND_Y - obstacleH, obstacleW, obstacleH, SSD1306_WHITE);
}

void drawDinoGameOver() {
  display.drawRect(5, 5, 118, 54, SSD1306_WHITE);
  display.drawRect(7, 7, 114, 50, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(36, 15);
  display.print(F("GAME OVER"));
  display.setCursor(20, 30);
  display.print(F("Score: "));
  display.print(score);
  display.setCursor(16, 45);
  display.print(F("Press OK to retry"));
}

// ===================== DOOM =====================
const int MAP_W = 16;
const int MAP_H = 16;

const char mapData[MAP_H][MAP_W + 1] PROGMEM = {
  "################",
  "#..............#",
  "#...####.......#",
  "#.......#......#",
  "#.......#..##..#",
  "#.......#......#",
  "#..##..........#",
  "#..##....####..#",
  "#........#.....#",
  "#..######.#.####",
  "#........#.....#",
  "#..######.###..#",
  "#..............#",
  "#..#########...#",
  "#..............#",
  "################"
};

float playerX = 3.5f;
float playerY = 3.5f;
float playerA = 0.0f;

const float FOV = 60.0f * (M_PI / 180.0f);
const float MOVE_SPEED = 0.08f;
const float ROT_SPEED  = 0.10f;

#define MAX_ENEMIES 4

struct Enemy {
  float x, y;
  bool alive;
  unsigned long respawnAt;
};

Enemy enemies[MAX_ENEMIES];
int kills = 0;
int deaths = 0;
int mostKilled = 0;
long totalKills = 0;
long totalDeaths = 0;
bool doomStarted = false;
bool doomGameOver = false;

const uint8_t enemySprite[] PROGMEM = {
  0b00111100,
  0b01111110,
  0b11100111,
  0b11111111,
  0b11011011,
  0b01111110,
  0b00111100,
  0b01100110,
  0b11000011,
  0b11000011,
  0b01100110,
  0b00111100
};
const int ENEMY_SPRITE_W = 8;
const int ENEMY_SPRITE_H = 12;

void drawEnemySprite(int screenX, int y0, int size) {
  float scaleY = (float)size / ENEMY_SPRITE_H;
  float scaleX = scaleY * 0.75f;

  int drawW = max(5, (int)(ENEMY_SPRITE_W * scaleX));
  int startX = screenX - drawW / 2;

  for (int sy = 0; sy < ENEMY_SPRITE_H; sy++) {
    uint8_t row = pgm_read_byte(&enemySprite[sy]);
    int py = y0 + (int)(sy * scaleY);

    for (int sx = 0; sx < ENEMY_SPRITE_W; sx++) {
      if (row & (0x80 >> sx)) {
        int px = startX + (int)(sx * scaleX);
        display.drawPixel(px,     py, SSD1306_BLACK);
        display.drawPixel(px + 1, py, SSD1306_BLACK);
        if (scaleY > 1.2f) {
          display.drawPixel(px,     py + 1, SSD1306_BLACK);
          display.drawPixel(px + 1, py + 1, SSD1306_BLACK);
        }
      }
    }
  }
}

bool isWall(int mx, int my) {
  if (mx < 0 || my < 0 || mx >= MAP_W || my >= MAP_H) return true;
  char c = pgm_read_byte(&(mapData[my][mx]));
  return c == '#';
}

float castRay(float angle) {
  float sinA = sinf(angle);
  float cosA = cosf(angle);
  float dist = 0.0f;
  const float step = 0.04f;
  const float maxDist = 16.0f;
  while (dist < maxDist) {
    float rx = playerX + cosA * dist;
    float ry = playerY + sinA * dist;
    if (isWall((int)rx, (int)ry)) break;
    dist += step;
  }
  return (dist >= maxDist) ? maxDist : dist;
}

bool hasLineOfSight(float targetX, float targetY) {
  float dx = targetX - playerX;
  float dy = targetY - playerY;
  float dist = sqrtf(dx * dx + dy * dy);
  if (dist < 0.1f) return true;

  float ang = atan2f(dy, dx);
  float wallDist = castRay(ang);
  return wallDist > dist - 0.15f;
}

void spawnEnemy(int i) {
  float ex, ey;
  int tries = 0;
  do {
    ex = 2.0f + random(0, 12);
    ey = 2.0f + random(0, 12);
    tries++;
  } while (
    (isWall((int)ex, (int)ey) ||
     (fabs(ex - playerX) < 4.0f && fabs(ey - playerY) < 4.0f))
    && tries < 50
  );

  if (tries >= 50) {
    ex = 12.0f;
    ey = 12.0f;
  }

  enemies[i].x = ex;
  enemies[i].y = ey;
  enemies[i].alive = true;
  enemies[i].respawnAt = 0;
}

void resetDoom() {
  playerX = 3.5f;
  playerY = 3.5f;
  playerA = 0.0f;
  kills = 0;
  deaths = 0;
  doomGameOver = false;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    spawnEnemy(i);
  }
}

void updateEnemies() {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].alive) {
      if (millis() > enemies[i].respawnAt) {
        spawnEnemy(i);
      }
      continue;
    }

    float dx = playerX - enemies[i].x;
    float dy = playerY - enemies[i].y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < 0.55f) {
      deaths++;
      totalDeaths++;
      prefs.putLong("totaldeaths", totalDeaths);
      doomGameOver = true;
      if (kills > mostKilled) {
        mostKilled = kills;
        prefs.putInt("mostkilled", mostKilled);
      }
      return;
    }

    if (dist > 0.5f) {
      float speed = 0.032f;
      float nx = enemies[i].x + (dx / dist) * speed;
      float ny = enemies[i].y + (dy / dist) * speed;

      if (!isWall((int)nx, (int)enemies[i].y)) enemies[i].x = nx;
      if (!isWall((int)enemies[i].x, (int)ny)) enemies[i].y = ny;
    }
  }
}

void tryShoot() {
  int bestIdx = -1;
  float bestDist = 999.0f;

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].alive) continue;

    float dx = enemies[i].x - playerX;
    float dy = enemies[i].y - playerY;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > 9.0f) continue;

    if (!hasLineOfSight(enemies[i].x, enemies[i].y)) continue;

    float ang = atan2f(dy, dx);
    float diff = ang - playerA;
    while (diff >  M_PI) diff -= 2 * M_PI;
    while (diff < -M_PI) diff += 2 * M_PI;

    if (fabs(diff) < 0.18f) {
      if (dist < bestDist) {
        bestDist = dist;
        bestIdx = i;
      }
    }
  }

  if (bestIdx >= 0) {
    enemies[bestIdx].alive = false;
    enemies[bestIdx].respawnAt = millis() + 2500 + random(0, 1500);
    kills++;
    totalKills++;
    prefs.putLong("totalkills", totalKills);
  }
}

void updateDoom() {
  if (isHeld(btnLeft))  playerA -= ROT_SPEED;
  if (isHeld(btnRight)) playerA += ROT_SPEED;

  float nx = playerX, ny = playerY;
  if (isHeld(btnUp)) {
    nx = playerX + cosf(playerA) * MOVE_SPEED;
    ny = playerY + sinf(playerA) * MOVE_SPEED;
  }
  if (!isWall((int)nx, (int)playerY)) playerX = nx;
  if (!isWall((int)playerX, (int)ny)) playerY = ny;

  updateEnemies();

  static bool wasShooting = false;
  if (isHeld(btnOk)) {
    if (!wasShooting) {
      tryShoot();
      wasShooting = true;
    }
  } else {
    wasShooting = false;
  }
}

void renderDoom() {
  display.clearDisplay();

  const int COL_STEP = 2;
  for (int x = 0; x < SCREEN_WIDTH; x += COL_STEP) {
    float rayAng = (playerA - FOV / 2.0f) + (x / (float)SCREEN_WIDTH) * FOV;
    float dist = castRay(rayAng);
    float corr = cosf(rayAng - playerA);
    dist *= corr;
    int lineH = (int)(SCREEN_HEIGHT / (dist + 0.001f));
    if (lineH > SCREEN_HEIGHT) lineH = SCREEN_HEIGHT;
    int y0 = (SCREEN_HEIGHT - lineH) / 2;
    display.drawFastVLine(x, y0, lineH, SSD1306_WHITE);
    if (COL_STEP > 1) display.drawFastVLine(x + 1, y0, lineH, SSD1306_WHITE);
  }

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].alive) continue;

    float dx = enemies[i].x - playerX;
    float dy = enemies[i].y - playerY;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < 0.3f || dist > 14.0f) continue;
    if (!hasLineOfSight(enemies[i].x, enemies[i].y)) continue;

    float ang = atan2f(dy, dx);
    float diff = ang - playerA;
    while (diff >  M_PI) diff -= 2 * M_PI;
    while (diff < -M_PI) diff += 2 * M_PI;

    if (fabs(diff) > FOV / 1.5f) continue;

    int screenX = (int)((0.5f + diff / FOV) * SCREEN_WIDTH);

    int size = (int)(34.0f / dist);
    if (size < 7)  size = 7;
    if (size > 42) size = 42;

    int y0 = (SCREEN_HEIGHT - size) / 2;
    drawEnemySprite(screenX, y0, size);
  }

  int cx = SCREEN_WIDTH / 2;
  int cy = SCREEN_HEIGHT / 2;
  display.drawFastHLine(cx - 4, cy, 9, SSD1306_WHITE);
  display.drawFastVLine(cx, cy - 4, 9, SSD1306_WHITE);

  static uint32_t flashUntil = 0;
  if (isHeld(btnOk)) flashUntil = millis() + 50;
  if ((int32_t)(flashUntil - millis()) > 0) {
    display.fillRect(SCREEN_WIDTH - 18, 0, 18, 9, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("DOOM "));
  display.print(kills);
}

void drawDoomStart() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(48, 6);
  display.print(F("DOOM"));

  display.setCursor(14, 22);
  display.print(F("Total K: "));
  display.print(totalKills);

  display.setCursor(14, 34);
  display.print(F("Total D: "));
  display.print(totalDeaths);

  display.setCursor(10, 46);
  display.print(F("BEST: "));
  display.print(mostKilled);

  display.setCursor(18, 56);
  display.print(F("> PRESS OK PLAY <"));
}

void drawDoomGameOver() {
  display.clearDisplay();
  display.drawRect(5, 5, 118, 54, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(36, 10);
  display.print(F("YOU DIED"));

  display.setCursor(14, 22);
  display.print(F("Run Kills: "));
  display.print(kills);

  display.setCursor(14, 32);
  display.print(F("Total K/D: "));
  display.print(totalKills);
  display.print(F("/"));
  display.print(totalDeaths);

  display.setCursor(10, 48);
  display.print(F("BEST: "));
  display.print(mostKilled);
}

// ===================== PING PONG =====================
const int PONG_PADDLE_W = 3;
const int PONG_PADDLE_H = 14;
const int PONG_PLAYER_X = 4;
const int PONG_AI_X     = SCREEN_WIDTH - 4 - PONG_PADDLE_W;
const int PONG_WIN_SCORE = 7;
const float PONG_PLAYER_SPEED = 3.0f;
const float PONG_AI_SPEED     = 1.8f;
const float PONG_BALL_MAX_VX  = 4.5f;

float pongPlayerY = (SCREEN_HEIGHT - PONG_PADDLE_H) / 2.0f;
float pongAiY     = (SCREEN_HEIGHT - PONG_PADDLE_H) / 2.0f;
float pongBallX = SCREEN_WIDTH / 2.0f;
float pongBallY = SCREEN_HEIGHT / 2.0f;
float pongBallVx = 2.0f;
float pongBallVy = 1.0f;
int pongPlayerScore = 0;
int pongAiScore = 0;
long pongWins = 0;
bool pongStarted = false;
bool pongGameOver = false;
bool pongPlayerWon = false;

void pongServe(int dir) {
  pongBallX = SCREEN_WIDTH / 2.0f;
  pongBallY = SCREEN_HEIGHT / 2.0f;
  pongBallVx = 2.0f * dir;
  pongBallVy = random(-15, 16) / 10.0f;
  if (fabsf(pongBallVy) < 0.5f) pongBallVy = 0.8f;
}

void resetPong() {
  pongPlayerY = (SCREEN_HEIGHT - PONG_PADDLE_H) / 2.0f;
  pongAiY     = (SCREEN_HEIGHT - PONG_PADDLE_H) / 2.0f;
  pongPlayerScore = 0;
  pongAiScore = 0;
  pongGameOver = false;
  pongPlayerWon = false;
  pongServe(random(0, 2) ? 1 : -1);
}

void endPong() {
  pongGameOver = true;
  pongPlayerWon = (pongPlayerScore > pongAiScore);
  if (pongPlayerWon) {
    pongWins++;
    prefs.putLong("pongwins", pongWins);
  }
}

void updatePong() {
  if (isHeld(btnLeft))  pongPlayerY -= PONG_PLAYER_SPEED;
  if (isHeld(btnRight)) pongPlayerY += PONG_PLAYER_SPEED;
  if (pongPlayerY < 0) pongPlayerY = 0;
  if (pongPlayerY > SCREEN_HEIGHT - PONG_PADDLE_H) pongPlayerY = SCREEN_HEIGHT - PONG_PADDLE_H;

  float aiCenter = pongAiY + PONG_PADDLE_H / 2.0f;
  if (pongBallVx > 0) {
    if (pongBallY > aiCenter + 2) pongAiY += PONG_AI_SPEED;
    else if (pongBallY < aiCenter - 2) pongAiY -= PONG_AI_SPEED;
  } else {
    float mid = (SCREEN_HEIGHT - PONG_PADDLE_H) / 2.0f;
    if (pongAiY < mid - 1) pongAiY += 0.5f;
    else if (pongAiY > mid + 1) pongAiY -= 0.5f;
  }
  if (pongAiY < 0) pongAiY = 0;
  if (pongAiY > SCREEN_HEIGHT - PONG_PADDLE_H) pongAiY = SCREEN_HEIGHT - PONG_PADDLE_H;

  pongBallX += pongBallVx;
  pongBallY += pongBallVy;

  if (pongBallY < 0) {
    pongBallY = 0;
    pongBallVy = -pongBallVy;
  }
  if (pongBallY > SCREEN_HEIGHT - 2) {
    pongBallY = SCREEN_HEIGHT - 2;
    pongBallVy = -pongBallVy;
  }

  if (pongBallVx < 0 &&
      pongBallX <= PONG_PLAYER_X + PONG_PADDLE_W &&
      pongBallX >= PONG_PLAYER_X - 2 &&
      pongBallY + 2 >= pongPlayerY &&
      pongBallY <= pongPlayerY + PONG_PADDLE_H) {
    pongBallVx = -pongBallVx * 1.06f;
    if (pongBallVx > PONG_BALL_MAX_VX) pongBallVx = PONG_BALL_MAX_VX;
    float offset = (pongBallY + 1 - (pongPlayerY + PONG_PADDLE_H / 2.0f)) / (PONG_PADDLE_H / 2.0f);
    pongBallVy = offset * 2.5f;
    pongBallX = PONG_PLAYER_X + PONG_PADDLE_W + 1;
  }

  if (pongBallVx > 0 &&
      pongBallX + 2 >= PONG_AI_X &&
      pongBallX + 2 <= PONG_AI_X + PONG_PADDLE_W + 2 &&
      pongBallY + 2 >= pongAiY &&
      pongBallY <= pongAiY + PONG_PADDLE_H) {
    pongBallVx = -pongBallVx * 1.06f;
    if (pongBallVx < -PONG_BALL_MAX_VX) pongBallVx = -PONG_BALL_MAX_VX;
    float offset = (pongBallY + 1 - (pongAiY + PONG_PADDLE_H / 2.0f)) / (PONG_PADDLE_H / 2.0f);
    pongBallVy = offset * 2.5f;
    pongBallX = PONG_AI_X - 3;
  }

  if (pongBallX < -2) {
    pongAiScore++;
    if (pongAiScore >= PONG_WIN_SCORE) {
      endPong();
    } else {
      pongServe(-1);
    }
  } else if (pongBallX > SCREEN_WIDTH) {
    pongPlayerScore++;
    if (pongPlayerScore >= PONG_WIN_SCORE) {
      endPong();
    } else {
      pongServe(1);
    }
  }
}

void renderPong() {
  display.clearDisplay();

  for (int y = 0; y < SCREEN_HEIGHT; y += 6) {
    display.drawFastVLine(SCREEN_WIDTH / 2, y, 3, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(SCREEN_WIDTH / 2 - 20, 2);
  display.print(pongPlayerScore);
  display.setCursor(SCREEN_WIDTH / 2 + 15, 2);
  display.print(pongAiScore);

  display.fillRect(PONG_PLAYER_X, (int)pongPlayerY, PONG_PADDLE_W, PONG_PADDLE_H, SSD1306_WHITE);
  display.fillRect(PONG_AI_X, (int)pongAiY, PONG_PADDLE_W, PONG_PADDLE_H, SSD1306_WHITE);
  display.fillRect((int)pongBallX, (int)pongBallY, 2, 2, SSD1306_WHITE);

  display.display();
}

void drawPongStart() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(34, 6);
  display.print(F("PING PONG"));
  display.setCursor(14, 20);
  display.print(F("Wins: "));
  display.print(pongWins);
  display.setCursor(6, 33);
  display.print(F("L=up  R=down"));
  display.setCursor(8, 50);
  display.print(F("> Press OK to Play <"));
  display.display();
}

void drawPongGameOver() {
  display.clearDisplay();
  display.drawRect(5, 5, 118, 54, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(pongPlayerWon ? 40 : 38, 12);
  display.print(pongPlayerWon ? F("YOU WIN!") : F("YOU LOSE"));
  display.setCursor(34, 26);
  display.print(pongPlayerScore);
  display.print(F(" : "));
  display.print(pongAiScore);
  display.setCursor(20, 38);
  display.print(F("Total wins: "));
  display.print(pongWins);
  display.setCursor(16, 48);
  display.print(F("Press OK to retry"));
  display.display();
}

// ===================== TETRIS =====================
#define TETRIS_W 10
#define TETRIS_H 16
#define BLOCK_SIZE 4

const int BOARD_X = 0;
const int BOARD_Y = 0;

const uint16_t TETROMINOES[7][4] = {
  { 0x0F00, 0x2222, 0x00F0, 0x4444 },
  { 0x0660, 0x0660, 0x0660, 0x0660 },
  { 0x0E40, 0x4C40, 0x4E00, 0x4640 },
  { 0x06C0, 0x8C40, 0x6C00, 0x4620 },
  { 0x0C60, 0x4C80, 0xC600, 0x2640 },
  { 0x0E20, 0x44C0, 0x8E00, 0x6440 },
  { 0x0E80, 0xC440, 0x2E00, 0x4460 }
};

uint8_t board[TETRIS_H][TETRIS_W];
int currentPiece, currentRot, currentX, currentY;
int nextPiece;
long tetrisScore = 0;
long tetrisHighScore = 0;
int tetrisLevel = 1;
int tetrisLines = 0;
unsigned long lastFall = 0;
unsigned long fallSpeed = 500;
bool tetrisStarted = false;
bool tetrisGameOver = false;

bool canPlace(int piece, int rot, int x, int y) {
  uint16_t shape = TETROMINOES[piece][rot];
  for (int i = 0; i < 16; i++) {
    if (shape & (0x8000 >> i)) {
      int px = x + (i % 4);
      int py = y + (i / 4);
      if (px < 0 || px >= TETRIS_W || py >= TETRIS_H) return false;
      if (py >= 0 && board[py][px]) return false;
    }
  }
  return true;
}

void newPiece() {
  currentPiece = nextPiece;
  nextPiece = random(0, 7);
  currentRot = 0;
  currentX = 3;
  currentY = 0;

  if (!canPlace(currentPiece, currentRot, currentX, currentY)) {
    tetrisGameOver = true;
    if (tetrisScore > tetrisHighScore) {
      tetrisHighScore = tetrisScore;
      prefs.putLong("tetrishigh", tetrisHighScore);
    }
  }
}

void placePiece() {
  uint16_t shape = TETROMINOES[currentPiece][currentRot];
  for (int i = 0; i < 16; i++) {
    if (shape & (0x8000 >> i)) {
      int px = currentX + (i % 4);
      int py = currentY + (i / 4);
      if (py >= 0 && py < TETRIS_H && px >= 0 && px < TETRIS_W) {
        board[py][px] = 1;
      }
    }
  }
}

int clearLines() {
  int cleared = 0;
  for (int y = TETRIS_H - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < TETRIS_W; x++) {
      if (!board[y][x]) {
        full = false;
        break;
      }
    }
    if (full) {
      cleared++;
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < TETRIS_W; x++) {
          board[yy][x] = board[yy - 1][x];
        }
      }
      for (int x = 0; x < TETRIS_W; x++) board[0][x] = 0;
      y++;
    }
  }
  return cleared;
}

void resetTetris() {
  memset(board, 0, sizeof(board));
  tetrisScore = 0;
  tetrisLevel = 1;
  tetrisLines = 0;
  fallSpeed = 500;
  tetrisGameOver = false;
  nextPiece = random(0, 7);
  newPiece();
  lastFall = millis();
}

void updateTetris(bool clickLeft, bool clickRight, bool clickUp) {
  if (clickLeft) {
    if (canPlace(currentPiece, currentRot, currentX - 1, currentY)) currentX--;
  }
  if (clickRight) {
    if (canPlace(currentPiece, currentRot, currentX + 1, currentY)) currentX++;
  }
  if (clickUp) {
    int newRot = (currentRot + 1) % 4;
    if (canPlace(currentPiece, newRot, currentX, currentY)) currentRot = newRot;
  }

  bool softDrop = isHeld(btnOk);

  unsigned long now = millis();
  unsigned long speed = softDrop ? 40 : fallSpeed;

  if (now - lastFall >= speed) {
    if (canPlace(currentPiece, currentRot, currentX, currentY + 1)) {
      currentY++;
    } else {
      placePiece();
      int cleared = clearLines();
      if (cleared > 0) {
        tetrisLines += cleared;
        tetrisScore += cleared * cleared * 100 * tetrisLevel;
        tetrisLevel = 1 + tetrisLines / 10;
        fallSpeed = max(80UL, 500UL - (tetrisLevel - 1) * 40);
      }
      newPiece();
    }
    lastFall = now;
  }
}

void drawBlock(int x, int y, bool fill) {
  int sx = BOARD_X + x * BLOCK_SIZE;
  int sy = BOARD_Y + y * BLOCK_SIZE;
  if (fill) {
    display.fillRect(sx, sy, BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
  } else {
    display.drawRect(sx, sy, BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
  }
}

void renderTetris() {
  display.clearDisplay();

  for (int y = 0; y < TETRIS_H; y++) {
    for (int x = 0; x < TETRIS_W; x++) {
      if (board[y][x]) {
        drawBlock(x, y, true);
      }
    }
  }

  uint16_t shape = TETROMINOES[currentPiece][currentRot];
  for (int i = 0; i < 16; i++) {
    if (shape & (0x8000 >> i)) {
      int px = currentX + (i % 4);
      int py = currentY + (i / 4);
      if (py >= 0) drawBlock(px, py, true);
    }
  }

  display.drawRect(BOARD_X - 1, BOARD_Y - 1, TETRIS_W * BLOCK_SIZE + 1, TETRIS_H * BLOCK_SIZE + 1, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(48, 0);
  display.print(F("SCORE"));
  display.setCursor(48, 10);
  display.print(tetrisScore);

  display.setCursor(48, 22);
  display.print(F("LEVEL"));
  display.setCursor(48, 32);
  display.print(tetrisLevel);

  display.setCursor(48, 44);
  display.print(F("NEXT"));

  uint16_t nextShape = TETROMINOES[nextPiece][0];
  for (int i = 0; i < 16; i++) {
    if (nextShape & (0x8000 >> i)) {
      int px = 11 + (i % 4);
      int py = 12 + (i / 4);
      display.fillRect(px * 3, py * 3, 2, 2, SSD1306_WHITE);
    }
  }

  display.display();
}

void drawTetrisStart() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, 8);
  display.print(F("TETRIS"));
  display.setCursor(10, 24);
  display.print(F("Hi-Score: "));
  display.print(tetrisHighScore);
  display.setCursor(6, 40);
  display.print(F("L/R move  UP rotate"));
  display.setCursor(8, 52);
  display.print(F("> Press OK to Play <"));
  display.display();
}

void drawTetrisGameOver() {
  display.clearDisplay();
  display.drawRect(5, 5, 118, 54, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(36, 12);
  display.print(F("GAME OVER"));
  display.setCursor(20, 26);
  display.print(F("Score: "));
  display.print(tetrisScore);
  display.setCursor(16, 40);
  display.print(F("Press OK to retry"));
  display.display();
}

// ===================== MENU =====================
void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.drawRect(22, 1, 84, 13, SSD1306_WHITE);
  display.setCursor(34, 4);
  display.print(F("SELECT GAME"));

  const char* names[MENU_COUNT] = { "DINO GAME", "DOOM", "PING PONG", "TETRIS" };
  const int xs[MENU_COUNT] = { 40, 48, 34, 44 };

  for (int i = 0; i < MENU_COUNT; i++) {
    int top = 14 + i * 11;
    if (menuSelection == i) {
      display.fillRect(8, top, 112, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(xs[i], top + 1);
    display.print(names[i]);
  }

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print(F("L/R=select   OK=start"));
}

// ===================== SETUP / LOOP =====================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_OK,    INPUT_PULLDOWN);
  pinMode(PIN_UP,    INPUT_PULLDOWN);
  pinMode(PIN_LEFT,  INPUT_PULLDOWN);
  pinMode(PIN_RIGHT, INPUT_PULLDOWN);

  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED failed"));
    for (;;);
  }

  prefs.begin("dinogame", false);
  highScore       = prefs.getLong("highscore", 0);
  mostKilled      = prefs.getInt("mostkilled", 0);
  totalKills      = prefs.getLong("totalkills", 0);
  totalDeaths     = prefs.getLong("totaldeaths", 0);
  pongWins        = prefs.getLong("pongwins", 0);
  tetrisHighScore = prefs.getLong("tetrishigh", 0);

  randomSeed(analogRead(0));

  display.clearDisplay();
  display.display();
}

void loop() {
  bool clickOk    = readClick(btnOk);
  bool clickUp    = readClick(btnUp);
  bool clickLeft  = readClick(btnLeft);
  bool clickRight = readClick(btnRight);

  if (appState == STATE_MENU) {
    if (clickLeft)  menuSelection = (menuSelection + MENU_COUNT - 1) % MENU_COUNT;
    if (clickRight) menuSelection = (menuSelection + 1) % MENU_COUNT;
    if (clickOk) {
      if (menuSelection == MENU_DINO) {
        appState = STATE_DINO;
        dinoStarted = false;
        resetDino();
      } else if (menuSelection == MENU_DOOM) {
        appState = STATE_DOOM;
        doomStarted = false;
        resetDoom();
      } else if (menuSelection == MENU_PONG) {
        appState = STATE_PONG;
        pongStarted = false;
        resetPong();
      } else {
        appState = STATE_TETRIS;
        tetrisStarted = false;
        resetTetris();
      }
    }
    drawMenu();
    display.display();
    delay(30);
    return;
  }

  if (isLongPress(btnOk)) {
    appState = STATE_MENU;
    btnOk.pressStart = 0;
    return;
  }

  if (appState == STATE_DINO) {
    if (!dinoStarted) {
      if (clickOk) {
        dinoStarted = true;
        resetDino();
      }
      display.clearDisplay();
      drawDinoStart();
      display.display();
      delay(FRAME_DELAY_MS);
      return;
    }

    if (dinoGameOver) {
      if (clickOk) resetDino();
      display.clearDisplay();
      drawDinoGameOver();
      display.display();
      delay(FRAME_DELAY_MS);
      return;
    }

    bool jump = clickOk || clickUp;
    updateDino(jump);
    updateObstacle();
    updateScore();
    if (checkCollision()) endDino();

    display.clearDisplay();
    drawDinoHud();
    drawGround();
    drawDinoSprite();
    drawObstacle();
    display.display();
    delay(FRAME_DELAY_MS);
    return;
  }

  if (appState == STATE_DOOM) {
    if (!doomStarted) {
      if (clickOk) {
        doomStarted = true;
        resetDoom();
      }
      drawDoomStart();
      display.display();
      delay(30);
      return;
    }

    if (doomGameOver) {
      drawDoomGameOver();
      display.display();
      delay(30);
      if (clickOk) {
        resetDoom();
        doomStarted = true;
      }
      return;
    }

    updateDoom();
    renderDoom();
    display.display();
    delay(20);
    return;
  }

  if (appState == STATE_PONG) {
    if (!pongStarted) {
      if (clickOk) {
        pongStarted = true;
        resetPong();
      } else {
        drawPongStart();
      }
      delay(30);
      return;
    }

    if (pongGameOver) {
      drawPongGameOver();
      if (clickOk) resetPong();
      delay(30);
      return;
    }

    updatePong();
    renderPong();
    delay(20);
    return;
  }

  if (appState == STATE_TETRIS) {
    if (!tetrisStarted) {
      if (clickOk) {
        tetrisStarted = true;
        resetTetris();
      } else {
        drawTetrisStart();
      }
      delay(30);
      return;
    }

    if (tetrisGameOver) {
      drawTetrisGameOver();
      if (clickOk) {
        resetTetris();
        tetrisStarted = true;
      }
      delay(30);
      return;
    }

    updateTetris(clickLeft, clickRight, clickUp);
    renderTetris();
    delay(20);
  }
}