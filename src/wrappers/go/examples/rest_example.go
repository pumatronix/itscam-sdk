// rest_example.go
//
// ITSCAM SDK REST API example (Go).
//
// Usage:
//
//	go run rest_example.go <host> <user> <password> [--https]
//
// Copyright (c) 2026 Pumatronix
package main

import (
	"flag"
	"fmt"
	"log"
	"os"

	"github.com/pumatronix/itscam-sdk-go/itscam"
)

func main() {
	useHttps := flag.Bool("https", false, "use HTTPS instead of HTTP")
	insecure := flag.Bool("insecure", false,
		"skip TLS server certificate verification")
	flag.Parse()
	if flag.NArg() < 3 {
		fmt.Fprintln(os.Stderr,
			"Usage: rest_example [--https] [--insecure] <host> <user> <pass>")
		os.Exit(1)
	}
	host := flag.Arg(0)
	user := flag.Arg(1)
	pass := flag.Arg(2)

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

	if err := rest.SetBaseUrl(host, port, scheme); err != nil {
		log.Fatal(err)
	}
	if *insecure {
		rest.SetVerifyServerCertificate(false)
	}

	body, err := rest.Login(user, pass, 10000)
	if err != nil {
		log.Fatalf("login failed: %v", err)
	}
	fmt.Println("login OK:", body)

	info, err := rest.Get("/api/equipment/misc/readonly/volatile", 10000)
	if err != nil {
		log.Fatalf("volatile read failed: %v", err)
	}
	fmt.Println("\nvolatile info:", info)
}
