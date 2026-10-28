package models

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
}

type GeocodeResponse struct {
	Hits []Location `json:"hits"`
}

type Weather struct {
	Main struct {
		Temp      float64 `json:"temp"`
		FeelsLike float64 `json:"feels_like"`
		Humidity  int     `json:"humidity"`
	} `json:"main"`

	Conditions []struct {
		Description string `json:"description"`
	} `json:"weather"`
}

type PlaceSummary struct {
	XID   string `json:"xid"`
	Name  string `json:"name"`
	Kinds string `json:"kinds"`
}

type Place struct {
	XID   string `json:"xid"`
	Name  string `json:"name"`
	Kinds string `json:"kinds"`

	WikipediaExtracts struct {
		Text string `json:"text"`
	} `json:"wikipedia_extracts"`

	Info struct {
		Descr string `json:"descr"`
	} `json:"info"`
}

func (p Place) Description() string {
	if p.WikipediaExtracts.Text != "" {
		return p.WikipediaExtracts.Text
	}
	if p.Info.Descr != "" {
		return p.Info.Descr
	}
	return "Описание отсутствует"
}

type Report struct {
	Location Location
	Weather  Weather
	Places   []Place
}
