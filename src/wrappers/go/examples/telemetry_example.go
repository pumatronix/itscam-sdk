// telemetry_example.go
//
// ITSCAM SDK system telemetry example (Go).
//
// Reads the state of the whole camera once, then follows the documents the
// camera pushes for a while.
//
// Usage:
//
//	go run telemetry_example.go [--seconds N] [--https] [--insecure] <host> <user> <password>
//
// Copyright (c) 2026 Pumatronix
package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"time"

	"github.com/pumatronix/itscam-sdk-go/itscam"
)

func printHealth(telemetry itscam.SystemTelemetry) {
	health := telemetry.Health
	if health == nil || health.Status == nil {
		fmt.Println("health: unavailable")
		return
	}
	fmt.Println("health:", *health.Status)
	for _, issue := range health.Issues {
		fmt.Printf("  [%s] %s: %s\n", issue.Severity, issue.ID, issue.Message)
	}
}

func main() {
	seconds := flag.Int("seconds", 30, "how long to follow the stream")
	useHttps := flag.Bool("https", false, "use HTTPS instead of HTTP")
	insecure := flag.Bool("insecure", false,
		"skip TLS server certificate verification")
	flag.Parse()
	if flag.NArg() < 3 {
		fmt.Fprintln(os.Stderr, "Usage: telemetry_example [--seconds N] "+
			"[--https] [--insecure] <host> <user> <pass>")
		os.Exit(1)
	}

	scheme := "http"
	port := uint16(80)
	if *useHttps {
		scheme = "https"
		port = 443
	}

	rest, err := itscam.NewRestClient()
	if err != nil {
		log.Fatal(err)
	}
	defer rest.Close()

	if err := rest.SetBaseUrl(flag.Arg(0), port, scheme); err != nil {
		log.Fatal(err)
	}
	if *insecure {
		rest.SetVerifyServerCertificate(false)
	}
	if _, err := rest.Login(flag.Arg(1), flag.Arg(2), 10000); err != nil {
		log.Fatalf("login failed: %v", err)
	}

	// Polling: one read of the whole camera
	telemetry, err := rest.GetSystemTelemetry(10000)
	if err != nil {
		log.Fatalf("read failed: %v", err)
	}
	printHealth(telemetry)

	// Stream: one document at once, then one every 5 s.  The callback runs
	// on the SDK's callback pool: do not block in it.
	err = rest.StartTelemetryStream(func(update itscam.TelemetryUpdate) {
		fmt.Printf("\nupdate #%d\n", update.Sequence)
		if typed, err := update.Telemetry(); err == nil {
			printHealth(typed)
		}
	}, 0)
	if err != nil {
		log.Fatalf("stream failed: %v", err)
	}
	time.Sleep(time.Duration(*seconds) * time.Second)
	rest.StopTelemetryStream()

	if latest, ok, err := rest.LatestTelemetry(); err == nil && ok {
		fmt.Printf("\nlast document: #%d\n", latest.Sequence)
	}
}
