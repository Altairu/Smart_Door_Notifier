// ライブラリのインクルード
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED ディスプレイの幅 (ピクセル単位)
#define SCREEN_HEIGHT 32  // OLED ディスプレイの高さ (ピクセル単位)

#define OLED_RESET 4  //リセットピン番号 (Arduino リセットピンを共有する場合は -1)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String message;  //シリアルモニターに書き込まれたメッセージを保存する
int text_size;   //ディスプレイの文字サイズを変更します


// Wi-Fi ネットワークの設定
char ssid[] = "wifi_name";  // Wi-Fi ネットワークの名前
char pass[] = "wifi_pass";  // Wi-Fi ネットワークのパスワード

// LINE Notify の設定
char server[] = "notify-api.line.me";     // サーバーのアドレス
char token[] = "YOUR_ACCESS_TOKEN_HERE";  // LINE Notify のトークン
int port = 443;                           // サーバーのポート番号

// HTTP クライアントの作成
WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, server, port);

// ドアセンサーのピン番号
const int DOOR_PIN = 6;

// ドアの状態
bool doorState = false;      // 開いているか閉まっているか
bool lastDoorState = false;  // 前回の状態

void setup() {
  // シリアル通信の開始
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // 128x32 のアドレス 0x3C
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  //ライブラリの初期化に失敗した場合は永久ループします
  }
  display.setTextColor(SSD1306_WHITE);  //テキストを白に設定する
  while (!Serial)
    ;
  // ドアセンサーのピンモード設定
  pinMode(DOOR_PIN, INPUT_PULLUP);
  // Wi-Fi 接続の開始
  Serial.print("Wi-Fi 接続中...");
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }
  Serial.println("完了");
  display.clearDisplay();
}

void loop() {
  // ドアセンサーの値を読み取る
  int doorValue = digitalRead(DOOR_PIN);

  // ドアの状態を判定する
  if (doorValue == LOW) {
    doorState = true;  // ドアが開いている
  } else {
    doorState = false;  // ドアが閉まっている
  }
  // ドアの状態が変わった場合
  if (doorState != lastDoorState) {
    // LINE Notify に送るメッセージを作成する
    String message;
    if (doorState == true) {
      message = "ドアが開きました";
      display.clearDisplay();
      display.println("open");
    } else {
      message = "ドアが閉まりました";
      display.clearDisplay();
      display.println("close");
    }

    // HTTP POST リクエストを送る
    client.beginRequest();
    client.post("/api/notify");
    client.sendHeader("Authorization", "Bearer " + String(token));
    client.sendHeader("Content-Type", "application/x-www-form-urlencoded");
    client.sendHeader("Content-Length", message.length() + 8);
    client.beginBody();
    client.print("message=");
    client.print(message);
    client.endRequest();

    // HTTP レスポンスを受け取る
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();

    // HTTP レスポンスを表示する
    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);

    // 前回の状態を更新する
    lastDoorState = doorState;
  }
  display.setCursor(0, 0);
  display.setTextSize(3);
  display.display();  //transfers the buffer to the display
  // 少し待つ
  delay(100);
}