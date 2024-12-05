#include <SPI.h>

// 腳位定義
#define BUTTON_UP_PIN 8
#define BUTTON_DOWN_PIN 9
#define BUTTON_LEFT_PIN 10
#define BUTTON_RIGHT_PIN 11
#define RED_LED 5
#define GREEN_LED 7

#define GRID_SIZE 8

uint8_t cube[8][8];
uint8_t snake[64][2];  // 儲存蛇身的座標 (最多64個)
uint8_t snakeLength = 1; // 初始蛇長度
uint8_t direction = 0;  // 0: 上, 1: 下, 2: 左, 3: 右
uint8_t food[2];        // 食物位置
bool gameOver = false;
uint16_t timer = 0;
uint16_t speed = 300;   // 移動速度 (毫秒)

// 初始化
void setup() {
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  randomSeed(analogRead(0));
  clearCube();
  snake[0][0] = 4; // 蛇頭初始位置
  snake[0][1] = 4;
  spawnFood();     // 生成食物
}

// 遊戲主循環
void loop() {
  if (gameOver) {
    digitalWrite(RED_LED, HIGH);
    return;
  }

  timer++;
  if (timer > speed) {
    timer = 0;
    moveSnake();
    renderCube();
  }

  // 檢查按鈕輸入
  if (digitalRead(BUTTON_UP_PIN) == LOW && direction != 1) direction = 0;
  if (digitalRead(BUTTON_DOWN_PIN) == LOW && direction != 0) direction = 1;
  if (digitalRead(BUTTON_LEFT_PIN) == LOW && direction != 3) direction = 2;
  if (digitalRead(BUTTON_RIGHT_PIN) == LOW && direction != 2) direction = 3;
}

// 顯示 LED 矩陣
void renderCube() {
  clearCube();
  for (uint8_t i = 0; i < snakeLength; i++) {
    setVoxel(snake[i][0], 0, snake[i][1]);
  }
  setVoxel(food[0], 0, food[1]); // 顯示食物
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(SS, LOW);
    SPI.transfer(0x01 << i);
    for (uint8_t j = 0; j < 8; j++) {
      SPI.transfer(cube[i][j]);
    }
    digitalWrite(SS, HIGH);
  }
}

// 移動蛇
void moveSnake() {
  uint8_t nextX = snake[0][0];
  uint8_t nextY = snake[0][1];

  switch (direction) {
    case 0: nextY--; break; // 上
    case 1: nextY++; break; // 下
    case 2: nextX--; break; // 左
    case 3: nextX++; break; // 右
  }

  // 撞牆或自身
  if (nextX >= GRID_SIZE || nextY >= GRID_SIZE || nextX < 0 || nextY < 0 || checkCollision(nextX, nextY)) {
    gameOver = true;
    return;
  }

  // 吃到食物
  if (nextX == food[0] && nextY == food[1]) {
    snakeLength++;
    spawnFood();
  }

  // 更新蛇身位置
  for (int i = snakeLength - 1; i > 0; i--) {
    snake[i][0] = snake[i - 1][0];
    snake[i][1] = snake[i - 1][1];
  }
  snake[0][0] = nextX;
  snake[0][1] = nextY;
}

// 生成食物
void spawnFood() {
  do {
    food[0] = random(0, GRID_SIZE);
    food[1] = random(0, GRID_SIZE);
  } while (checkCollision(food[0], food[1]));
}

// 檢查是否撞到自身
bool checkCollision(uint8_t x, uint8_t y) {
  for (uint8_t i = 0; i < snakeLength; i++) {
    if (snake[i][0] == x && snake[i][1] == y) return true;
  }
  return false;
}

// 設置 LED 點亮
void setVoxel(uint8_t x, uint8_t y, uint8_t z) {
  cube[7 - y][7 - z] |= (0x01 << x);
}

// 清空 LED
void clearCube() {
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      cube[i][j] = 0;
    }
  }
}
