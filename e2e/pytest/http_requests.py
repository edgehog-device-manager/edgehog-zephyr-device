# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

"""
Contains useful wrappers for HTTPS requests.
"""

import base64
import json
import logging

import curlify
import requests
from dateutil import parser
from datetime import datetime

from configuration import Configuration

logger = logging.getLogger(__name__)


def http_get_server_data(
    e2e_cfg: Configuration,
    interface: str,
    path: str | None = None,
    quiet: bool = False,
    limit: int | None = None,
    since_after: datetime | None = None,
    to: datetime | None = None,
) -> object:
    url = (
        e2e_cfg.appengine_url
        + "/v1/"
        + e2e_cfg.realm
        + "/devices/"
        + e2e_cfg.device_id
        + "/interfaces/"
        + interface
    )

    if path is not None:
        url += path

    headers = {"Authorization": "Bearer " + e2e_cfg.appengine_token}

    params: dict[str, str] = {}

    if since_after is not None:
        params["since_after"] = since_after.isoformat()

    if limit is not None:
        params["limit"] = str(limit)

    if to is not None:
        params["to"] = to.isoformat()

    res = requests.get(
        url, headers=headers, params=params, timeout=5, verify=e2e_cfg.appengine_cert
    )
    logger.info(curlify.to_curl(res.request))
    if res.status_code != 200:
        if not quiet:
            logger.error(res.text)
        raise requests.HTTPError(f"GET request failed. response {res}")

    return res.json().get("data", {})


def http_post_server_data(
    e2e_cfg: Configuration, interface: str, endpoint: str, data: dict, quiet: bool = False
):
    url = (
        e2e_cfg.appengine_url
        + "/v1/"
        + e2e_cfg.realm
        + "/devices/"
        + e2e_cfg.device_id
        + "/interfaces/"
        + interface
        + endpoint
    )
    json_data = json.dumps({"data": data}, default=str)
    headers = {
        "Authorization": "Bearer " + e2e_cfg.appengine_token,
        "Content-Type": "application/json",
    }
    res = requests.post(
        url=url, data=json_data, headers=headers, timeout=5, verify=e2e_cfg.appengine_cert
    )
    logger.info(curlify.to_curl(res.request))
    if res.status_code != 200:
        if not quiet:
            logger.error(res.text)
        raise requests.HTTPError(f"POST request failed. response {res}")


def http_delete_server_data(
    e2e_cfg: Configuration, interface: str, endpoint: str, quiet: bool = False
):
    url = (
        e2e_cfg.appengine_url
        + "/v1/"
        + e2e_cfg.realm
        + "/devices/"
        + e2e_cfg.device_id
        + "/interfaces/"
        + interface
        + endpoint
    )
    headers = {
        "Authorization": "Bearer " + e2e_cfg.appengine_token,
        "Content-Type": "application/json",
    }
    res = requests.delete(url, headers=headers, timeout=5, verify=e2e_cfg.appengine_cert)
    logger.info(curlify.to_curl(res.request))
    if res.status_code != 204:
        if not quiet:
            logger.error(res.text)
        raise requests.HTTPError("DELETE request failed.")


def http_prepare_transmit_data(interface, path, value):
    logger.info(f"Preparing transmit data, interface {interface}, path {path}, value {value}")
    mapping = interface.get_mapping(path)
    if mapping.type == "binaryblob":
        return base64.b64encode(value).decode("utf-8")
    if mapping.type == "binaryblobarray":
        return [base64.b64encode(v).decode("utf-8") for v in value]
    return value


def http_decode_received_data(interface, path, received_data):
    logger.info(
        f"Decoding received data, interface {interface}, path {path}, value {received_data}"
    )
    mapping = interface.get_mapping(path)
    if (received_data == None) and (mapping.type.endswith("array")):
        return []
    if mapping.type == "longinteger":
        return int(received_data)
    if mapping.type == "longintegerarray":
        return [int(e) for e in received_data]
    if mapping.type == "datetime":
        return parser.parse(received_data)
    if mapping.type == "datetimearray":
        return [parser.parse(e) for e in received_data]
    if mapping.type == "binaryblob":
        return base64.b64decode(received_data)
    if mapping.type == "binaryblobarray":
        return [base64.b64decode(e) for e in received_data]
    return received_data
