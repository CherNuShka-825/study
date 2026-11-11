package main

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"image"
	_ "image/gif"
	_ "image/jpeg"
	_ "image/png"
	"net/url"
	"strconv"
	"strings"

	"fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/canvas"
	"fyne.io/fyne/v2/container"
	"fyne.io/fyne/v2/dialog"
	"fyne.io/fyne/v2/widget"
	_ "golang.org/x/image/webp"

	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/api"
	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/models"
	"github.com/CherNuShka-825/study/semester_5/seti/lab3/internal/service"
)

func runGUI(parent context.Context, s *service.Service) error {
	a := app.NewWithID("org.nsu.placeslab")
	window := a.NewWindow("Места")
	window.Resize(fyne.NewSize(980, 800))

	ctx, cancel := context.WithCancel(parent)
	defer cancel()
	window.SetOnClosed(cancel)

	buildGUI(window, ctx, s)
	window.ShowAndRun()
	return nil
}

func buildGUI(window fyne.Window, ctx context.Context, s *service.Service) {
	queryEntry := widget.NewEntry()
	queryEntry.SetPlaceHolder("Название локации")

	typeEntry := widget.NewEntry()
	typeEntry.SetPlaceHolder("Например: ")

	radiusEntry := widget.NewEntry()
	radiusEntry.SetText(strconv.Itoa(defaultRadius))

	locationSelect := widget.NewSelect(nil, nil)
	locationSelect.PlaceHolder = "Выберите локацию"
	locationSelect.Disable()

	reportArea := container.NewStack()

	searchButton := widget.NewButton("Найти локации", nil)
	reportButton := widget.NewButton("Получить информацию", nil)
	reportButton.Disable()
	cancelButton := widget.NewButton("Отменить запрос", nil)
	cancelButton.Disable()

	var locations []models.Location
	var searchCancel, reportCancel, photoCancel context.CancelFunc
	var searchSeq, reportSeq uint64
	var searching, reporting bool

	cancelSearch := func() {
		searchSeq++
		if searchCancel != nil {
			searchCancel()
			searchCancel = nil
		}
		searching = false
	}

	cancelReport := func() {
		reportSeq++
		if reportCancel != nil {
			reportCancel()
			reportCancel = nil
		}
		reporting = false
	}

	cancelPhotos := func() {
		if photoCancel != nil {
			photoCancel()
			photoCancel = nil
		}
	}

	refreshControls := func() {
		if searching {
			searchButton.SetText("Искать заново")
		} else {
			searchButton.SetText("Найти локации")
		}

		if reporting {
			reportButton.SetText("Загрузить заново")
		} else {
			reportButton.SetText("Получить информацию")
		}

		searchButton.Enable()
		queryEntry.Enable()
		typeEntry.Enable()
		radiusEntry.Enable()

		if searching || len(locations) == 0 {
			locationSelect.Disable()
		} else {
			locationSelect.Enable()
		}

		if !searching && locationSelect.SelectedIndex() >= 0 {
			reportButton.Enable()
		} else {
			reportButton.Disable()
		}

		if searching || reporting {
			cancelButton.Enable()
		} else {
			cancelButton.Disable()
		}
	}

	locationSelect.OnChanged = func(_ string) {
		cancelReport()
		cancelPhotos()
		reportArea.RemoveAll()
		refreshControls()
	}

	searchButton.OnTapped = func() {
		query := strings.TrimSpace(queryEntry.Text)
		locationType := strings.TrimSpace(typeEntry.Text)
		if query == "" {
			dialog.ShowError(errors.New("введите название локации"), window)
			return
		}

		cancelSearch()
		cancelReport()
		cancelPhotos()
		locations = nil
		locationSelect.ClearSelected()
		locationSelect.SetOptions(nil)
		reportArea.RemoveAll()

		searchCtx, stop := context.WithCancel(ctx)
		searchCancel = stop
		seq := searchSeq
		searching = true
		refreshControls()

		go func() {
			result := <-s.SearchLocationsByTypeAsync(searchCtx, query, locationType)
			fyne.Do(func() {
				if ctx.Err() != nil || searchCtx.Err() != nil || seq != searchSeq {
					return
				}
				stop()
				searchCancel = nil
				searching = false

				if result.Err != nil {
					refreshControls()
					dialog.ShowError(result.Err, window)
					return
				}

				locations = result.Value
				if len(locations) == 0 {
					refreshControls()
					dialog.ShowInformation("Поиск", "Локации не найдены", window)
					return
				}

				options := make([]string, len(locations))
				for i, loc := range locations {
					options[i] = fmt.Sprintf("%d. %s, %s — %s", i+1, loc.Name, loc.Country, api.RussianOSMType(loc.OSMKey, loc.OSMValue))
				}
				locationSelect.SetOptions(options)
				refreshControls()
			})
		}()
	}

	reportButton.OnTapped = func() {
		index := locationSelect.SelectedIndex()
		if index < 0 || index >= len(locations) {
			dialog.ShowError(errors.New("выберите локацию"), window)
			return
		}

		radius, err := strconv.Atoi(strings.TrimSpace(radiusEntry.Text))
		if err != nil || radius < 1 || radius > maxRadius {
			dialog.ShowError(fmt.Errorf("радиус должен быть от 1 до %d метров", maxRadius), window)
			return
		}

		cancelReport()
		cancelPhotos()
		reportArea.RemoveAll()

		selected := locations[index]
		reportCtx, stop := context.WithCancel(ctx)
		reportCancel = stop
		seq := reportSeq
		reporting = true
		refreshControls()

		go func() {
			result := <-s.BuildReportAsync(reportCtx, selected, radius)
			fyne.Do(func() {
				if ctx.Err() != nil || reportCtx.Err() != nil || seq != reportSeq {
					return
				}
				stop()
				reportCancel = nil
				reporting = false
				refreshControls()

				if result.Err != nil {
					dialog.ShowError(result.Err, window)
					return
				}

				photoCtx, stopPhotos := context.WithCancel(ctx)
				photoCancel = stopPhotos
				reportArea.Add(createReportView(result.Value, radius, photoCtx, s))
			})
		}()
	}

	cancelButton.OnTapped = func() {
		cancelSearch()
		cancelReport()
		cancelPhotos()
		refreshControls()
	}

	queryEntry.OnSubmitted = func(_ string) {
		searchButton.OnTapped()
	}

	header := container.NewVBox(
		widget.NewLabelWithStyle("Поиск локаций", fyne.TextAlignLeading, fyne.TextStyle{Bold: true}),
		queryEntry,
		widget.NewLabel("Тип локации (необязательно):"),
		typeEntry,
		searchButton,
		widget.NewSeparator(),
		widget.NewLabel("Локация:"),
		locationSelect,
		widget.NewLabel("Радиус поиска (метры):"),
		radiusEntry,
		reportButton,
		cancelButton,
		widget.NewSeparator(),
	)

	window.SetContent(container.NewBorder(header, nil, nil, nil, reportArea))
}

