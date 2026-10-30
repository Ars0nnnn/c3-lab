#include "c3lab/intr.hpp"

namespace c3lab::intr {

constexpr int to_flags(level lv) noexcept {
  switch (lv) {
  case level::l1:
    return ESP_INTR_FLAG_LEVEL1; // 1 << 1
  case level::l2:
    return ESP_INTR_FLAG_LEVEL2;
  case level::l3:
    return ESP_INTR_FLAG_LEVEL3;
  }
  return 0; // 非法 → 让调用方拒绝
}

esp_err_t alloc(std::uint32_t source, level lv, isr_t fn, void *arg,
                handle &out) noexcept {
  out = {}; // ① 先清空

  if (fn == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  const int flags = to_flags(lv); // ② level → flag（见下）
  if (flags == 0) {
    return ESP_ERR_INVALID_ARG;
  } //   非法 level 在这里被挡

  intr_handle_t raw = nullptr;
  esp_err_t err =
      esp_intr_alloc(static_cast<int>(source), flags, fn, arg, &raw); // ③
  if (err != ESP_OK) {
    return err;
  } // ④ 原样返回

  const int intno = esp_intr_get_intno(raw); // ⑤ 自检
  if (intno < 0 || cpu_int_of(source) != static_cast<std::uint32_t>(intno)) {
    (void)esp_intr_free(raw); //    不一致 → 回滚
    return ESP_ERR_INVALID_STATE;
  }

  out.raw = raw; // ⑥ 提交
  out.source = source;
  return ESP_OK;
}

esp_err_t free(handle &h) noexcept {
  if (h.raw == nullptr) {
    return ESP_OK;
  } // 幂等：重复 free 不报错
  esp_err_t err = esp_intr_free(h.raw);
  h = {}; // 成功后清空（避免 double free）
  return err;
}

} // namespace c3lab::intr