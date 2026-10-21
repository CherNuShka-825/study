package main

import (
	"fmt"
	"sync/atomic"
	"time"
)

func reportSpeeds(client string, counter *atomic.Uint64, start time.Time, done <-chan struct{}) {
	ticker := time.NewTicker(speedInterval)
	defer ticker.Stop()

	lastTime := start
	var lastBytes uint64

	printSpeed := func() {
		now := time.Now()
		total := counter.Load()
		interval := now.Sub(lastTime).Seconds()
		duration := now.Sub(start).Seconds()
		if interval <= 0 || duration <= 0 {
			return
		}

		currentSpeed := float64(total-lastBytes) / interval
		averageSpeed := float64(total) / duration
		fmt.Printf(
			"[%s] current: %.2f MB/s, average: %.2f MB/s, received: %.2f MB\n",
			client,
			currentSpeed/bytesPerMB,
			averageSpeed/bytesPerMB,
			float64(total)/bytesPerMB,
		)
		lastBytes = total
		lastTime = now
	}

	for {
		select {
		case <-ticker.C:
			printSpeed()
		case <-done:
			if lastTime.Equal(start) || counter.Load() != lastBytes {
				printSpeed()
			}
			return
		}
	}
}
