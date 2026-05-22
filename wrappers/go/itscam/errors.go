package itscam

// Error types for the ITSCAM SDK.

// ConnectionError is returned when a connection error occurs.
type ConnectionError struct {
	msg string
}

func (e *ConnectionError) Error() string {
	return e.msg
}

// TimeoutError is returned when an operation times out.
type TimeoutError struct {
	msg string
}

func (e *TimeoutError) Error() string {
	return e.msg
}

// AuthError is returned when authentication fails.
type AuthError struct {
	msg string
}

func (e *AuthError) Error() string {
	return e.msg
}
