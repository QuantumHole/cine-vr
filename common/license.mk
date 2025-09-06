# SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

LICENSE = "GPL-3.0-or-later"

license-lint:
	reuse lint

license-annotate:
	reuse annotate --year $(shell date "+%Y") --copyright "QuantumHole <QuantumHole@github.com>" --license $(LICENSE) Makefile README.md .gitignore source/* common/*.mk
	reuse annotate --year $(shell date "+%Y") --copyright "QuantumHole <QuantumHole@github.com>" --license $(LICENSE) --style python common/*.cfg
	reuse download $(LICENSE)
