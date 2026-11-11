// Go wrapper smoke test, against tests/standin/server.js.
//
// Copyright (c) 2026 Pumatronix
//
// Same checks as the other languages (tests/README.md).  Environment:
// ITSCAM_TEST_HOST (127.0.0.1), ITSCAM_TEST_PORT (18080).  Build with cgo
// pointing at the core: CGO_CFLAGS=-I<src/core> CGO_LDFLAGS=-L<build dir>.
package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"
	"sync/atomic"
	"time"

	"github.com/pumatronix/itscam-sdk-go/itscam"
)

var failures int

func check(ok bool, name, detail string) {
	status := "PASS"
	if !ok {
		status = "FAIL"
		failures++
	}
	if detail != "" {
		fmt.Printf("%s %s: %s\n", status, name, detail)
	} else {
		fmt.Printf("%s %s\n", status, name)
	}
}

func env(name, fallback string) string {
	if v := os.Getenv(name); v != "" {
		return v
	}
	return fallback
}

func main() {
	host := env("ITSCAM_TEST_HOST", "127.0.0.1")
	port, _ := strconv.Atoi(env("ITSCAM_TEST_PORT", "18080"))
	fmt.Printf("Go smoke against %s:%d\n", host, port)

	version := itscam.GetSDKVersion()
	check(version != "", "version", version)

	rest, err := itscam.NewRestClient()
	if err != nil {
		check(false, "rest create", err.Error())
	} else {
		if err := rest.SetBaseUrl(host, uint16(port), "http"); err != nil {
			check(false, "rest base url", err.Error())
		}
		_, err := rest.Login("admin", "1234", 10000)
		check(err == nil, "rest login", errText(err))

		profiles, err := rest.GetProfiles(10000)
		if err != nil {
			check(false, "rest typed get", err.Error())
		} else {
			ok := len(profiles) == 1 && profiles[0].ID == 3 && profiles[0].Name != nil &&
				*profiles[0].Name == "Câmera São João"
			detail := "no profile"
			if len(profiles) > 0 {
				name := "<unset>"
				if profiles[0].Name != nil {
					name = *profiles[0].Name
				}
				detail = fmt.Sprintf("id %d, name %q", profiles[0].ID, name)
			}
			check(ok, "rest typed get", detail)
		}

		_, err = rest.Get("/api/does-not-exist", 10000)
		check(err != nil && strings.Contains(err.Error(), "invalid parameter"),
			"rest 404 maps to invalid parameter", errText(err))
		rest.Close()
	}

	cgi, err := itscam.NewCgiClient()
	if err != nil {
		check(false, "cgi create", err.Error())
	} else {
		if err := cgi.SetBaseUrl(host, uint16(port), "http"); err != nil {
			check(false, "cgi base url", err.Error())
		}
		images, err := cgi.GetSnapshot(nil, 15000)
		if err != nil || len(images) != 1 {
			check(false, "cgi snapshot + jpeg plate", fmt.Sprintf("%d images, %s", len(images), errText(err)))
		} else {
			plates := itscam.ExtractPlateRecognitions(
				itscam.ParseJpegCommentTags(itscam.ExtractJpegComment(images[0].Data)))
			detail := "no plate"
			if len(plates) > 0 {
				detail = plates[0].Plate
			}
			check(len(plates) == 1 && plates[0].Plate == "ABC1D23", "cgi snapshot + jpeg plate", detail)
		}
		cgi.Close()
	}

	if rest, err := itscam.NewRestClient(); err == nil {
		rest.SetBaseUrl(host, uint16(port), "http")
		rest.Login("admin", "1234", 10000)
		telemetry, err := rest.GetSystemTelemetry(10000)
		if err != nil {
			check(false, "telemetry read", err.Error())
		} else {
			h := telemetry.Health
			ok := h != nil && h.Status != nil && *h.Status == "warning" &&
				len(h.Issues) == 1 && h.Issues[0].ID == "storageFull"
			status := "no health"
			if h != nil && h.Status != nil {
				status = "health " + *h.Status
			}
			check(ok, "telemetry read", status)
		}

		// The stand-in pushes every 200 ms
		var received int32
		err = rest.StartTelemetryStream(func(itscam.TelemetryUpdate) { atomic.AddInt32(&received, 1) }, 0)
		for i := 0; i < 100 && atomic.LoadInt32(&received) < 2; i++ {
			time.Sleep(50 * time.Millisecond)
		}
		rest.StopTelemetryStream()
		latest, ok, lerr := rest.LatestTelemetry()
		check(err == nil && atomic.LoadInt32(&received) >= 2 && lerr == nil && ok && latest.Sequence >= 2,
			"telemetry stream", fmt.Sprintf("%d documents %s", atomic.LoadInt32(&received), errText(err)))
		rest.Close()
	} else {
		check(false, "telemetry", err.Error())
	}

	// Nothing listens on port 1: fails fast and leaves no thread behind
	client, err := itscam.NewClient()
	if err != nil {
		check(false, "binary create", err.Error())
	} else {
		err := client.Connect(host, 1, 2*time.Second)
		check(err != nil, "binary connect to a closed port fails", errText(err))
		client.Close()
	}

	if failures > 0 {
		fmt.Println("Go smoke: FAILED")
		os.Exit(1)
	}
	fmt.Println("Go smoke: OK")
}

func errText(err error) string {
	if err == nil {
		return ""
	}
	return err.Error()
}
