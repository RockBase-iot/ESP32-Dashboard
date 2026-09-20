from __future__ import annotations

import tempfile
from pathlib import Path

from PIL import Image, ImageDraw

from generate_architecture_word_docs import (
    ArchitectureDocument,
    COLOR_AMBER,
    COLOR_GREEN,
    COLOR_RED,
    ROOT,
    arrow,
    load_font,
    rounded_box,
)


OUTPUT = (
    ROOT
    / "docs"
    / "architecture"
    / "ESP32-Dashboard-Server-Standard-Widget-Framework-v1.0.docx"
)


def create_extraction_diagram(path: Path) -> None:
    image = Image.new("RGB", (1700, 980), "white")
    draw = ImageDraw.Draw(image)
    title_font = load_font(34, True)
    box_font = load_font(22, True)
    small_font = load_font(18)
    draw.text((55, 30), "From Fixed Pages to a Standard Widget Catalog", font=title_font, fill="#1E293B")

    rounded_box(
        draw,
        (70, 130, 490, 390),
        "#FCE8E8",
        "#C53030",
        "16 Current Pages\nWeather / Calendar / Time\nFinance / News / Notes",
        box_font,
    )
    rounded_box(
        draw,
        (640, 130, 1060, 390),
        "#FFF4D6",
        "#B7791F",
        "28 Catalog Entries\nSchemas + Presets + Bindings\nEditor-facing definitions",
        box_font,
    )
    rounded_box(
        draw,
        (1210, 130, 1630, 390),
        "#E8F3EC",
        "#2F855A",
        "12 Runtime Kernels\nSmall stable renderer set\nShared by many widgets",
        box_font,
    )
    arrow(draw, (490, 260), (640, 260), width=4)
    arrow(draw, (1060, 260), (1210, 260), width=4)

    rounded_box(draw, (120, 555, 500, 760), "#E8EEF5", "#2E74B5", "Layout Nodes\nstack / grid / absolute\npanel / slot", box_font)
    rounded_box(draw, (660, 555, 1040, 760), "#F4F6F9", "#51657A", "Widget Nodes\nprops / bindings / states\nrenderer kernel", box_font)
    rounded_box(draw, (1200, 555, 1580, 760), "#EDE9FE", "#6B46C1", "Recipes\nPage composition\nNo new device code", box_font)
    arrow(draw, (500, 658), (660, 658), width=4)
    arrow(draw, (1040, 658), (1200, 658), width=4)

    draw.text(
        (120, 855),
        "Minimize work: add a new catalog manifest or Recipe before adding a new runtime renderer kernel.",
        font=small_font,
        fill="#64748B",
    )
    image.save(path)


def create_framework_diagram(path: Path) -> None:
    image = Image.new("RGB", (1700, 1120), "white")
    draw = ImageDraw.Draw(image)
    title_font = load_font(34, True)
    box_font = load_font(21, True)
    small_font = load_font(18)
    draw.text((55, 30), "Server Standard Widget Framework", font=title_font, fill="#1E293B")

    rounded_box(draw, (80, 120, 470, 300), "#E8EEF5", "#2E74B5", "Widget Catalog\nManifest / Schema\nProfile compatibility", box_font)
    rounded_box(draw, (655, 120, 1045, 300), "#FFF4D6", "#B7791F", "Editor Runtime\nPalette / Inspector\nBinding editor", box_font)
    rounded_box(draw, (1230, 120, 1620, 300), "#E8F3EC", "#2F855A", "Source Runtime\nSourceInstance\nNormalizedSnapshot", box_font)
    arrow(draw, (470, 210), (655, 210), width=4)
    arrow(draw, (1230, 210), (1045, 210), width=4)

    rounded_box(draw, (185, 420, 655, 635), "#F4F6F9", "#51657A", "Validation Pipeline\nSchema / binding / layout\nfont / color / memory", box_font)
    rounded_box(draw, (1045, 420, 1515, 635), "#EDE9FE", "#6B46C1", "Deterministic Preview\nC++ core -> WASM/native\nFresh/Stale/Empty/Error", box_font)
    arrow(draw, (850, 300), (420, 420), width=4)
    arrow(draw, (850, 300), (1280, 420), width=4)

    rounded_box(draw, (320, 760, 750, 965), "#E8F3EC", "#2F855A", "Widget Compiler\nPageDocument -> Device IR\nresources + dependencies", box_font)
    rounded_box(draw, (950, 760, 1380, 965), "#FCE8E8", "#C53030", "Display Package\nCBOR payload + assets\nhash + signature", box_font)
    arrow(draw, (420, 635), (535, 760), width=4)
    arrow(draw, (1280, 635), (1165, 760), width=4)
    arrow(draw, (750, 862), (950, 862), width=4)

    draw.text(
        (285, 1030),
        "The server owns authoring, validation, binding and compilation. The device owns runtime state, local sensors, rendering and refresh policy.",
        font=small_font,
        fill="#64748B",
    )
    image.save(path)


