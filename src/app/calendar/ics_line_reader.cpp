#include "ics_line_reader.h"

#include <utility>

StringIcsByteReader::StringIcsByteReader(std::string data) : _data(std::move(data)) {}

int StringIcsByteReader::readByte() {
    if (_pos >= _data.size()) {
        return -1;
    }
    return static_cast<unsigned char>(_data[_pos++]);
}

uint32_t StringIcsByteReader::bytesRead() const {
    return static_cast<uint32_t>(_pos);
}

IcsLineReader::IcsLineReader(IcsByteReader &reader) : _reader(reader) {}

IcsLogicalLine IcsLineReader::next() {
    IcsLogicalLine result;
    std::string logical;
    uint32_t physicalCount = 0;

    while (true) {
        std::string physical;
        bool hadBytes = false;
        IcsLineReadStatus status = IcsLineReadStatus::Ok;
        if (_hasPending) {
            physical = _pending;
            _pending.clear();
            _hasPending = false;
            hadBytes = true;
        } else {
            readPhysicalLine(physical, hadBytes, status);
        }
        if (!hadBytes) {
            result.status = logical.empty() ? IcsLineReadStatus::End : IcsLineReadStatus::Ok;
            result.text = logical;
            result.physicalLines = physicalCount;
            return result;
        }
        ++physicalCount;
        if (status != IcsLineReadStatus::Ok) {
            result.status = status;
            result.physicalLines = physicalCount;
            return result;
        }

        const bool folded = !physical.empty() && (physical[0] == ' ' || physical[0] == '\t');
        const std::string fragment = folded ? physical.substr(1) : physical;
        if (!folded && !logical.empty()) {
            _pending = physical;
            _hasPending = true;
            result.status = IcsLineReadStatus::Ok;
            result.text = logical;
            result.physicalLines = physicalCount - 1;
            return result;
        }
        if (logical.size() + fragment.size() > ICS_LOGICAL_LINE_MAX) {
            result.status = IcsLineReadStatus::LogicalLineTooLong;
            result.physicalLines = physicalCount;
            return result;
        }
        logical += fragment;
    }
}

uint32_t IcsLineReader::bytesRead() const {
    return _reader.bytesRead();
}

bool IcsLineReader::readPhysicalLine(std::string &out, bool &hadBytes,
                                     IcsLineReadStatus &status) {
    out.clear();
    hadBytes = false;
    status = IcsLineReadStatus::Ok;
    while (true) {
        const int byte = _reader.readByte();
        if (byte < 0) {
            return hadBytes;
        }
        hadBytes = true;
        const char c = static_cast<char>(byte);
        if (c == '\n') {
            return true;
        }
        if (c == '\r') {
            continue;
        }
        if (out.size() >= ICS_PHYSICAL_LINE_MAX) {
            status = IcsLineReadStatus::PhysicalLineTooLong;
            return true;
        }
        out.push_back(c);
    }
}
