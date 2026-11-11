package main

import (
	"context"
	"errors"
	"flag"
	"os"
	"os/signal"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/api"
	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/service"
)

const (
	graphhopperKey     = "GRAPHHOPPER_KEY"
	openweatherKey     = "OPENWEATHER_KEY"
	wikipediaUserAgent = "WIKIPEDIA_USER_AGENT"

	defaultRadius = 3000
	maxRadius     = 10000
)

func run() error {
	console := flag.Bool(
		"console",
		false,
		"run in console mode",
	)
	flag.Parse()

	keys := api.Keys{
		GraphHopper:        os.Getenv(graphhopperKey),
		OpenWeather:        os.Getenv(openweatherKey),
		WikipediaUserAgent: os.Getenv(wikipediaUserAgent),
	}

	if keys.GraphHopper == "" ||
		keys.OpenWeather == "" ||
		keys.WikipediaUserAgent == "" {
		return errors.New("API configuration is missing")
	}

	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
	)
	defer stop()

	s := &service.Service{
		API: api.New(keys),
	}

	if *console {
		return runConsole(ctx, s)
	}

	return runGUI(ctx, s)
}
