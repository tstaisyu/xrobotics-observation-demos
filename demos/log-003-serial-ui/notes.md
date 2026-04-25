# log-003-serial-ui

## 目的
M5Stack Gray の IMU から急な動きの変化を検知し、状態変化イベントを Serial で JSON Lines 出力する。PC 側で受信し、WebSocket 経由でブラウザにリアルタイム表示する観測デモ。

## 構成
- `firmware/log_003_serial_ui.ino`
  - M5 IMU の加速度 `ax, ay, az` を取得
  - 前回値との差分ベクトルから `delta` を計算
  - `delta >= threshold` で `MOTION_SPIKE`、それ以外は `STABLE`
  - 状態が変化した瞬間だけ Serial に JSON Lines を出力
- `app/server.js`
  - Express で `public/` を配信
  - SerialPort + ReadlineParser で1行ずつ受信
  - JSON parse できた行のみ WebSocket で broadcast
- `app/public/*`
  - 現在状態、delta、最終受信時刻、直近10件のイベントを表示

## 起動方法
```bash
cd demos/log-003-serial-ui/app
npm install
SERIAL_PORT=/dev/ttyUSB0 npm start
```

ブラウザで `http://localhost:3000` を開く。

## SERIAL_PORT の指定方法
- デフォルト: `/dev/ttyUSB0`
- 別ポートを使う場合:

```bash
SERIAL_PORT=/dev/ttyUSB1 npm start
```

## 参照想定
本ディレクトリは `xrobotics.jp` の log-003 から参照する観測デモ実装を想定している。