func createReportView(
	report models.Report,
	radius int,
	photoCtx context.Context,
	s *service.Service,
) fyne.CanvasObject {
	locationCard := widget.NewCard(
		"Локация",
		report.Location.Name,
		container.NewVBox(
			widget.NewLabel("Тип: "+api.RussianOSMType(report.Location.OSMKey, report.Location.OSMValue)),
			widget.NewLabel(fmt.Sprintf("Координаты: %.5f, %.5f", report.Location.Point.Lat, report.Location.Point.Lng)),
		),
	)

	var weatherContent fyne.CanvasObject
	weatherSubtitle := ""
	if report.WeatherError != "" {
		weatherSubtitle = "Ошибка загрузки"
		label := widget.NewLabel(report.WeatherError)
		label.Wrapping = fyne.TextWrapWord
		weatherContent = label
	} else {
		weather := report.Weather
		if len(weather.Conditions) > 0 {
			weatherSubtitle = weather.Conditions[0].Description
		}
		weatherContent = container.NewVBox(
			widget.NewLabel(fmt.Sprintf("Температура: %.1f °C (ощущается %.1f °C)", weather.Main.Temp, weather.Main.FeelsLike)),
			widget.NewLabel(fmt.Sprintf("Влажность: %d%%; ветер: %.1f м/с", weather.Main.Humidity, weather.Wind.Speed)),
			widget.NewLabel(fmt.Sprintf("Давление: %.0f мм рт. ст.", weather.PressureMMHg())),
		)
	}
	weatherCard := widget.NewCard("Погода", weatherSubtitle, weatherContent)

	top := container.NewVBox(
		container.NewGridWithColumns(2, locationCard, weatherCard),
		widget.NewLabelWithStyle(
			fmt.Sprintf("Интересные места в радиусе %d м: %d", radius, len(report.Places)),
			fyne.TextAlignLeading,
			fyne.TextStyle{Bold: true},
		),
	)

	if report.PlacesError != "" {
		errLabel := widget.NewLabel("Ошибка поиска мест: " + report.PlacesError)
		errLabel.Wrapping = fyne.TextWrapWord
		top.Add(errLabel)
	}

	if failed := report.FailedDescriptions(); failed > 0 {
		top.Add(widget.NewLabel(fmt.Sprintf(
			"Не удалось загрузить описания: %d из %d",
			failed, len(report.Places),
		)))
	}

	if len(report.Places) == 0 {
		if report.PlacesError == "" {
			top.Add(widget.NewLabel("Места не найдены"))
		}
		return top
	}

	loader := &photoLoader{
		ctx:     photoCtx,
		service: s,
		cache:   make(map[int]*photoCacheEntry),
	}

	details := container.NewVBox(widget.NewLabel("Нажмите на место в списке"))
	selectedIndex := -1

	placesList := widget.NewList(
		func() int { return len(report.Places) },
		func() fyne.CanvasObject { return widget.NewLabel("Место") },
		func(id widget.ListItemID, object fyne.CanvasObject) {
			name := report.Places[id].Name
			if name == "" {
				name = "Без названия"
			}
			object.(*widget.Label).SetText(fmt.Sprintf("%d. %s", id+1, name))
		},
	)

	placesList.OnSelected = func(id widget.ListItemID) {
		if id < 0 || id >= len(report.Places) {
			return
		}
		selectedIndex = id
		place := report.Places[id]
		name := place.Name
		if name == "" {
			name = "Без названия"
		}

		title := widget.NewLabelWithStyle(name, fyne.TextAlignLeading, fyne.TextStyle{Bold: true})
		description := widget.NewLabel(place.Description())
		description.Wrapping = fyne.TextWrapWord

		objects := []fyne.CanvasObject{title, description}
		if place.PageID > 0 {
			articleURL := &url.URL{
				Scheme: "https",
				Host:   "ru.wikipedia.org",
				Path:   "/",
			}
			q := articleURL.Query()
			q.Set("curid", strconv.Itoa(place.PageID))
			articleURL.RawQuery = q.Encode()
			objects = append(objects, widget.NewHyperlink("Открыть статью Wikipedia", articleURL))
		}

		imageArea := container.NewVBox()
		objects = append(objects, imageArea)
		details.Objects = objects
		details.Refresh()

		if place.Thumbnail.Source == "" {
			imageArea.Add(widget.NewLabel("Фотография отсутствует"))
			return
		}

		imageArea.Add(widget.NewLabel("Загрузка фотографии..."))
		loader.load(place, func(img image.Image, err error) {
			if selectedIndex != id || photoCtx.Err() != nil {
				return
			}
			imageArea.RemoveAll()
			if err != nil {
				imageArea.Add(widget.NewLabel("Не удалось загрузить фотографию: " + err.Error()))
				return
			}

			picture := canvas.NewImageFromImage(img)
			picture.FillMode = canvas.ImageFillContain
			picture.SetMinSize(fyne.NewSize(300, 210))
			imageArea.Add(picture)
		})
	}

	split := container.NewHSplit(placesList, container.NewVScroll(details))
	split.Offset = 0.36

	return container.NewBorder(top, nil, nil, nil, split)
}

