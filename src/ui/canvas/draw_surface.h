#pragma once

#include <stdint.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

enum class TextAlign : uint8_t {
    Left,
    Center,
    Right,
};

struct Rect {
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 0;
    int16_t h = 0;
};

constexpr uint16_t kDashboardBlack = 0x0000;
constexpr uint16_t kDashboardWhite = 0xFFFF;
constexpr uint16_t kDashboardAccent = 0xF800;

class IDrawSurface {
public:
    virtual ~IDrawSurface() = default;

    virtual int16_t width() const = 0;
    virtual int16_t height() const = 0;

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) = 0;
    virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void fillScreen(uint16_t color) = 0;
    virtual void drawText(int16_t x, int16_t y, const std::string &text, uint16_t color,
                          TextAlign align = TextAlign::Left, uint8_t size = 1) = 0;
    virtual int16_t measureText(const std::string &text, uint8_t size = 1) const = 0;
};

class MemoryDrawSurface final : public IDrawSurface {
public:
    struct TextOp {
        int16_t x = 0;
        int16_t y = 0;
        std::string text;
        uint16_t color = 0;
        TextAlign align = TextAlign::Left;
        uint8_t size = 1;
    };

    MemoryDrawSurface(int16_t width, int16_t height);

    int16_t width() const override { return _width; }
    int16_t height() const override { return _height; }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) override;
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
    void fillScreen(uint16_t color) override;
    void drawText(int16_t x, int16_t y, const std::string &text, uint16_t color,
                  TextAlign align = TextAlign::Left, uint8_t size = 1) override;
    int16_t measureText(const std::string &text, uint8_t size = 1) const override;

    size_t outOfBoundsCount() const { return _outOfBounds; }
    size_t invalidColorCount() const { return _invalidColor; }
    const std::vector<TextOp> &textOps() const { return _textOps; }

private:
    bool isAllowedColor(uint16_t color) const;
    void touchPixel(int16_t x, int16_t y, uint16_t color);

    int16_t _width;
    int16_t _height;
    size_t _outOfBounds = 0;
    size_t _invalidColor = 0;
    std::vector<TextOp> _textOps;
};
