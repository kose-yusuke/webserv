// ResponseTypes.hpp
#pragma once

enum ConnectionPolicy {
  CP_KEEP_ALIVE, // keep-alive中 かつ healthy
  CP_WILL_CLOSE, // Connection: close 受信済み
  CP_MUST_CLOSE  // graceful closeのプロセスに進む
};
