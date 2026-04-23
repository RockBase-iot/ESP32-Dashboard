#include "locale_data.h"

extern const LocaleData LOCALE_ZH_CN = {
    .d_t_fmt      = "%Y年%m月%d日 %H:%M:%S",
    .d_fmt        = "%Y/%m/%d",
    .t_fmt        = "%H:%M:%S",
    .t_fmt_ampm   = "%p%I:%M:%S",
    .am_str       = "\u4e0a\u5348",  // 上午
    .pm_str       = "\u4e0b\u5348",  // 下午
    .day          = { "\u661f\u671f\u65e5", "\u661f\u671f\u4e00",
                      "\u661f\u671f\u4e8c", "\u661f\u671f\u4e09",
                      "\u661f\u671f\u56db", "\u661f\u671f\u4e94",
                      "\u661f\u671f\u516d" },
    .abday        = { "\u65e5", "\u4e00", "\u4e8c", "\u4e09",
                      "\u56db", "\u4e94", "\u516d" },
    .mon          = { "1\u6708", "2\u6708", "3\u6708", "4\u6708",
                      "5\u6708", "6\u6708", "7\u6708", "8\u6708",
                      "9\u6708", "10\u6708", "11\u6708", "12\u6708" },
    .abmon        = { "1\u6708", "2\u6708", "3\u6708", "4\u6708",
                      "5\u6708", "6\u6708", "7\u6708", "8\u6708",
                      "9\u6708", "10\u6708", "11\u6708", "12\u6708" },
    .owm_lang            = "zh_cn",
    .txt_feels_like      = "\u4f53\u611f\u6e29\u5ea6",   // 体感温度
    .txt_sunrise         = "\u65e5\u51fa",               // 日出
    .txt_sunset          = "\u65e5\u843d",               // 日落
    .txt_wind            = "\u98ce\u901f",               // 风速
    .txt_humidity        = "\u6e7f\u5ea6",               // 湿度
    .txt_uv_index        = "\u7d2b\u5916\u7ebf\u6307\u6570", // 紫外线指数
    .txt_pressure        = "\u6c14\u538b",               // 气压
    .txt_air_quality     = "\u7a7a\u6c14\u8d28\u91cf",  // 空气质量
    .txt_visibility      = "\u80fd\u89c1\u5ea6",         // 能见度
    .txt_indoor_temperature = "\u5ba4\u5185\u6e29\u5ea6", // 室内温度
    .txt_no_data         = "\u65e0\u6570\u636e",         // 无数据
    .txt_loading         = "\u52a0\u8f7d\u4e2d\u2026",  // 加载中…
    .txt_wifi_error      = "WiFi\u9519\u8bef",           // WiFi错误
    .txt_weather_error   = "\u5929\u6c14\u83b7\u53d6\u5931\u8d25", // 天气获取失败
};
