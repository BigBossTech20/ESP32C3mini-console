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

enum AppState { STATE_MENU, STATE_DINO, STATE_RAYCASTER, STATE_PONG, STATE_TETRIS, STATE_SNAKE };
AppState appState = STATE_MENU;

enum MenuItem { MENU_DINO = 0, MENU_RAYCASTER = 1, MENU_PONG = 2, MENU_TETRIS = 3, MENU_SNAKE = 4 };
const int MENU_COUNT = 5;
const int MENU_VISIBLE = 4;
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

void drawStars() {
  const uint8_t stars[][2] = {
    {3,5},{12,14},{25,3},{38,18},{47,8},{61,22},{74,4},{89,15},
    {103,9},{117,19},{8,28},{22,35},{41,31},{55,42},{70,27},
    {95,38},{110,33},{15,48},{33,55},{58,51},{82,47},{105,56}
  };
  for (uint8_t i = 0; i < 22; i++) {
    display.drawPixel(stars[i][0], stars[i][1], SSD1306_WHITE);
  }
}

void drawMeteor(int x, int y) {
  display.drawPixel(x,   y,   SSD1306_WHITE);
  display.drawPixel(x+1, y,   SSD1306_WHITE);
  display.drawPixel(x+2, y+1, SSD1306_WHITE);
  display.drawPixel(x-1, y-1, SSD1306_WHITE);
}

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
  score = 0; dinoY = DINO_GROUND_Y; dinoVy = 0; isJumping = false;
  gameSpeed = SPEED_START; obstacleX = SCREEN_WIDTH;
  spawnObstacle(); dinoGameOver = false; lastScoreTime = millis();
}

void updateDino(bool jumpPressed) {
  if (jumpPressed && !isJumping) { dinoVy = JUMP_VELOCITY; isJumping = true; }
  dinoY += dinoVy; dinoVy += GRAVITY;
  if (dinoY > DINO_GROUND_Y) { dinoY = DINO_GROUND_Y; dinoVy = 0; isJumping = false; }
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
    score += SCORE_PER_TICK; lastScoreTime = millis();
  }
}

bool checkCollision() {
  bool xOverlap = (obstacleX < DINO_X + DINO_W) && (obstacleX + obstacleW > DINO_X);
  int obstacleTopY = GROUND_Y - obstacleH;
  return xOverlap && ((dinoY + DINO_H) > obstacleTopY);
}

void endDino() {
  dinoGameOver = true;
  if (score > highScore) { highScore = score; prefs.putLong("highscore", highScore); }
}

void drawDinoStart() {
  display.clearDisplay(); drawStars(); drawMeteor(100, 12); drawMeteor(20, 40);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(34, 14); display.print(F("DINO GAME"));
  display.drawLine(28, 24, 100, 24, SSD1306_WHITE);
  display.setCursor(22, 34); display.print(F("Hi-Score: ")); display.print(highScore);
}

void drawDinoHud() {
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(55, 2); display.print(F("SCORE:")); display.print(score);
  display.setCursor(55, 12); display.print(F("HI:")); display.print(highScore);
}

void drawGround() { display.drawLine(0, GROUND_Y, SCREEN_WIDTH - 1, GROUND_Y, SSD1306_WHITE); }
void drawDinoSprite() { display.fillRect(DINO_X, dinoY, DINO_W, DINO_H, SSD1306_WHITE); }
void drawObstacle() { display.fillRect(obstacleX, GROUND_Y - obstacleH, obstacleW, obstacleH, SSD1306_WHITE); }

void drawDinoGameOver() {
  display.clearDisplay(); drawStars();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(34, 18); display.print(F("GAME OVER"));
  display.setCursor(24, 34); display.print(F("Score: ")); display.print(score);
}

const int MAP_W = 16, MAP_H = 16;
const char mapData[MAP_H][MAP_W + 1] PROGMEM = {
  "################","#..............#","#...####.......#","#.......#......#",
  "#.......#..##..#","#.......#......#","#..##..........#","#..##....####..#",
  "#........#.....#","#..######.#.####","#........#.....#","#..######.###..#",
  "#..............#","#..#########...#","#..............#","################"
};

float playerX = 3.5f, playerY = 3.5f, playerA = 0.0f;
const float FOV = 60.0f * (M_PI / 180.0f);
const float MOVE_SPEED = 0.08f, ROT_SPEED = 0.10f;
#define MAX_ENEMIES 4

struct Enemy { float x, y; bool alive; unsigned long respawnAt; };
Enemy enemies[MAX_ENEMIES];
int kills = 0, deaths = 0, mostKilled = 0;
long totalKills = 0, totalDeaths = 0;
bool raycasterStarted = false, raycasterGameOver = false;

const uint8_t enemySprite[] PROGMEM = {
  0b00111100,0b01111110,0b11100111,0b11111111,0b11011011,0b01111110,
  0b00111100,0b01100110,0b11000011,0b11000011,0b01100110,0b00111100
};
const int ENEMY_SPRITE_W = 8, ENEMY_SPRITE_H = 12;

