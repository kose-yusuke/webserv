# 課題要件
```
const char* Parse::valid_keys[] = {
    "listen", "root", "index", "error_page", "autoindex", "server_name", "allow_methods", "client_max_body_size", "return", "cgi_extension", "upload_path", "alias"
};

const char* Parse::required_keys[] = {
    "listen", "root"
};
```

## 本プロジェクトでのConfigParseの構造

std::vector<std::pair<std::map<std::string, std::vector<std::string> >, 
      std::map<std::string, std::map<std::string, std::vector<std::string> > > > > server_location_configs;
std::vector<std::map<std::string, std::vector<std::string> > > server_configs;
std::vector<std::map<std::string, std::map<std::string, std::vector<std::string> > > > locations_configs;

- Serverとlocationは紐づいている(pair)
- 全体として, 複数のServerが存在する
- 1つのServerに複数のlocationが存在する
- server_configは, key, value(valueが複数存在する場合があるため, vector管理)
- locations_configsは, <location_path, std::map<std::string, std::vector<std::string> > > (後半のmapはserver_configと同じ要領)

### エラーハンドリング
- nest管理({が欠如しているなど) (対応済み)
- server block内にrootの重複 (対応済み)
- server block内にlocation pathの重複 (対応済み)
- invalidなipとport (対応済み)

### 用語
- directive : serverの動作を制御する命令
    - listenディレクティブ : 待ち受けするポート番号およびipを指定する (複数個指定可能)
    - server_nameディレクティブ : ホスト名を指定. 並べて書くと複数個指定可能？
    - rootディレクティブ : ドキュメントルートを設定
    - client_max_body_sizeディレクティブ : クライアントのリクエストボディの最大許容サイズを指定. 超過した場合は413(Request Entity Too Large) エラー.
    - error_pageディレクティブ : エラーが起きたときにどのページに飛ばすかを指定. = を用いて次のようにも書ける. (error_page 404 = /404.html;)
    ```
    error_page 404             /404.html;
    error_page 505 502 503 504 /500.html;
    ```
- context : これらのdirectiveをグループ化する役割
    - server 
    - location
        - location / {...}とすればすべてのパスが一致. 完全一致が優先される.



# NGINX Beginner’s Guideの内容を日本語訳して抜粋
Link : https://nginx.org/en/docs/beginners_guide.html

## 設定ファイルの構造
nginxはモジュールで構成されており、設定ファイル内のディレクティブによって制御されます。ディレクティブは単純ディレクティブとブロックディレクティブに分けられます。

単純ディレクティブは、名前とパラメータをスペースで区切った形式で記述され、末尾にセミコロン（;）を付けます。
ブロックディレクティブは、単純ディレクティブと同じ形式ですが、末尾にセミコロンではなく、追加の指示を**波括弧（{}）**内に記述します。
コンテキストとは、ブロックディレクティブのうち、さらに他のディレクティブを内包できるものを指します（例：events、http、server、location など）。
設定ファイル内で、どのコンテキストにも属さないディレクティブはメインコンテキストに属します。
events や http はメインコンテキストに属し、server は http に、location は server に属します。

なお、# 記号以降の行の内容はコメントとして扱われます。

## 静的コンテンツの配信
Web サーバーの重要な役割の一つは、静的ファイル（画像や HTML ページなど）の配信です。本手順では、リクエストの種類に応じて異なるローカルディレクトリからファイルを提供する設定を行います。
具体的には、以下の 2 つのディレクトリを使用します。

- /data/www（HTML ファイルを格納）
- /data/images（画像ファイルを格納）
この構成を実現するために、nginx の設定ファイルを編集し、http ブロック内に server ブロックを作成し、さらに 2 つの location ブロックを設定します。

```
http {
    server {
    }
}
```

## server ブロック内に location ブロックを追加
nginx の設定ファイルには、複数の server ブロック を定義でき、それぞれが異なるポート番号やサーバー名に基づいてリクエストを処理します。
リクエストがどの server にルーティングされるか決定された後、その server ブロック内の location ディレクティブがリクエストされた URI に基づいてマッチングされます。

ここでは、新しい location ブロックを server ブロックに追加します。

```
location / {
    root /data/www;
}
```

## 2つ目の location ブロックの追加
前述の location / ブロックは、リクエストされた URI に対して最も短いプレフィックス（長さ 1 の /）を持つため、他の location ブロックが一致しない場合に適用されます。
一方で、nginx は URI に最も長く一致する location ブロックを優先的に選択するため、より特定のパスに対応する location ブロックを追加することで、適切なコンテンツ配信を実現できます。

### location のマッチングの基本ルール
nginx は location ディレクティブを以下の順番で評価します。

1. 完全一致 (=)
location = /exact_match のように定義されたブロックが、完全に一致するリクエスト URI に対して最優先で適用される。
2. 最も長く一致するプレフィックス (/path/)
通常の location /path/ のようなディレクティブは、リクエスト URI の先頭部分と最も長く一致するものが選択される。
3. 正規表現 (~ または ~*)
~ は大文字・小文字を区別する正規表現、~* は大文字・小文字を区別しない正規表現。
通常のプレフィックスマッチよりも後に評価されるが、もしマッチした場合はそれが適用される。
4. デフォルトの /（全てにマッチする）
どの location にも一致しなかった場合、最も一般的な location / が適用される。

```
location /images/ {
    root /data;
}
```

## サーバーブロックの最終的な設定
location /images/ は /images/ で始まるリクエストと一致します。
location / もすべてのリクエストにマッチしますが、/images/ の方がプレフィックスが長いため優先されます。
その結果、以下のような server ブロックを作成できます。

```
server {
    location / {
        root /data/www;
    }

    location /images/ {
        root /data;
    }
}
```

## FastCGI プロキシの設定
nginx は FastCGI サーバー にリクエストを転送することができます。これは、PHP などの FastCGI アプリケーション を実行する際に利用されます。

通常、proxy_pass を使用してリバースプロキシを設定しますが、FastCGI サーバーを利用する場合は fastcgi_pass を使用します。
また、FastCGI サーバーにリクエストを適切に処理させるために、fastcgi_param を使用して必要なパラメータを設定します。

```
server {
    location / {
        fastcgi_pass  localhost:9000;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        fastcgi_param QUERY_STRING    $query_string;
    }

    location ~ \.(gif|jpg|png)$ {
        root /data/images;
    }
}
```