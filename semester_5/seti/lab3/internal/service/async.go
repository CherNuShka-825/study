// Package service
package service

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/reactivex/rxgo/v2"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/api"
	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/models"
)

const reportTimeout = 30 * time.Second

type Result[T any] struct {
	Value T
	Err   error
}

type Future[T any] <-chan Result[T]

type Service struct {
	API *api.Client
}

type weatherResult struct {
	Value models.Weather
	Err   string
}

type placesResult struct {
	Value []models.Place
	Err   string
}

func toFuture[T any](
	ctx context.Context,
	cancel context.CancelFunc,
	stream rxgo.Observable,
) Future[T] {
	output := make(chan Result[T], 1)

	go func() {
		defer close(output)
		defer cancel()

		items := stream.Observe(rxgo.WithContext(ctx))

		select {
		case item, ok := <-items:
			if !ok {
				if err := ctx.Err(); err != nil {
					output <- Result[T]{Err: err}
				} else {
					output <- Result[T]{
						Err: fmt.Errorf("observable completed without a result"),
					}
				}
				return
			}

			if item.Error() {
				output <- Result[T]{Err: item.E}
				return
			}

			value, ok := item.V.(T)
			if !ok {
				output <- Result[T]{
					Err: fmt.Errorf("unexpected result type %T", item.V),
				}
				return
			}

			output <- Result[T]{Value: value}

		case <-ctx.Done():
			output <- Result[T]{Err: ctx.Err()}
		}
	}()

	return output
}

func (s *Service) SearchLocationsAsync(
	parent context.Context,
	query string,
) Future[[]models.Location] {
	ctx, cancel := context.WithCancel(parent)
	stream := s.API.LocationsAsync(ctx, query)
	return toFuture[[]models.Location](ctx, cancel, stream)
}

func (s *Service) SearchLocationsByTypeAsync(
	parent context.Context,
	query string,
	locationType string,
) Future[[]models.Location] {
	if strings.TrimSpace(locationType) == "" {
		return s.SearchLocationsAsync(parent, query)
	}
	ctx, cancel := context.WithCancel(parent)
	stream := s.API.LocationsByTypeAsync(ctx, query, locationType)
	return toFuture[[]models.Location](ctx, cancel, stream)
}

func singleValue(ctx context.Context, value any) rxgo.Observable {
	return rxgo.Start(
		[]rxgo.Supplier{
			func(_ context.Context) rxgo.Item {
				return rxgo.Of(value)
			},
		},
		rxgo.WithContext(ctx),
	)
}

func (s *Service) BuildReportAsync(
	parent context.Context,
	location models.Location,
	radius int,
) Future[models.Report] {
	ctx, cancel := context.WithTimeout(parent, reportTimeout)

	weather := s.API.WeatherAsync(ctx, location.Point).
		Map(
			func(_ context.Context, value any) (any, error) {
				return weatherResult{
					Value: value.(models.Weather),
				}, nil
			},
			rxgo.WithContext(ctx),
		).
		OnErrorReturn(
			func(err error) any {
				return weatherResult{Err: err.Error()}
			},
			rxgo.WithContext(ctx),
		)

	places := s.API.PlacesAsync(ctx, location.Point, radius)

	completePlaces := places.FlatMap(func(item rxgo.Item) rxgo.Observable {
		if item.Error() {
			return rxgo.Thrown(item.E)
		}

		summaries := item.V.([]models.PlaceSummary)
		if len(summaries) == 0 {
			return singleValue(ctx, []models.Place{})
		}

		all := singleValue(ctx, make([]models.Place, len(summaries)))
		for i, summary := range summaries {
			var details rxgo.Observable

			if summary.PageID <= 0 {
				details = rxgo.Just(models.Place{
					PageID:    summary.PageID,
					Name:      summary.Name,
					LoadError: "некорректный PageID",
				})(rxgo.WithContext(ctx))
			} else {
				details = s.API.PlaceDetailsAsync(ctx, summary).
					OnErrorReturn(
						func(err error) any {
							return models.Place{
								PageID:    summary.PageID,
								Name:      summary.Name,
								LoadError: err.Error(),
							}
						},
						rxgo.WithContext(ctx),
					)
			}

			all = all.ZipFromIterable(
				details,
				func(_ context.Context, acc, next any) (any, error) {
					ordered := acc.([]models.Place)
					ordered[i] = next.(models.Place)
					return ordered, nil
				},
				rxgo.WithContext(ctx),
			)
		}
		return all
	}, rxgo.WithContext(ctx))

	completePlaces = completePlaces.
		Map(
			func(_ context.Context, value any) (any, error) {
				found, ok := value.([]models.Place)
				if !ok {
					return nil, fmt.Errorf("unexpected places type %T", value)
				}
				return placesResult{Value: found}, nil
			},
			rxgo.WithContext(ctx),
		).
		OnErrorReturn(
			func(err error) any {
				return placesResult{Err: err.Error()}
			},
			rxgo.WithContext(ctx),
		)

	report := weather.ZipFromIterable(
		completePlaces,
		func(_ context.Context, w, p any) (any, error) {
			if err := ctx.Err(); err != nil {
				return nil, err
			}

			weatherData := w.(weatherResult)
			placesData := p.(placesResult)

			return models.Report{
				Location:     location,
				Weather:      weatherData.Value,
				Places:       placesData.Value,
				WeatherError: weatherData.Err,
				PlacesError:  placesData.Err,
			}, nil
		},
		rxgo.WithContext(ctx),
	)

	return toFuture[models.Report](ctx, cancel, report)
}

func (s *Service) LoadImageAsync(
	parent context.Context,
	imageURL string,
) Future[[]byte] {
	ctx, cancel := context.WithTimeout(parent, 3*time.Second)
	return toFuture[[]byte](ctx, cancel, s.API.ImageAsync(ctx, imageURL))
}
