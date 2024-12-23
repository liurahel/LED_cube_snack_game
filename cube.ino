#include <SPI.h>
#include <SoftwareSerial.h>

// 腳位定義
#define BUTTON_UP_PIN 8
#define BUTTON_DOWN_PIN 9
#define BUTTON_LEFT_PIN 10
#define BUTTON_RIGHT_PIN 11
#define RED_LED 5
#define GREEN_LED 7
#define TRIG_PIN 12
#define ECHO_PIN 13

#define GRID_SIZE 8

// 藍牙設定
SoftwareSerial BTSerial(10, 11); // RX, TX (用於藍牙模組)

// 遊戲狀態變數
uint8_t cube[8][8];
uint8_t snake[64][2];
uint8_t snakeLength = 1;
uint8_t direction = 0;  // 0: 上, 1: 下, 2: 左, 3: 右
uint8_t food[2];
bool gameOver = false;
uint16_t timer = 0;
uint16_t speed = 300;   // 移動速度 (毫秒)

const char* ssid = "yourSSID"; // 替換為你的Wi-Fi名稱
const char* password = "yourPassword"; // 替換為你的Wi-Fi密碼
String serverUrl = "http://yourserver.com/upload_score"; // 替換為你的伺服器 URL

void setup() {
  // 初始化按鈕、LED和感測器
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // 初始化SPI和藍牙
  SPI.begin();
  BTSerial.begin(9600); // 初始化藍牙串口
  Serial.begin(9600);   // 用於檢查控制指令

  // 連接 Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  randomSeed(analogRead(0));
  clearCube();
  snake[0][0] = 4; // 蛇頭初始位置
  snake[0][1] = 4;
  spawnFood();     // 生成食物
}

void loop() {
  if (gameOver) {
    digitalWrite(RED_LED, HIGH);
    uploadScore(snakeLength - 1); // 遊戲結束上傳分數
    return;
  }

  // 檢測物體靠近啟動遊戲
  long duration, distance;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) * 0.0344; // 計算距離 (單位：cm)
  
  if (distance < 10) {  // 距離小於10公分啟動遊戲
    digitalWrite(GREEN_LED, HIGH);
  }

  // 藍牙控制（讀取 BlueControl 發送的指令）
  if (BTSerial.available()) {
    char directionChar = BTSerial.read();
    Serial.println(directionChar);  // 輸出來檢查指令

    // 根據指令控制貪吃蛇的移動
    if (directionChar == 'U' && direction != 1) direction = 0; // 上
    if (directionChar == 'D' && direction != 0) direction = 1; // 下
    if (directionChar == 'L' && direction != 3) direction = 2; // 左
    if (directionChar == 'R' && direction != 2) direction = 3; // 右
  }

  // 遊戲邏輯
  timer++;
  if (timer > speed) {
    timer = 0;
    moveSnake();
    renderCube();
  }

  // 按鈕控制（可選）
  if (digitalRead(BUTTON_UP_PIN) == LOW && direction != 1) direction = 0;
  if (digitalRead(BUTTON_DOWN_PIN) == LOW && direction != 0) direction = 1;
  if (digitalRead(BUTTON_LEFT_PIN) == LOW && direction != 3) direction = 2;
  if (digitalRead(BUTTON_RIGHT_PIN) == LOW && direction != 2) direction = 3;
}

// 更新蛇
void moveSnake() {
  uint8_t nextX = snake[0][0];
  uint8_t nextY = snake[0][1];

  switch (direction) {
    case 0: nextY--; break; // 上
    case 1: nextY++; break; // 下
    case 2: nextX--; break; // 左
    case 3: nextX++; break; // 右
  }

  if (nextX >= GRID_SIZE || nextY >= GRID_SIZE || nextX < 0 || nextY < 0 || checkCollision(nextX, nextY)) {
    gameOver = true;
    return;
  }

  if (nextX == food[0] && nextY == food[1]) {
    snakeLength++;
    spawnFood();
  }

  for (int i = snakeLength - 1; i > 0; i--) {
    snake[i][0] = snake[i - 1][0];
    snake[i][1] = snake[i - 1][1];
  }
  snake[0][0] = nextX;
  snake[0][1] = nextY;
}

void spawnFood() {
  do {
    food[0] = random(0, GRID_SIZE);
    food[1] = random(0, GRID_SIZE);
  } while (checkCollision(food[0], food[1]));
}

bool checkCollision(uint8_t x, uint8_t y) {
  for (uint8_t i = 0; i < snakeLength; i++) {
    if (snake[i][0] == x && snake[i][1] == y) return true;
  }
  return false;
}

void renderCube() {
  clearCube();
  for (uint8_t i = 0; i < snakeLength; i++) {
    setVoxel(snake[i][0], 0, snake[i][1]);
  }
  setVoxel(food[0], 0, food[1]);
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(SS, LOW);
    SPI.transfer(0x01 << i);
    for (uint8_t j = 0; j < 8; j++) {
      SPI.transfer(cube[i][j]);
    }
    digitalWrite(SS, HIGH);
  }
}

void setVoxel(uint8_t x, uint8_t y, uint8_t z) {
  cube[7 - y][7 - z] |= (0x01 << x);
}

void clearCube() {
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      cube[i][j] = 0;
    }
  }
}

void uploadScore(int score) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String payload = "score=" + String(score);
    int httpCode = http.POST(payload);
    http.end();
  }
}

