# Webserv

## 実行方法
./webserv *.conf

## 課題概要
簡易的な, nginx likeなHTTP ServerをC++ 98で実装する.
## 実装機能
GET/POST/DELETE method, CGI, I/O Multiplexingなど

## 課題要件
- configuration fileを引数に取ること
- execveを他のWeb serverで使用しないこと
- サーバーがリクエストを受け取った際, 処理が止まって他のリクエストが処理できなくなるBlockingが発生しないようにする必要がある
- クライアントに対して, 状況に応じたBounce（リダイレクトやエラー通知など）が行われるべき
- poll()やその代替機能（例: epoll, select, kqueue）を1回の呼び出しで, 全てのI/O操作を管理しなければならない
- poll()またはその代替機能を使用する際にreadとwriteの両方を同時に監視しなければならない
- poll()またはその代替機能を必ず介してからでなければ, read()やwrite()操作を行ってはいけない
- readやwrite操作の後にerrnoの値を確認してはならない
- 設定ファイルを読み込む前に, poll()またはその代替機能を使用する必要はない
- FD_SET, FD_CLR, FD_ISSET, FD_ZEROのようなあらゆるマクロや定義は使用できる
- サーバーへのリクエストが無期限に応答待ち状態（ハング）にならないようにするべき
- サーバーが生成するレスポンスや動作が, Webブラウザで正常に機能すること
- nginxをベースラインとして想定している
- HTTPレスポンスのStatus codeは正確でなければならない
- デフォルトのエラーページを持つ必要がある
- forkはCGI以外（PHPやPythonなど）に使ってはいけない
- 完全に静的なWebsiteを提供する必要がある
- クライアントがファイルをアップロードできるようにする必要がある
- 少なくともGET, POST, DELETE methodを実装する
- ストレステストをして, 高負荷でもサーバーが動くようにする
- サーバーは複数のポートをlistenできなければならない

## Configuration fileについて
- 各サーバーのportとホストを選択する
- server nameを設定する(任意)
- ホスト名:ポート名の最初のサーバーが, そのホスト名:ポート名のデフォルトになる(つまり, 他のサーバーに属さないすべてのリクエストに応答する)
- デフォルトエラーページをセットアップする
- クライアントのボディサイズを制限する
- 以下の1つまたは複数のルール/コンフィギュレーションでルートを設定する（ルートは正規表現を使用しない）:
    - ルートに受け入れられるHTTP メソッドのリストを定義する
    - HTTPリダイレクトの定義をする
    - ファイルが検索されるべきディレクトリまたはファイルを定義する（例えば, url /kapouetが/tmp/wwwにルートされている場合, url /kapouet/pouic/toto/pouetは/tmp/www/pouic/toto/pouet）
    - ディレクトリ一覧の表示/非表示を切り替える (autoindexのこと？)
    - リクエストがディレクトリの場合, デフォルトのレスポンスファイルを設定する
    - 特定のファイル拡張子(例えば.php)に基づいてCGIを実行する
    - POSTメソッドとGETメソッドで動作するようにする
    - ルートがアップロードされたファイルを受け入れることができるようにし, ファイルを保存する場所を設定する
        - CGIとは？
        - CGIを直接呼び出さないので, PATH_INFOとしてフルパスを使用する
        - チャンクされたリクエストの場合, サーバーはチャンクを解除する必要がある
        - CGIは, リクエストボディの終わりとしてEOFを期待する
        - CGIの出力も同じ. CGIからcontent_lengthが返されない場合, EOFは返されたデータの終わりを示す
        - プログラムは, 要求されたファイルを第一引数としてCGIを呼び出す必要がある
        - CGIは, 相対パスによるファイルアクセスのために, 正しいディレクトリで実行されなければならない
        - サーバーは, 1つのCGI（php-CGI, Pythonなど）で動作するはず
        - レビューでは, すべての機能が動作することをテストするため, いくつかの設定ファイルとデフォルトの基本ファイルを準備する必要がある
- ある動作について疑問がある場合, NGINXの動作と自分のプログラムの動作を比較する必要がある. 例えば, server_nameがどのように動作するかを確認する. 小さなテスターを用意されているので, ブラウザとテストがすべて問題なく動作するか確認する. このテストに合格することは必須ではないが, いくつかのバグを見つけるのに役立つ.

## Bonus課題
- クッキーとセッション管理をサポートする
- multiple CGIを扱う

## MacOSの場合
- MacOSではwrite()が他のUnix系OSと同じ挙動をしない可能性があるため, ノンブロッキングモードを明示的に設定して, 期待通りの動作を保証する必要がある
- fcntl()で使用できるフラグはF_SETFL, O_NONBLOCK, FD_CLOEXECのみ. それ以外のフラグを使うことは禁止されている
- ファイルディスクリプタを開いた後, fcntl()を使ってノンブロッキングモード（O_NONBLOCK）を設定することで, 他のUnix OSと同様の動作を実現する

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
- [C++でHTTPサーバーを作った](https://jun-networks.hatenablog.com/entry/2022/12/05/234522)

### 通信
- [Socket()とかBind()とかを理解する](https://qiita.com/Michinosuke/items/0778a5344bdf81488114)
- [Beej](https://beej.us/guide/bgnet/html/split/)

### nginx
- [nginx Beginner’s Guide](https://nginx.org/en/docs/beginners_guide.html)
- [nginx.confの解説、書き方](https://qiita.com/ponponnsan/items/23e1aa6f7dd4eadde5df)
- [max body sizeのdefault](https://qiita.com/roytiger/items/97c549a5627c229c7de2)

### 42 推奨資料
- [RFC](https://triple-underscore.github.io/http1-ja.html)

### CGI
- [PythonでCGIを用いたWebアプリケーションを作る](https://qiita.com/TSKY/items/b041de0572e6586c889c)
- [CGIの仕様](https://www.tohoho-web.com/wwwcgi3.htm)

### I/O Multiplexing
- [man kqueue](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/kqueue.2.html) – Appleの公式マニュアル
- [Kqueue: A generic and scalable event notification facility](https://people.freebsd.org/~jlemon/papers/kqueue_freenix.pdf) – Jonathan Lemon (FreeBSD)
- [The Linux Programming Interface](https://www.man7.org/tlpi/) – 63章：select, poll, epollの比較