void drawEnemySprite(int screenX, int y0, int size) {
  float scaleY = (float)size / ENEMY_SPRITE_H;
  float scaleX = scaleY * 0.75f;
  int drawW = max(5, (int)(ENEMY_SPRITE_W * scaleX));
  int startX = screenX - drawW / 2;
  for (int pass = 0; pass < 2; pass++) {
    uint16_t color = (pass == 0) ? SSD1306_WHITE : SSD1306_BLACK;
    int expand = (pass == 0) ? 1 : 0;
    for (int sy = 0; sy < ENEMY_SPRITE_H; sy++) {
      uint8_t row = pgm_read_byte(&enemySprite[sy]);
      int py = y0 + (int)(sy * scaleY);
      if (py < 0 || py >= SCREEN_HEIGHT) continue;
      for (int sx = 0; sx < ENEMY_SPRITE_W; sx++) {
        if (!(row & (0x80 >> sx))) continue;
        int px = startX + (int)(sx * scaleX);
        for (int oy = -expand; oy <= expand + (scaleY > 1.2f ? 1 : 0); oy++) {
          for (int ox = -expand; ox <= expand + 1; ox++) {
            int dx = px + ox, dy = py + oy;
            if (dx < 0 || dx >= SCREEN_WIDTH || dy < 0 || dy >= SCREEN_HEIGHT) continue;
            display.drawPixel(dx, dy, color);
          }
        }
      }
    }
  }
}

bool isWall(int mx, int my) {
  if (mx < 0 || my < 0 || mx >= MAP_W || my >= MAP_H) return true;
  return pgm_read_byte(&(mapData[my][mx])) == '#';
}

float castRay(float angle) {
  float sinA = sinf(angle), cosA = cosf(angle), dist = 0.0f;
  const float step = 0.04f, maxDist = 16.0f;
  while (dist < maxDist) {
    if (isWall((int)(playerX + cosA * dist), (int)(playerY + sinA * dist))) break;
    dist += step;
  }
  return (dist >= maxDist) ? maxDist : dist;
}

bool hasLineOfSight(float targetX, float targetY) {
  float dx = targetX - playerX, dy = targetY - playerY;
  float dist = sqrtf(dx * dx + dy * dy);
  if (dist < 0.1f) return true;
  return castRay(atan2f(dy, dx)) > dist - 0.15f;
}

void spawnEnemy(int i) {
  float ex, ey; int tries = 0; bool bad = true;
  do {
    ex = 2.5f + random(0, 11); ey = 2.5f + random(0, 11); tries++;
    float d = sqrtf((ex - playerX) * (ex - playerX) + (ey - playerY) * (ey - playerY));
    bad = isWall((int)ex, (int)ey) || d < 6.0f;
  } while (bad && tries < 80);
  if (bad) {
    ex = (playerX < 8.0f) ? 12.5f : 2.5f;
    ey = (playerY < 8.0f) ? 12.5f : 2.5f;
    if (isWall((int)ex, (int)ey)) { ex = 8.5f; ey = 8.5f; }
  }
  enemies[i].x = ex; enemies[i].y = ey; enemies[i].alive = true; enemies[i].respawnAt = 0;
}

void resetRaycaster() {
  playerX = 3.5f; playerY = 3.5f; playerA = 0.0f;
  kills = 0; deaths = 0; raycasterGameOver = false;
  for (int i = 0; i < MAX_ENEMIES; i++) spawnEnemy(i);
}

void updateEnemies() {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].alive) {
      if (millis() > enemies[i].respawnAt) spawnEnemy(i);
      continue;
    }
    float dx = playerX - enemies[i].x, dy = playerY - enemies[i].y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.35f) {
      deaths++; totalDeaths++; prefs.putLong("totaldeaths", totalDeaths);
      raycasterGameOver = true;
      if (kills > mostKilled) { mostKilled = kills; prefs.putInt("mostkilled", mostKilled); }
      return;
    }
    if (dist > 0.4f) {
      float speed = 0.020f;
      float nx = enemies[i].x + (dx / dist) * speed;
      float ny = enemies[i].y + (dy / dist) * speed;
      if (!isWall((int)nx, (int)enemies[i].y)) enemies[i].x = nx;
      if (!isWall((int)enemies[i].x, (int)ny)) enemies[i].y = ny;
    }
  }
}

void tryShoot() {
  int bestIdx = -1; float bestDist = 999.0f;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].alive) continue;
    float dx = enemies[i].x - playerX, dy = enemies[i].y - playerY;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > 9.0f || !hasLineOfSight(enemies[i].x, enemies[i].y)) continue;
    float ang = atan2f(dy, dx), diff = ang - playerA;
    while (diff > M_PI) diff -= 2 * M_PI;
    while (diff < -M_PI) diff += 2 * M_PI;
    if (fabs(diff) < 0.28f && dist < bestDist) { bestDist = dist; bestIdx = i; }
  }
  if (bestIdx >= 0) {
    enemies[bestIdx].alive = false;
    enemies[bestIdx].respawnAt = millis() + 2500 + random(0, 1500);
    kills++; totalKills++; prefs.putLong("totalkills", totalKills);
  }
}

void updateRaycaster() {
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
  if (isHeld(btnOk)) { if (!wasShooting) { tryShoot(); wasShooting = true; } }
  else wasShooting = false;
}

