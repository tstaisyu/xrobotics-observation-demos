# log-004 adjustable rule demo

## 目的
M5Stack Gray の IMU で衝撃を検知し、検知回数が指定回数に達すると保護モードへ移行する観測デモ。PC UI 側から保護モード移行までの検知回数ルールを変更し、Serial / WebSocket 経由で M5 側の判定条件に反映する想定。

## 構成
- `firmware/src/main.cpp`
  - log-003 をベースにした M5Stack Gray 側のファームウェア
  - 今後、PC UI から変更した検知回数ルールを反映する拡張を行う
- `app/server.js`
  - Node.js + Express + ws + serialport のシンプル構成
  - 今後、UI から変更したルールを M5 側へ転送する想定
- `app/public/*`
  - log-003 をベースにしたブラウザ UI
  - 今後、保護モード移行までの検知回数を変更する UI を追加する想定

## 起動方法
```bash
cd demos/log-004-adjustable-rule/app
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
本ディレクトリは `xrobotics.jp` の log-004 から参照する観測デモ実装を想定している。
