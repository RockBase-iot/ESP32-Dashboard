#include "page_weather.h"

void PageWeatherBase::setWeatherData(const WeatherData    &weather,
                                     const AirQualityData &aqi,
                                     const LocaleData     &loc,
                                     const AppConfig      &cfg) {
    _weather = &weather;
    _aqi     = &aqi;
    _loc     = &loc;
    _cfg     = &cfg;
}
