/project
  ├── HttpRequest.hpp
  ├── HttpRequest.cpp
  ├── HttpResponse.hpp
  ├── HttpResponse.cpp


# Method処理

## Postの挙動
シナリオ 1: ファイルをアップロードする (成功する場合)
```
POST /uploads/file.txt HTTP/1.1
Host: example.com
Content-Type: text/plain
Content-Length: 20

This is test content.
```
location_support
```
HTTP/1.1 201 Created
```

アップロードの許可がない場合、リソース取得のフローになるが、これはGETと同じ処理になる模様.
→リソース取得関連でコードまとめた方が良さそう. 


適切なエンドポイントに送信	POSTデータを処理し、適切なレスポンスを返す（例: 201 Created）
許可されていない場所に送信	POSTデータを処理せず、GETのようにリソースを探す（リソースがなければ 404 Not Found）
リソースがあるがPOSTを許可していない	405 Method Not Allowed（POSTは禁止）
アクセス自体が禁止されている	403 Forbidden


## DELETEの挙動
```
DELETE /public/file.txt HTTP/1.1
```
- ファイルが存在 → 削除成功 → 204 No Content
- ファイルが存在しない → 404 Not Found
- 削除失敗 → 500 Internal Server Error

## 分かっていないこと
- ファイルをどう削除する？(unlinkとか使えそうな関数がsubject.pdfにない)
- file操作関連の関数をなんかクラスとかにまとめた方が読みやすい？
  - 関数名で何してるか明示的であればその方が可読性は高そう.