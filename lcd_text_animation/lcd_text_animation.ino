/*
 ==============================================================================
  專案名稱: Arduino Uno v3 LCD 文字動畫互動系統 (Text Animation System)
  課程適用: ATB507 Interactive Arts and Technology / 互動藝術與科技
  硬體配備:
    1. Arduino Uno R3 (v3)
    2. LCD 1602 顯示螢幕 (支援 I2C 四線模式 或 原生平行 16 腳位模式)
    3. Digital Push Button v3 數位按鈕模組 (DFRobot Gravity / 標準按鈕)
 
  功能亮點:
    - 6種精美文字動畫模式 (開機儀式、黑客解碼、雙向走馬燈、打字機敘事、像素心跳音波、彈跳文字)
    - 採用非阻塞 millis() 狀態機架構，按鈕隨按隨響，毫無延遲卡頓
    - 支援「短按切換動畫模式」與「長按切換播放速度 (正常/倍速)」
    - 內建 8 組自訂 5x8 點陣像素圖案 (大小愛心跳動、4段式等化器音波、音符、游標)
    - 透過宏開關 #define USE_I2C_LCD 自由切換 I2C 與 平行 LCD 接口
 ==============================================================================
*/

// ========================【硬體規格與模式配置】========================
// 若使用 I2C 轉接板 (4條線: GND, VCC, SDA, SCL)，請設為 true
// 若使用標準 16 腳位 LCD1602 (無 I2C 模組)，請設為 false
#define USE_I2C_LCD true

// 按鈕接腳 (Digital Push Button v3 的信號腳 D/OUT)
// 推薦接到數位腳位 2 (Arduino Uno 的中斷腳位)
#define BUTTON_PIN 2

// 按鈕按下時的電平 (DFRobot Digital Push Button v3 預設按下為 HIGH)
// 若使用上拉電阻或按下為低電平的按鈕，可改為 LOW
#define BUTTON_PRESSED_STATE HIGH

// ========================【引入程式庫與初始化】========================
#if USE_I2C_LCD
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  // 常見 I2C 位址為 0x27 或 0x3F，若螢幕無顯示請嘗試改為 0x3F
  LiquidCrystal_I2C lcd(0x27, 16, 2);
#else
  #include <LiquidCrystal.h>
  // 標準平行腳位接法: RS, Enable, D4, D5, D6, D7
  LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#endif

// ========================【自訂 5x8 點陣字元圖形】========================
// LCD1602 最多支援 8 個自訂圖元 (編號 0 ~ 7)
const byte customCharData[8][8] = {
  // 0: 大愛心 (Big Heart)
  {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000
  },
  // 1: 小愛心 (Small Heart)
  {
    0b00000,
    0b00000,
    0b01010,
    0b01110,
    0b00100,
    0b00000,
    0b00000,
    0b00000
  },
  // 2: 音符 (Music Note)
  {
    0b00010,
    0b00011,
    0b00010,
    0b00010,
    0b01110,
    0b11110,
    0b01100,
    0b00000
  },
  // 3: 等化器 1 格高 (Bar 1)
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11111
  },
  // 4: 等化器 2 格高 (Bar 2)
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11111,
    0b11111,
    0b11111
  },
  // 5: 等化器 3 格高 (Bar 3)
  {
    0b00000,
    0b00000,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
  },
  // 6: 等化器 全滿 (Bar 4)
  {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
  },
  // 7: 終端實心方塊 (Solid Block Cursor)
  {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
  }
};

// ========================【系統狀態與動畫變數】========================
const int TOTAL_MODES = 6;
int currentMode = 0;              // 當前動畫模式 (0 ~ 5)
bool isFastSpeed = false;          // 長按切換為加速模式

// 按鈕防抖與長按檢測變數
int lastButtonState = !BUTTON_PRESSED_STATE;
unsigned long buttonPressStartTime = 0;
bool longPressTriggered = false;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 40;     // 40ms 軟體防抖
const unsigned long LONG_PRESS_TIME = 1000;  // 1.0秒判定為長按

// 動畫計時變數 (完全採用非阻塞 millis() 調度)
unsigned long lastAnimUpdate = 0;
int animStep = 0;
int subStep = 0;

// 動畫文字字串常數
const char* const WELCOME_TITLE = "ATB507 ARDUINO";
const char* const WELCOME_SUB   = "[PRESS BUTTON]";

const char* const DECRYPT_LINE1 = "INTERACTIVE ART";
const char* const DECRYPT_LINE2 = "CREATIVE CODE OK";

const char* const MARQUEE_LINE1 = "ATB507 INTERACTIVE ARTS AND TECHNOLOGY * ARDUINO UNO V3 * ";
const char* const MARQUEE_LINE2 = "DIGITAL PUSH BUTTON V3 * 1602 LCD ANIMATION SHOWCASE * ";

