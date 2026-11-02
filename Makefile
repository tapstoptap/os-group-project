CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic
LDLIBS = -pthread
PYTHON = python3

.PHONY: all clean check bench
all: pi_single

pi_single: src/pi_single.c src/common.c src/common.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/pi_single.c src/common.c $(LDFLAGS) $(LDLIBS) -o $@

ifeq ($(words $(wildcard tests/check_common.c tests/verify_baseline.py)),2)
check: pi_single
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc tests/check_common.c src/common.c $(LDFLAGS) $(LDLIBS) -o check_common
	./check_common
	$(PYTHON) tests/verify_baseline.py --binary ./pi_single
	rm -f check_common check_common.exe
else
check:
	@printf '%s\n' 'Local test files are missing. tests/ is intentionally ignored by Git.' 'Restore tests/ to run make check; make and make bench do not require it.' >&2
	@exit 1
endif

bench: pi_single
	$(PYTHON) scripts/benchmark_baseline.py --binary ./pi_single --points 10000000 --seed 42 --runs 5

clean:
	rm -f pi_single pi_single.exe check_common check_common.exe
