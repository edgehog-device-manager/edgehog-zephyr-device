# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

# ------------------------------------------------------------------------------
# Configuration options

BUILDDIR ?= _build
DOC_TAG ?= development
LIBRARY_PATH ?=

# ------------------------------------------------------------------------------
# Documentation targets

.PHONY: configure clean doxygen

doxygen: configure
	cmake --build ${BUILDDIR} --target $@

configure:
	cmake \
		-GNinja \
		-B${BUILDDIR} \
		-S. \
		-DDOC_TAG=${DOC_TAG} \
		-DLIBRARY_PATH=${LIBRARY_PATH} \
		-DEXTENDED_DOCS=${EXTENDED_DOCS}

clean:
	cmake --build ${BUILDDIR} --target clean