const char* const TYPEWRITER_TEXT1 = "Hello, ATB507!";
const char* const TYPEWRITER_TEXT2 = "Code is Art :)";

// ========================【函式原型宣告】========================
void handleButton();
void switchMode(int newMode);
void displayModeBanner(const char* title);
void updateAnimation();

void animMode0_Welcome();
void animMode1_Decrypt();
void animMode2_Marquee();
void animMode3_Typewriter();
void animMode4_PixelVisualizer();
void animMode5_BounceText();

// ========================【初始化 setup()】========================
void setup() {
  Serial.begin(9600);
  Serial.println(F("Arduino LCD Text Animation System Initialized!"));

  // 按鈕腳位設定
  pinMode(BUTTON_PIN, INPUT);

  // 初始化 LCD 螢幕
  #if USE_I2C_LCD
    lcd.init();
    lcd.backlight();
  #else
    lcd.begin(16, 2);
  #endif

  // 註冊 8 組自訂字元
  for (int i = 0; i < 8; i++) {
    lcd.createChar(i, (uint8_t*)customCharData[i]);
  }

  lcd.clear();
  switchMode(0);
}

// ========================【主循環 loop()】========================
void loop() {
  // 1. 處理按鈕輸入 (高頻響應，毫秒級防抖)
  handleButton();

  // 2. 驅動當前文字動畫 (非阻塞更新)
  updateAnimation();
}

// ========================【按鈕檢測與狀態切換】========================
void handleButton() {
  int reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  // 檢測電平變化並執行防抖
  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
    // 按鍵剛剛被按下
    if (reading == BUTTON_PRESSED_STATE && buttonPressStartTime == 0) {
      buttonPressStartTime = now;
      longPressTriggered = false;
    }
    // 按鍵持續按住中：檢測是否觸發長按
    else if (reading == BUTTON_PRESSED_STATE && buttonPressStartTime > 0) {
      if (!longPressTriggered && (now - buttonPressStartTime >= LONG_PRESS_TIME)) {
        longPressTriggered = true;
        // 長按事件：切換動畫速度 (正常 / 加速)
        isFastSpeed = !isFastSpeed;
        
        // 螢幕短暫提示速度切換
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F(">> SPEED TOGGLE"));
        lcd.setCursor(0, 1);
        if (isFastSpeed) {
          lcd.print(F("MODE: 2X FAST >>"));
        } else {
          lcd.print(F("MODE: 1X NORMAL"));
        }
        delay(600); // 短暫提示停留
        switchMode(currentMode); // 重新載入當前動畫
      }
    }
    // 按鍵放開
    else if (reading != BUTTON_PRESSED_STATE && buttonPressStartTime > 0) {
      // 若未觸發長按，則視為「短按」：切換至下一個動畫模式
      if (!longPressTriggered) {
        int nextMode = (currentMode + 1) % TOTAL_MODES;
        switchMode(nextMode);
      }
      buttonPressStartTime = 0;
    }
  }

  lastButtonState = reading;
}

// ========================【切換動畫模式】========================
void switchMode(int newMode) {
  currentMode = newMode;
  animStep = 0;
  subStep = 0;
  lastAnimUpdate = millis();
  lcd.clear();

  Serial.print(F("Switched to Mode: "));
  Serial.println(currentMode);

  // 顯示模式過渡標籤
  switch (currentMode) {
    case 0: displayModeBanner("MODE 0: WELCOME"); break;
    case 1: displayModeBanner("MODE 1: DECRYPT"); break;
    case 2: displayModeBanner("MODE 2: MARQUEE"); break;
    case 3: displayModeBanner("MODE 3: TYPEWRITER"); break;
    case 4: displayModeBanner("MODE 4: PIXEL BEAT"); break;
    case 5: displayModeBanner("MODE 5: BOUNCE"); break;
  }
}

// 模式切換時的短暫過渡效果
void displayModeBanner(const char* title) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F(">> SELECT MODE <<"));
  lcd.setCursor(0, 1);
  lcd.print(title);
  delay(350); // 短暫提示停留
  lcd.clear();
}

// ========================【動畫總調度器】========================
void updateAnimation() {
  switch (currentMode) {
    case 0: animMode0_Welcome(); break;
    case 1: animMode1_Decrypt(); break;
    case 2: animMode2_Marquee(); break;
    case 3: animMode3_Typewriter(); break;
    case 4: animMode4_PixelVisualizer(); break;
    case 5: animMode5_BounceText(); break;
  }
}

