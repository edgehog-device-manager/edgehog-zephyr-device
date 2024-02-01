# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

# ------------------------------------------------------------------------------
# Configuration options

BUILDDIR ?= _build
DOC_TAG ?= development
EDGEHOG_DEVICE_BASE ?=

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
		-DEDGEHOG_DEVICE_BASE=${EDGEHOG_DEVICE_BASE}

clean:
	cmake --build ${BUILDDIR} --target clean