void renderRaycaster() {
  display.clearDisplay();
  const int COL_STEP = 2;
  for (int x = 0; x < SCREEN_WIDTH; x += COL_STEP) {
    float rayAng = (playerA - FOV / 2.0f) + (x / (float)SCREEN_WIDTH) * FOV;
    float dist = castRay(rayAng) * cosf(rayAng - playerA);
    int lineH = (int)(SCREEN_HEIGHT / (dist + 0.001f));
    if (lineH > SCREEN_HEIGHT) lineH = SCREEN_HEIGHT;
    int y0 = (SCREEN_HEIGHT - lineH) / 2;
    display.drawFastVLine(x, y0, lineH, SSD1306_WHITE);
    if (COL_STEP > 1) display.drawFastVLine(x + 1, y0, lineH, SSD1306_WHITE);
  }
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].alive) continue;
    float dx = enemies[i].x - playerX, dy = enemies[i].y - playerY;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.2f || dist > 12.0f) continue;
    if (!hasLineOfSight(enemies[i].x, enemies[i].y) && dist > 2.5f) continue;
    float ang = atan2f(dy, dx), diff = ang - playerA;
    while (diff > M_PI) diff -= 2 * M_PI;
    while (diff < -M_PI) diff += 2 * M_PI;
    if (fabs(diff) > FOV * 0.75f) continue;
    int screenX = (int)((0.5f + diff / FOV) * SCREEN_WIDTH);
    int size = (int)(40.0f / dist);
    if (size < 8) size = 8; if (size > 48) size = 48;
    drawEnemySprite(screenX, (SCREEN_HEIGHT - size) / 2, size);
  }
  int cx = SCREEN_WIDTH / 2, cy = SCREEN_HEIGHT / 2;
  display.drawFastHLine(cx - 4, cy, 9, SSD1306_WHITE);
  display.drawFastVLine(cx, cy - 4, 9, SSD1306_WHITE);
  static uint32_t flashUntil = 0;
  if (isHeld(btnOk)) flashUntil = millis() + 50;
  if ((int32_t)(flashUntil - millis()) > 0) display.fillRect(SCREEN_WIDTH - 18, 0, 18, 9, SSD1306_WHITE);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0); display.print(kills);
}

void drawRaycasterStart() {
  display.clearDisplay(); drawStars(); drawMeteor(15, 18); drawMeteor(105, 45);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(34, 8); display.print(F("RAYCASTER"));
  display.drawLine(30, 18, 98, 18, SSD1306_WHITE);
  display.setCursor(20, 28); display.print(F("Total K: ")); display.print(totalKills);
  display.setCursor(20, 38); display.print(F("Total D: ")); display.print(totalDeaths);
  display.setCursor(20, 48); display.print(F("BEST: ")); display.print(mostKilled);
}

