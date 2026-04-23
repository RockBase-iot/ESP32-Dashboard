#pragma once

#include "weather_data.h"

// WeatherClass — fetches and parses Open-Meteo weather and AQI data.
//
// Call fetchWeather() and fetchAirQuality() once per wake cycle, then read
// the results via weather() and airQuality().
class WeatherClass {
public:
    // Fetch current / hourly / daily weather from api.open-meteo.com.
    // Returns true on success.
    bool fetchWeather(double lat, double lon);

    // Fetch air quality data from air-quality-api.open-meteo.com.
    // Returns true on success.
    bool fetchAirQuality(double lat, double lon);

    const WeatherData    &weather()    const { return _weather; }
    const AirQualityData &airQuality() const { return _aqi; }

private:
    bool _parseWeatherResponse(const String &body);
    bool _parseAqiResponse(const String &body);

    WeatherData    _weather;
    AirQualityData _aqi;
};
