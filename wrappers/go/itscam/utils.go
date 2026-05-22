package itscam

import (
	"os"
	"path/filepath"
)

// StoreFile stores data to a file, creating directories as needed.
func StoreFile(path string, data []byte) bool {
	// Create parent directories if they don't exist
	dir := filepath.Dir(path)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return false
	}
	
	// Write the file
	if err := os.WriteFile(path, data, 0644); err != nil {
		return false
	}
	
	return true
}

// CreateFolder creates a directory, including all parent directories.
func CreateFolder(path string) bool {
	if err := os.MkdirAll(path, 0755); err != nil {
		return false
	}
	return true
}
