from __future__ import annotations

import tempfile
from pathlib import Path

from PIL import Image, ImageDraw

from generate_architecture_word_docs import (
    ArchitectureDocument,
    COLOR_AMBER,
    COLOR_RED,
    ROOT,
    arrow,
    load_font,
    rounded_box,
)


OUTPUT = ROOT / "docs" / "architecture" / "ESP32-Dashboard-Device-Refactoring-Plan-v1.0.docx"


def create_migration_diagram(path: Path) -> None:
    image = Image.new("RGB", (1700, 880), "white")
    draw = ImageDraw.Draw(image)
    title_font = load_font(34, True)
    box_font = load_font(22, True)
    small_font = load_font(19)
    draw.text((55, 30), "Incremental Device Runtime Refactoring", font=title_font, fill="#1E293B")

    phases = [
        ("Current", "DashboardApp\nFixed PageId\nLegacy Renderers", "#FCE8E8"),
        ("Step 1", "Runtime Shell\nLegacyRuntimeBridge\nSame behavior", "#FFF4D6"),
        ("Step 2", "Dynamic Page Model\nLegacyRendererRegistry\nSame pixels", "#E8EEF5"),
        ("Step 3", "Display Package\nSnapshot Bridge\nAtomic slots", "#E8F3EC"),
        ("Step 4", "Widget Runtime\nPage-by-page migration\nRemove bridge last", "#EDE9FE"),
    ]
    boxes = []
    x = 40
    for stage, text, fill in phases:
        draw.text((x + 60, 145), stage, font=box_font, fill="#2E74B5")
        box = (x, 205, x + 280, 430)
        rounded_box(draw, box, fill, "#51657A", text, box_font, radius=16)
        boxes.append(box)
        x += 335
    for left, right in zip(boxes, boxes[1:]):
        arrow(draw, (left[2], 318), (right[0], 318), width=4)

    draw.rounded_rectangle((90, 560, 1610, 770), radius=18, fill="#F4F6F9", outline="#51657A", width=3)
    draw.text((125, 590), "Kept throughout the migration", font=box_font, fill="#1F4D78")
    draw.text(
        (125, 645),
        "GxEPD2 + BSP  |  current page renderers  |  Provider/cache/calendar/time logic  |  NVS settings  |  existing tests",
        font=small_font,
        fill="#334155",
    )
    draw.text(
        (125, 705),
        "Every step remains buildable, flashable and reversible. No server dependency is required until package delivery is ready.",
        font=small_font,
        fill="#64748B",
    )
    image.save(path)


def create_component_map(path: Path) -> None:
    image = Image.new("RGB", (1650, 980), "white")
    draw = ImageDraw.Draw(image)
    title_font = load_font(34, True)
    box_font = load_font(22, True)
    small_font = load_font(18)
    draw.text((55, 30), "Minimum-Change Runtime Composition", font=title_font, fill="#1E293B")

    rounded_box(draw, (80, 130, 520, 310), "#E8EEF5", "#2E74B5", "RuntimeController\nWake / Mode / Commands", box_font)
    rounded_box(draw, (610, 130, 1050, 310), "#FFF4D6", "#B7791F", "PackagePageManager\npageId / pageNo / cursor", box_font)
    rounded_box(draw, (1140, 130, 1580, 310), "#E8F3EC", "#2F855A", "DependencyPlanner\nExisting providers + cache", box_font)
    arrow(draw, (520, 220), (610, 220), width=4)
    arrow(draw, (1050, 220), (1140, 220), width=4)

    rounded_box(draw, (190, 430, 760, 650), "#F4F6F9", "#51657A", "LegacyRendererRegistry\nrendererKey -> current render_* functions\nPageWeather adapter included", box_font)
    rounded_box(draw, (890, 430, 1460, 650), "#EDE9FE", "#6B46C1", "DisplayRuntimeAdapter\nDrawSurface + IBoard + GxEPD2\nfirstPage / nextPage / hibernate", box_font)
    arrow(draw, (825, 310), (480, 430), width=4)
    arrow(draw, (825, 310), (1175, 430), width=4)
    arrow(draw, (760, 540), (890, 540), width=4)

    draw.rounded_rectangle((240, 770, 1410, 910), radius=16, fill="#FCE8E8", outline="#C53030", width=3)
    draw.text((285, 800), "GxEPD2 remains the only panel driver library", font=box_font, fill="#9B1C1C")
    draw.text((285, 850), "The refactor changes ownership boundaries, not the proven panel waveform and driver implementation.", font=small_font, fill="#64748B")
    image.save(path)


