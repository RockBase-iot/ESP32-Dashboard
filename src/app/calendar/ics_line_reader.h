#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>

static constexpr size_t ICS_PHYSICAL_LINE_MAX = 512;
static constexpr size_t ICS_LOGICAL_LINE_MAX = 4096;

enum class IcsLineReadStatus : uint8_t {
    Ok,
    End,
    PhysicalLineTooLong,
    LogicalLineTooLong,
};

struct IcsLogicalLine {
    IcsLineReadStatus status = IcsLineReadStatus::End;
    std::string text;
    uint32_t physicalLines = 0;
};

class IcsByteReader {
public:
    virtual ~IcsByteReader() = default;
    virtual int readByte() = 0;
    virtual uint32_t bytesRead() const = 0;
};

class StringIcsByteReader final : public IcsByteReader {
public:
    explicit StringIcsByteReader(std::string data);
    int readByte() override;
    uint32_t bytesRead() const override;

private:
    std::string _data;
    size_t _pos = 0;
};

class IcsLineReader {
public:
    explicit IcsLineReader(IcsByteReader &reader);

    IcsLogicalLine next();
    uint32_t bytesRead() const;

private:
    bool readPhysicalLine(std::string &out, bool &hadBytes, IcsLineReadStatus &status);

    IcsByteReader &_reader;
    bool _hasPending = false;
    std::string _pending;
};
