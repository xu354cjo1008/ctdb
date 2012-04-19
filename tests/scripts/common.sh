# Hey Emacs, this is a -*- shell-script -*- !!!  :-)

# Common variables and functions for all CTDB tests.

export TEST_SUBDIR=$(dirname $0)

CTDB_DIR=$(dirname $(dirname "$TEST_SUBDIR"))

# Print a message and exit.
die ()
{
    echo "$1" >&2 ; exit ${2:-1}
}
