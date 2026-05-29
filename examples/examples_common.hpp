#pragma once
// ── libflorid 示例公共工具 ────────────────────────────────────

#include <asio.hpp>
#include <florid/Arm.hpp>
#include <florid/traits/WillowTraits.hpp>

#include <exception>
#include <iostream>
#include <string>

namespace example {

// ══════════════════════════════════════════════════════════════
//  CLI 解析
// ══════════════════════════════════════════════════════════════
inline bool parseIP(const char *text, std::string &out) {
  asio::error_code ec;
  asio::ip::make_address(text, ec);
  if (ec)
    return false;
  out = text;
  return true;
}

inline bool parsePort(const char *text, unsigned short &out) {
  try {
    size_t n = 0;
    const std::string s{text};
    unsigned long v = std::stoul(s, &n, 10);
    if (n != s.size() || v == 0 || v > 65535)
      return false;
    out = static_cast<unsigned short>(v);
    return true;
  } catch (...) {
    return false;
  }
}

inline bool parseHz(const char *text, double &out) {
  try {
    size_t n = 0;
    const std::string s{text};
    out = std::stod(s, &n);
    return n == s.size() && out > 0.0;
  } catch (...) {
    return false;
  }
}

// ══════════════════════════════════════════════════════════════
//  Arm 安全默认配置
// ══════════════════════════════════════════════════════════════
inline void applyDefaults(florid::Arm &arm) {
  using florid::WillowTraits;
  arm.setCollisionBehavior(WillowTraits::collision_lower,
                           WillowTraits::collision_upper);
  arm.setWatchdogTimeout(florid::Duration(WillowTraits::watchdog_timeout_ms));
}

// ══════════════════════════════════════════════════════════════
//  try/catch 包装
// ══════════════════════════════════════════════════════════════
template <typename F> inline int runExample(F &&body) {
  try {
    body();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}

} // namespace example
