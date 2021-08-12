
/*
    Description: Use WEIGHT Unit to read the analog value returned by the pressure sensor, 
    convert it into intuitive weight data and send it to M5Core,Press ButtonA to calibrate
*/

#include "HX711.h"
#include <M5StickC.h>

// load cell (beam): TAL220B
//   定格容量 5kg=5000.0f g
//   定格出力  1.0+-0.1mV/V = 0.001f [V/V]
//    (= 供給電圧1Vに対し、定格容量の荷重をかけた場合に出てくる出力電圧。実際にはこれに供給電圧を掛ける)
//   ロードセル供給電圧： 4.2987f Vdd w/ M5StickC via HX711 Weight Unit = 2^24bit ADC
// （4.2987Vは、Unitに対するM5StickC(あるいはArduino)からの5V供給に対しHX711チップの内部分圧抵抗を考慮して
// 　ロードセルに実際に供給される電圧である）→ ADC 1bitあたり 4.2987/(2^24) [V]
//   (処理の流れ)
//   bitデータの取得して平均値を計算（bit）＋オフセットbitデータ(offset)を差し引き
//   → 電圧変換 (bit-offset) * (4.2987/(2^24))
//   →グラム変換＝電圧[V]／スケール[V/g] 
//   ※ スケールは、HX711デフォルトの電圧アンプのゲインファクタが128なので、以下のように計算する。
//   scale [V/g] = 定格出力 0.001V/V * 4.2987V /定格容量5000.0g * ゲインファクタ128 

// HX711 circuit wiring on M5StickC
const int LOADCELL_DOUT_PIN = 33;
const int LOADCELL_SCK_PIN = 32;

HX711 scale;
const long LOADCELL_OFFSET = 50682624; // 仮の値
const long LOADCELL_DIVIDER = 5895655; // 仮の値　

void setup() {
  M5.begin();
  
  Serial.begin(115200);
  Serial.println("Initializing the scale");

  M5.Lcd.setRotation(1);
  M5.Lcd.setCursor(3,3,3);  // LCD = 80x160 pixels.
  M5.Lcd.print("Initializing Scale...");
  
  // Initialize library with data output pin, clock input pin and gain factor.
  // Channel selection is made by passing the appropriate gain:
  // - With a gain factor of 64 or 128, channel A is selected
  // - With a gain factor of 32, channel B is selected
  // By omitting the gain factor parameter, the library
  // default "128" (Channel A) is used here.
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(LOADCELL_DIVIDER); // 仮の値でscaleをセット
  scale.set_offset(LOADCELL_OFFSET); // 仮の値でoffsetをセット
  
  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());             // print a raw bit data (24bit) reading from the ADC
  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));   // print the average of 20 readings from the ADC
  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));       // print the average of 5 readings from the ADC minus the tare weight (not set yet)
  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);    // print the average of 5 readings from the ADC minus tare weight (not set) divided
                                            // by the SCALE parameter (not set yet)

  // 以下のset_scale()中の数値は、基準となる（既知の）重りを載せて、表示値を見ながら調整する必要がある。
  // ロードセルの種類・個体差、気温条件、設置条件などで異なる可能性があるので、可能な限り実験毎に確認することが望ましい。
  //（Btn.Aを押して0resetする部分にもあり）                                        
  scale.set_scale(205.0f);                   // this value is obtained by calibrating the scale with known weights; see the README for details (要調整)
  scale.tare();                             // reset the scale to 0
  
  Serial.println("After setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());             // print a raw reading from the ADC
  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));   // print the average of 20 readings from the ADC
  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));       // print the average of 5 readings from the ADC minus the tare weight, set with tare()
  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);    // print the average of 5 readings from the ADC minus tare weight, divided
                                            // by the SCALE parameter set with set_scale 

  Serial.println("Readings start:");
  M5.Lcd.setCursor(3,5,2);
  M5.Lcd.print("Click btn.A to 0 reset.");
}

void loop() {
  
  if (M5.BtnA.wasPressed()) {
    scale.set_offset(LOADCELL_OFFSET + scale.read());
    scale.set_scale(205.0f); // 要調整
    scale.tare();
  }
  M5.update();
  
  int weight = scale.get_units(10); //10回計測の平均値 [g] 
  
  M5.Lcd.setCursor(40,30,4);
  M5.Lcd.fillRect(0, 30, 160, 30, TFT_BLACK);
  M5.Lcd.printf("%1d g", weight);
  M5.Lcd.fillRect(0, 80, 160, 80, TFT_BLACK);
  M5.Lcd.fillRect(0, 80, weight*0.16, 80, TFT_BLUE);
  
  Serial.print("average:\t");
  Serial.println(weight, 1);
  
}
