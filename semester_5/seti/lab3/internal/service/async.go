// Package service
package service

import (
	"context"
	"fmt"
	"sync"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/api"
	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/models"
)

const maxConcurrentDetails = 2

type Result[T any] struct {
	Value T
	Err   error
}

type Future[T any] <-chan Result[T]

type Service struct {
	API *api.Client
}

func (s *Service) SearchLocationsAsync(
	ctx context.Context,
	query string,
) Future[[]models.Location] {
	ch := make(chan Result[[]models.Location], 1)

	go func() {
		defer close(ch)

		locations, err := s.API.Locations(ctx, query)

		ch <- Result[[]models.Location]{
			Value: locations,
			Err:   err,
		}
	}()

	return ch
}

func (s *Service) BuildReportAsync(
	parent context.Context,
	location models.Location,
	radius int,
) Future[models.Report] {
	ch := make(chan Result[models.Report], 1)

	go func() {
		defer close(ch)

		if radius < 1 || radius > 10000 {
			ch <- Result[models.Report]{
				Err: fmt.Errorf(
					"radius must be between 1 and 10000 meters",
				),
			}
			return
		}

		ctx, cancel := context.WithCancel(parent)
		defer cancel()

		report := models.Report{
			Location: location,
		}

		var wg sync.WaitGroup
		var once sync.Once
		var firstErr error

		fail := func(err error) {
			if err == nil {
				return
			}

			once.Do(func() {
				firstErr = err
				cancel()
			})
		}

		wg.Add(2)

		go func() {
			defer wg.Done()

			weather, err := s.API.Weather(
				ctx,
				location.Point,
			)
			if err != nil {
				fail(err)
				return
			}

			report.Weather = weather
		}()

		go func() {
			defer wg.Done()

			s.loadPlaces(
				ctx,
				location.Point,
				radius,
				&report,
				&wg,
				fail,
			)
		}()

		wg.Wait()

		if firstErr == nil && parent.Err() != nil {
			firstErr = parent.Err()
		}

		ch <- Result[models.Report]{
			Value: report,
			Err:   firstErr,
		}
	}()

	return ch
}

func (s *Service) loadPlaces(
	ctx context.Context,
	point models.Point,
	radius int,
	report *models.Report,
	wg *sync.WaitGroup,
	fail func(error),
) {
	places, err := s.API.Places(ctx, point, radius)
	if err != nil {
		fail(err)
		return
	}

	report.Places = make([]models.Place, len(places))

	sem := make(chan struct{}, maxConcurrentDetails)

	for i, summary := range places {
		wg.Add(1)

		go func(i int, summary models.PlaceSummary) {
			defer wg.Done()

			if summary.PageID <= 0 {
				fail(fmt.Errorf(
					"place %d has invalid PageID",
					i+1,
				))
				return
			}

			select {
			case sem <- struct{}{}:
				defer func() {
					<-sem
				}()

			case <-ctx.Done():
				return
			}

			place, err := s.API.PlaceDetails(
				ctx,
				summary,
			)
			if err != nil {
				fail(err)
				return
			}

			report.Places[i] = place
		}(i, summary)
	}
}
