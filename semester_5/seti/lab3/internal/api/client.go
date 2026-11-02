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
	graphHopperURL = "https://graphhopper.com/api/1/geocode"
	openWeatherURL = "https://api.openweathermap.org/data/2.5/weather"
	wikipediaURL   = "https://ru.wikipedia.org/w/api.php"

	maxResponseSize = 1024 * 1024
	maxImageSize    = 2 * 1024 * 1024

	geocodeLimit = "5"
	languageRU   = "ru"
	weatherUnits = "metric"
	placesLimit  = "10"
	extractChars = "500"
	thumbSize    = "240"

	maxRadius = 10000
)

type Keys struct {
	GraphHopper        string
	OpenWeather        string
	WikipediaUserAgent string
}

type Endpoints struct {
	Geocode   string
	Weather   string
	Wikipedia string
}

func DefaultEndpoints() Endpoints {
	return Endpoints{
		Geocode:   graphHopperURL,
		Weather:   openWeatherURL,
		Wikipedia: wikipediaURL,
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
			Timeout: 12 * time.Second,
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

	if c.Keys.WikipediaUserAgent != "" {
		req.Header.Set("User-Agent", c.Keys.WikipediaUserAgent)
	}

	resp, err := c.HTTP.Do(req)
	if err != nil {

		if urlErr, ok := errors.AsType[*url.Error](err); ok {
			err = urlErr.Err
		}

		return fmt.Errorf(
			"request to %s failed: %w",
			u.Host,
			err,
		)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf(
			"request to %s: HTTP %s",
			u.Host,
			resp.Status,
		)
	}

	data, err := io.ReadAll(
		io.LimitReader(resp.Body, maxResponseSize+1),
	)
	if err != nil {
		return fmt.Errorf("read response: %w", err)
	}

	if len(data) > maxResponseSize {
		return fmt.Errorf(
			"response from %s exceeds size limit",
			u.Host,
		)
	}

	if err := json.Unmarshal(data, target); err != nil {
		return fmt.Errorf(
			"decode JSON from %s: %w",
			u.Host,
			err,
		)
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
	radius int,
) ([]models.PlaceSummary, error) {
	if radius < 1 || radius > maxRadius {
		return nil, fmt.Errorf(
			"radius must be between 1 and %d meters",
			maxRadius,
		)
	}

	params := url.Values{}

	coord := strconv.FormatFloat(point.Lat, 'f', -1, 64) +
		"|" +
		strconv.FormatFloat(point.Lng, 'f', -1, 64)

	params.Set("action", "query")
	params.Set("list", "geosearch")
	params.Set("gscoord", coord)
	params.Set("gsradius", strconv.Itoa(radius))
	params.Set("gslimit", placesLimit)
	params.Set("format", "json")
	params.Set("formatversion", "2")

	var response struct {
		Query struct {
			Geosearch []models.PlaceSummary `json:"geosearch"`
		} `json:"query"`

		Error *struct {
			Info string `json:"info"`
		} `json:"error"`
	}

	err := c.getJSON(
		ctx,
		c.Endpoints.Wikipedia,
		params,
		&response,
	)
	if err != nil {
		return nil, fmt.Errorf("wikipedia geosearch: %w", err)
	}

	if response.Error != nil {
		return nil, fmt.Errorf(
			"wikipedia geosearch: %s",
			response.Error.Info,
		)
	}

	return response.Query.Geosearch, nil
}

func (c *Client) PlaceDetails(
	ctx context.Context,
	summary models.PlaceSummary,
) (models.Place, error) {
	params := url.Values{}

	params.Set("action", "query")
	params.Set("prop", "extracts|pageimages")
	params.Set("pageids", strconv.Itoa(summary.PageID))
	params.Set("exintro", "1")
	params.Set("explaintext", "1")
	params.Set("exchars", extractChars)
	params.Set("piprop", "thumbnail")
	params.Set("pithumbsize", thumbSize)
	params.Set("format", "json")
	params.Set("formatversion", "2")

	var response struct {
		Query struct {
			Pages []models.Place `json:"pages"`
		} `json:"query"`

		Error *struct {
			Info string `json:"info"`
		} `json:"error"`
	}

	err := c.getJSON(
		ctx,
		c.Endpoints.Wikipedia,
		params,
		&response,
	)
	if err != nil {
		return models.Place{}, fmt.Errorf(
			"wikipedia page %d: %w",
			summary.PageID,
			err,
		)
	}

	if response.Error != nil {
		return models.Place{}, fmt.Errorf(
			"wikipedia page %d: %s",
			summary.PageID,
			response.Error.Info,
		)
	}

	if len(response.Query.Pages) == 0 {
		return models.Place{}, fmt.Errorf(
			"wikipedia page %d not found",
			summary.PageID,
		)
	}

	place := response.Query.Pages[0]

	if place.PageID == 0 {
		place.PageID = summary.PageID
	}

	if place.Name == "" {
		place.Name = summary.Name
	}

	if place.Thumbnail.Source != "" {
		data, err := c.downloadImage(
			ctx,
			place.Thumbnail.Source,
		)

		if err == nil {
			place.ImageData = data
		}
	}

	return place, nil
}

func (c *Client) downloadImage(
	ctx context.Context,
	rawURL string,
) ([]byte, error) {
	u, err := url.Parse(rawURL)
	if err != nil {
		return nil, err
	}

	if u.Scheme != "https" ||
		u.Hostname() != "upload.wikimedia.org" {
		return nil, errors.New("invalid image host")
	}

	req, err := http.NewRequestWithContext(
		ctx,
		http.MethodGet,
		rawURL,
		nil,
	)
	if err != nil {
		return nil, err
	}

	req.Header.Set(
		"User-Agent",
		c.Keys.WikipediaUserAgent,
	)

	resp, err := c.HTTP.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf(
			"image HTTP %s",
			resp.Status,
		)
	}

	data, err := io.ReadAll(
		io.LimitReader(resp.Body, maxImageSize+1),
	)
	if err != nil {
		return nil, err
	}

	if len(data) > maxImageSize {
		return nil, errors.New("image is too large")
	}

	return data, nil
}
