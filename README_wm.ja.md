# Window Manager (wm) 使い方ガイド

## 概要

`wm` は CPL システムに含まれる端末多重化システムです。1980年代に開発された歴史的なツールで、現代の `tmux` や `screen` のような機能を提供します。

## 基本的な使い方

### 起動

```bash
# ビルドディレクトリから
cd build
./wm

# またはインストール済みの場合
wm
```

### ウィンドウ操作の基本コマンド

Window Manager は **制御文字** を使ってウィンドウを操作します。デフォルトのコマンドプレフィックスは `Ctrl-Z` です。

#### 主要なコマンド

| コマンド | 機能 |
|---------|------|
| `Ctrl-Z c` | 新しいウィンドウを作成 (create) |
| `Ctrl-Z n` | 次のウィンドウに移動 (next) |
| `Ctrl-Z p` | 前のウィンドウに移動 (previous) |
| `Ctrl-Z k` | 現在のウィンドウを削除 (kill) |
| `Ctrl-Z l` | ウィンドウ一覧を表示 (list) |
| `Ctrl-Z r` | 画面を再描画 (redraw) |
| `Ctrl-Z z` | `Ctrl-Z` 文字を送信 (リテラル) |
| `Ctrl-Z q` | Window Manager を終了 (quit) |

### 使用例

#### 1. 基本的なセッション

```bash
# wm を起動
./wm

# 新しいウィンドウを作成
# Ctrl-Z を押してから c を押す

# 別のウィンドウでコマンドを実行
# Ctrl-Z n で切り替え

# ウィンドウ一覧を確認
# Ctrl-Z l を押す
```

#### 2. 複数のプログラムを同時に実行

```bash
# Window 1: CPL インタプリタ
./cpl

# Ctrl-Z c で新しいウィンドウを作成
# Window 2: テキストエディタ
vi myprogram.cpl

# Ctrl-Z c でさらに新しいウィンドウ
# Window 3: システムモニタリング
top

# Ctrl-Z n / Ctrl-Z p でウィンドウ間を移動
```

## 技術的な詳細

### アーキテクチャ

`wm` は以下の仕組みで動作します：

1. **PTY (Pseudo-Terminal)** - 各ウィンドウに仮想端末を割り当て
2. **Process Management** - 各ウィンドウで独立したプロセスを実行
3. **Signal Handling** - `SIGCHLD` でプロセスの終了を検知
4. **Terminal Control** - termios で端末の raw モードを制御

### ソースコード構成

- `wm.c` - メインプログラム (PTY管理、シグナル処理)
- `winlib.c/h` - ウィンドウライブラリ関数
- `display.c/h` - 画面表示管理
- `term.c/h` - 端末制御 (termios)
- `termcap` - 端末機能データベース

### modernization のポイント

Version 4.0 では以下の modernization が行われています：

- **BSD PTY → POSIX openpty()** - `/dev/ptyXX` の手動イテレーションから標準APIへ
- **signal() → sigaction()** - より信頼性の高いシグナル処理
- **sgtty → termios** - 古い端末制御APIから POSIX 標準へ
- **K&R C → ANSI C (C11)** - 関数プロトタイプ、安全な文字列操作

## トラブルシューティング

### 端末が正しく動作しない

```bash
# TERM 環境変数を設定
export TERM=xterm-256color

# または
export TERM=xterm
```

### PTY デバイスにアクセスできない

```bash
# Linux: /dev/pts が マウントされているか確認
mount | grep devpts

# PTY マスターデバイスの確認
ls -la /dev/ptmx
```

### ウィンドウが作成できない

- システムの PTY 数の上限に達している可能性があります
- 既存のウィンドウを閉じてから再試行してください

### 制御文字が送信できない

- `Ctrl-Z z` で `Ctrl-Z` そのものを送信できます
- シェルの job control と競合する場合は、シェルの設定を確認してください

## 制限事項

1. **端末互換性** - 特定の端末機能が必要です (cursorAddrと画面制御)
2. **Unicode サポート** - ASCII ベースで設計されています
3. **パフォーマンス** - 1987年の研究用インタプリタです
4. **プラットフォーム** - POSIX 準拠システム (Linux, macOS, BSD)

## 歴史的背景

このウィンドウマネージャは 1985-1987年に開発されました。当時としては画期的な機能でした：

- 複数のプロセスを独立した「ウィンドウ」で実行
- 端末の多重化
- プロセス間の切り替え

現代の `tmux` や `screen` の先駆けとなった実装です。

## さらに詳しく

- 完全な仕様: `handout.tex` を参照
- 実装の詳細: `src/wm.c` のコメントを参照
- ビルド方法: `CLAUDE.md` の "Building and Running" セクション
