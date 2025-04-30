## 構成:
Server             <- 1つの server{} block を表す設定
VirtualHostRouter  <- host:port に属する Server 群のrouter
ServerRegistry     <- 検索用の registry
ServerBuilder      <- config から Server 群を生成し、VirtualHostRouter に登録
SocketBuilder　　　 <- server fd作成

## 実行順序:
0. confファイルの server_config, location_configs を解析する
1. ServerRegistry のインスタンスを用意する
2. ServerBuilderが、config情報に基づいてServerインスタンスを作成し、ServerRegistryに登録する
3. その際、同じポート番号の Server 達をまとめて VirtualHostRouter にまとめる
4. ServerRegistryが、initialize()をしてSocketBuilderを呼び出し、各サーバーのsocketを有効なものとする
   └ 各 Server が listenポート・server_name・location などの情報を持ってる
5. Requestのヘッダ情報を元に、VirtualHostRouterが適切なServerインスタンスを選ぶ

## 各クラスの説明
### Server クラス
- `server_config` と `location_configs` を保持
- Hostヘッダーの値と `server_name` の照合を担当
- `matches_host()` 関数で、受信したリクエストと一致するかを判定

### VirtualHostRouter クラス
- ひとつのポートに対して複数の `Server` インスタンスをまとめる
- Hostヘッダーに応じたルーティング (`route_by_host()`) を担当
- マッチするサーバーがなかった場合は `default_server` にフォールバックする

### ServerRegistry クラス
- `ListenEntry`（fd, ip, port, VirtualHostRouter）を管理する
- fd → VirtualHostRouter のマッピングを保持し、リクエスト受付時に使用される
- Serverの登録 (`add()`)、ソケットの初期化 (`initialize()`)、fdの削除 (`remove()`) を担当

### ClientRegistry クラス
- fd → Client のマッピングを管理
- クライアントの接続を追加 (`add()`)、削除 (`remove()`) できる
- fdのclose操作も責任を持って行う（ClientRegistryがfdの所有権を持つ）

### ServerBuilder クラス
- 設定（ConfigMapとLocationMap）から Serverインスタンスを生成する
- ServerインスタンスをServerRegistryに登録する

### SocketBuilder クラス
- ソケット作成・設定 (`socket/bind/listen`) を担当する



