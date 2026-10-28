package main

import (
	"bufio"
	"context"
	"errors"
	"fmt"
	"os"
	"os/signal"
	"strconv"
	"strings"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/api"
	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/models"
	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/service"
)

const (
	graphhopperKey = "GRAPHHOPPER_KEY"
	openweatherKey = "OPENWEATHER_KEY"
	opentripmapKey = "OPENTRIPMAP_KEY"
)

func run() error {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
	)
	defer stop()

	keys := api.Keys{
		GraphHopper: os.Getenv(graphhopperKey),
		OpenWeather: os.Getenv(openweatherKey),
		OpenTripMap: os.Getenv(opentripmapKey),
	}

	if keys.GraphHopper == "" ||
		keys.OpenWeather == "" ||
		keys.OpenTripMap == "" {
		return errors.New("API keys are not configured")
	}

	s := service.Service{
		API: api.New(keys),
	}

	scanner := bufio.NewScanner(os.Stdin)

	fmt.Print("Введите название локации: ")

	if !scanner.Scan() {
		return inputError(scanner)
	}

	query := strings.TrimSpace(scanner.Text())
	if query == "" {
		return errors.New("название локации пустое")
	}

	locationsFuture := s.SearchLocationsAsync(ctx, query)
	locationsResult := <-locationsFuture

	if locationsResult.Err != nil {
		return locationsResult.Err
	}

	locations := locationsResult.Value

	if len(locations) == 0 {
		fmt.Println("Локации не найдены")
		return nil
	}

	fmt.Println("\nНайденные локации:")

	for i, loc := range locations {
		fmt.Printf(
			"%d. %s, %s (%.5f, %.5f)\n",
			i+1,
			loc.Name,
			loc.Country,
			loc.Point.Lat,
			loc.Point.Lng,
		)
	}

	fmt.Print("\nВыберите номер локации: ")

	if !scanner.Scan() {
		return inputError(scanner)
	}

	choice, err := strconv.Atoi(
		strings.TrimSpace(scanner.Text()),
	)

	if err != nil || choice < 1 || choice > len(locations) {
		return errors.New("некорректный номер локации")
	}

	selected := locations[choice-1]

	reportFuture := s.BuildReportAsync(ctx, selected)
	reportResult := <-reportFuture

	if reportResult.Err != nil {
		return reportResult.Err
	}

	printReport(reportResult.Value)

	return nil
}

func inputError(scanner *bufio.Scanner) error {
	if err := scanner.Err(); err != nil {
		return fmt.Errorf("read error: %w", err)
	}
	return errors.New("unexpected end of input")
}

func printReport(report models.Report) {
	fmt.Printf("\n=== %s ===\n", report.Location.Name)

	fmt.Printf(
		"Координаты: %.5f, %.5f\n",
		report.Location.Point.Lat,
		report.Location.Point.Lng,
	)

	fmt.Println("\nПогода:")

	fmt.Printf(
		"Температура: %.1f °C\n",
		report.Weather.Main.Temp,
	)

	fmt.Printf(
		"Ощущается как: %.1f °C\n",
		report.Weather.Main.FeelsLike,
	)

	fmt.Printf(
		"Влажность: %d%%\n",
		report.Weather.Main.Humidity,
	)

	if len(report.Weather.Conditions) > 0 {
		fmt.Println(
			"Описание:",
			report.Weather.Conditions[0].Description,
		)
	}

	fmt.Printf(
		"\nИнтересные места (%d):\n",
		len(report.Places),
	)

	for i, place := range report.Places {
		name := place.Name
		if name == "" {
			name = "Без названия"
		}

		fmt.Printf("\n%d. %s\n", i+1, name)
		fmt.Printf("XID: %s\n", place.XID)
		fmt.Printf("Описание: %s\n", place.Description())
	}
}