def build_document(asset_dir: Path) -> None:
    migration = asset_dir / "device_refactor_migration.png"
    components = asset_dir / "device_refactor_components.png"
    create_migration_diagram(migration)
    create_component_map(components)

    d = ArchitectureDocument(
        "设备端渐进重构实施设计",
        "以最小工作量演进到 EInk Dashboard Runtime，并继续使用 GxEPD2",
        "RB-EPD-REFACTOR-DEVICE-001",
        "设备端重构策略、兼容层、阶段任务、回归门禁、工作量和回滚方案",
    )
    d.add_cover(version="1.0", status="内部评审与实施准备稿", date="2026-08-21")
    d.add_toc()

    d.heading("1. 文档目的与结论", 1)
    d.paragraph("本文把设备端架构设计转化为可执行的渐进重构路线，目标是在不重写 GxEPD2 驱动、不一次性重写 16 个页面、不推翻现有数据源的前提下，尽快获得动态页面模型、统一 Runtime、按页同步、页面包回滚和后续 Widget 化能力。本文只输出重构设计，不修改固件代码。")
    d.add_callout(
        "推荐结论",
        "采用兼容层式渐进重构（Strangler Pattern）。先让新 Runtime 调用旧页面和旧数据，再逐步替换页面模型与渲染组件。任何阶段都必须保持可编译、可烧录、可回滚。",
    )
    d.table(
        ["必须保留", "本轮不做", "第一里程碑必须得到"],
        [
            ["GxEPD2、现有 BSP、页面画面、Provider、缓存、日历/时区、Focus 逻辑", "一次性 Widget 化、换显示库、重写全部数据源、强依赖服务器", "RuntimeController + 动态页面清单 + LegacyRendererRegistry"],
            ["当前 Web 配网和设备设置能力、NVS 数据、Release 烧录流程", "立刻删除固定 PageId、立刻删除 AppConfig、追求完美插件生态", "同一固件可加载内置兼容包或 LittleFS 页面包"],
        ],
        [3100, 3100, 3160],
    )

    d.heading("2. 当前代码基线与重构判断", 1)
    d.heading("2.1 现状量化", 2)
    d.table(
        ["项目", "当前情况", "判断"],
        [
            ["顶层编排", "DashboardApp.cpp 约 1,955 行", "首要拆分点，但不能直接全部重写"],
            ["页面", "16 个固定 PageId + 特殊 legacy weather 首页", "需要统一为 pageId/pageNo，但先通过适配器兼容"],
            ["渲染", "400 x 300 页面多已使用 DrawSurface；PageWeather 直接使用 GFX", "大多数页面可原样注册，PageWeather 单独包装"],
            ["显示抽象", "IBoard 暴露 IEpdDriver、Adafruit_GFX 和颜色能力", "可以渐进扩展，无需更换 GxEPD2"],
            ["数据", "Provider、CacheStore、SourceRuntimeCache、同步调度已有基础", "保留并统一输出 Snapshot，不重写抓取逻辑"],
            ["交互功耗", "Button、WakeCoordinator、Focus 模型和深睡逻辑已有测试", "保留纯逻辑，重构编排所有权"],
            ["测试", "约 37 个测试套件、34 个 C++ 测试文件", "足以建立渐进式回归网"],
        ],
        [1750, 3800, 3810],
    )
    d.heading("2.2 当前主要耦合点", 2)
    d.bullets([
        "DashboardApp 同时负责唤醒识别、硬件初始化、AP、联网校时、数据同步、页面选择、渲染、Focus、交互窗口和休眠。",
        "DisplayPageState 使用特殊 homeWeather=-1，PageManager 使用固定 PageId，和新架构的服务器 pageId/pageNo 不一致。",
        "_renderDashboardPage 内含大型 switch，页面注册、数据构造和显示刷新都集中在一个函数。",
        "Weather、Calendar、Finance、News 以不同全局/局部对象传入，缺少统一 Snapshot 容器。",
        "RenderCoordinator 只有跳过/延后判断，尚未承担 Profile、颜色平面、ghosting 和刷新类型。",
        "AppConfig 同时包含设备设置、页面设置和数据源配置，未来页面包与本地设置需要分开。",
    ])
    d.heading("2.3 值得直接复用的资产", 2)
    d.table(
        ["资产", "处理方式", "原因"],
        [
            ["GxEPD2 + NM-EPD-420 Board.cpp", "原样保留，外包一层 DisplayRuntimeAdapter", "已验证的面板驱动和波形最有价值"],
            ["DrawSurface/GfxSurface", "直接作为 LegacyRendererRegistry 的绘制入口", "已经隔离大部分页面与 GxEPD2"],
            ["render_* 与 PageWeather", "第一阶段全部保留", "先保证像素结果和功能不变"],
            ["Provider/Parser/CacheStore", "包装为 SnapshotProviderBridge", "已有真实数据、缓存和失败回退"],
            ["PageManager 算法", "保留导航算法，替换底层条目类型", "上一页/下一页/轮转逻辑成熟"],
            ["Focus/WorldClock/Calendar 纯模型", "原样保留并由 Runtime Mode 调用", "复杂逻辑已有测试"],
            ["Web 配网和 NVS", "第一阶段不改 Schema，只增加迁移层", "减少设备现场升级风险"],
        ],
        [2300, 3300, 3760],
    )

    d.heading("3. 三种重构路线比较", 1)
    d.table(
        ["路线", "做法", "工作量/风险", "结论"],
        [
            ["A 一次性重写", "直接实现完整 Runtime、Package、Widget、字体和全部页面", "工作量最大；长时间不可烧录；回归风险高", "不采用"],
            ["B 兼容层渐进替换", "新控制骨架调用旧页面/数据，逐步替换模型和控件", "初期多一层适配，但每步可验证、可回滚", "推荐"],
            ["C 服务器图片播放器", "服务器直接输出整页位图，设备只下载显示", "设备简单，但本地传感器、Focus、离线和局刷能力退化", "仅保留 Remote Frame 可选模式"],
        ],
        [1700, 3400, 2850, 1410],
    )
    d.paragraph("路线 B 的关键不是长期保留两套系统，而是给旧能力一个明确的退出通道。兼容层只服务迁移，每个 Legacy Renderer 都必须有迁移状态和删除门禁。")
    d.figure(migration, "图 1 渐进重构路线：先换骨架，再换模型，最后换控件")

    d.heading("4. 最小变更目标架构", 1)
    d.figure(components, "图 2 第一里程碑的最小 Runtime 组合")
    d.heading("4.1 第一里程碑只增加五个边界", 2)
    d.table(
        ["边界", "职责", "复用内容"],
        [
            ["RuntimeController", "统一启动、模式、命令和休眠流程", "现有 DashboardApp helper 暂通过 Bridge 调用"],
            ["PackagePageManager", "读取 pageId/pageNo/rendererKey/dependencies，处理游标和轮转", "复用 PageManager 导航算法"],
            ["LegacyRendererRegistry", "rendererKey 映射到现有 render_* 和 PageWeather", "页面代码和像素输出不变"],
            ["SnapshotBundle", "把 Weather/Calendar/Finance/News/Device/Time 放入统一只读上下文", "复用现有 snapshot/model"],
            ["DisplayRuntimeAdapter", "统一 firstPage/nextPage/surface/hibernate/capabilities", "内部继续调用 IBoard 和 GxEPD2"],
        ],
        [2200, 3800, 3360],
    )
    d.heading("4.2 第一里程碑不增加的复杂度", 2)
    d.bullets([
        "不引入多个长期运行 FreeRTOS 任务；先保持同步 Wake-Render-Sleep，只把状态和命令显式化。",
        "不实现完整 Widget 布局引擎；LegacyRendererRegistry 仍负责整页绘制。",
        "不立即实现在线签名服务；先支持内置兼容包和本地 fixture 包，接口预留签名校验。",
        "不重写天气、日历、股票、新闻 Provider；只把结果装进 SnapshotBundle。",
        "不删除 AppConfig；先把它明确划分为 DeviceSettings、LegacyPageSettings 和 SourceSettings 视图。",
    ])

    d.heading("5. GxEPD2 保留与驱动边界", 1)
    d.heading("5.1 决策", 2)
    d.paragraph("GxEPD2 继续作为唯一墨水屏驱动库。重构只改变调用边界，不替换库、不修改已验证的面板类、不让服务器页面包决定驱动实现。")
    d.heading("5.2 最小适配方案", 2)
    d.code("""Runtime / Renderer
  -> DisplayRuntimeAdapter
       -> IBoard::epd().firstPage() / nextPage() / hibernate()
       -> IBoard::gfx()
       -> GfxSurface
       -> GxEPD2 panel instance inside Board.cpp""")
    d.table(
        ["阶段", "显示接口变化", "GxEPD2 变化"],
        [
            ["R1", "新增 DisplayRuntimeAdapter 包装现有 IBoard", "零变化"],
            ["R2", "增加静态 DisplayCapabilities/Profile ID", "零变化"],
            ["R4", "RefreshPlanner 输出 Full/Skip；BWR 暂不开放局刷", "仍调用现有 firstPage/nextPage"],
            ["R7", "如需局刷，再扩展 refreshPartial(rect)", "只在真实 BW 面板测试后实现"],
        ],
        [1400, 4100, 3860],
    )
    d.heading("5.3 不应在第一阶段做的驱动抽象", 2)
    d.bullets([
        "不要先实现覆盖所有 GxEPD2 面板的巨大虚接口。",
        "不要把 Adafruit_GFX/GxEPD2 对象传入 Widget；兼容 renderer 只能通过 LegacyRenderContext 使用。",
        "不要根据运行时探测结果自动切换 BW/BWR Profile；固件 Target 必须明确绑定。",
        "不要为了架构完整性重写面板分页缓冲和颜色平面，除非现有实现无法满足包渲染。",
    ])

    d.heading("6. 兼容页面模型", 1)
    d.heading("6.1 内置兼容包", 2)
    d.paragraph("在服务器 Display Package 可用前，固件根据现有 AppConfig 生成一个只读的 BuiltinCompatibilityPackage。它把当前 16 个 PageId 和特殊天气首页转换为统一页面条目，因此可以先删除新 Runtime 对特殊 page 0 的认识，而不改变用户看到的页面。")
    d.code("""PackagePageEntry
  pageId: "legacy.weather-home" | "legacy.overview" | ...
  pageNo: generated continuous index
  rendererKey: "legacy.page-weather" | "legacy.render-overview" | ...
  enabled: bool
  autoRotate: bool
  dependencies: [weather, calendar, finance, news, indoor]
  interactionMode: normal | focus""")
    d.heading("6.2 LegacyRendererRegistry", 2)
    d.table(
        ["rendererKey", "现有实现", "迁移优先级"],
        [
            ["legacy.page-weather", "PageWeather::draw()", "保留到 Weather Widget 完整验收"],
            ["legacy.render-weather-today", "renderWeatherTodayPage", "中期迁移"],
            ["legacy.render-overview/agenda/calendar", "现有 CalendarPageSnapshot renderers", "后期按共用控件迁移"],
            ["legacy.render-world-clock/focus", "现有 time renderers + model", "Focus 状态机稳定后迁移"],
            ["legacy.render-finance/news", "现有 Finance/News snapshot renderers", "数据 Snapshot 契约稳定后迁移"],
        ],
        [3000, 3900, 2460],
    )
    d.heading("6.3 页面模型切换顺序", 2)
    d.numbers([
        "先让现有页面通过 BuiltinCompatibilityPackage 运行，页面显示完全不变。",
        "再让同一 PackagePageManager 读取 LittleFS fixture package。",
        "再接入服务器下载的 candidate package 和 active/previous 槽位。",
        "最后逐个把 rendererKey 从 legacy.* 替换为 widget.*。",
    ])

    d.heading("7. 数据与同步的最小改造", 1)
    d.heading("7.1 统一上下文，不重写 Provider", 2)
    d.code("""RuntimeSnapshots
  weather: existing WeatherPageSnapshot / WeatherClass view
  calendar: existing CalendarPageSnapshot
  finance: existing FinancePageSnapshot
  news: existing NewsPageSnapshot
  time: TimeSnapshot
  device: IP / battery / sensor / sync state""")
    d.bullets([
        "现有 syncCalendarPageSnapshot、syncFinancePageSnapshot、syncNewsPageSnapshot 和 _fetchData 暂保留。",
        "新增 LegacySnapshotBridge 只负责把现有对象转换为统一只读视图，不复制大 payload。",
        "DependencyPlanner 读取页面 dependencies，继续调用现有 SyncScheduler/ProviderRegistry。",
        "等页面包和 Snapshot Schema 稳定后，再逐个把 Provider 输出改为标准 Snapshot。",
    ])
    d.heading("7.2 消除全局 Snapshot 的顺序", 2)
    d.numbers([
        "把 gFinanceSnapshot/gNewsSnapshot 收口到 RuntimeSessionContext。",
        "renderer 通过 const RuntimeSnapshots& 读取，不直接访问全局。",
        "测试中构造独立 RuntimeSnapshots fixture。",
        "所有页面切换通过 DependencyPlanner 补齐缺失数据。",
    ])
    d.heading("7.3 Freshness 兼容", 2)
    d.paragraph("第一阶段不要求所有 Provider 立刻输出统一 JSON。只需为每类现有 snapshot 增加 SourceStatus 视图，并统一映射 Ready/Stale/Empty/Error。页面 renderer 原有空状态继续使用。")

    d.heading("8. RuntimeController 的渐进拆分", 1)
    d.heading("8.1 DashboardApp 的保留方式", 2)
    d.paragraph("DashboardApp::run() 暂时保留为 Arduino setup() 的单一入口，但函数体逐步缩减为构造依赖和调用 RuntimeController::runWakeCycle()。旧 helper 先移动到 LegacyRuntimeBridge，不在第一阶段重写内部实现。")
    d.code("""setup()
  -> DashboardApp::run()
       -> RuntimeCompositionRoot::create(board)
       -> RuntimeController::runWakeCycle()

LegacyRuntimeBridge
  connectAndSync()
  acquireLegacySnapshots()
  renderLegacyPage()
  runLegacyFocusSession()
  enterLegacyConfigPortal()""")
    d.heading("8.2 拆分顺序", 2)
    d.table(
        ["顺序", "从 DashboardApp 移出", "目标模块", "行为变化"],
        [
            ["1", "wake/cold/AP 判定", "RuntimeBootstrap + WakeContext", "无"],
            ["2", "页面选择和游标", "PackagePageManager", "内部模型变化，外部页面不变"],
            ["3", "数据依赖判断", "DependencyPlanner", "继续按页同步"],
            ["4", "渲染 switch", "LegacyRendererRegistry", "无"],
            ["5", "交互/Focus 模式切换", "RuntimeModeController", "消除网络重连和错误跳页"],
            ["6", "sleep commit", "PowerCoordinator", "无"],
            ["7", "AP/Web 维护模式", "MaintenanceMode", "无"],
        ],
        [900, 2600, 3000, 2860],
    )
    d.heading("8.3 并发原则", 2)
    d.add_callout(
        "最小工作量原则",
        "第一阶段保持同步单线程 Wake-Render-Sleep，不立即引入 Network/Render/Storage 多任务。只有出现可测量的阻塞或看门狗问题时，再把网络下载移到独立 Worker。",
        COLOR_AMBER,
    )

    d.heading("9. 分阶段实施计划", 1)
    d.paragraph("以下阶段按功能可交付切分。每个阶段都应独立合入，并在 NM-EPD-420 上完成 build、upload_all、冷启动、按键、页面一圈和深睡恢复验证。")
    d.table(
        ["阶段", "目标", "主要新增/修改", "估算", "退出门禁"],
        [
            ["R0 基线冻结", "锁定当前行为和像素基准", "测试清单、串口基准、页面截图/hash fixtures", "2-3 人日", "现有 16+1 页面和 Focus/深睡可复现"],
            ["R1 Runtime 壳", "新控制器接管流程但调用旧 helper", "runtime_controller、bootstrap、legacy_runtime_bridge", "4-6 人日", "与旧固件行为等价"],
            ["R2 显示边界", "GxEPD2 收口到 Adapter", "display_runtime_adapter、capabilities、legacy_render_context", "3-5 人日", "页面像素/实物显示不变"],
            ["R3 动态页面", "统一 pageId/pageNo，去除 Runtime 特殊首页", "package_page、package_page_manager、builtin_compat_package", "5-8 人日", "内置包完整轮转"],
            ["R4 数据上下文", "统一 snapshots 和按页依赖", "runtime_snapshots、legacy_snapshot_bridge、dependency_planner", "5-8 人日", "只同步当前页依赖"],
            ["R5 本地页面包", "LittleFS active/candidate/previous", "package_store、validator、manifest", "6-10 人日", "故障注入不破坏 active"],
            ["R6 平台接入", "下载、hash、签名和 ack", "package_client、credential/session adapter", "6-10 人日", "服务器包可灰度激活/回滚"],
            ["R7 Widget 迁移", "按页面逐步替换 legacy renderer", "widget registry、layout/font/color 最小集", "按页面 2-5 人日", "每迁一页删除一个 legacy 映射"],
        ],
        [1200, 1700, 2900, 1100, 2460],
    )
    d.heading("9.1 最小可用重构版本", 2)
    d.paragraph("R0-R5 完成后即可称为设备 Runtime v2 MVP：它已经具备统一 Runtime、动态多页清单、GxEPD2 适配边界、按页同步、本地页面包和原子回滚，但页面内部仍可全部使用 Legacy Renderer。这样可以先与服务器端页面编排和包交付联调，再投入 Widget 化。")
    d.table(
        ["范围", "单人估算", "2 名固件并行估算", "得到的价值"],
        [
            ["R0-R3", "14-22 人日", "约 2-3 周", "新 Runtime + 动态页面，画面不变"],
            ["R0-R5", "25-40 人日", "约 4-6 周", "可加载/回滚本地页面包"],
            ["R0-R6", "31-50 人日", "约 5-8 周", "与服务器端端到端联通"],
            ["R7 全部页面", "另计 35-70 人日", "按页面并行", "完全声明式 Widget Runtime"],
        ],
        [1800, 1900, 2200, 3460],
    )

    d.heading("10. 未来文件与职责规划", 1)
    d.paragraph("以下是实施时建议的文件边界，不要求在第一提交一次性创建。只有当前阶段用到的文件才建立。")
    d.code("""src/app/runtime/
  runtime_controller.*          # wake-cycle state and commands
  runtime_bootstrap.*           # wake/profile/config bootstrap
  runtime_session_context.*     # snapshots, page, chrome, timestamps
  legacy_runtime_bridge.*       # temporary access to current helpers

src/app/runtime/package/
  package_page.*
  package_page_manager.*
  builtin_compat_package.*
  package_store.*               # added at R5
  package_validator.*           # added at R5

src/app/runtime/data/
  runtime_snapshots.*
  legacy_snapshot_bridge.*
  dependency_planner.*

src/app/runtime/render/
  legacy_renderer_registry.*
  display_runtime_adapter.*
  legacy_render_context.*

src/app/runtime/modes/
  interactive_mode.*
  focus_mode.*
  maintenance_mode.*
  power_coordinator.*""")
    d.heading("10.1 现有文件处理矩阵", 2)
    d.table(
        ["现有文件/目录", "R1-R3", "R4-R6", "最终状态"],
        [
            ["src/app/dashboardApp.*", "缩减为 composition root", "继续缩减", "只保留入口或删除"],
            ["src/app/page/*", "复用算法并增加 package manager", "固定 PageId 仅供 legacy registry", "PageId 删除"],
            ["src/app/display/display_page_state.*", "由兼容转换层调用", "不再作为 Runtime 真值", "R6 后删除"],
            ["src/app/provider/cache/source/*", "不改", "接入 RuntimeSnapshots", "长期保留并演进"],
            ["src/ui/layouts/epd_400x300/*", "全部保留", "按页面标记 legacy", "R7 逐个删除"],
            ["src/bsp/nm_display_420/*", "不改面板实现", "补 Profile/Driver ID", "长期保留 GxEPD2"],
            ["src/app/web/*", "不改", "只读展示包状态", "保留设备配置/维护"],
        ],
        [2800, 2100, 2200, 2260],
    )

    d.heading("11. 测试和回归策略", 1)
    d.heading("11.1 先复用现有测试", 2)
    d.table(
        ["现有测试", "重构期间用途"],
        [
            ["test_page_manager / test_display_page_state", "证明导航、游标和旧页面转换保持一致"],
            ["test_render_coordinator / test_layout_bounds", "证明跳过/延后和页面边界不回退"],
            ["test_provider_registry / test_source_runtime_cache / test_cache_manifest", "证明数据与缓存无需重写"],
            ["test_focus_clock / test_button_controller / test_pending_button_action", "防止 Focus、长按和翻页回归"],
            ["test_wake_coordinator / test_gpio_wake_decoder", "防止深睡唤醒与按键重启回归"],
            ["calendar/timezone/finance/news/weather tests", "证明领域逻辑原样复用"],
        ],
        [4300, 5060],
    )
    d.heading("11.2 每阶段新增测试", 2)
    d.table(
        ["阶段", "建议测试套件", "必须覆盖"],
        [
            ["R1", "test_runtime_controller", "冷启动、AP、正常、Focus 恢复、失败休眠命令顺序"],
            ["R2", "test_display_runtime_adapter", "firstPage/nextPage/hibernate 调用和能力声明"],
            ["R3", "test_builtin_compat_package / test_package_page_manager", "17 页映射、主页、排序、轮转、NVS 游标迁移"],
            ["R4", "test_dependency_planner / test_runtime_snapshots", "按页依赖、缺失、stale、全局对象消除"],
            ["R5", "test_display_package_store", "截断、CRC/hash、掉电、active/candidate/previous"],
            ["R6", "test_package_client", "ETag、Range、签名错误、Profile 错误、ack"],
            ["R7", "test_widget_* + pixel fixtures", "每个迁移页面与 legacy 输出对比"],
        ],
        [1000, 3300, 5060],
    )
    d.heading("11.3 真机回归门禁", 2)
    d.numbers([
        "编译 nm-display-420，检查 Flash/Heap/PSRAM 增量。",
        "upload_all 到 NM-EPD-420，确认 LittleFS 与固件一致。",
        "冷启动显示默认主页；BOOT 下一页、USER 上一页；完整轮转所有启用页面。",
        "深睡后 BOOT/USER 唤醒并恢复当前 packageId/pageId。",
        "Focus 未激活短按翻页；长按立即启动；激活时长按停止且不重连网络、不跳主页。",
        "断网、错误 ICS、缓存 stale、低电量和候选包损坏均显示可解释状态。",
        "连续多轮刷新无红黑层异常、重启循环和内存持续下降。",
    ])

    d.heading("12. 兼容、迁移与回滚", 1)
    d.heading("12.1 NVS 兼容", 2)
    d.bullets([
        "R1-R2 不修改 NVS key。",
        "R3 新增 packageId/pageId 游标时，首次启动从旧 page=-1/PageId 转换并写新 key，旧 key 保留一个版本周期。",
        "AppConfig 页面 mask/order 继续作为 BuiltinCompatibilityPackage 输入，直到服务器包成功激活。",
        "恢复出厂和解绑逻辑同时清理新旧键，避免残留状态。",
    ])
    d.heading("12.2 固件功能开关", 2)
    d.paragraph("开发阶段可使用编译期开关 DASHBOARD_RUNTIME_V2 对比新旧入口，但 Release 不长期维护用户可切换的双 Runtime。每阶段验收后把新路径设为默认，旧路径只保留到下一个里程碑。")
    d.heading("12.3 回滚级别", 2)
    d.table(
        ["级别", "回滚对象", "触发条件"],
        [
            ["页面包回滚", "candidate -> active/previous", "包校验或首次渲染失败"],
            ["配置回滚", "新 NVS view -> 旧 AppConfig", "迁移版本不支持"],
            ["固件回滚", "OTA previous slot 或完整 Release bin", "Runtime 崩溃、启动循环、显示驱动回归"],
            ["开发回滚", "关闭 DASHBOARD_RUNTIME_V2", "阶段性对比和问题定位"],
        ],
        [1900, 3400, 4060],
    )

    d.heading("13. 何时删除旧代码", 1)
    d.paragraph("旧代码不能因为“新模块已经存在”就删除，必须按使用证据删除。")
    d.table(
        ["旧能力", "删除门禁"],
        [
            ["特殊 homeWeather/-1", "所有启动、按键、深睡和服务器主页测试只使用 package pageId/pageNo"],
            ["固定 PageId 作为 Runtime 真值", "所有启用页面来自 PackagePageEntry；PageId 仅剩 legacy renderer key 映射"],
            ["DashboardApp helper", "RuntimeController 测试和真机流程不再调用该 helper"],
            ["gFinanceSnapshot/gNewsSnapshot", "所有 renderer 从 RuntimeSnapshots 读取"],
            ["单个 legacy renderer", "对应 Widget 页面像素/内容/真机验收通过两个 Release 周期"],
            ["LegacyRendererRegistry", "所有生产页面均为 widget.* 或 remote-frame，恢复页仍为固件内置"],
        ],
        [3600, 5760],
    )

    d.heading("14. 工作量控制原则", 1)
    d.bullets([
        "每次只替换一个所有权边界，不同时重写控制流、数据和画面。",
        "优先做 Adapter、Registry 和 Bridge；等调用关系稳定后再移动实现。",
        "页面外观不在 R1-R6 调整，避免把架构回归与 UI 变化混在一起。",
        "平台接口未完成前使用本地 fixture package，不阻塞设备端。",
        "只为当前 Profile 实现必须能力；多色、多分辨率通过 Schema 和测试 fixture 预留，不预写所有驱动。",
        "测试先锁定行为，再移动代码；每个阶段保持单一可烧录主线。",
        "任何抽象若不能减少 DashboardApp、固定 PageId 或 GxEPD2 泄漏，就暂不引入。",
    ])
    d.add_callout(
        "成本警戒线",
        "如果 R1-R3 超过 22 人日，说明阶段内混入了 Widget、服务器协议或 UI 重绘，应立即缩回到兼容包和 Legacy Renderer。",
        COLOR_RED,
    )

    d.heading("15. 推荐启动顺序", 1)
    d.numbers([
        "冻结当前 NM-EPD-420 页面与交互基准，记录串口流程和设备截图。",
        "先实现 RuntimeController + LegacyRuntimeBridge，让调用关系变清楚但行为不变。",
        "增加 DisplayRuntimeAdapter，将 GxEPD2 调用收口但不改驱动。",
        "用 BuiltinCompatibilityPackage 替代特殊首页和固定页面顺序，让动态 pageId/pageNo 跑通。",
        "统一 RuntimeSnapshots 和 DependencyPlanner，消除全局 Snapshot。",
        "加入 LittleFS 页面包槽位和验证，再与服务器端联调。",
        "选择 World Clock 或简单 Clock 页面作为第一个 Widget 垂直切片；随后迁移 Weather Today。",
        "只有在主要标准页面完成 Widget 化后，才删除 LegacyRendererRegistry。",
    ])
    d.heading("15.1 第一批内部评审决策", 2)
    d.bullets([
        "批准 GxEPD2 长期保留，不开展显示库替换。",
        "批准 R0-R5 作为 Device Runtime v2 MVP，Widget 全迁移不阻塞 MVP。",
        "批准 BuiltinCompatibilityPackage 作为旧 AppConfig 到新页面包的过渡机制。",
        "批准同步单线程 Runtime 作为第一阶段，不预先引入复杂 FreeRTOS 并发。",
        "批准 LegacyRendererRegistry 有明确退出门禁，而非永久双架构。",
        "批准以 R1-R3 14-22 人日作为第一成本检查点。",
    ])

    d.heading("16. 文档交付边界", 1)
    d.paragraph("本文是重构实施设计，不包含固件修改、服务器实现或烧录操作。进入编码阶段前，应把 R0-R3 拆成可执行任务，明确每个测试、文件和提交边界；R4-R6 在页面包和服务器协议冻结后再展开。")
    d.save(OUTPUT)


def main() -> None:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="esp32_dashboard_refactor_") as temp:
        build_document(Path(temp))
    print(OUTPUT)


if __name__ == "__main__":
    main()
