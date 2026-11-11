package api

import (
	"context"
	"errors"
	"fmt"
	"net/url"
	"strconv"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/models"
	"github.com/reactivex/rxgo/v2"
)

const (
	wikipediaURL = "https://ru.wikipedia.org/w/api.php"
	placesLimit  = "10"
	extractChars = "500"
	thumbSize    = "240"
	maxRadius    = 10000
)

type wikipediaError struct {
	Info string `json:"info"`
}

type geoSearchResponse struct {
	Query struct {
		Geosearch []models.PlaceSummary `json:"geosearch"`
	} `json:"query"`
	Error *wikipediaError `json:"error"`
}

func (c *Client) PlacesAsync(ctx context.Context, point models.Point, radius int) rxgo.Observable {
	if radius < 1 || radius > maxRadius {
		return rxgo.Thrown(fmt.Errorf("radius must be between 1 and %d meters", maxRadius))
	}

	params := url.Values{}
	coord := strconv.FormatFloat(point.Lat, 'f', -1, 64) + "|" +
		strconv.FormatFloat(point.Lng, 'f', -1, 64)
	params.Set("action", "query")
	params.Set("list", "geosearch")
	params.Set("gscoord", coord)
	params.Set("gsradius", strconv.Itoa(radius))
	params.Set("gslimit", placesLimit)
	params.Set("format", "json")
	params.Set("formatversion", "2")

	return getJSONAsync[geoSearchResponse](c, ctx, c.Endpoints.Wikipedia, params, false).
		Map(func(_ context.Context, value any) (any, error) {
			response := value.(geoSearchResponse)
			if response.Error != nil {
				return nil, fmt.Errorf("wikipedia geosearch: %s", response.Error.Info)
			}
			return response.Query.Geosearch, nil
		}, rxgo.WithContext(ctx))
}

type pageResponse struct {
	Query struct {
		Pages []models.Place `json:"pages"`
	} `json:"query"`
	Error *wikipediaError `json:"error"`
}

func (c *Client) PlaceDetailsAsync(ctx context.Context, summary models.PlaceSummary) rxgo.Observable {
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

	return getJSONAsync[pageResponse](c, ctx, c.Endpoints.Wikipedia, params, true).
		Map(func(_ context.Context, value any) (any, error) {
			response := value.(pageResponse)
			if response.Error != nil {
				return nil, fmt.Errorf("wikipedia page %d: %s", summary.PageID, response.Error.Info)
			}
			if len(response.Query.Pages) == 0 {
				return nil, fmt.Errorf("wikipedia page %d not found", summary.PageID)
			}

			place := response.Query.Pages[0]
			if place.PageID == 0 {
				place.PageID = summary.PageID
			}
			if place.Name == "" {
				place.Name = summary.Name
			}
			return place, nil
		}, rxgo.WithContext(ctx))
}

func (c *Client) ImageAsync(ctx context.Context, rawURL string) rxgo.Observable {
	u, err := url.Parse(rawURL)
	if err != nil {
		return rxgo.Thrown(err)
	}
	if u.Scheme != "https" ||
		(u.Hostname() != "upload.wikimedia.org" &&
			u.Hostname() != "thumb.wikimedia.org") {
		return rxgo.Thrown(errors.New("invalid image host"))
	}

	return c.getBytesAsync(ctx, rawURL, nil, maxImageSize, false)
}
