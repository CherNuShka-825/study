// Package api:
// functions get recuests
package api

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strconv"
	"time"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/models"
)

const (
	graphHopperURL       = "https://graphhopper.com/api/1/geocode"
	openWeatherURL       = "https://api.openweathermap.org/data/2.5/weather"
	openTripMapRadiusURL = "https://api.opentripmap.com/0.1/ru/places/radius"
	openTripMapXIDURL    = "https://api.opentripmap.com/0.1/ru/places/xid"

	maxResponseSize = 1024 * 1024

	geocodeLimit = "5"
	languageRU   = "ru"
	weatherUnits = "metric"
	placesRadius = "3000"
	placesLimit  = "10"
	placesFormat = "json"
)

type Keys struct {
	GraphHopper string
	OpenWeather string
	OpenTripMap string
}

type Endpoints struct {
	Geocode      string
	Weather      string
	PlacesRadius string
	PlaceByXID   string
}

func DefaultEndpoints() Endpoints {
	return Endpoints{
		Geocode:      graphHopperURL,
		Weather:      openWeatherURL,
		PlacesRadius: openTripMapRadiusURL,
		PlaceByXID:   openTripMapXIDURL,
	}
}

type Client struct {
	HTTP      *http.Client
	Keys      Keys
	Endpoints Endpoints
}

func New(keys Keys) *Client {
	return &Client{
		HTTP: &http.Client{
			Timeout: 10 * time.Second,
		},
		Keys:      keys,
		Endpoints: DefaultEndpoints(),
	}
}

func (c *Client) getJSON(
	ctx context.Context,
	rawURL string,
	params url.Values,
	target any,
) error {
	u, err := url.Parse(rawURL)
	if err != nil {
		return fmt.Errorf("invalid URL: %w", err)
	}

	q := u.Query()

	for key, values := range params {
		for _, value := range values {
			q.Add(key, value)
		}
	}

	u.RawQuery = q.Encode()

	req, err := http.NewRequestWithContext(
		ctx,
		http.MethodGet,
		u.String(),
		nil,
	)
	if err != nil {
		return fmt.Errorf("create request: %w", err)
	}

	resp, err := c.HTTP.Do(req)
	if err != nil {
		if urlErr, ok := errors.AsType[*url.Error](err); ok {
			err = urlErr.Err
		}

		return fmt.Errorf("request to %s failed: %w", u.Host, err)
	}
	defer resp.Body.Close()

	if resp.StatusCode < http.StatusOK ||
		resp.StatusCode >= http.StatusMultipleChoices {
		return fmt.Errorf("request to %s: HTTP %s", u.Host, resp.Status)
	}

	decoder := json.NewDecoder(
		io.LimitReader(resp.Body, maxResponseSize),
	)

	if err := decoder.Decode(target); err != nil {
		return fmt.Errorf("decode JSON from %s: %w", u.Host, err)
	}

	return nil
}

func (c *Client) Locations(
	ctx context.Context,
	query string,
) ([]models.Location, error) {
	params := url.Values{}

	params.Set("q", query)
	params.Set("limit", geocodeLimit)
	params.Set("locale", languageRU)
	params.Set("key", c.Keys.GraphHopper)

	var response models.GeocodeResponse

	err := c.getJSON(
		ctx,
		c.Endpoints.Geocode,
		params,
		&response,
	)
	if err != nil {
		return nil, fmt.Errorf("GraphHopper: %w", err)
	}

	return response.Hits, nil
}

func coordinates(params url.Values, point models.Point) {
	params.Set(
		"lat",
		strconv.FormatFloat(point.Lat, 'f', -1, 64),
	)

	params.Set(
		"lon",
		strconv.FormatFloat(point.Lng, 'f', -1, 64),
	)
}

func (c *Client) Weather(
	ctx context.Context,
	point models.Point,
) (models.Weather, error) {
	params := url.Values{}

	coordinates(params, point)

	params.Set("units", weatherUnits)
	params.Set("lang", languageRU)
	params.Set("appid", c.Keys.OpenWeather)

	var weather models.Weather

	err := c.getJSON(
		ctx,
		c.Endpoints.Weather,
		params,
		&weather,
	)
	if err != nil {
		return weather, fmt.Errorf("OpenWeatherMap: %w", err)
	}

	return weather, nil
}

func (c *Client) Places(
	ctx context.Context,
	point models.Point,
) ([]models.PlaceSummary, error) {
	params := url.Values{}

	coordinates(params, point)

	params.Set("radius", placesRadius)
	params.Set("limit", placesLimit)
	params.Set("format", placesFormat)
	params.Set("apikey", c.Keys.OpenTripMap)

	var places []models.PlaceSummary

	err := c.getJSON(
		ctx,
		c.Endpoints.PlacesRadius,
		params,
		&places,
	)
	if err != nil {
		return nil, fmt.Errorf("OpenTripMap places: %w", err)
	}

	return places, nil
}

func (c *Client) PlaceDetails(
	ctx context.Context,
	summary models.PlaceSummary,
) (models.Place, error) {
	params := url.Values{}
	params.Set("apikey", c.Keys.OpenTripMap)

	endpoint := c.Endpoints.PlaceByXID +
		"/" + url.PathEscape(summary.XID)

	var place models.Place

	err := c.getJSON(
		ctx,
		endpoint,
		params,
		&place,
	)
	if err != nil {
		return place, fmt.Errorf(
			"OpenTripMap place %s: %w",
			summary.XID,
			err,
		)
	}

	if place.XID == "" {
		place.XID = summary.XID
	}

	if place.Name == "" {
		place.Name = summary.Name
	}

	if place.Kinds == "" {
		place.Kinds = summary.Kinds
	}

	return place, nil
}
