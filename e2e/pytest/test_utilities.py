# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import base64
import bson


def encode_shell_bson(value: object):
    payload = {"v": value}
    bson_payload = bson.dumps(payload)
    return base64.b64encode(bson_payload).decode("ascii")
