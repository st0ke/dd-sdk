#!/usr/bin/env python3
# DD game project
# Copyright (C) 2026 Alexander Boldyrev <boldir@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see https://www.gnu.org/licenses/.

import argparse
import json
import pathlib
import sys
import urllib.request


REPO = "nlohmann/json"
DEFAULT_VERSION = "latest"


def fetch_text(url):
	request = urllib.request.Request(url, headers={"User-Agent": "dd-sdk-vendor-updater"})
	with urllib.request.urlopen(request) as response:
		return response.read().decode("utf-8")


def latest_version():
	payload = fetch_text(f"https://api.github.com/repos/{REPO}/releases/latest")
	data = json.loads(payload)
	tag = data["tag_name"]
	return tag[1:] if tag.startswith("v") else tag


def version_tag(version):
	return version if version.startswith("v") else f"v{version}"


def download(version, destination):
	tag = version_tag(version)
	base = f"https://raw.githubusercontent.com/{REPO}/{tag}"
	files = {
		destination / "include" / "nlohmann" / "json.hpp": f"{base}/single_include/nlohmann/json.hpp",
		destination / "LICENSE.MIT": f"{base}/LICENSE.MIT",
	}

	for path, url in files.items():
		path.parent.mkdir(parents=True, exist_ok=True)
		path.write_text(fetch_text(url), encoding="utf-8", newline="")

	(destination / "VERSION").write_text(f"{version}\n", encoding="utf-8")


def main():
	parser = argparse.ArgumentParser(description="Update vendored nlohmann/json.")
	parser.add_argument(
		"version",
		nargs="?",
		default=DEFAULT_VERSION,
		help="Version to vendor, for example 3.12.0. Defaults to latest GitHub release.",
	)
	args = parser.parse_args()

	destination = pathlib.Path(__file__).resolve().parent
	version = latest_version() if args.version == "latest" else args.version
	download(version, destination)
	print(f"Vendored nlohmann/json {version} in {destination}")


if __name__ == "__main__":
	try:
		main()
	except Exception as exc:
		print(f"error: {exc}", file=sys.stderr)
		sys.exit(1)