// ==============================================================================
// 模式 0: 開機儀式與互動引導 (Welcome & Interaction Guide)
// 第一行呼吸置中顯示，第二行閃爍指示箭頭提示點擊按鈕
// ==============================================================================
void animMode0_Welcome() {
  unsigned long interval = isFastSpeed ? 250 : 500;
  if (millis() - lastAnimUpdate < interval) return;
  lastAnimUpdate = millis();

  // 第一行：常駐置中標題
  lcd.setCursor(1, 0);
  lcd.print(WELCOME_TITLE);

  // 第二行：按鈕引導與跳動圖示
  lcd.setCursor(0, 1);
  if (animStep % 2 == 0) {
    lcd.write(byte(0)); // 大愛心圖示
    lcd.print(F(" PRESS BUTTON "));
    lcd.write(byte(0));
  } else {
    lcd.write(byte(1)); // 小愛心圖示
    lcd.print(F(" >>>> PUSH <<<< "));
    lcd.write(byte(1));
  }
  animStep++;
}

// ==============================================================================
// 模式 1: 黑客矩陣密碼解碼動畫 (Hacker Cyber Decrypt Animation)
// 字符初始時隨機亂碼閃爍，隨後從左到右一個個「破解鎖定」成真實文字
// ==============================================================================
void animMode1_Decrypt() {
  unsigned long interval = isFastSpeed ? 40 : 80;
  if (millis() - lastAnimUpdate < interval) return;
  lastAnimUpdate = millis();

  int len1 = strlen(DECRYPT_LINE1);
  int len2 = strlen(DECRYPT_LINE2);

  // 隨機字符集供解碼過渡使用
  const char randomCharset[] = "0123456789ABCDEF$#@%&*!~";
  int charsetSize = sizeof(randomCharset) - 1;

  // 行 1 處理
  lcd.setCursor(0, 0);
  for (int i = 0; i < 16; i++) {
    if (i < len1) {
      if (i < animStep) {
        lcd.print(DECRYPT_LINE1[i]); // 已破解完成的字元
      } else if (i == animStep) {
        lcd.write(byte(7));          // 正在破解的光標方塊
      } else {
        lcd.print(randomCharset[random(0, charsetSize)]); // 雜湊亂碼
      }
    } else {
      lcd.print(' ');
    }
  }

  // 行 2 處理 (在行 1 破解一定長度後開始破解行 2)
  lcd.setCursor(0, 1);
  for (int i = 0; i < 16; i++) {
    if (i < len2) {
      if (animStep >= len1 && i < (animStep - len1)) {
        lcd.print(DECRYPT_LINE2[i]); // 已破解完成的字元
      } else if (animStep >= len1 && i == (animStep - len1)) {
        lcd.write(byte(7));          // 解碼光標
      } else {
        lcd.print(randomCharset[random(0, charsetSize)]);
      }
    } else {
      lcd.print(' ');
    }
  }

  // 解碼步進推進
  subStep++;
  if (subStep % 2 == 0) {
    animStep++;
  }

  // 解碼全部完成後停留並重啟循環
  if (animStep > (len1 + len2 + 8)) {
    animStep = 0;
    subStep = 0;
    lcd.clear();
  }
}

// ==============================================================================
// 模式 2: 雙向平滑走馬燈 (Dual-Direction Smooth Marquee)
// 第一行向左奔流滾動，第二行向右奔流滾動，構成流體視覺
// ==============================================================================
void animMode2_Marquee() {
  unsigned long interval = isFastSpeed ? 160 : 300;
  if (millis() - lastAnimUpdate < interval) return;
  lastAnimUpdate = millis();

  int len1 = strlen(MARQUEE_LINE1);
  int len2 = strlen(MARQUEE_LINE2);

  // 第一行：向左滾動 (Left Scroll)
  lcd.setCursor(0, 0);
  for (int i = 0; i < 16; i++) {
    int charIndex = (animStep + i) % len1;
    lcd.print(MARQUEE_LINE1[charIndex]);
  }

  // 第二行：向右滾動 (Right Scroll)
  lcd.setCursor(0, 1);
  for (int i = 0; i < 16; i++) {
    int charIndex = (len2 * 50 - animStep + i) % len2;
    if (charIndex < 0) charIndex += len2;
    lcd.print(MARQUEE_LINE2[charIndex]);
  }

  animStep++;
}

