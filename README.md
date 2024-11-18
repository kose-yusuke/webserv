# Webserv

## 実行方法
./webserv *.conf

## 課題概要
簡易的なHTTP ServerをC++ 98で実装する.

## 使用可能な関数一覧
Everything in C++ 98.

### ファイルとディレクトリ操作
- `write`: ファイル記述子にデータを書き込む
- `access`: ファイルの存在やアクセス権限をチェックする
- `open`: ファイルを開いてファイル記述子を取得する
- `read`: ファイル記述子からデータを読み込む
- `close`: 開いたファイル記述子を閉じる
- `opendir`: ディレクトリを開いて、ディレクトリストリームを取得する
- `readdir`: `opendir`で開いたディレクトリストリームからエントリを読み込む
- `closedir`: `opendir`で開いたディレクトリストリームを閉じる
- `chdir`: 作業ディレクトリを変更する

### プロセス管理
- `fork`: 新しいプロセスを作成する
- `waitpid`: 特定の子プロセスが終了するのを待つ
- `execve`: プロセスに新しいプログラムをロードして実行する
- `kill`: プロセスにシグナルを送信する

### シグナル管理
- `signal`: シグナルを設定する

### ファイル情報取得
- `stat`: ファイルの情報を取得する

### 入出力制御
- `dup`: 既存のファイル記述子を複製する
- `dup2`: 指定されたファイル記述子に既存のファイル記述子をコピーする
- `pipe`: ２つのファイル記述子を作成し、データのパイプラインを形成する

### エラー処理
- `strerror`: エラーメッセージを返す
- `gai_strerror`: getaddrinfo 関数で発生したエラー番号を文字列に変換して返す
- `errno`:  システムコールや標準ライブラリ関数のエラー番号を格納する

### ソケット
- `socketpair`: ソケットペアを作成する
- `htons`: ホストバイトオーダーをネットワークバイトオーダー（short型）に変換する
- `htonl`: ホストバイトオーダーをネットワークバイトオーダー（long型）に変換する
- `ntohs`: ネットワークバイトオーダーをホストバイトオーダー（short型）に変換する
- `ntohl`: ネットワークバイトオーダーをホストバイトオーダー（long型）に変換する
- `select`: 複数のファイルディスクリプタ（ソケット含む）を監視する
- `poll`: 複数のファイルディスクリプタを監視する
- `epoll((epoll_create, epoll_ctl, epoll_wait))`: より効率的な監視を行う
    - `epoll_create`: epollインスタンスを作成する
    - `epoll_ctl`: epollインスタンスに監視対象を追加・削除・修正する
    - `epoll_wait`: イベントが発生するまで待機する
- `kqueue (kqueue, kevent)`: FreeBSD系のOSで使用されるイベント監視機構
    - `kqueue`: イベントキューを作成する
    - `kevent`: イベントを登録したり、発生したイベントを取得したりする
- `socket`: ソケットを作成しエンドポイントを返す
- `accept`: 接続要求を受け付け、接続済みソケットを作成する
- `listen`: ソケットを接続待ち状態にする
- `send`: ソケットを介してデータを送信する
- `recv`: ソケットを介してデータを受信する
- `bind`: ソケットにアドレスを割り当てる
- `connect`: サーバーに接続を試みる
- `getaddrinfo`: ホスト名などを元にアドレス情報の構造体を作成する
- `freeaddrinfo`: getaddrinfo によって確保されたアドレス情報を解放する
- `setsockopt`: ソケットと関連したオプションを設定する
- `getsockname`: 指定のソケットについて現在のnameを返す
- `getprotobyname`: nameで指定されたプロトコルに対応する名前とプロトコル番号を含む構造体へのポインタを返す
- `fcntl`: ファイルディスクリプタに対する各種操作を行う

## 資料
### 雰囲気を掴む
- [HTTPサーバーの実装 | 42Tokyo Webserv](https://qiita.com/ryhara/items/c46fe320332b237b5c0d)
- [Webサーバの動作を理解するための事前知識とC++によるフルスクラッチで実装して理解を深めよう](https://qiita.com/hayashier/items/73168c08451896914da5)

### nginx
- [nginx Beginner’s Guide](https://nginx.org/en/docs/beginners_guide.html)
### 42 推奨資料
- [RFC](https://triple-underscore.github.io/http1-ja.html)