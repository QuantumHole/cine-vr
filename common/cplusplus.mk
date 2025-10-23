# SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

BUILD_DIR=build
ANALYSIS_DIR=analysis

ifeq ($(PROJECT_ROOT),)
	$(error PROJECT_ROOT is undefined)
endif

ifeq ($(CC), gcc)
	COMPILER ?= gnu
else ifeq ($(CC), cc)
	COMPILER ?= gnu
else ifeq ($(CC), clang)
	COMPILER ?= llvm
else ifeq ($(notdir $(CXX)), c++-analyzer)
#	COMPILER ?= llvm
endif

ifeq ($(CXX), g++)
	COMPILER ?= gnu
else ifeq ($(CXX), clang++)
	COMPILER ?= llvm
else ifeq ($(notdir $(CXX)), ccc-analyzer)
#	COMPILER ?= llvm
endif

ifeq ($(COMPILER), llvm)
	WARNINGS = \
		-Weverything \
		-Wno-padded \
		-Wno-undefined-func-template \
		-Wno-switch-enum \
		-Wno-unsafe-buffer-usage \
		-Wno-covered-switch-default \
		-Wno-error=global-constructors \
		-Wno-error=exit-time-destructors \
		-Wno-error=sign-conversion \
		-Wno-error=shorten-64-to-32 \
		-Wno-error=implicit-int-float-conversion \
		-Wno-error=suggest-destructor-override \
		-Wno-error=unused-macros \
		-Wno-error=suggest-override \
		-Wno-error=inconsistent-missing-destructor-override \
		-Wno-error=zero-as-null-pointer-constant \
		-Wno-error=float-conversion \
		-Wno-error=enum-enum-conversion \
		-Wno-error=disabled-macro-expansion \
		-Wno-c++98-compat \
		-Wno-c++98-compat-pedantic \

else ifeq ($(COMPILER), gnu)
	WARNINGS = \
		-Wall \
		-Wextra \
		-pedantic \
		-Winline \
		-Wformat=2 \
		-Wswitch-default \
		-Wfloat-equal \
		-Wundef \
		-Wshadow \
		-Wwrite-strings \
		-Wmissing-declarations \
		-Wredundant-decls \
		-Wsuggest-attribute=pure \
		-Wsuggest-attribute=const \
		-Wsuggest-attribute=noreturn \
		-Warray-bounds=2 \
		-Wduplicated-cond \
		-Wlogical-op \
		-Wtrampolines \
		-Wformat-signedness \
		-Wconversion \
		-fsanitize=undefined \
		-Waligned-new=all \
		-Walloc-zero \
		-Walloca \
		-Wanalyzer-symbol-too-complex \
		-Wanalyzer-too-complex \
		-Warith-conversion \
		-Wcast-align=strict \
		-Wcast-qual \
		-Wdate-time \
		-Wdisabled-optimization \
		-Wdouble-promotion \
		-Wduplicated-branches \
		-Wflex-array-member-not-at-end \
		-Winit-self \
		-Winline \
		-Winvalid-pch \
		-Winvalid-utf8 \
		-Wmissing-include-dirs \
		-Wmultichar \
		-Wnull-dereference \
		-Wopenacc-parallelism \
		-Wstack-protector \
		-Wsuggest-attribute=cold \
		-Wsuggest-attribute=format \
		-Wsuggest-attribute=malloc \
		-Wsuggest-attribute=returns_nonnull \
		-Wsuggest-final-methods \
		-Wsuggest-final-types \
		-Wtrivial-auto-var-init \
		-Wunused-macros \
		-Wvector-operation-performance \
		# -Wabi \                 # only sensible as e.g. -Wabi=11
		# -Wabi-tag \             # mixing ABI version is required in most cases.
		# -Waggregate-return \    # no return of e.g. vectors
		# -Wpadded \              # padding is so widespread that this flag is not helpful.
		# -Wstrict-flex-arrays \  # only together with -fstrict-flex-arrays
		# -Wswitch-enum \         # avoid listing every enum value
		# -Wsystem-headers \      # This warns about errors/warnings in system headers.

ifndef NO_ASAN
	WARNINGS += \
		-fsanitize=address \

endif

	CFLAGS += \
		-Wbad-function-cast \
		-Wc++-compat \
		-Wjump-misses-init \
		-Wmissing-prototypes \
		-Wmissing-variable-declarations \
		-Wnested-externs \
		-Wold-style-definition \
		-Wstrict-prototypes \
		-Wtraditional \
		-Wtraditional-conversion \
		-Wunsuffixed-float-constants \

	CXXFLAGS += \
		-Weffc++ \
		-Wold-style-cast \
		-Wuseless-cast \
		-Wzero-as-null-pointer-constant \

endif

INCLUDES += \
	-I /usr/include \
	-I. \
	-Isource \

ifneq ($(DEPENDENCIES),)
ifneq ($(DEPENDENCIES),"")
INCLUDES += \
	$(shell pkg-config --cflags $(DEPENDENCIES) | sed -e 's/-I/-isystem /g') \

LDFLAGS += \
	$(shell pkg-config --libs-only-L --libs-only-other $(DEPENDENCIES)) \

LDLIBS += \
	$(shell pkg-config --libs-only-l $(DEPENDENCIES)) \

endif
endif

ifdef NO_ASAN
COMMON_FLAGS += \
	-O2 \

else
COMMON_FLAGS += \
	-g \
	-O0 \

endif

COMMON_FLAGS += \
	-c \
	-Werror \
	$(WARNINGS) \
	$(INCLUDES) \
	# -pipe \      # does not work for target all-warnings/flag -Q

CFLAGS += \
	-std=c99 \
	$(COMMON_FLAGS) \

CXXFLAGS += \
	-std=c++14 \
	$(COMMON_FLAGS) \

LDFLAGS += \
	-L/usr/lib \
	-fsanitize=undefined \

ifndef NO_ASAN
LDFLAGS += \
	-fsanitize=address \

endif

.PHONY: all gnu llvm format clean analyse

all:
	scan-build -o $(ANALYSIS_DIR) --use-cc="$(CC)" --use-c++="$(CXX)" $(MAKE) $(TARGET) COMPILER=$(COMPILER)

gnu:
	CC="gcc" CXX="g++" $(MAKE) all CC="gcc" CXX="g++" COMPILER="gnu"

llvm:
	CC="clang" CXX="clang++" $(MAKE) all CC="clang" CXX="clang++" COMPILER="llvm"

format:
	find $(PROJECT_ROOT) -type f \( -name '*.cpp' -o -name '*.c' -o -name '*.h' \) -exec uncrustify --replace --no-backup -c $(PROJECT_ROOT)/common/uncrustify.cfg {} +

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	rm -rf $(ANALYSIS_DIR)

analyse: clean
	cppcheck --enable=all --force --inconclusive --std=c++03 -I source source

all-defines:
	@echo | $(CXX) -dM -E -x c++ $(CXXFLAGS) - | sort

all-warnings:
	$(CXX) $(CXXFLAGS) -Q --help=warnings

$(BUILD_DIR):
	@echo [dir] $@
	@mkdir -p $@

$(BUILD_DIR)/%.o: source/%.cpp | $(BUILD_DIR)
	@echo [$(CXX)] $<
	@$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: source/%.c | $(BUILD_DIR)
	@echo [$(CC)] $<
	@$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: source/*/%.cpp | $(BUILD_DIR)
	@echo [$(CXX)] $<
	@$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: source/*/%.c | $(BUILD_DIR)
	@echo [$(CC)] $<
	@$(CC) $(CFLAGS) -o $@ $<
