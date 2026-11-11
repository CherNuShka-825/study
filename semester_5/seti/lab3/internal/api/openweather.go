package api

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net/url"
	"strconv"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/models"
	"github.com/reactivex/rxgo/v2"
)

const (
	openWeatherURL = "https://api.openweathermap.org/data/2.5/weather"
	weatherUnits   = "metric"
)

func coordinates(params url.Values, point models.Point) {
	params.Set("lat", strconv.FormatFloat(point.Lat, 'f', -1, 64))
	params.Set("lon", strconv.FormatFloat(point.Lng, 'f', -1, 64))
}

func (c *Client) WeatherAsync(ctx context.Context, point models.Point) rxgo.Observable {
	params := url.Values{}
	coordinates(params, point)
	params.Set("units", weatherUnits)
	params.Set("lang", languageRU)
	params.Set("appid", c.Keys.OpenWeather)

	type weatherFields struct {
		Main *struct {
			Temp      *float64 `json:"temp"`
			FeelsLike *float64 `json:"feels_like"`
			Humidity  *int     `json:"humidity"`
			Pressure  *int     `json:"pressure"`
		} `json:"main"`
		Wind *struct {
			Speed *float64 `json:"speed"`
		} `json:"wind"`
	}

	return getJSONAsync[json.RawMessage](c, ctx, c.Endpoints.Weather, params, false).
		Map(func(_ context.Context, value any) (any, error) {
			raw := value.(json.RawMessage)
			var fields weatherFields
			if err := json.Unmarshal(raw, &fields); err != nil {
				return nil, fmt.Errorf("invalid weather response: %w", err)
			}
			if fields.Main == nil || fields.Main.Temp == nil ||
				fields.Main.FeelsLike == nil || fields.Main.Humidity == nil ||
				fields.Main.Pressure == nil || fields.Wind == nil ||
				fields.Wind.Speed == nil {
				return nil, errors.New("openweathermap: missing required weather fields")
			}
			if *fields.Main.Humidity < 0 || *fields.Main.Humidity > 100 ||
				*fields.Main.Pressure <= 0 || *fields.Wind.Speed < 0 {
				return nil, errors.New("openweathermap: invalid weather values")
			}

			var weather models.Weather
			if err := json.Unmarshal(raw, &weather); err != nil {
				return nil, fmt.Errorf("decode weather: %w", err)
			}
			return weather, nil
		}, rxgo.WithContext(ctx))
}
