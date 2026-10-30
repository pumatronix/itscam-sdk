package itscam

/*
#include "../../../core/c_api/itscam_sdk_c.h"
*/
import "C"
import (
	"os"
	"path/filepath"
)

// SetCallbackThreads sets how many threads run client callbacks.  Callbacks
// of every client run on a pool of SDK threads shared by all clients, in
// order and one at a time per client.  count is the maximum number of
// threads; 0 restores the default (the number of CPU cores, at most 4).
func SetCallbackThreads(count uint32) {
	C.ITSCAM_setCallbackThreads(C.uint32_t(count))
}

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