void drawRaycasterGameOver() {
  display.clearDisplay(); drawStars();
  display.drawRect(6, 4, 116, 56, SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(36, 12); display.print(F("YOU DIED"));
  display.setCursor(16, 26); display.print(F("Run Kills: ")); display.print(kills);
  display.setCursor(16, 36); display.print(F("Total K/D: "));
  display.print(totalKills); display.print(F("/")); display.print(totalDeaths);
  display.setCursor(16, 48); display.print(F("BEST: ")); display.print(mostKilled);
}

const int PONG_PADDLE_W = 3, PONG_PADDLE_H = 14;
const int PONG_PLAYER_X = 4;
const int PONG_AI_X = SCREEN_WIDTH - 4 - PONG_PADDLE_W;
const int PONG_WIN_SCORE = 7;
const float PONG_PLAYER_SPEED = 3.0f, PONG_AI_SPEED = 1.8f, PONG_BALL_MAX_VX = 4.5f;

float pongPlayerY = (SCREEN_HEIGHT - PONG_PADDLE_H) / 2.0f;
float pongAiY = (SCREEN_HEIGHT - PONG_PADDLE_H) / 2.0f;
float pongBallX = SCREEN_WIDTH / 2.0f, pongBallY = SCREEN_HEIGHT / 2.0f;
float pongBallVx = 2.0f, pongBallVy = 1.0f;
int pongPlayerScore = 0, pongAiScore = 0;
long pongWins = 0;
bool pongStarted = false, pongGameOver = false, pongPlayerWon = false;

void pongServe(int dir) {
  pongBallX = SCREEN_WIDTH / 2.0f; pongBallY = SCREEN_HEIGHT / 2.0f;
  pongBallVx = 2.0f * dir;
  pongBallVy = random(-15, 16) / 10.0f;
  if (fabsf(pongBallVy) < 0.5f) pongBallVy = 0.8f;
}

void resetPong() {
  pongPlayerY = (SCREEN_HEIGHT - PONG_PADDLE_H) / 2.0f;
  pongAiY = (SCREEN_HEIGHT - PONG_PADDLE_H) / 2.0f;
  pongPlayerScore = 0; pongAiScore = 0;
  pongGameOver = false; pongPlayerWon = false;
  pongServe(random(0, 2) ? 1 : -1);
}

void endPong() {
  pongGameOver = true;
  pongPlayerWon = (pongPlayerScore > pongAiScore);
  if (pongPlayerWon) { pongWins++; prefs.putLong("pongwins", pongWins); }
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

  pongBallX += pongBallVx; pongBallY += pongBallVy;
  if (pongBallY < 0) { pongBallY = 0; pongBallVy = -pongBallVy; }
  if (pongBallY > SCREEN_HEIGHT - 2) { pongBallY = SCREEN_HEIGHT - 2; pongBallVy = -pongBallVy; }

  if (pongBallVx < 0 && pongBallX <= PONG_PLAYER_X + PONG_PADDLE_W && pongBallX >= PONG_PLAYER_X - 2 &&
      pongBallY + 2 >= pongPlayerY && pongBallY <= pongPlayerY + PONG_PADDLE_H) {
    pongBallVx = -pongBallVx * 1.06f;
    if (pongBallVx > PONG_BALL_MAX_VX) pongBallVx = PONG_BALL_MAX_VX;
    float offset = (pongBallY + 1 - (pongPlayerY + PONG_PADDLE_H / 2.0f)) / (PONG_PADDLE_H / 2.0f);
    pongBallVy = offset * 2.5f; pongBallX = PONG_PLAYER_X + PONG_PADDLE_W + 1;
  }
  if (pongBallVx > 0 && pongBallX + 2 >= PONG_AI_X && pongBallX + 2 <= PONG_AI_X + PONG_PADDLE_W + 2 &&
      pongBallY + 2 >= pongAiY && pongBallY <= pongAiY + PONG_PADDLE_H) {
    pongBallVx = -pongBallVx * 1.06f;
    if (pongBallVx < -PONG_BALL_MAX_VX) pongBallVx = -PONG_BALL_MAX_VX;
    float offset = (pongBallY + 1 - (pongAiY + PONG_PADDLE_H / 2.0f)) / (PONG_PADDLE_H / 2.0f);
    pongBallVy = offset * 2.5f; pongBallX = PONG_AI_X - 3;
  }
  if (pongBallX < -2) {
    pongAiScore++;
    if (pongAiScore >= PONG_WIN_SCORE) endPong(); else pongServe(-1);
  } else if (pongBallX > SCREEN_WIDTH) {
    pongPlayerScore++;
    if (pongPlayerScore >= PONG_WIN_SCORE) endPong(); else pongServe(1);
  }
}

void renderPong() {
  display.clearDisplay();
  for (int y = 0; y < SCREEN_HEIGHT; y += 6)
    display.drawFastVLine(SCREEN_WIDTH / 2, y, 3, SSD1306_WHITE);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(SCREEN_WIDTH / 2 - 20, 2); display.print(pongPlayerScore);
  display.setCursor(SCREEN_WIDTH / 2 + 15, 2); display.print(pongAiScore);
  display.fillRect(PONG_PLAYER_X, (int)pongPlayerY, PONG_PADDLE_W, PONG_PADDLE_H, SSD1306_WHITE);
  display.fillRect(PONG_AI_X, (int)pongAiY, PONG_PADDLE_W, PONG_PADDLE_H, SSD1306_WHITE);
  display.fillRect((int)pongBallX, (int)pongBallY, 2, 2, SSD1306_WHITE);
  display.display();
}

void drawPongStart() {
  display.clearDisplay(); drawStars(); drawMeteor(90, 10);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(34, 14); display.print(F("PING PONG"));
  display.drawLine(30, 24, 98, 24, SSD1306_WHITE);
  display.setCursor(28, 36); display.print(F("Wins: ")); display.print(pongWins);
}

void drawPongGameOver() {
  display.clearDisplay();
  drawStars();
  drawMeteor(18, 48);
  drawMeteor(100, 12);
  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  if (pongPlayerWon) { display.setCursor(40, 3); display.print(F("YOU WIN")); }
  else { display.setCursor(38, 3); display.print(F("YOU LOSE")); }
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(42, 28);
  display.print(pongPlayerScore); display.print(F(" : ")); display.print(pongAiScore);
  display.setCursor(28, 44); display.print(F("Wins: ")); display.print(pongWins);
}

#define TETRIS_W 12
#define TETRIS_H 12
#define BLOCK_SIZE 5
const int BOARD_X = 4, BOARD_Y = 2;

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
int currentPiece, currentRot, currentX, currentY, nextPiece;
long tetrisScore = 0, tetrisHighScore = 0;
int tetrisLevel = 1, tetrisLines = 0;
unsigned long lastFall = 0, fallSpeed = 500;
bool tetrisStarted = false, tetrisGameOver = false;

#define MAX_PARTICLES 48
struct Particle {
  float x, y, vx, vy;
  uint8_t life, maxLife;
  bool active;
};
Particle particles[MAX_PARTICLES];

void spawnParticles(int cellX, int cellY, int count) {
  float cx = BOARD_X + cellX * BLOCK_SIZE + BLOCK_SIZE / 2.0f;
  float cy = BOARD_Y + cellY * BLOCK_SIZE + BLOCK_SIZE / 2.0f;
  int spawned = 0;
  for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
    if (particles[i].active) continue;
    float ang = random(0, 628) / 100.0f;
    float spd = 0.4f + random(0, 120) / 100.0f;
    particles[i].x = cx + random(-2, 3);
    particles[i].y = cy + random(-2, 3);
    particles[i].vx = cosf(ang) * spd;
    particles[i].vy = sinf(ang) * spd - 0.3f;
    particles[i].maxLife = 18 + random(0, 14);
    particles[i].life = particles[i].maxLife;
    particles[i].active = true;
    spawned++;
  }
}

void updateParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    particles[i].x += particles[i].vx;
    particles[i].y += particles[i].vy;
    particles[i].vy += 0.08f;
    particles[i].vx *= 0.96f;
    if (particles[i].life > 0) particles[i].life--;
    else particles[i].active = false;
  }
}

void drawParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    int px = (int)particles[i].x, py = (int)particles[i].y;
    if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) continue;
    display.drawPixel(px, py, SSD1306_WHITE);
    if (particles[i].life > particles[i].maxLife / 2) {
      if (px + 1 < SCREEN_WIDTH) display.drawPixel(px + 1, py, SSD1306_WHITE);
      if (py + 1 < SCREEN_HEIGHT) display.drawPixel(px, py + 1, SSD1306_WHITE);
    }
  }
}

bool canPlace(int piece, int rot, int x, int y) {
  uint16_t shape = TETROMINOES[piece][rot];
  for (int i = 0; i < 16; i++) {
    if (shape & (0x8000 >> i)) {
      int px = x + (i % 4), py = y + (i / 4);
      if (px < 0 || px >= TETRIS_W || py >= TETRIS_H) return false;
      if (py >= 0 && board[py][px]) return false;
    }
  }
  return true;
}

void newPiece() {
  currentPiece = nextPiece; nextPiece = random(0, 7);
  currentRot = 0; currentX = 4; currentY = 0;
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
      int px = currentX + (i % 4), py = currentY + (i / 4);
      if (py >= 0 && py < TETRIS_H && px >= 0 && px < TETRIS_W) board[py][px] = 1;
    }
  }
}

int clearLines() {
  int cleared = 0;
  for (int y = TETRIS_H - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < TETRIS_W; x++) if (!board[y][x]) { full = false; break; }
    if (full) {
      for (int x = 0; x < TETRIS_W; x++) spawnParticles(x, y, 3);
      cleared++;
      for (int yy = y; yy > 0; yy--)
        for (int x = 0; x < TETRIS_W; x++) board[yy][x] = board[yy - 1][x];
      for (int x = 0; x < TETRIS_W; x++) board[0][x] = 0;
      y++;
    }
  }
  return cleared;
}

void resetTetris() {
  memset(board, 0, sizeof(board));
  tetrisScore = 0; tetrisLevel = 1; tetrisLines = 0;
  fallSpeed = 500; tetrisGameOver = false;
  nextPiece = random(0, 7);
  for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
  newPiece(); lastFall = millis();
}