// ==============================================================================
// 模式 3: 打字機敘事動畫 (Typewriter Storyteller Effect)
// 文字像打字機一樣逐字打印，帶閃爍下劃線游標，標點符號自動暫停
// ==============================================================================
void animMode3_Typewriter() {
  unsigned long interval = isFastSpeed ? 80 : 160;
  if (millis() - lastAnimUpdate < interval) return;
  lastAnimUpdate = millis();

  int len1 = strlen(TYPEWRITER_TEXT1);
  int len2 = strlen(TYPEWRITER_TEXT2);

  if (subStep == 0) {
    // 階段 0: 打印第一行
    if (animStep <= len1) {
      lcd.setCursor(0, 0);
      for (int i = 0; i < animStep; i++) {
        lcd.print(TYPEWRITER_TEXT1[i]);
      }
      if (animStep < len1) {
        lcd.print('_'); // 閃爍打字游標
      }
      animStep++;
    } else {
      subStep = 1; // 第一行完畢，準備打印第二行
      animStep = 0;
    }
  } else if (subStep == 1) {
    // 階段 1: 第一行保持，打印第二行
    lcd.setCursor(0, 0);
    lcd.print(TYPEWRITER_TEXT1);

    if (animStep <= len2) {
      lcd.setCursor(0, 1);
      for (int i = 0; i < animStep; i++) {
        lcd.print(TYPEWRITER_TEXT2[i]);
      }
      if (animStep < len2) {
        lcd.print('_');
      }
      animStep++;
    } else {
      subStep = 2; // 全部打印完畢，進入閱讀停留階段
      animStep = 0;
    }
  } else if (subStep == 2) {
    // 階段 2: 停頓閱讀，游標持續呼吸閃爍
    lcd.setCursor(0, 0);
    lcd.print(TYPEWRITER_TEXT1);
    lcd.setCursor(0, 1);
    lcd.print(TYPEWRITER_TEXT2);
    lcd.setCursor(len2, 1);
    if ((animStep % 2) == 0) {
      lcd.print('_');
    } else {
      lcd.print(' ');
    }

    animStep++;
    if (animStep > 12) { // 閱讀停留結束，清屏重新開始
      subStep = 0;
      animStep = 0;
      lcd.clear();
    }
  }
}

// ==============================================================================
// 模式 4: 像素微動畫與音波等化器 (Pixel Heartbeat & Audio Equalizer)
// 運用 8 組 5x8 自訂點陣：愛心跳動 + 4 段動態音頻頻譜柱狀圖
// ==============================================================================
void animMode4_PixelVisualizer() {
  unsigned long interval = isFastSpeed ? 100 : 200;
  if (millis() - lastAnimUpdate < interval) return;
  lastAnimUpdate = millis();

  // 第一行：愛心跳動與標題
  lcd.setCursor(0, 0);
  if (animStep % 2 == 0) {
    lcd.write(byte(0)); // 大心
    lcd.print(F(" TECH PULSE "));
    lcd.write(byte(0));
    lcd.print(F(" "));
    lcd.write(byte(2)); // 音符
  } else {
    lcd.write(byte(1)); // 小心
    lcd.print(F(" TECH PULSE "));
    lcd.write(byte(1));
    lcd.print(F(" "));
    lcd.print(F(" "));
  }

  // 第二行：16 欄動態音頻頻譜柱狀圖 (擬真等化器跳動)
  lcd.setCursor(0, 1);
  for (int col = 0; col < 16; col++) {
    // 結合三角函數與隨機噪聲產生平滑起伏波形
    int level = (int)(2.0 + 1.8 * sin((animStep * 0.4) + (col * 0.6))) + random(0, 2);
    if (level < 3) level = 3; // customChar 3對應 bar1
    if (level > 6) level = 6; // customChar 6對應 bar4
    lcd.write(byte(level));
  }

  animStep++;
}

// ==============================================================================
// 模式 5: 彈跳文字 (Ping-Pong Bouncing Text)
// 經典螢幕保護程式效果，文字塊在 16 格寬度內左右來回撞牆反彈
// ==============================================================================
void animMode5_BounceText() {
  unsigned long interval = isFastSpeed ? 90 : 180;
  if (millis() - lastAnimUpdate < interval) return;
  lastAnimUpdate = millis();

  static int posX = 0;
  static int dirX = 1;
  static int row = 0;
  const char bounceWord[] = "<ATB507>";
  int wordLen = strlen(bounceWord);
  int maxPos = 16 - wordLen;

  // 清除當前行
  lcd.setCursor(0, row);
  lcd.print(F("                "));

  // 更新座標
  posX += dirX;
  if (posX >= maxPos) {
    posX = maxPos;
    dirX = -1;
    row = 1 - row; // 撞牆後換到另一行！
  } else if (posX <= 0) {
    posX = 0;
    dirX = 1;
    row = 1 - row; // 撞牆後換行
  }

  // 繪製彈跳文字與邊界火花
  lcd.setCursor(posX, row);
  lcd.print(bounceWord);

  // 在另一行顯示狀態資訊
  int otherRow = 1 - row;
  lcd.setCursor(0, otherRow);
  lcd.print(F("BOUNCE ANIMATION"));
}
