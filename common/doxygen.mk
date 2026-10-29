# SPDX-FileCopyrightText: 2026 QuantumHole <QuantumHole@github.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

CONFIG_FILE = "common/Doxyfile"
BROWSER = xdg-open

doxygen-doc:
	doxygen $(CONFIG_FILE)

doxygen-update:
	doxygen -u $(CONFIG_FILE)

doxygen-show:
	$(BROWSER) html/index.html &
