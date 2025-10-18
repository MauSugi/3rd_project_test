#include <WiFi.h>
#include "time.h" // NTP時刻同期のための標準ライブラリ

// ★★★ 接続情報とターゲット時刻の設定 ★★★
const char* ssid = "GlocalNet_0XEONY";         // 接続に成功したSSID
const char* password = "65666475"; // 接続に成功したパスワード

// NTP設定
const char* ntpServer = "ntp.nict.jp"; // 日本国内の信頼できるNTPサーバー
const long gmtOffset_sec = 9 * 3600;   // JST (日本標準時) はGMT+9時間
const int daylightOffset_sec = 0;      // 日本はサマータイム (DST) なし

// ターゲット時刻 (この時間になったらアクションを実行)
const int targetHour = 13;   // 例: 午後2時 (14時)
const int targetMinute = 40; // 例: 30分

// アクションを実行するピン (前回使用したGPIO 2)
const int actionPin = 2;

// アクション制御フラグ
// 誤動作防止のため、1日に一度だけアクションを実行したかを記録
bool actionDoneToday = false;

// ----------------------------------------------------

// NTP時刻設定関数
void initTime() {
  // タイムゾーンとNTPサーバーを設定
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.print("Waiting for NTP time synchronization");
  struct tm timeinfo;
  
  // 時刻同期が完了するまで待機
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nTime synchronized successfully.");

  // 現在時刻の表示
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%F %T", &timeinfo);
  Serial.print("Current Time: ");
  Serial.println(time_str);
}

void setup() {
  Serial.begin(115200);
  pinMode(actionPin, OUTPUT);
  digitalWrite(actionPin, LOW); // 初期状態はOFF

  // Wi-Fi接続
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("#");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // NTP時刻同期を実行
  initTime();
}

void loop() {
  struct tm timeinfo;
  
  // 現在時刻を取得
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time. Re-syncing...");
    initTime(); // 時刻取得に失敗したら再同期を試みる
    return;
  }

  // 現在時刻の時と分を取得
  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;

  // 1. ターゲット時刻チェック
  if (currentHour == targetHour && currentMinute == targetMinute) {
    
    // 2. 当日実行済みフラグチェック
    if (!actionDoneToday) {
      // ------------------------------------------------
      // ★★★ ここに実行したいアクションを記述 ★★★
      // ------------------------------------------------
      Serial.print(">>> Target Time Reached: ");
      Serial.printf("%02d:%02d\n", currentHour, currentMinute);
      
      digitalWrite(actionPin, HIGH); // GPIO 2 のLEDをONにする
      
      // ------------------------------------------------
      
      actionDoneToday = true; // アクションを実行済みに設定
    }
  }

  // 3. フラグリセットチェック (次の日になったらフラグをリセットする)
  // アクションが実行済みで、かつターゲット時刻を過ぎてから時間が変わったとき (例: ターゲットが14:30、今は15:00)
  if (actionDoneToday && currentHour != targetHour) {
    // 日付が変わることを検出するより、ターゲット時刻を過ぎたらリセットする方が簡単
    // より正確には、深夜0時を過ぎた最初のチェックでリセットします
    if (currentHour == 0 && currentMinute == 1) { // 0時1分にリセット
        actionDoneToday = false;
        digitalWrite(actionPin, LOW); // アクションをリセット（LEDをOFF）
        Serial.println("Action flag reset. Ready for the next day.");
    }
  }
  
  // ログ出力と遅延 (デバッグ用)
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
  Serial.print("Current: ");
  Serial.println(time_str);

  delay(5000); // 5秒ごとにチェック
}