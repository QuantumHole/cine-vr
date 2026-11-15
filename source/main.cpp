// SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application.h"

int main(void)
{
	Application& app = Application::instance();
	const int status = app.run();

	delete &app;
	return status;
}