def build_document(asset_dir: Path) -> None:
    extraction = asset_dir / "widget_extraction.png"
    framework = asset_dir / "widget_framework.png"
    create_extraction_diagram(extraction)
    create_framework_diagram(framework)

    d = ArchitectureDocument(
        "服务器端标准控件数据结构与框架",
        "从现有 16 个页面提取可复用 Widget Catalog，并以最小设备端工作量完成编排、预览和发布",
        "RB-EPD-ARCH-SERVER-WIDGET-001",
        "标准控件目录、数据契约、服务器模块、API、存储、编译、测试和实施路线",
    )
    d.add_cover(version="1.0", status="内部评审与启动稿", date="2026-08-21")
    d.add_toc()

    d.heading("1. 文档目的与核心结论", 1)
    d.paragraph(
        "本文基于当前 Overview、Agenda、Weekly Timeline、Monthly、Notes、Weather、Indoor Climate、World Clock、Focus、Finance、Economic、News 和 Milestones 页面，定义服务器端标准控件的数据结构与运行框架。目标不是把每个固定页面复制成一个新控件，而是提取稳定的布局节点、通用数据控件、少量领域控件和可复用 Recipe。本文属于架构阶段，不修改设备固件或服务器业务代码。"
    )
    d.add_callout(
        "推荐结论",
        "服务器首版提供约 28 个可选控件定义，但设备端只需要约 12 个稳定 Renderer Kernel。页面差异优先通过 Manifest、Preset、Binding 和 Recipe 表达；只有现有 Kernel 无法表达时才增加设备端渲染代码。",
    )
    d.table(
        ["层", "数量建议", "主要职责", "是否新增设备代码"],
        [
            ["Layout Node", "5 类", "stack、grid、absolute、panel、slot；确定边界和排列", "使用统一 LayoutEngine"],
            ["Runtime Kernel", "约 12 类", "文本、图标、指标、列表、时间线、日历、图表、时钟等稳定绘制原语", "需要，数量严格受控"],
            ["Catalog Widget", "约 28 个", "面向编辑器的业务名称、Schema、Preset、绑定和状态策略", "通常不需要"],
            ["Recipe", "当前 16 页起步", "把多个控件组成可发布页面模板", "不需要"],
        ],
        [1750, 1350, 4100, 2160],
    )
    d.figure(extraction, "图 1 固定页面到标准控件目录的抽象关系")

    d.heading("2. 抽象原则与边界", 1)
    d.heading("2.1 四层模型", 2)
    d.numbers([
        "Layout Node 只负责空间关系，不直接绑定业务数据。",
        "Runtime Kernel 是设备固件中的稳定绘制实现，不包含第三方数据源逻辑。",
        "Catalog Widget 是服务器端自描述对象，向编辑器声明属性、绑定、状态、兼容性和预算。",
        "Recipe 是页面级组合模板，可引用多个 Catalog Widget，但不能携带任意脚本。",
    ])
    d.heading("2.2 最小工作量规则", 2)
    d.bullets([
        "能通过现有 Kernel 的 preset 实现时，只新增 Widget Manifest。",
        "能通过多个现有 Widget 组合实现时，只新增 Recipe。",
        "只有出现新的测量、布局或像素绘制语义时，才新增 Runtime Kernel。",
        "服务器负责字段选择、排序、过滤、单位、格式和静态计算；设备不执行任意表达式。",
        "设备负责本地时间推进、Focus 状态、本地传感器、按键、缓存、freshness 和刷新策略。",
        "GxEPD2 只存在于设备 DisplayDriverAdapter，服务器控件模型不引用 GxEPD2 类型。",
    ])

    d.heading("3. 现有页面到标准控件的映射", 1)
    d.table(
        ["现有页面", "主要标准控件", "实现方式"],
        [
            ["Overview", "page_chrome、date_summary、agenda_list、metric_group、notes_list、milestone_list", "Recipe 组合"],
            ["Today Agenda", "agenda_list、timeline、notes_list、milestone_list", "Recipe 组合"],
            ["Weekly Timeline", "week_schedule、timeline、event_block", "calendar + timeline Kernel"],
            ["Monthly Overview", "month_calendar、event_marker", "calendar Kernel preset"],
            ["Local Notes", "notes_list、checklist", "list Kernel preset"],
            ["Weather Today", "weather_current、hourly_series、daily_forecast_strip", "condition + chart + list"],
            ["Weekly Weather", "weather_range_trend、daily_forecast_strip", "range chart + list"],
            ["Indoor Climate", "metric_group、climate_status、history_series", "metric + chart"],
            ["World Clock", "clock_collection、timezone_status", "clock + repeater"],
            ["Focus Clock", "focus_timer、session_summary、control_hint", "专用 Focus Kernel + text"],
            ["Stock Info", "quote_list、sparkline、market_status", "table/list + chart"],
            ["Portfolio Summary", "portfolio_total、holding_table、top_movers", "metric + table/list"],
            ["Economic Calendar", "economic_event_list、impact_badge", "timeline/list preset"],
            ["Headlines", "feed_list、source_label", "list Kernel preset"],
            ["Today in History", "dated_content_list、content_excerpt", "list + text"],
            ["Important Milestones", "milestone_list、countdown", "list + clock/countdown"],
        ],
        [2200, 4700, 2460],
    )

    d.heading("4. 第一版 Runtime Kernel", 1)
    d.paragraph("以下 Kernel 是设备端真正需要稳定实现和版本化的最小集合。服务器端多个 Widget 可以共享同一个 Kernel。")
    d.table(
        ["Kernel ID", "能力", "可承载的 Catalog Widget"],
        [
            ["render.text", "单行/多行、截断、对齐、字体角色", "text、title、caption、date_summary、empty_message"],
            ["render.icon", "内置图标、状态图标、逻辑颜色", "icon、weather_icon、impact_icon、status_icon"],
            ["render.metric", "标签、主值、单位、辅助值、强调色", "metric、portfolio_total、climate_metric、countdown_value"],
            ["render.list", "固定/自适应行、marker、主副文本、最多条数", "agenda、notes、checklist、feed、quotes、milestones"],
            ["render.table", "列定义、对齐、行样式、条件强调", "holding_table、key_value、forecast_summary"],
            ["render.timeline", "时间轴、事件块、当前时间和当天强调", "agenda_timeline、weekly_schedule、economic_events"],
            ["render.calendar", "月/周网格、日期、事件标记、today", "month_calendar、week_calendar"],
            ["render.series", "折线、柱、面积、双轴、sparkline", "hourly_weather、sensor_history、stock_sparkline"],
            ["render.range", "高低区间、趋势线、标签", "weekly_weather_range、price_range"],
            ["render.clock", "本地/时区时间、日期、倒计时", "clock、world_clock、anniversary_countdown"],
            ["render.condition", "图标、主值、条件、关键指标", "weather_current、climate_status、air_quality"],
            ["render.focus", "专注/休息/结束状态和分钟级倒计时", "focus_timer"],
        ],
        [2050, 3450, 3860],
    )
    d.add_callout(
        "首版限制",
        "page_chrome、panel、grid、stack、divider 和 spacer 属于 Layout/Theme，不作为独立业务 Kernel；这样避免每个页面重复实现页眉页脚与边框。",
        COLOR_AMBER,
    )

    d.heading("5. 服务器 Catalog Widget", 1)
    d.heading("5.1 基础和通用数据控件", 2)
    d.table(
        ["类别", "Widget ID", "主要用途"],
        [
            ["Foundation", "core.text / core.icon / core.metric / core.key_value", "文字、图标、数值、键值信息"],
            ["Collection", "collection.list / collection.checklist / collection.table", "任务、购物、打卡、Notes、持仓、摘要"],
            ["Time", "time.clock / time.world_clock / time.countdown / time.focus_timer", "本地时间、世界时钟、纪念日、番茄钟"],
            ["Calendar", "calendar.agenda / calendar.timeline / calendar.month / calendar.week_schedule", "日程、月历和周时间线"],
            ["Chart", "chart.timeseries / chart.range / chart.sparkline", "天气、传感器、股票、汇率和加密货币趋势"],
            ["Environment", "weather.current / weather.forecast_strip / environment.climate", "天气和室内环境"],
            ["Finance", "finance.quote_list / finance.portfolio / finance.economic_events", "行情、持仓和经济日历"],
            ["Content", "content.feed / content.quote / content.word_card / content.history", "RSS、每日一句、单词和历史内容"],
            ["Status", "status.summary / status.entity_list / status.build / status.delivery", "服务器、CI、Home Assistant 和物流"],
        ],
        [1550, 4050, 3760],
    )
    d.heading("5.2 Catalog Widget Manifest", 2)
    d.code('''{
  "widget_id": "weather.current",
  "version": "1.0.0",
  "name_i18n": {"zh-CN": "当前天气", "en-US": "Current Weather"},
  "category": "environment",
  "kernel": "render.condition@1",
  "profiles": {
    "required": ["color.black", "font.role.body"],
    "optional": ["color.accent", "icon.weather"],
    "forbidden": []
  },
  "props_schema": {
    "title": {"type": "string", "maxLength": 24, "default": "Weather"},
    "show_feels_like": {"type": "boolean", "default": true},
    "density": {"type": "enum", "values": ["compact", "normal"]}
  },
  "binding_schema": {
    "temperature": {"type": "number", "unit_family": "temperature", "required": true},
    "condition": {"type": "enum", "semantic": "weather_condition", "required": true},
    "feels_like": {"type": "number", "unit_family": "temperature", "required": false}
  },
  "states": ["fresh", "stale", "empty", "error"],
  "layout": {"min_w": 120, "min_h": 72, "aspect": [1.2, 2.4]},
  "budget": {"base_bytes": 128, "max_items": 1, "refresh_weight": 2},
  "fixtures": ["weather.sunny.zh", "weather.rain.en", "weather.stale"]
}''')
    d.paragraph("Manifest 是编辑器、校验器和编译器的共同契约。它不携带渲染代码，只引用固件已支持的 Kernel 和版本。")

    d.heading("6. 页面与控件实例数据结构", 1)
    d.heading("6.1 WidgetInstance", 2)
    d.code('''{
  "node_id": "node-weather-current",
  "kind": "widget",
  "widget_id": "weather.current",
  "widget_version": "1.0.0",
  "frame": {"x": 18, "y": 62, "w": 148, "h": 128},
  "constraints": {"min_w": 120, "min_h": 72, "overflow": "clip"},
  "style": {
    "font_role": "body",
    "value_font_role": "display-lg",
    "color_role": "primary",
    "accent_role": "accent"
  },
  "props": {"title": "Chengdu", "show_feels_like": true, "density": "compact"},
  "bindings": {
    "temperature": {"source": "src-weather-home", "path": "$.current.temp_c", "format": "temperature.user"},
    "condition": {"source": "src-weather-home", "path": "$.current.condition"},
    "feels_like": {"source": "src-weather-home", "path": "$.current.feels_like_c", "format": "temperature.user"}
  },
  "state_policy": {"stale": "badge", "empty": "message", "error": "last_good"},
  "visibility": {"when": "binding.temperature.exists"}
}''')
    d.heading("6.2 LayoutNode 与 PageDocument", 2)
    d.code('''{
  "page_id": "weather-home",
  "page_no": 0,
  "title": "Weather Today",
  "target_profile": "nm-epd-420-bwr-400x300-v1",
  "chrome": {"preset": "calm-grid", "show_wifi": true, "show_battery": true},
  "root": {
    "node_id": "root",
    "kind": "layout",
    "layout": "absolute",
    "frame": {"x": 0, "y": 0, "w": 400, "h": 300},
    "children": ["node-weather-current", "node-hourly", "node-forecast"]
  },
  "nodes": ["...WidgetInstance or nested LayoutNode..."],
  "refresh_policy": {"interval_sec": 1800, "urgent_on": ["weather_alert"]},
  "dependencies": ["src-weather-home"],
  "state_previews": ["fresh", "stale", "empty", "error"]
}''')
    d.bullets([
        "草稿中 page_id 稳定，page_no 在发布时由服务器根据拖动顺序重新生成。",
        "坐标始终是目标 Device Profile 的原生像素；不在设备端做任意比例缩放。",
        "LayoutNode 与 WidgetInstance 使用同一 node_id 空间，便于选择、撤销、差异比较和诊断。",
        "草稿保存 JSON；发布时编译为确定性 Device IR，并裁剪无用字段和资源。",
    ])

    d.heading("7. 数据绑定与 Snapshot 契约", 1)
    d.heading("7.1 Binding", 2)
    d.table(
        ["字段", "含义", "首版限制"],
        [
            ["source", "引用 Workspace 下的 SourceInstance", "必须显式存在"],
            ["path", "从 NormalizedSnapshot 读取字段", "JSON Pointer/受限 JSONPath"],
            ["format", "时间、数字、货币、百分比、温度等格式器", "仅白名单"],
            ["select", "数组排序、过滤、截取", "声明式且有最大条数"],
            ["fallback", "字段缺失时的静态值或替代字段", "不能引用 Secret"],
            ["unit", "目标单位或 user preference", "按 unit_family 校验"],
            ["map", "枚举到图标/逻辑颜色/标签的映射", "编译期固化"],
        ],
        [1750, 4100, 3510],
    )
    d.heading("7.2 NormalizedSnapshot", 2)
    d.code('''{
  "snapshot_schema": "weather.openmeteo@1",
  "source_instance_id": "src-weather-home",
  "snapshot_version": 182,
  "observed_at": "2026-08-21T08:30:00Z",
  "fetched_at": "2026-08-21T08:30:03Z",
  "expires_at": "2026-08-21T09:00:00Z",
  "stale_after": "2026-08-21T10:00:00Z",
  "state": "ready",
  "quality": {"partial": false, "warnings": []},
  "data": {
    "location": {"city": "Chengdu", "region": "Sichuan", "country": "CN"},
    "current": {"temp_c": 31.2, "feels_like_c": 34.0, "condition": "cloudy"},
    "hourly": [],
    "daily": []
  }
}''')
    d.heading("7.3 类型检查", 2)
    d.paragraph("BindingValidator 使用 Widget binding_schema 与 SourceDefinition schema 做静态检查，至少验证类型、数组维度、单位族、枚举语义、可空性和最大条数。无效绑定不能进入发布阶段。")

    d.heading("8. 状态、freshness 与墨水屏语义", 1)
    d.table(
        ["状态", "服务器预览", "设备显示策略"],
        [
            ["fresh", "使用最新 Snapshot", "正常逻辑颜色和内容"],
            ["stale", "生成过期角标/时间提示预览", "保留 last-good；灰显或 badge"],
            ["empty", "使用控件定义的 empty fixture", "显示明确空状态，不显示伪造示例数据"],
            ["error", "显示可解释错误类别", "优先 last-good；否则紧凑错误状态"],
            ["partial", "标记缺失字段和降级布局", "隐藏可选区域，保留核心字段"],
        ],
        [1500, 3900, 3960],
    )
    d.bullets([
        "每个 Widget Manifest 必须声明 fresh/stale/empty/error 的默认策略和 fixtures。",
        "Recipe 发布前必须生成四种状态预览，避免只验证理想数据。",
        "服务器不把过期数据伪装为 fresh；设备根据本地时间再次计算 freshness。",
        "控件只能声明内容变化权重，不得直接请求 GxEPD2 全刷或局刷。",
    ])

    d.heading("9. 服务器端框架", 1)
    d.figure(framework, "图 2 服务器端标准控件框架")
    d.heading("9.1 模块职责", 2)
    d.table(
        ["模块", "职责", "建议位置"],
        [
            ["WidgetCatalog", "加载内置/插件 Manifest，按 Profile、版本和权限筛选", "packages/widget-catalog"],
            ["WidgetSchemaService", "为 Palette、Inspector 和 OpenAPI 提供 Schema", "packages/contracts + api"],
            ["BindingEngine", "字段发现、类型检查、格式和受限选择表达式", "packages/binding-engine"],
            ["EditorAdapter", "把 Manifest 转成控件面板、属性表单和绑定表单", "packages/editor-core"],
            ["LayoutValidator", "边界、重叠、最小尺寸、字体和安全区检查", "packages/layout-engine"],
            ["BudgetEstimator", "节点、字形、图表点、资源、Heap/PSRAM/Flash 估算", "packages/compiler"],
            ["PreviewRenderer", "调用共享 C++17 WASM/native 核心生成确定性预览", "packages/renderer-wasm"],
            ["WidgetCompiler", "PageDocument 转 Device IR，裁剪属性、绑定和资源", "services/compiler"],
            ["FixtureRegistry", "保存标准 fresh/stale/empty/error Snapshot", "packages/widget-fixtures"],
        ],
        [2150, 4550, 2660],
    )
    d.heading("9.2 WidgetRegistry 接口", 2)
    d.code('''interface WidgetRegistry {
  register(manifest: WidgetManifest): void;
  resolve(widgetId: string, version: string): WidgetManifest;
  listForProfile(profile: DeviceProfile, locale: string): WidgetSummary[];
  validateInstance(instance: WidgetInstance, context: ValidationContext): ValidationResult;
  compile(instance: WidgetInstance, context: CompileContext): DeviceWidgetNode;
  fixtures(widgetId: string, version: string): PreviewFixture[];
}''')
    d.heading("9.3 推荐代码组织", 2)
    d.code('''packages/
  contracts/              # TypeBox JSON Schema and generated TS/C++ contracts
  widget-catalog/         # built-in manifests and registry
  widget-fixtures/        # fresh/stale/empty/error fixtures
  binding-engine/         # path, type, unit and formatter validation
  layout-engine/          # scene graph constraints and budget checks
  editor-core/            # palette, inspector adapters and commands
  renderer-wasm/          # shared deterministic preview core
  device-ir/              # compact compiled node model
apps/
  api/src/modules/widgets/
  web/src/features/editor/widgets/
  worker/src/jobs/widget-preview/
  worker/src/jobs/package-compile/
services/
  renderer-native/        # CI and release reference renderer
  compiler/               # PageDocument to Display Package''')

    d.heading("10. 编译后的 Device IR", 1)
    d.paragraph("服务器草稿使用可读 JSON，设备包不直接携带完整 Manifest。编译器把 Widget ID、属性名、绑定路径和状态策略转换为数字 ID、常量表和依赖表，建议使用 CBOR 或版本化二进制格式。")
    d.code('''DevicePageIR {
  page_id_hash: u32
  page_no: u8
  chrome_preset: u8
  node_count: u8
  nodes: [
    {
      node_id: u16,
      kernel_id: u8,
      frame: [u16, u16, u16, u16],
      style_id: u8,
      props_ref: u16,
      bindings_ref: u16,
      state_policy: u8
    }
  ]
  dependency_refs: [u16]
}''')
    d.bullets([
        "Manifest、国际化说明、编辑器提示和不必要默认值不进入设备包。",
        "编译器为每个页面生成依赖列表，设备只同步当前页所需 Snapshot。",
        "编译器固定 Widget API、Kernel 版本、Font Pack 和 Profile hash。",
        "Device IR 解析失败时 candidate 不得替换 active，继续使用 previous/active 包。",
    ])

    d.heading("11. API 设计", 1)
    d.table(
        ["接口", "用途", "关键返回/约束"],
        [
            ["GET /v1/widget-definitions", "按 Profile、分类、语言获取控件目录", "摘要、Schema URL、兼容状态"],
            ["GET /v1/widget-definitions/{id}/{version}", "获取完整 Manifest", "ETag；版本不可变"],
            ["GET /v1/source-instances/{id}/schema", "获取可绑定字段", "脱敏；不返回 Secret"],
            ["POST /v1/widget-bindings/validate", "验证控件与数据字段兼容", "字段级错误和建议格式器"],
            ["PUT /v1/projects/{id}/pages/{pageId}", "保存规范化 PageDocument 草稿", "If-Match revision"],
            ["POST /v1/projects/{id}/validate", "执行完整发布前校验", "Schema、布局、预算、fixtures"],
            ["POST /v1/projects/{id}/preview", "生成指定状态和 Snapshot 的预览", "异步 job；Profile 固定"],
            ["POST /v1/projects/{id}/publish", "编译并签名 Display Package", "幂等键；不可变版本"],
        ],
        [3150, 3400, 2810],
    )

    d.heading("12. 数据库存储", 1)
    d.paragraph("为减少首版工作量，不建议把每个 PageNode 拆成独立关系表。草稿使用版本化 JSONB，稳定聚合和不可变制品使用关系表与对象存储。")
    d.table(
        ["对象", "存储", "关键字段"],
        [
            ["widget_definitions", "PostgreSQL", "widget_id、semver、kernel、manifest_json、hash、status"],
            ["source_definitions", "PostgreSQL", "source_id、schema_version、schema_json、auth/protocol"],
            ["source_instances", "PostgreSQL", "workspace、definition、params_json、key_ref、state"],
            ["page_drafts", "PostgreSQL JSONB", "variant、page_id、revision、document_json、updated_by"],
            ["project_variants", "PostgreSQL", "target_profile、home_page_id、page_order、revision"],
            ["preview_artifacts", "S3/MinIO", "profile、state、snapshot_hash、image、pixel_hash"],
            ["display_packages", "PostgreSQL + S3", "version、profile、widget_api、artifact、hash、signature"],
            ["normalized_snapshots", "PostgreSQL/Redis", "source_instance、version、freshness、payload"],
        ],
        [2300, 2300, 4760],
    )

    d.heading("13. 发布校验流水线", 1)
    d.numbers([
        "解析并迁移 PageDocument Schema，规范化节点顺序和默认值。",
        "解析 Widget Manifest 与 Kernel 版本，拒绝未知或撤销版本。",
        "根据 Device Profile 检查分辨率、色板、字体、输入和刷新能力。",
        "校验 Binding 类型、单位族、数组上限、可空性和 Secret 边界。",
        "执行布局、越界、重叠、文本容量和安全区检查。",
        "估算节点、图表点、字形、图标、Heap、PSRAM、Flash 和刷新成本。",
        "对 fresh/stale/empty/error fixtures 执行确定性预览并保存 pixel hash。",
        "编译 Device IR、依赖表、字体和资源，生成 SBOM 与兼容报告。",
        "签名并原子发布不可变 Display Package。",
    ])
    d.add_callout("发布门禁", "任何 Profile、Kernel、Binding、字体或预算硬错误都必须阻止发布，不能留给设备运行时猜测或自动修正。", COLOR_RED)

    d.heading("14. 标准 Recipe", 1)
    d.paragraph("当前固定页面不需要消失，而应转化为首批官方 Recipe。Recipe 只保存布局、Widget 实例、绑定占位符、默认样式和 fixtures。")
    d.table(
        ["Recipe ID", "来源页面", "可复用场景"],
        [
            ["recipe.weather-today.400x300", "Weather Today", "城市天气、室外传感器、农业气象"],
            ["recipe.weekly-weather.400x300", "Weekly Weather", "天气、价格区间、能耗预测"],
            ["recipe.agenda-overview.400x300", "Overview/Agenda", "日程、任务、家庭计划"],
            ["recipe.calendar-month.400x300", "Monthly Overview", "月历、打卡、值班安排"],
            ["recipe.timeline-week.400x300", "Weekly Timeline", "会议、课程、CI 发布窗口"],
            ["recipe.world-clock.400x300", "World Clock", "跨时区团队、服务器区域状态"],
            ["recipe.focus.400x300", "Focus Clock", "番茄钟、倒计时、值班计时"],
            ["recipe.finance-watch.400x300", "Stock/Portfolio", "股票、汇率、加密货币、资产摘要"],
            ["recipe.status-list.400x300", "News/Economic/Notes", "RSS、CI、HA、物流、清单"],
        ],
        [3100, 2400, 3860],
    )

    d.heading("15. 测试与兼容性", 1)
    d.table(
        ["测试层", "内容", "通过标准"],
        [
            ["Manifest", "Schema、版本、i18n、Kernel、Profile、预算", "全部内置 Widget 无错误"],
            ["Binding", "类型、单位、枚举、数组、缺失字段", "错误可定位到 node/binding"],
            ["Layout", "边界、重叠、长文本、最小尺寸", "所有标准 Profile 通过"],
            ["State fixtures", "fresh/stale/empty/error/partial", "每个 Widget 均有预览"],
            ["Renderer", "native/WASM/device fixture pixel hash", "逻辑色板 hash 一致"],
            ["Budget", "节点、字形、资源、内存和图表点", "硬限制阻止发布"],
            ["Package", "确定性、签名、回滚、旧 Schema", "同输入同 hash；设备兼容"],
            ["Real panel", "BW/BWR 真机截图和刷新", "关键 Recipe 通过人工验收"],
        ],
        [1850, 4500, 3010],
    )

    d.heading("16. 最小实施路线与工作量", 1)
    d.table(
        ["阶段", "交付", "预计人日", "退出条件"],
        [
            ["W0 契约冻结", "Manifest、Instance、Binding、Snapshot、PageDocument、Device IR", "4-6", "Schema 评审通过"],
            ["W1 Catalog MVP", "12 Kernel 映射、28 Manifest、fixtures、Registry", "6-10", "可列举和校验控件"],
            ["W2 Editor 接入", "Palette、Inspector、Binding Editor、Recipe 导入", "8-12", "可编辑 Weather/Agenda"],
            ["W3 Preview/Validate", "WASM/native 预览、四状态、预算", "8-14", "与参考图一致"],
            ["W4 Compiler", "Device IR、资源裁剪、依赖表、Package", "7-12", "设备可加载候选包"],
            ["W5 页面迁移", "16 个现有页面转官方 Recipe", "6-10", "无新增固定页面代码"],
        ],
        [1500, 3850, 1450, 2560],
    )
    d.paragraph("服务器 Widget Framework MVP 预计 33-54 人日，可由 Schema/Catalog、Editor、Renderer/Compiler 三条工作流并行。若先只打通 Weather Today 和 Today Agenda 两个垂直切片，可在 18-28 人日内验证完整链路。")
    d.add_callout(
        "首个垂直切片",
        "优先迁移 Weather Today：它同时覆盖 page chrome、condition、metric、series、forecast list、单位转换、图标、freshness 和数据绑定。第二个切片选择 Today Agenda，验证列表、时间线、空状态和日历 Snapshot。",
        COLOR_GREEN,
    )

    d.heading("17. 评审需要冻结的决策", 1)
    d.bullets([
        "是否批准 12 个 Runtime Kernel 作为首版上限，新增 Kernel 必须经过架构评审。",
        "是否批准 28 个 Catalog Widget 主要通过 Manifest/Preset 复用 Kernel。",
        "是否批准 PageDraft 使用 JSONB，避免首版过度关系化 PageNode。",
        "是否批准 JSON authoring model + CBOR/binary Device IR 的双模型。",
        "是否批准服务器只允许白名单格式器和选择表达式，不执行用户脚本。",
        "是否批准所有 Widget 必须提供 fresh/stale/empty/error fixtures。",
        "是否批准 Weather Today 和 Today Agenda 作为首批端到端垂直切片。",
    ])

    d.heading("18. 最终架构结论", 1)
    d.paragraph(
        "服务器端标准控件框架的核心不是建立大量页面组件，而是建立稳定的 Widget Manifest、Binding、Snapshot、PageDocument、Profile 校验和 Device IR。当前 16 个页面可以完整转化为官方 Recipe；约 28 个面向用户的控件只依赖约 12 个设备 Runtime Kernel。后续增加任务、购物、打卡、服务器状态、CI、Home Assistant、物流、RSS、单词卡、汇率和加密货币时，多数只需增加 SourceDefinition、Widget preset 或 Recipe，不需要修改 GxEPD2 和设备显示驱动。"
    )

    d.save(OUTPUT)


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="esp32-dashboard-widget-doc-") as temp_dir:
        build_document(Path(temp_dir))
    print(OUTPUT)


if __name__ == "__main__":
    main()
