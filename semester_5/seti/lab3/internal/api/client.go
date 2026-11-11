// Package api: api work
package api

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/url"
	"time"

	"github.com/jzx17/gofetch"
	"github.com/reactivex/rxgo/v2"
)

const (
	maxResponseSize      int64 = 1024 * 1024
	maxImageSize         int64 = 2 * 1024 * 1024
	maxConcurrentDetails       = 2
	languageRU                 = "ru"
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
	HTTP        *gofetch.Client
	Keys        Keys
	Endpoints   Endpoints
	detailSlots chan struct{}
}

func New(keys Keys) *Client {
	return &Client{
		HTTP: gofetch.NewClient(
			gofetch.WithTimeout(12*time.Second),
			gofetch.WithAutoBufferResponse(false),
		),
		Keys:        keys,
		Endpoints:   DefaultEndpoints(),
		detailSlots: make(chan struct{}, maxConcurrentDetails),
	}
}

func (c *Client) getBytesAsync(
	ctx context.Context,
	rawURL string,
	params url.Values,
	limit int64,
	limitedDetails bool,
) rxgo.Observable {
	u, err := url.Parse(rawURL)
	if err != nil {
		return rxgo.Thrown(fmt.Errorf("invalid URL: %w", err))
	}

	q := u.Query()
	for key, values := range params {
		for _, value := range values {
			q.Add(key, value)
		}
	}
	u.RawQuery = q.Encode()

	items := make(chan rxgo.Item, 1)
	go func() {
		defer close(items)
		emit := func(item rxgo.Item) {
			select {
			case items <- item:
			case <-ctx.Done():
			}
		}

		if limitedDetails {
			select {
			case c.detailSlots <- struct{}{}:
				defer func() { <-c.detailSlots }()
			case <-ctx.Done():
				return
			}
		}

		headers := map[string]string{}
		if c.Keys.WikipediaUserAgent != "" {
			headers["User-Agent"] = c.Keys.WikipediaUserAgent
		}

		responseFuture := c.HTTP.GetAsync(ctx, u.String(), headers)

		select {
		case <-ctx.Done():
			return
		case result, ok := <-responseFuture:
			if !ok {
				emit(rxgo.Error(fmt.Errorf("empty HTTP result from %s", u.Host)))
				return
			}
			if result.Error != nil {
				emit(rxgo.Error(fmt.Errorf("request to %s: %w", u.Host, result.Error)))
				return
			}
			if result.Response == nil {
				emit(rxgo.Error(fmt.Errorf("empty HTTP response from %s", u.Host)))
				return
			}

			resp := result.Response
			defer resp.CloseBody()
			if !resp.IsSuccess() {
				emit(rxgo.Error(fmt.Errorf("request to %s: HTTP %s", u.Host, resp.Status)))
				return
			}

			data, err := io.ReadAll(io.LimitReader(resp.Body, limit+1))
			if err != nil {
				emit(rxgo.Error(fmt.Errorf("read response from %s: %w", u.Host, err)))
				return
			}
			if int64(len(data)) > limit {
				emit(rxgo.Error(fmt.Errorf("response from %s exceeds %d bytes", u.Host, limit)))
				return
			}
			emit(rxgo.Of(data))
		}
	}()

	return rxgo.FromChannel(items, rxgo.WithContext(ctx))
}

func getJSONAsync[T any](
	c *Client,
	ctx context.Context,
	rawURL string,
	params url.Values,
	limitedDetails bool,
) rxgo.Observable {
	return c.getBytesAsync(ctx, rawURL, params, maxResponseSize, limitedDetails).
		Map(func(_ context.Context, value any) (any, error) {
			var target T
			if err := json.Unmarshal(value.([]byte), &target); err != nil {
				return nil, fmt.Errorf("decode JSON: %w", err)
			}
			return target, nil
		}, rxgo.WithContext(ctx))
}
