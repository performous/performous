#!/usr/bin/env python3

import os
from argparse import ArgumentParser
from dataclasses import dataclass, field
from hashlib import sha256
from xml.etree import ElementTree as ET

import requests


GITHUB_USER = "performous"
GITHUB_REPO = "performous"
GITHUB_HEADERS = {
    "Accept": "application/vnd.github+json",
    "X-GitHub-Api-Version": "2022-11-28",
}


@dataclass
class Artifact:
    type: str
    location: str
    checksum: str
    size: int

    def to_xml(self):
        tag = ET.Element("artifact", {"type": self.type})
        loc = ET.Element("location")
        loc.text = self.location
        tag.append(loc)
        c = ET.Element("checksum", {"type": "sha256"})
        c.text = self.checksum
        tag.append(c)
        s = ET.Element("size", {"type": "download"})
        s.text = str(self.size)
        tag.append(s)
        return tag


@dataclass
class Release:
    version: str
    date: str
    url: str
    description: str
    type: str = "stable"
    artifacts: list[Artifact] = field(default_factory=list)

    def to_xml(self):
        tag = ET.Element(
            "release", {"version": self.version, "date": self.date, "type": self.type}
        )
        desc = ET.Element("description")
        desc.text = self.description
        tag.append(desc)
        url = ET.Element("url")
        url.text = self.url
        tag.append(url)
        artifacts = ET.Element("artifacts")
        tag.append(artifacts)
        for artifact in self.artifacts:
            artifacts.append(artifact.to_xml())
        return tag


def make_artifact(**kwargs):
    checksum = sha256()
    response = requests.get(kwargs["location"], allow_redirects=True)
    response.raise_for_status()
    filename = (
        response.headers["Content-Disposition"].split("; ")[1].replace("filename=", "")
    )
    print(f"  processing asset {filename}...")
    if "size" not in kwargs:
        kwargs["size"] = len(response.content)
    checksum.update(response.content)
    kwargs["checksum"] = checksum.hexdigest()
    return Artifact(**kwargs)


def get_releases():
    session = requests.Session()
    session.headers = GITHUB_HEADERS
    response = session.get(
        f"https://api.github.com/repos/{GITHUB_USER}/{GITHUB_REPO}/releases"
    )
    response.raise_for_status()
    for release_data in reversed(
        sorted(response.json(), key=lambda r: r["published_at"])
    ):
        release = Release(
            version=release_data["tag_name"],
            date=release_data["published_at"][:10],
            url=release_data["html_url"],
            description=release_data["body"].replace("\r\n", "\n").replace(" \n", "\n"),
        )
        print(f"Processing release {release.version}")
        if "rc" in release.version:
            release.type = "development"
        tarball_url = f"https://github.com/{GITHUB_USER}/{GITHUB_REPO}/archive/refs/tags/{release.version}.tar.gz"
        release.artifacts.append(make_artifact(type="source", location=tarball_url))
        for asset in release_data["assets"]:
            release.artifacts.append(
                make_artifact(
                    type="binary",
                    location=asset["browser_download_url"],
                    size=asset["size"],
                )
            )
        yield release


def parse_args():
    parser = ArgumentParser()
    parser.add_argument("metainfo", help="The path to the metainfo XML file")
    args = parser.parse_args()
    if not os.path.exists(args.metainfo):
        parser.error(f"Can't find {args.metainfo}")
    return args


def main():
    args = parse_args()
    tree = ET.parse(args.metainfo)
    releases_tag = tree.getroot().find("releases")
    # Clean
    for child in list(releases_tag.iter("release")):
        releases_tag.remove(child)
    # Add
    for release in get_releases():
        releases_tag.append(release.to_xml())
    ET.indent(tree)
    tree.write(args.metainfo, xml_declaration=True, encoding="utf-8")


if __name__ == "__main__":
    main()
