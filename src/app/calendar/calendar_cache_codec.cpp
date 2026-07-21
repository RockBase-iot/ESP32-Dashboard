#include "calendar_cache_codec.h"

#include <ArduinoJson.h>

namespace {
std::string toString(const std::vector<uint8_t> &bytes) {
    return std::string(bytes.begin(), bytes.end());
}

std::vector<uint8_t> toBytes(const std::string &text) {
    return std::vector<uint8_t>(text.begin(), text.end());
}

template <typename T>
void writeString(JsonObject obj, const char *key, const T &value) {
    obj[key] = value;
}

CalendarEvent readEvent(JsonObjectConst obj) {
    CalendarEvent event;
    event.sourceId = obj["sourceId"] | "";
    event.uid = obj["uid"] | "";
    event.recurrenceId = obj["recurrenceId"] | "";
    event.summary = obj["summary"] | "";
    event.location = obj["location"] | "";
    event.description = obj["description"] | "";
    event.startUtc = obj["startUtc"] | 0;
    event.endUtc = obj["endUtc"] | 0;
    event.allDay = obj["allDay"] | false;
    event.sequence = obj["sequence"] | 0;
    event.dtstampUtc = obj["dtstampUtc"] | 0;
    event.status = static_cast<CalendarEventStatus>(static_cast<uint8_t>(obj["status"] | 0));
    event.origin = static_cast<CalendarEventOrigin>(static_cast<uint8_t>(obj["origin"] | 0));
    event.rrule = obj["rrule"] | "";
    for (JsonVariantConst v : obj["rdate"].as<JsonArrayConst>()) {
        event.rdate.push_back(v.as<std::string>());
    }
    for (JsonVariantConst v : obj["exdate"].as<JsonArrayConst>()) {
        event.exdate.push_back(v.as<std::string>());
    }
    event.startTzid = obj["startTzid"] | "";
    event.endTzid = obj["endTzid"] | "";
    event.startFloating = obj["startFloating"] | false;
    event.endFloating = obj["endFloating"] | false;
    return event;
}
}  // namespace

std::vector<uint8_t> encodeCalendarEvents(const std::vector<CalendarEvent> &events) {
    JsonDocument doc;
    JsonArray array = doc["events"].to<JsonArray>();
    for (const CalendarEvent &event : events) {
        JsonObject obj = array.add<JsonObject>();
        writeString(obj, "sourceId", event.sourceId);
        writeString(obj, "uid", event.uid);
        writeString(obj, "recurrenceId", event.recurrenceId);
        writeString(obj, "summary", event.summary);
        writeString(obj, "location", event.location);
        writeString(obj, "description", event.description);
        obj["startUtc"] = event.startUtc;
        obj["endUtc"] = event.endUtc;
        obj["allDay"] = event.allDay;
        obj["sequence"] = event.sequence;
        obj["dtstampUtc"] = event.dtstampUtc;
        obj["status"] = static_cast<uint8_t>(event.status);
        obj["origin"] = static_cast<uint8_t>(event.origin);
        writeString(obj, "rrule", event.rrule);
        JsonArray rdate = obj["rdate"].to<JsonArray>();
        for (const std::string &value : event.rdate) {
            rdate.add(value);
        }
        JsonArray exdate = obj["exdate"].to<JsonArray>();
        for (const std::string &value : event.exdate) {
            exdate.add(value);
        }
        writeString(obj, "startTzid", event.startTzid);
        writeString(obj, "endTzid", event.endTzid);
        obj["startFloating"] = event.startFloating;
        obj["endFloating"] = event.endFloating;
    }
    std::string json;
    serializeJson(doc, json);
    return toBytes(json);
}

std::vector<CalendarEvent> decodeCalendarEvents(const std::vector<uint8_t> &bytes) {
    JsonDocument doc;
    if (deserializeJson(doc, toString(bytes)) != DeserializationError::Ok) {
        return {};
    }
    std::vector<CalendarEvent> events;
    JsonArrayConst array = doc["events"].as<JsonArrayConst>();
    for (JsonVariantConst item : array) {
        events.push_back(readEvent(item.as<JsonObjectConst>()));
    }
    return events;
}
