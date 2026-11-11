// Must NOT compile (tools/verify/verify.sh checks it fails, with the SDK's
// message): optional-lite is set to the standard optional before the SDK's
// header, whose structs then have another layout than the library's.
#define optional_CONFIG_SELECT_OPTIONAL optional_OPTIONAL_STD
#include <nonstd/optional.hpp>
#include "itscam_sdk.h"

int main() { return 0; }
