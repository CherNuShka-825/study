package main

import (
	"bufio"
	"context"
	"errors"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/api"
	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/models"
	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/service"
)

func runConsole(ctx context.Context, s *service.Service) error {
	scanner := bufio.NewScanner(os.Stdin)

	query, err := askConsole(scanner, "Название локации: ")
	if err != nil {
		return err
	}
	if query == "" {
		return errors.New("название локации не может быть пустым")
	}

	locationType, err := askConsole(scanner, "Тип локации: ")
	if err != nil {
		return err
	}

	locationsFuture := s.SearchLocationsByTypeAsync(ctx, query, locationType)
	var locations []models.Location
	select {
	case result, ok := <-locationsFuture:
		if !ok {
			return errors.New("поиск локаций завершился без результата")
		}
		if result.Err != nil {
			return result.Err
		}
		locations = result.Value
	case <-ctx.Done():
		return ctx.Err()
	}

	if len(locations) == 0 {
		fmt.Println("Локации не найдены")
		return nil
	}

	fmt.Println("Найденные локации:")
	for i, location := range locations {
		fmt.Printf("%d. %s, %s — %s\n", i+1, location.Name, location.Country, api.RussianOSMType(location.OSMKey, location.OSMValue))
	}

	var selected int
	for {
		line, err := askConsole(scanner, "Номер локации: ")
		if err != nil {
			return err
		}

		selected, err = strconv.Atoi(line)
		if err == nil && selected >= 1 && selected <= len(locations) {
			break
		}
		fmt.Printf("Введите число от 1 до %d\n", len(locations))
	}

	var radius int
	for {
		line, err := askConsole(scanner, fmt.Sprintf(
			"Радиус поиска мест (1–%d м, Enter — %d): ",
			maxRadius, defaultRadius,
		))
		if err != nil {
			return err
		}

		if line == "" {
			radius = defaultRadius
			break
		}

		radius, err = strconv.Atoi(line)
		if err == nil && radius >= 1 && radius <= maxRadius {
			break
		}
		fmt.Printf("Радиус должен быть целым числом от 1 до %d\n", maxRadius)
	}

	reportFuture := s.BuildReportAsync(ctx, locations[selected-1], radius)
	select {
	case result, ok := <-reportFuture:
		if !ok {
			return errors.New("формирование отчёта завершилось без результата")
		}
		if result.Err != nil {
			return result.Err
		}
		printConsoleReport(result.Value, radius)
		return nil
	case <-ctx.Done():
		return ctx.Err()
	}
}

func askConsole(scanner *bufio.Scanner, prompt string) (string, error) {
	fmt.Print(prompt)
	if scanner.Scan() {
		return strings.TrimSpace(scanner.Text()), nil
	}
	if err := scanner.Err(); err != nil {
		return "", err
	}
	return "", io.EOF
}

func printConsoleReport(report models.Report, radius int) {
	fmt.Printf("\nЛокация: %s\n", report.Location.Name)
	fmt.Printf("Тип: %s\n", api.RussianOSMType(report.Location.OSMKey, report.Location.OSMValue))
	fmt.Printf("Координаты: %.5f, %.5f\n", report.Location.Point.Lat, report.Location.Point.Lng)

	fmt.Println("\nПогода:")
	if report.WeatherError != "" {
		fmt.Println("Ошибка загрузки погоды:", report.WeatherError)
	} else {
		weather := report.Weather
		if len(weather.Conditions) != 0 {
			fmt.Println("Условия:", weather.Conditions[0].Description)
		}
		fmt.Printf("Температура: %.1f °C, ощущается как %.1f °C\n", weather.Main.Temp, weather.Main.FeelsLike)
		fmt.Printf("Влажность: %d%%, ветер: %.1f м/с\n", weather.Main.Humidity, weather.Wind.Speed)
		fmt.Printf("Давление: %.0f мм рт. ст.\n", weather.PressureMMHg())
	}

	fmt.Printf("\nМеста в радиусе %d м:\n", radius)
	if report.PlacesError != "" {
		fmt.Println("Ошибка поиска мест:", report.PlacesError)
		return
	}
	if len(report.Places) == 0 {
		fmt.Println("Места не найдены")
		return
	}
	for i, place := range report.Places {
		name := place.Name
		if name == "" {
			name = "Без названия"
		}
		fmt.Printf("\n%d. %s\n", i+1, name)
		fmt.Println(place.Description())
	}
	if failed := report.FailedDescriptions(); failed > 0 {
		fmt.Printf("\nНе удалось получить описания: %d из %d\n", failed, len(report.Places))
	}
}
