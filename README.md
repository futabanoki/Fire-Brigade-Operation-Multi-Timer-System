# 消防操法 4人同時計測システム

**Fire Brigade Operation Multi-Timer System**

消防操法訓練で、指揮者・一番員・二番員・三番員の動作時間を、4台のスマートフォンで同時に計測・記録・比較するためのシステムです。

Google Apps Script / Google Sheetsを計測・集計基盤として使用し、Raspberry Piと128×32 HUB75 LEDマトリクスによる大型表示にも対応しています。

> 本システムは消防操法の訓練・動作分析を目的として個人開発した計測支援システムです。公式競技の計時・審査装置を置き換えるものではありません。

## 主な機能

* 指揮者・一番員・二番員・三番員を4台のスマートフォンで同時計測
* 指揮者のSTART操作で全端末の基準時刻を共有
* STARTボタンを押した瞬間を開始時刻として記録
* 各員のLAPボタンをタップした瞬間を記録
* 実際の消防操法の動作名をLAPボタンに表示
* 指揮者の最終LAPで自動STOP
* 計測結果をGoogle Sheetsへ自動保存
* チーム別・役割別の結果比較
* Androidスマートフォン対応
* Raspberry Piによる128×32 LED大型表示
* systemdによるLEDシステム自動起動

## 計測方式

通信遅延を計測値に含めないことを重視しています。

```text
指揮者がSTARTをタップ
        │
        ├─ スマートフォンで押下時刻を確定
        │
        └─ Google Apps Scriptへ送信
                     │
                     ▼
            全端末で開始時刻を共有
```

STOPについても同様に、指揮者の最終LAPを押した瞬間の時刻を基準にします。

そのため、Google Apps Scriptとの通信に時間がかかっても、通信時間そのものを競技タイムに加算しません。

## システム構成

```text
                    Google Apps Script
                           │
                    Google Sheets
                           │
          ┌────────────────┼────────────────┐
          │                │                │
       指揮者            一番員           二番員
       スマホ            スマホ           スマホ
                           │
                        三番員
                        スマホ

                           │
                           ▼
                     Raspberry Pi
                           │
                    timer_api.sh
                           │
                    timer.json
                           │
                    timer_led6
                           │
                           ▼
                  128×32 LED Matrix
```

## ディレクトリ

```text
gas/
    Code.gs
    index.html

raspberry-pi/
    timer_led6.cpp
    timer_api.sh
    systemd/

docs/
    setup.md
    operation.md
    architecture.md

images/
```

## スマートフォン

同一のWebアプリをURLパラメータによって役割分けします。

```text
...?role=commander
...?role=no1
...?role=no2
...?role=no3
```

### 指揮者

* START
* 動作別LAP
* 最終LAPによる自動STOP
* 集計結果表示

### 一番員・二番員・三番員

* 動作別LAP
* 指揮者のSTART/STOPとの同期
* 自動保存

各LAPは画面の表示更新時刻ではなく、ボタンをタップした瞬間の `Date.now()` を基準に記録します。

## Google Sheets

主に次のシートを使用します。

```text
Competitors
Results
Control
```

`Results` には概ね次の情報を保存します。

```text
日時
チーム
役割
総合タイム
lap1
lap2
lap3
lap4
lap5
lap6
```

## Raspberry Pi LED表示

構成例：

* Raspberry Pi Zero W
* HUB75 LED Matrix 64×32 × 2枚
* 合計128×32 pixels
* rpi-rgb-led-matrix
* IPA BDF Font

表示例：

```text
待機

計測中

14.9秒
```

実行例：

```bash
sudo ./timer_led6 \
  --led-rows=32 \
  --led-cols=64 \
  --led-chain=2 \
  --led-slowdown-gpio=2 \
  --led-brightness=20 \
  --led-pwm-bits=4
```

## 自動起動

systemdサービスを使用します。

```text
timer-api.service
timer-led.service
```

起動の流れ：

```text
Raspberry Pi
    ↓
Network
    ↓
timer-api.service
    ↓
/tmp/timer.json
    ↓
timer-led.service
    ↓
LED Matrix
```

## 使用技術

* Google Apps Script
* Google Sheets
* HTML / CSS / JavaScript
* C++
* Bash
* Raspberry Pi OS
* systemd
* curl
* HUB75 LED Matrix
* rpi-rgb-led-matrix

## セキュリティ

公開リポジトリには次の情報を含めないでください。

* 実際のGoogle Apps ScriptデプロイURL
* Google Sheets ID
* APIキー
* Wi-Fi SSID / Password
* 個人メールアドレス
* SSH秘密鍵
* Google認証情報
* 個人を特定できる計測データ

サンプル値または環境変数へ置き換えてください。

## License

このリポジトリ独自のソースコードはMIT Licenseで公開します。

外部ライブラリ、フォントその他の第三者著作物については、それぞれのライセンスが適用されます。

## Disclaimer

本システムは消防操法の訓練および動作分析を目的とした非公式の計測支援システムです。

公式競技で使用する場合の計時・審査・判定については、各大会および各消防機関等の実施要領・操作要領・審査基準に従ってください。
