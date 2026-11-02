// Package models:
// types for json
package models

const hPaToMMHg = 0.750061683

type Point struct {
	Lat float64 `json:"lat"`
	Lng float64 `json:"lng"`
}

type Location struct {
	Name    string `json:"name"`
	City    string `json:"city"`
	State   string `json:"state"`
	Country string `json:"country"`
	Point   Point  `json:"point"`

	OSMKey   string `json:"osm_key"`
	OSMValue string `json:"osm_value"`
}

func (l Location) Type() string {
	if l.OSMKey != "" && l.OSMValue != "" {
		return l.OSMKey + "=" + l.OSMValue
	}
	if l.OSMValue != "" {
		return l.OSMValue
	}
	return "тип не указан"
}

type GeocodeResponse struct {
	Hits []Location `json:"hits"`
}

type Weather struct {
	Main struct {
		Temp      float64 `json:"temp"`
		FeelsLike float64 `json:"feels_like"`
		Humidity  int     `json:"humidity"`
		Pressure  int     `json:"pressure"`
	} `json:"main"`

	Wind struct {
		Speed float64 `json:"speed"`
	} `json:"wind"`

	Conditions []struct {
		Description string `json:"description"`
	} `json:"weather"`
}

func (w Weather) PressureMMHg() float64 {
	return float64(w.Main.Pressure) * hPaToMMHg
}

type PlaceSummary struct {
	PageID   int     `json:"pageid"`
	Name     string  `json:"title"`
	Lat      float64 `json:"lat"`
	Lon      float64 `json:"lon"`
	Distance float64 `json:"dist"`
}

type Place struct {
	PageID  int    `json:"pageid"`
	Name    string `json:"title"`
	Extract string `json:"extract"`

	Thumbnail struct {
		Source string `json:"source"`
	} `json:"thumbnail"`

	ImageData []byte `json:"-"`
}

func (p Place) Description() string {
	if p.Extract != "" {
		return p.Extract
	}
	return "Описание отсутствует"
}

type Report struct {
	Location Location
	Weather  Weather
	Places   []Place
}
