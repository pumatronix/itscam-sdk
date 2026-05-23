// cgi_snapshot_example.go
//
// ITSCAM SDK CGI client example (Go).  Triggers a snapshot.cgi capture,
// writes each resulting image to disk, fetches a lastframe.cgi preview
// and streams MJPEG for 5 seconds.
//
// CGI endpoints are unauthenticated by default on the camera
// (configCgi.blockAPI=false), so --user and --password are optional;
// pass them only when the camera has CGI auth turned on.
//
// Usage:
//
//	go run cgi_snapshot_example.go [--https] [--insecure] \
//	    [--user U --password P] <host>
//
// Copyright (c) 2026 Pumatronix
package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"sync/atomic"
	"time"

	"github.com/pumatronix/itscam-sdk-go/itscam"
)

func main() {
	useHttps := flag.Bool("https", false, "use HTTPS instead of HTTP")
	insecure := flag.Bool("insecure", false,
		"skip TLS server certificate verification")
	user := flag.String("user", "",
		"opt-in CGI auth username (omit when blockAPI=false)")
	pass := flag.String("password", "", "opt-in CGI auth password")
	flag.Parse()
	if flag.NArg() < 1 {
		fmt.Fprintln(os.Stderr,
			"Usage: cgi_snapshot_example [--https] [--insecure] "+
				"[--user U --password P] <host>")
		os.Exit(1)
	}
	host := flag.Arg(0)

	scheme := "http"
	port := uint16(80)
	if *useHttps {
		scheme = "https"
		port = 443
	}

	cgi, err := itscam.NewCgiClient()
	if err != nil {
		log.Fatal(err)
	}
	defer cgi.Close()

	if err := cgi.SetBaseUrl(host, port, scheme); err != nil {
		log.Fatal(err)
	}
	if *insecure {
		cgi.SetVerifyServerCertificate(false)
	}
	if *user != "" && *pass != "" {
		if err := cgi.Login(*user, *pass, 10000); err != nil {
			log.Fatalf("login failed: %v", err)
		}
		fmt.Printf("Logged in as %q\n", *user)
	} else {
		fmt.Println("No credentials supplied; talking to the camera " +
			"without CGI authentication.")
	}

	fmt.Println("Fetching lastframe.cgi...")
	last, err := cgi.GetLastFrame(10000)
	if err != nil {
		log.Fatalf("lastframe.cgi failed: %v", err)
	}
	if err := os.WriteFile("lastframe.jpg", last.Data, 0644); err != nil {
		log.Fatal(err)
	}
	fmt.Printf("  -> lastframe.jpg (%s, %d bytes)\n",
		last.MimeType, len(last.Data))

	fmt.Println("Triggering snapshot.cgi (Q=80, mosaic off)...")
	req := itscam.NewSnapshotCgiRequest()
	req.Quality = 80
	imgs, err := cgi.GetSnapshot(req, 15000)
	if err != nil {
		log.Fatalf("snapshot.cgi failed: %v", err)
	}
	fmt.Printf("  -> received %d image(s)\n", len(imgs))
	for i, img := range imgs {
		path := fmt.Sprintf("snapshot-%d.jpg", i)
		if err := os.WriteFile(path, img.Data, 0644); err != nil {
			log.Fatal(err)
		}
		fmt.Printf("     %s: %s, %d bytes\n", path,
			img.MimeType, len(img.Data))
	}

	fmt.Println("Streaming MJPEG for 5 seconds...")
	var frames int64
	if err := cgi.StartMjpegStream(func(f itscam.CgiStreamFrame) {
		n := atomic.AddInt64(&frames, 1)
		if n == 1 {
			_ = os.WriteFile("mjpeg-first.jpg", f.Data, 0644)
		}
	}, 10000); err != nil {
		log.Fatalf("startMjpegStream failed: %v", err)
	}
	time.Sleep(5 * time.Second)
	cgi.StopMjpegStream()
	fmt.Printf("  -> received %d MJPEG frame(s)\n",
		atomic.LoadInt64(&frames))
}