type photoLoader struct {
	ctx     context.Context
	service *service.Service
	cache   map[int]*photoCacheEntry
}

type photoCacheEntry struct {
	loading   bool
	image     image.Image
	err       error
	callbacks []func(image.Image, error)
}

func (l *photoLoader) load(place models.Place, callback func(image.Image, error)) {
	if l.ctx.Err() != nil {
		return
	}

	if entry, exists := l.cache[place.PageID]; exists {
		if entry.loading {
			entry.callbacks = append(entry.callbacks, callback)
		} else {
			callback(entry.image, entry.err)
		}
		return
	}

	entry := &photoCacheEntry{
		loading:   true,
		callbacks: []func(image.Image, error){callback},
	}
	l.cache[place.PageID] = entry

	go func() {
		result := <-l.service.LoadImageAsync(l.ctx, place.Thumbnail.Source)
		err := result.Err
		var decoded image.Image
		if err == nil {
			decoded, _, err = image.Decode(bytes.NewReader(result.Value))
		}
		if l.ctx.Err() != nil {
			return
		}

		fyne.Do(func() {
			if l.ctx.Err() != nil {
				return
			}
			entry.loading = false
			entry.image = decoded
			entry.err = err
			for _, done := range entry.callbacks {
				done(decoded, err)
			}
			entry.callbacks = nil
		})
	}()
}
