# SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

LICENSE = "GPL-3.0-or-later"
COPYRIGHT = "QuantumHole <QuantumHole@github.com>"

license-lint:
	reuse lint

license-annotate:
	reuse annotate --year $(shell date "+%Y") --copyright $(COPYRIGHT) --license $(LICENSE) Makefile README.md .gitignore common/*.mk
	reuse annotate --year $(shell date "+%Y") --copyright $(COPYRIGHT) --license $(LICENSE) --style cpp source/* source/*/* shaders/*.glsl actions/*.json
	reuse annotate --year $(shell date "+%Y") --copyright $(COPYRIGHT) --license $(LICENSE) --style python common/*.cfg
	reuse download $(LICENSE)
