/*
 * ITSCAM SDK Go GUI Example
 *
 * A desktop GUI application built with Wails to demonstrate
 * the ITSCAM SDK Go wrapper capabilities.
 *
 * Features:
 * - Connect to ITSCAM cameras
 * - View live capture events
 * - Display captured images
 * - Show device information
 *
 * Copyright (c) 2026 Pumatronix
 */

package main

import (
	"embed"

	"github.com/wailsapp/wails/v2"
	"github.com/wailsapp/wails/v2/pkg/options"
	"github.com/wailsapp/wails/v2/pkg/options/assetserver"
	"github.com/wailsapp/wails/v2/pkg/options/linux"
	"github.com/wailsapp/wails/v2/pkg/options/windows"
)

//go:embed all:frontend
var assets embed.FS

func main() {
	// Create application instance
	app := NewApp()

	// Create application with options
	err := wails.Run(&options.App{
		Title:  "ITSCAM Viewer",
		Width:  1024,
		Height: 768,
		AssetServer: &assetserver.Options{
			Assets: assets,
		},
		BackgroundColour: &options.RGBA{R: 27, G: 38, B: 54, A: 1},
		OnStartup:        app.startup,
		OnShutdown:       app.shutdown,
		Bind: []interface{}{
			app,
		},
		Windows: &windows.Options{
			WebviewIsTransparent: false,
			WindowIsTranslucent:  false,
			DisableWindowIcon:    false,
		},
		Linux: &linux.Options{
			ProgramName: "itscam-viewer",
		},
	})

	if err != nil {
		println("Error:", err.Error())
	}
}
