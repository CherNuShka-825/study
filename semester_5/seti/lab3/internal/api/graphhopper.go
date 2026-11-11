package api

import (
	"context"
	"encoding/json"
	"math"
	"net/url"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/models"
	"github.com/reactivex/rxgo/v2"
)

const (
	graphHopperURL = "https://graphhopper.com/api/1/geocode"
	geocodeLimit   = "5"
)

func (c *Client) LocationsAsync(ctx context.Context, query string) rxgo.Observable {
	return c.locationsAsync(ctx, query, "")
}

func (c *Client) LocationsByTypeAsync(ctx context.Context, query, locationType string) rxgo.Observable {
	osmTag, err := RussianLocationTypeTag(locationType)
	if err != nil {
		return rxgo.Thrown(err)
	}
	return c.locationsAsync(ctx, query, osmTag)
}

func (c *Client) locationsAsync(ctx context.Context, query, osmTag string) rxgo.Observable {
	params := url.Values{}
	params.Set("q", query)
	params.Set("limit", geocodeLimit)
	params.Set("locale", languageRU)
	params.Set("key", c.Keys.GraphHopper)
	if osmTag != "" {
		params.Set("provider", "default")
		params.Set("osm_tag", osmTag)
	}

	type geocodeResponse struct {
		Hits []json.RawMessage `json:"hits"`
	}
	type pointFields struct {
		Lat *float64 `json:"lat"`
		Lng *float64 `json:"lng"`
	}
	type locationFields struct {
		Point *pointFields `json:"point"`
	}

	return getJSONAsync[geocodeResponse](c, ctx, c.Endpoints.Geocode, params, false).
		Map(func(_ context.Context, value any) (any, error) {
			hits := value.(geocodeResponse).Hits
			locations := make([]models.Location, 0, len(hits))

			for _, raw := range hits {
				var check locationFields
				if err := json.Unmarshal(raw, &check); err != nil ||
					check.Point == nil ||
					check.Point.Lat == nil || check.Point.Lng == nil {
					continue
				}

				lat := *check.Point.Lat
				lng := *check.Point.Lng
				if math.IsNaN(lat) || math.IsNaN(lng) ||
					lat < -90 || lat > 90 || lng < -180 || lng > 180 {
					continue
				}

				var location models.Location
				if err := json.Unmarshal(raw, &location); err != nil {
					continue
				}
				if location.Name == "" {
					location.Name = location.City
				}
				if location.Name == "" {
					continue
				}
				locations = append(locations, location)
			}

			return locations, nil
		}, rxgo.WithContext(ctx))
}