void updateTetris(bool clickLeft, bool clickRight, bool clickUp) {
  if (clickLeft  && canPlace(currentPiece, currentRot, currentX - 1, currentY)) currentX--;
  if (clickRight && canPlace(currentPiece, currentRot, currentX + 1, currentY)) currentX++;
  if (clickUp) {
    int newRot = (currentRot + 1) % 4;
    if (canPlace(currentPiece, newRot, currentX, currentY)) currentRot = newRot;
  }
  bool softDrop = isHeld(btnOk);
  unsigned long now = millis();
  unsigned long speed = softDrop ? 40 : fallSpeed;
  if (now - lastFall >= speed) {
    if (canPlace(currentPiece, currentRot, currentX, currentY + 1)) currentY++;
    else {
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
  int sx = BOARD_X + x * BLOCK_SIZE, sy = BOARD_Y + y * BLOCK_SIZE;
  if (fill) display.fillRect(sx, sy, BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
  else display.drawRect(sx, sy, BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
}

void renderTetris() {
  display.clearDisplay();
  for (int y = 0; y < TETRIS_H; y++)
    for (int x = 0; x < TETRIS_W; x++)
      if (board[y][x]) drawBlock(x, y, true);

  uint16_t shape = TETROMINOES[currentPiece][currentRot];
  for (int i = 0; i < 16; i++) {
    if (shape & (0x8000 >> i)) {
      int px = currentX + (i % 4), py = currentY + (i / 4);
      if (py >= 0) drawBlock(px, py, true);
    }
  }
  display.drawRect(BOARD_X - 1, BOARD_Y - 1, TETRIS_W * BLOCK_SIZE + 1, TETRIS_H * BLOCK_SIZE + 1, SSD1306_WHITE);

  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(72, 4);  display.print(F("SCORE"));
  display.setCursor(72, 14); display.print(tetrisScore);
  display.setCursor(72, 28); display.print(F("LINES"));
  display.setCursor(72, 38); display.print(tetrisLines);

  updateParticles();
  drawParticles();
  display.display();
}

void drawTetrisStart() {
  display.clearDisplay(); drawStars(); drawMeteor(18, 14); drawMeteor(108, 38);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(42, 14); display.print(F("TETRIS"));
  display.drawLine(36, 24, 92, 24, SSD1306_WHITE);
  display.setCursor(20, 36); display.print(F("Hi-Score: ")); display.print(tetrisHighScore);
}

void drawTetrisGameOver() {
  display.clearDisplay();
  drawStars();
  drawMeteor(14, 10);
  drawMeteor(108, 50);
  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(34, 3); display.print(F("GAME OVER"));
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 28); display.print(F("Score: ")); display.print(tetrisScore);
  display.setCursor(28, 42); display.print(F("Best:  ")); display.print(tetrisHighScore);
}

#define SNAKE_COLS 30
#define SNAKE_ROWS 12
#define SNAKE_CELL 4
#define SNAKE_MAX  (SNAKE_COLS * SNAKE_ROWS)
const int SNAKE_OX = 4, SNAKE_OY = 14;
const int8_t SNAKE_DX[4] = { 0, 1, 0, -1 };
const int8_t SNAKE_DY[4] = { -1, 0, 1, 0 };

uint8_t snakeX[SNAKE_MAX];
uint8_t snakeY[SNAKE_MAX];
int snakeLen = 3;
int snakeDir = 1, snakeNextDir = 1;
int snakeFoodX = 0, snakeFoodY = 0;
long snakeScore = 0, snakeHighScore = 0;
unsigned long snakeInterval = 140, lastSnakeMove = 0;
bool snakeStarted = false, snakeGameOver = false;

void spawnSnakeFood() {
  int total = SNAKE_COLS * SNAKE_ROWS;
  int start = random(0, total);
  for (int k = 0; k < total; k++) {
    int idx = (start + k) % total;
    int fx = idx % SNAKE_COLS, fy = idx / SNAKE_COLS;
    bool occupied = false;
    for (int i = 0; i < snakeLen; i++) {
      if (snakeX[i] == fx && snakeY[i] == fy) { occupied = true; break; }
    }
    if (!occupied) { snakeFoodX = fx; snakeFoodY = fy; return; }
  }
}

void resetSnake() {
  snakeLen = 3;
  snakeX[0] = 15; snakeY[0] = 6;
  snakeX[1] = 14; snakeY[1] = 6;
  snakeX[2] = 13; snakeY[2] = 6;
  snakeDir = 1; snakeNextDir = 1;
  snakeScore = 0; snakeInterval = 140;
  snakeGameOver = false;
  spawnSnakeFood();
  lastSnakeMove = millis();
}

void endSnake() {
  snakeGameOver = true;
  if (snakeScore > snakeHighScore) {
    snakeHighScore = snakeScore;
    prefs.putLong("snakehigh", snakeHighScore);
  }
}

void updateSnake(bool cUp, bool cLeft, bool cRight, bool cDown) {
  if (cUp    && snakeDir != 2) snakeNextDir = 0;
  if (cRight && snakeDir != 3) snakeNextDir = 1;
  if (cDown  && snakeDir != 0) snakeNextDir = 2;
  if (cLeft  && snakeDir != 1) snakeNextDir = 3;

  unsigned long now = millis();
  if (now - lastSnakeMove < snakeInterval) return;
  lastSnakeMove = now;

  snakeDir = snakeNextDir;
  int nx = snakeX[0] + SNAKE_DX[snakeDir];
  int ny = snakeY[0] + SNAKE_DY[snakeDir];

  if (nx < 0 || nx >= SNAKE_COLS || ny < 0 || ny >= SNAKE_ROWS) { endSnake(); return; }

  bool ate = (nx == snakeFoodX && ny == snakeFoodY);
  int limit = ate ? snakeLen : snakeLen - 1;
  for (int i = 0; i < limit; i++) {
    if (snakeX[i] == nx && snakeY[i] == ny) { endSnake(); return; }
  }

  if (ate) snakeLen++;
  for (int i = snakeLen - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  snakeX[0] = nx; snakeY[0] = ny;

  if (ate) {
    snakeScore += 10;
    if (snakeInterval > 60) snakeInterval -= 3;
    if (snakeLen >= SNAKE_MAX) { endSnake(); return; }
    spawnSnakeFood();
  }
}

void renderSnake() {
  display.clearDisplay();
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(3, 3);  display.print(F("SCORE:")); display.print(snakeScore);

  display.drawRect(SNAKE_OX - 1, SNAKE_OY - 1,
                   SNAKE_COLS * SNAKE_CELL + 2, SNAKE_ROWS * SNAKE_CELL + 2, SSD1306_WHITE);

  display.drawRect(SNAKE_OX + snakeFoodX * SNAKE_CELL, SNAKE_OY + snakeFoodY * SNAKE_CELL, 3, 3, SSD1306_WHITE);

  for (int i = 0; i < snakeLen; i++) {
    display.fillRect(SNAKE_OX + snakeX[i] * SNAKE_CELL, SNAKE_OY + snakeY[i] * SNAKE_CELL, 3, 3, SSD1306_WHITE);
  }
}

void drawSnakeStart() {
  display.clearDisplay(); drawStars(); drawMeteor(100, 12); drawMeteor(20, 44);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(49, 14); display.print(F("SNAKE"));
  display.drawLine(40, 24, 88, 24, SSD1306_WHITE);
  display.setCursor(22, 34); display.print(F("Hi-Score: ")); display.print(snakeHighScore);
}

void drawSnakeGameOver() {
  display.clearDisplay();
  drawStars();
  drawMeteor(14, 10);
  drawMeteor(108, 50);
  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(34, 3); display.print(F("GAME OVER"));
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(34, 34); display.print(F("Score: ")); display.print(snakeScore);
}

float menuBarY = 17.0f;
float menuScrollY = 0.0f;
int menuScrollTop = 0;

void drawMenu() {
  display.clearDisplay(); drawStars();

  if (menuSelection < menuScrollTop) menuScrollTop = menuSelection;
  if (menuSelection > menuScrollTop + MENU_VISIBLE - 1) menuScrollTop = menuSelection - (MENU_VISIBLE - 1);

  float scrollTarget = menuScrollTop * 11;
  menuScrollY += (scrollTarget - menuScrollY) * 0.35f;
  if (fabsf(scrollTarget - menuScrollY) < 0.5f) menuScrollY = scrollTarget;

  float barTarget = 17 + (menuSelection - menuScrollTop) * 11;
  menuBarY += (barTarget - menuBarY) * 0.35f;
  if (fabsf(barTarget - menuBarY) < 0.5f) menuBarY = barTarget;
  int by = (int)(menuBarY + 0.5f);
  int sy = (int)(menuScrollY + 0.5f);

  display.fillRect(4, by - 1, 120, 13, SSD1306_BLACK);

  const char* names[MENU_COUNT] = { "DINO GAME", "RAYCASTER", "PING PONG", "TETRIS", "SNAKE" };
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < MENU_COUNT; i++) {
    int ty = 19 + i * 11 - sy;
    if (ty <= -8 || ty >= SCREEN_HEIGHT) continue;
    display.setCursor(22, ty);
    display.print(names[i]);
  }

  for (int r = 0; r < 9; r++) {
    int y = by + 1 + r;
    if (y < 0 || y >= SCREEN_HEIGHT) continue;
    int inset = (r == 0 || r == 8) ? 2 : ((r == 1 || r == 7) ? 1 : 0);
    for (int x = 8 + inset; x < 120 - inset; x++) {
      bool p = display.getPixel(x, y);
      display.drawPixel(x, y, p ? SSD1306_BLACK : SSD1306_WHITE);
    }
  }

  display.drawRoundRect(4, by - 1, 120, 13, 4, SSD1306_WHITE);

  display.fillRect(0, 62, 128, 2, SSD1306_BLACK);

  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(28, 3); display.print(F("SELECT GAME"));
}

const char BOOT_TITLE[] = "MINI CONSOLE";
const int BOOT_TITLE_LEN = 12;
const int BOOT_TITLE_X = 28;
const int BOOT_TITLE_Y = 28;
const int BOOT_FRAMES = 110;
const int BOOT_TEXT_START = 14;

void drawBootText(int t) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int revealed = t / 2;
  if (revealed > BOOT_TITLE_LEN) revealed = BOOT_TITLE_LEN;

  const char scramble[] = "#%&*+=<>?/\\|01";
  for (int i = 0; i < BOOT_TITLE_LEN && i <= revealed; i++) {
    char c = BOOT_TITLE[i];
    if (c == ' ') continue;
    if (i == revealed && revealed < BOOT_TITLE_LEN) {
      c = scramble[random(0, 14)];
    }
    display.setCursor(BOOT_TITLE_X + i * 6, BOOT_TITLE_Y);
    display.write(c);
  }

  bool textDone = (revealed >= BOOT_TITLE_LEN);

  if (textDone && t >= BOOT_TITLE_LEN * 2 + 4) {
    display.setCursor(49, 40);
    display.print(F("v 0.3"));
  }

  if (!textDone) return;

  const int textL = BOOT_TITLE_X - 2;
  const int textR = BOOT_TITLE_X + BOOT_TITLE_LEN * 6;
  const int textT = BOOT_TITLE_Y - 2;
  const int textB = BOOT_TITLE_Y + 9;

  int phase = t - BOOT_TITLE_LEN * 2;
  if (phase >= 0 && phase < 24) {
    int bandX = textL - 10 + phase * 5;
    for (int x = bandX; x < bandX + 7; x++) {
      if (x < textL || x >= textR) continue;
      for (int y = textT; y < textB; y++) {
        bool p = display.getPixel(x, y);
        display.drawPixel(x, y, p ? SSD1306_BLACK : SSD1306_WHITE);
      }
    }
    for (int x = bandX + 7; x < bandX + 11; x++) {
      if (x < textL || x >= textR) continue;
      for (int y = textT; y < textB; y++) {
        if (((x + y) & 1) == 0) {
          bool p = display.getPixel(x, y);
          display.drawPixel(x, y, p ? SSD1306_BLACK : SSD1306_WHITE);
        }
      }
    }
  }

  if (phase >= 24 && phase < 28) {
    int gy = textT + random(0, textB - textT - 2);
    int shift = random(-3, 4);
    for (int y = gy; y < gy + 2; y++) {
      if (shift > 0) {
        for (int x = textR - 1; x >= textL + shift; x--)
          display.drawPixel(x, y, display.getPixel(x - shift, y));
      } else if (shift < 0) {
        for (int x = textL; x < textR + shift; x++)
          display.drawPixel(x, y, display.getPixel(x - shift, y));
      }
    }
  }

  int letterStart = BOOT_TITLE_LEN * 2 + 28;
  for (int i = 0; i < BOOT_TITLE_LEN; i++) {
    if (BOOT_TITLE[i] == ' ') continue;
    int tt = t - letterStart - i * 2;
    if (tt < 0 || tt >= 4) continue;
    int lx = BOOT_TITLE_X + i * 6 - 1;
    for (int x = lx; x < lx + 7; x++) {
      for (int y = textT; y < textB; y++) {
        if (tt < 2 || (((x + y) & 1) == 0)) {
          bool p = display.getPixel(x, y);
          display.drawPixel(x, y, p ? SSD1306_BLACK : SSD1306_WHITE);
        }
      }
    }
  }
}

void playBootAnimation() {
  for (int frame = 0; frame < BOOT_FRAMES; frame++) {
    display.clearDisplay(); drawStars();
    int mx = -8 + frame * 4;
    int my = 10 + (frame * frame) / 80;
    if (my > 50) my = 50;
    if (mx < SCREEN_WIDTH + 12) {
      for (int t = 0; t < 6; t++) {
        int tx = mx - t * 3, ty = my - t / 2;
        if (tx >= 0 && tx < SCREEN_WIDTH && ty >= 0 && ty < SCREEN_HEIGHT) {
          display.drawPixel(tx, ty, SSD1306_WHITE);
          if (t < 3) display.drawPixel(tx, ty + 1, SSD1306_WHITE);
        }
      }
      drawMeteor(mx, my);
    }
    if (frame >= BOOT_TEXT_START) drawBootText(frame - BOOT_TEXT_START);
    display.display(); delay(28);
  }
  delay(300);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_OK, INPUT_PULLDOWN);
  pinMode(PIN_UP, INPUT_PULLDOWN);
  pinMode(PIN_LEFT, INPUT_PULLDOWN);
  pinMode(PIN_RIGHT, INPUT_PULLDOWN);
  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED failed"));
    for (;;);
  }
  prefs.begin("dinogame", false);
  highScore = prefs.getLong("highscore", 0);
  mostKilled = prefs.getInt("mostkilled", 0);
  totalKills = prefs.getLong("totalkills", 0);
  totalDeaths = prefs.getLong("totaldeaths", 0);
  pongWins = prefs.getLong("pongwins", 0);
  tetrisHighScore = prefs.getLong("tetrishigh", 0);
  snakeHighScore = prefs.getLong("snakehigh", 0);
  randomSeed(analogRead(0));
  display.clearDisplay(); display.display();
  playBootAnimation();
}

