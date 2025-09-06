# SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

LINTERS += lint_makefile
# FILES = $(shell )

.PHONY: lint_makefile

lint_makefile:
	if [ "$$(find . \( -name '*.mk' -o -name 'Makefile' \) -exec grep -e '^ ' {} \;)" != "" ]; then echo "ERROR: line in Makefile starts with space character."; exit 1; fi
	if [ "$$(find . \( -name '*.mk' -o -name 'Makefile' \) -exec grep -e '\r' {} \;)" != "" ]; then echo "ERROR: CR character in Makefile."; exit 1; fi
