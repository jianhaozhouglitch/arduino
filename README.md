# Arduino Uno v3 LCD 文字動畫互動系統 

> **適用課程**：ATB507 Interactive Arts and Technology (互動藝術與科技)  
> **硬體清單**：Arduino Uno R3 (v3)、1602 LCD 顯示螢幕 (支援 I2C 或平行模式)、Digital Push Button v3 數位按鈕模組

---

## 目錄
1. [專案簡介與特色](#1-專案簡介與特色)
2. [硬體接線圖解 (Wiring Diagram)](#2-硬體接線圖解-wiring-diagram)
3. [6 大文字動畫模式解析](#3-6-大文字動畫模式解析)
4. [按鈕互動操作機制](#4-按鈕互動操作機制)
5. [Arduino IDE 上傳與函式庫安裝步驟](#5-arduino-ide-上傳與函式庫安裝步驟)
6. [常見問題排查 (Troubleshooting)](#6-常見問題排查-troubleshooting)
7. [網頁版即時模擬器 (Web Simulator)](#7-網頁版即時模擬器-web-simulator)

---

## 1. 專案簡介與特色

本專案專為互動科技與創意編程設計，將生硬的字符型 LCD1602 螢幕轉化為生動的藝術載體：
- ⚡ **零延遲非阻塞架構**：採用 `millis()` 狀態機定時調度，徹底棄用會阻斷 CPU 的 `delay()`，按鈕隨按隨切換，反應時間小於 40ms。
- 🎨 **8 組自訂 5x8 像素圖元**：利用 `lcd.createChar()` 點陣編程，實作出「跳動心臟、音符、4 階動態音頻等化器柱狀圖、實心終端方塊」。
- 🔀 **高相容性**：同一套程式碼透過頂部的 `#define USE_I2C_LCD` 即可無縫支援 **4 線 I2C 模組** 或 **傳統 16 腳位平行 LCD**。
- 🔘 **多維度按鈕手勢**：支援「短按切換模式」與「長按切換播放速度 (1X / 2X)」。

---

## 2. 硬體接線圖解 (Wiring Diagram)

### 【接線方案 A】最推薦：I2C 介面 LCD1602 (只需 4 條線)
若你的 LCD 背面附帶黑色/藍色 I2C 轉接模組 (PCF8574)：

| 模組名稱 | 模組引腳 | Arduino Uno 引腳 | 說明 |
| :--- | :--- | :--- | :--- |
| **I2C LCD1602** | **GND** | **GND** | 接地共地 |
| | **VCC** | **5V** | 5V 供電 |
| | **SDA** | **A4** (或板端專用 SDA) | I2C 資料線 |
| | **SCL** | **A5** (或板端專用 SCL) | I2C 時鐘線 |
| **Digital Push Button v3** | **D / OUT (綠線/黃線)** | **Digital 2** | 按鈕信號輸入 (中斷腳位) |
| | **+ / VCC (紅線)** | **5V** | 模組供電 |
| | **- / GND (黑線)** | **GND** | 模組接地 |

```
       +---------------------------------------------+
       |               Arduino Uno R3                |
       |                                             |
       |    5V    GND    A4(SDA)  A5(SCL)   Pin 2    |
       +----+------+--------+-------+---------+------+
            |      |        |       |         |
            |      |        |       |         |
      +-----+------+--------+-------+--+      |
      | VCC   GND    SDA      SCL      |      |
      |       I2C LCD 1602 模組        |      |
      +--------------------------------+      |
                                              |
      +---------------------------------------+
      |  D(Signal)  + (5V)   - (GND)
      |  DFRobot Digital Push Button v3
      +---------------------------------------+
```

---

### 【接線方案 B】傳統 16 腳位平行 LCD1602 (無 I2C 轉接板)
若你的 LCD 為 16 根排針的原生平行介面：
1. 請至 `lcd_text_animation.ino` 第 23 行將 `#define USE_I2C_LCD true` 改為 `false`。
2. 接線對照：
   - **LCD RS** ➔ Arduino **Pin 12**
   - **LCD Enable (E)** ➔ Arduino **Pin 11**
   - **LCD D4** ➔ Arduino **Pin 5**
   - **LCD D5** ➔ Arduino **Pin 4**
   - **LCD D6** ➔ Arduino **Pin 3**
   - **LCD D7** ➔ Arduino **Pin 2** *(此時請將按鈕改接至 Pin 7，並修改程式中的 `BUTTON_PIN 7`)*
   - **LCD VSS, RW, K** ➔ Arduino **GND**
   - **LCD VDD, A (背光+)** ➔ Arduino **5V** (A 建議串接 220Ω 電阻)
   - **LCD V0 (對比度)** ➔ 10K 可變電阻中間腳

---

## 3. 6 大文字動畫模式解析

| 模式 | 名稱 | 視覺動畫效果描述 | 藝術意象 / 設計理念 |
| :---: | :--- | :--- | :--- |
| **Mode 0** | **開機儀式與引導**<br>`WELCOME` | 第一行常駐 `ATB507 ARDUINO`，第二行跳動大小愛心像素，並閃爍 `[PRESS BUTTON]` 與 `>>>> PUSH <<<<` 引導觀眾互動。 | 互動裝置的待機狀態 (Attract Loop)。 |
| **Mode 1** | **黑客矩陣解碼**<br>`DECRYPT` | 畫面初態充斥著高頻變化的十六進位亂碼字符，隨後如密碼破解般從左至右逐字「鎖定」，最終顯現完整的科技主題文字。 | 賽博朋克 / 數位訊號解鎖與數據流。 |
| **Mode 2** | **雙向奔流走馬燈**<br>`MARQUEE` | **雙行對向流動**：第一行長文字向左平滑捲動，第二行長文字向右奔流，產生雙向對立流體視覺。 | 訊息瀑布、城市霓虹滾動資訊幕。 |
| **Mode 3** | **打字機敘事動畫**<br>`TYPEWRITER` | 字符宛如老式打字機般逐字輸出，伴隨閃爍的終端游標 `_`，在句末停頓閱讀後自動進入下一段劇情對白。 | 敘事性互動 (Interactive Storytelling)。 |
| **Mode 4** | **像素心跳與音波頻譜**<br>`PIXEL BEAT` | 上方透過自訂點陣呈現呼吸式心跳動畫與音符；下方 16 欄即時動態計算三角函數波動，模擬高低起伏的音樂等化器頻譜。 | 生物信號與數位頻譜的跨界共振。 |
| **Mode 5** | **彈跳碰撞文字**<br>`BOUNCE` | 經典 DVD 屏保彈跳效果：標籤 `<ATB507>` 在 16 字符寬度內持續平滑位移，觸碰邊緣時彈跳反折並上下換行！ | 幾何物理運動與古典數位記憶。 |

---

## 4. 按鈕互動操作機制

程式內建雙重手勢判定：
1. 👆 **短按按鈕（< 1 秒）**：
   - 立即觸發切換動畫模式，畫面會短暫顯示過渡標籤 `>> SELECT MODE <<`，並依序循環：
     `Mode 0 ➔ Mode 1 ➔ Mode 2 ➔ Mode 3 ➔ Mode 4 ➔ Mode 5 ➔ Mode 0 ...`
2. ⏱️ **長按按鈕（按住 ≥ 1 秒）**：
   - 觸發 **加速模式 (Speed Toggle)**！
   - 螢幕提示 `MODE: 2X FAST >>` 或 `MODE: 1X NORMAL`，動畫將以 2 倍速度急速運算播放。

---

## 5. Arduino IDE 上傳與函式庫安裝步驟

1. **安裝 I2C LCD 函式庫** (若使用 I2C 模式)：
   - 打開 Arduino IDE。
   - 點擊左側圖示或選單中的 **Sketch (草稿碼) ➔ Include Library (匯入程式庫) ➔ Manage Libraries... (管理程式庫)**。
   - 在搜尋框輸入 `LiquidCrystal I2C`。
   - 找到由 **Frank de Brabander** 或 **Marco Schwartz** 維護的 `LiquidCrystal I2C` 程式庫，點擊 **Install (安裝)**。
2. **開啟專案檔案**：
   - 雙擊開啟 `lcd_text_animation/lcd_text_animation.ino`。
3. **選擇開發板與連接埠**：
   - **Tools ➔ Board ➔ Arduino AVR Boards ➔ Arduino Uno**。
   - **Tools ➔ Port ➔ 選擇 Uno 所在的 COM 連接埠** (例如 COM3 / COM4)。
4. **點擊「上傳 (Upload)」按鈕 (箭頭圖示)**：
   - 編譯並寫入完成後，LCD 即刻亮起並播放動畫！

---

## 6. 常見問題排查 (Troubleshooting)

1. **問：LCD 螢幕背光亮著，但只有整排白色方塊或一片空白，沒有字？**
   - **原因**：對比度未調節。
   - **解決方法**：使用十字螺絲起子，旋轉 I2C 背板上的**藍色電位器 (對比度調節鈕)**，順時針或逆時針慢慢轉動，直到文字清晰為止。
2. **問：上傳成功但 LCD 完全沒有任何反應？**
   - **原因**：I2C 晶片位址不匹配。市面上常見位址為 `0x27` 或 `0x3F`。
   - **解決方法**：至程式碼第 33 行，將 `LiquidCrystal_I2C lcd(0x27, 16, 2);` 改為 `LiquidCrystal_I2C lcd(0x3F, 16, 2);` 再重新上傳。
3. **問：按鈕按下後沒有反應或反應顛倒？**
   - **原因**：按鈕模組電平邏輯差異。DFRobot v3 預設按下為高電平 `HIGH`。
   - **解決方法**：若你的按鈕是按下輸出低電平，請至程式碼第 28 行將 `#define BUTTON_PRESSED_STATE HIGH` 改為 `LOW`。

---

## 7. 網頁版即時模擬器 (Web Simulator)

本專案附帶完整的網頁版 LCD 互動模擬器，無需接上實體硬體即可在瀏覽器中預覽所有文字動畫！
- **檔案路徑**：`preview/index.html`
- **使用方式**：直接使用 Chrome / Edge 瀏覽器打開 `preview/index.html`，點擊畫面中的紅色大按鈕即可實時體驗 6 種動畫與長按加速功能！