void loop() {
  bool clickOk = readClick(btnOk);
  bool clickUp = readClick(btnUp);
  bool clickLeft = readClick(btnLeft);
  bool clickRight = readClick(btnRight);

  if (appState == STATE_MENU) {
    if (clickLeft)  menuSelection = (menuSelection + MENU_COUNT - 1) % MENU_COUNT;
    if (clickRight) menuSelection = (menuSelection + 1) % MENU_COUNT;
    if (clickOk) {
      if (menuSelection == MENU_DINO) { appState = STATE_DINO; dinoStarted = false; resetDino(); }
      else if (menuSelection == MENU_RAYCASTER) { appState = STATE_RAYCASTER; raycasterStarted = false; resetRaycaster(); }
      else if (menuSelection == MENU_PONG) { appState = STATE_PONG; pongStarted = true; resetPong(); }
      else if (menuSelection == MENU_TETRIS) { appState = STATE_TETRIS; tetrisStarted = true; resetTetris(); }
      else { appState = STATE_SNAKE; snakeStarted = false; resetSnake(); }
    }
    drawMenu(); display.display(); delay(15); return;
  }

  if (isLongPress(btnOk)) { appState = STATE_MENU; btnOk.pressStart = 0; return; }

  if (appState == STATE_DINO) {
    if (!dinoStarted) {
      if (clickOk) { dinoStarted = true; resetDino(); }
      display.clearDisplay(); drawDinoStart(); display.display(); delay(FRAME_DELAY_MS); return;
    }
    if (dinoGameOver) {
      if (clickOk) resetDino();
      display.clearDisplay(); drawDinoGameOver(); display.display(); delay(FRAME_DELAY_MS); return;
    }
    updateDino(clickOk || clickUp); updateObstacle(); updateScore();
    if (checkCollision()) endDino();
    display.clearDisplay(); drawDinoHud(); drawGround(); drawDinoSprite(); drawObstacle();
    display.display(); delay(FRAME_DELAY_MS); return;
  }

  if (appState == STATE_RAYCASTER) {
    if (!raycasterStarted) {
      if (clickOk) { raycasterStarted = true; resetRaycaster(); }
      drawRaycasterStart(); display.display(); delay(30); return;
    }
    if (raycasterGameOver) {
      drawRaycasterGameOver(); display.display(); delay(30);
      if (clickOk) { resetRaycaster(); raycasterStarted = true; }
      return;
    }
    updateRaycaster(); renderRaycaster(); display.display(); delay(20); return;
  }

  if (appState == STATE_PONG) {
    if (!pongStarted) {
      if (clickOk) { pongStarted = true; resetPong(); }
      else drawPongStart();
      delay(30); return;
    }
    if (pongGameOver) {
      drawPongGameOver(); display.display();
      if (clickOk) resetPong();
      delay(30); return;
    }
    updatePong(); renderPong(); delay(20); return;
  }

  if (appState == STATE_TETRIS) {
    if (!tetrisStarted) {
      if (clickOk) { tetrisStarted = true; resetTetris(); }
      else drawTetrisStart();
      delay(30); return;
    }
    if (tetrisGameOver) {
      drawTetrisGameOver(); display.display();
      if (clickOk) { resetTetris(); tetrisStarted = true; }
      delay(30); return;
    }
    updateTetris(clickLeft, clickRight, clickUp);
    renderTetris(); delay(20); return;
  }

  if (appState == STATE_SNAKE) {
    if (!snakeStarted) {
      if (clickOk) { snakeStarted = true; resetSnake(); }
      drawSnakeStart(); display.display(); delay(30); return;
    }
    if (snakeGameOver) {
      drawSnakeGameOver(); display.display();
      if (clickOk) resetSnake();
      delay(30); return;
    }
    updateSnake(clickUp, clickLeft, clickRight, clickOk);
    renderSnake(); display.display(); delay(10);
  }
}