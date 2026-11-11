package api

import (
	_ "embed"
	"encoding/json"
	"fmt"
	"strings"
)

//go:embed osm_type_names.json
var osmTypeNamesJSON []byte

var osmTypeNames = func() map[string]string {
	var names map[string]string
	if err := json.Unmarshal(osmTypeNamesJSON, &names); err != nil {
		panic(fmt.Errorf("error configuration osm_type_names.json: %w", err))
	}
	if len(names) == 0 {
		panic("configuration osm_type_names.json is empty")
	}
	return names
}()

func RussianOSMType(key, value string) string {
	key = strings.TrimSpace(key)
	value = strings.TrimSpace(value)
	if name, ok := osmTypeNames[key+":"+value]; ok {
		return name
	}
	return key + "=" + value
}

func RussianLocationTypeTag(locationType string) (string, error) {
	normalize := func(s string) string {
		s = strings.ToLower(strings.TrimSpace(s))
		s = strings.ReplaceAll(s, "ё", "е")
		return s
	}
	locationType = normalize(locationType)
	if locationType == "" {
		return "", nil
	}
	for osmTag, russianName := range osmTypeNames {
		if normalize(russianName) == locationType {
			return osmTag, nil
		}
	}
	return "", fmt.Errorf("unknown location type: %q", locationType)
}
