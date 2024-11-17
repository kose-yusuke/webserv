#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <iostream> // For cout
#include <unistd.h> // For read

int main() {
    int server_fd;  // サーバー用のソケットファイルディスクリプタ
    struct sockaddr_in address;  // ソケットのアドレス情報
    int opt = 1;
    int port = 8080;  // ポート番号

    // 1. ソケットの作成
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket creation failed");
        return 1;
    }
    std::cout << "Socket created successfully\n";

    // 2. オプション設定（アドレスの再利用を許可）
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        return 1;
    }
    std::cout << "Socket options set successfully\n";

    // 3. アドレス構造体の設定
    address.sin_family = AF_INET;  // IPv4
    address.sin_addr.s_addr = INADDR_ANY;  // すべてのネットワークインターフェースで待ち受け
    address.sin_port = htons(port);  // ポート番号をネットワークバイトオーダーに変換

    // 4. ソケットをポートにバインド
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }
    std::cout << "Socket bound to port " << port << "\n";

    // 5. 接続待ち状態にする
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return 1;
    }
    std::cout << "Server is listening on port " << port << "\n";

    int new_socket;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};  // クライアントからのデータを格納するバッファ

    // 6. 接続の受け付け
    while (true) {  // サーバーをループさせる
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            return 1;
        }
        std::cout << "New connection accepted\n";

        // 7. クライアントからのデータ受信
        memset(buffer, 0, sizeof(buffer));  // バッファを初期化
        int valread = read(new_socket, buffer, 1024);  // データを読み取る
        if (valread > 0) {
            std::cout << "Received data: " << buffer << "\n";
        }

        // 簡単なレスポンスを返す
        const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
        send(new_socket, response, strlen(response), 0);

        // 接続を閉じる
        close(new_socket);
    }

    close(server_fd);
    return 0;
}
