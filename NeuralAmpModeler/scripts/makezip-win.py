import os
from pathlib import Path
import shutil
import sys
import zipfile

scriptpath = os.path.dirname(os.path.realpath(__file__))
projectpath = os.path.abspath(os.path.join(scriptpath, os.pardir))

IPLUG2_ROOT = os.path.join("..", "..", "iPlug2")

sys.path.insert(0, os.path.join(scriptpath, IPLUG2_ROOT, "Scripts"))

from get_archive_name import get_archive_name
from parse_config import parse_config


IGNORED_DISTRIBUTION_FILES = {".DS_Store"}


def add_file(archive, path, archive_name=None):
    if not path.is_file():
        raise FileNotFoundError(f"Required distribution file not found: {path}")

    archive.write(path, archive_name or path.name, zipfile.ZIP_DEFLATED)


def add_directory(archive, path):
    if not path.is_dir():
        raise FileNotFoundError(f"Required distribution directory not found: {path}")

    for child in sorted(path.rglob("*")):
        if child.is_file() and child.name not in IGNORED_DISTRIBUTION_FILES:
            archive.write(
                child,
                child.relative_to(path.parent),
                zipfile.ZIP_DEFLATED,
            )


def main():
    if len(sys.argv) != 3:
        print("Usage: make_zip.py demo[0/1] zip[0/1]")
        sys.exit(1)
    else:
        demo = int(sys.argv[1])
        make_binary_zip = int(sys.argv[2])

    config = parse_config(projectpath)
    binary_name = config["BUNDLE_NAME"]
    output_dir = Path(projectpath) / "build-win" / "out"
    example_collection = (
        Path(projectpath).parent
        / "distribution"
        / "nam-division-collection"
    )
    thumbnail_templates = [
        Path(projectpath).parent / "docs" / "images" / "amp-template.jpg",
        Path(projectpath).parent / "docs" / "images" / "cab-template.jpg",
    ]

    if output_dir.exists():
        shutil.rmtree(output_dir)

    output_dir.mkdir(parents=True)

    archive_name = get_archive_name(
        projectpath, "win", "demo" if demo == 1 else "full"
    )
    distribution_archive = output_dir / f"{archive_name}.zip"

    with zipfile.ZipFile(distribution_archive, mode="w") as archive:
        if make_binary_zip:
            vst3_bundle = Path(projectpath) / "build-win" / f"{binary_name}.vst3"
            app = Path(projectpath) / "build-win" / f"{binary_name}_x64.exe"
            add_directory(archive, vst3_bundle)
            add_file(archive, app)
        else:
            default_installer_name = (
                config["PLUG_NAME"] + " Demo Installer"
                if demo
                else config["PLUG_NAME"] + " Installer"
            )
            installer_name = os.environ.get(
                "INSTALLER_OUTPUT_BASE_FILENAME", default_installer_name
            )
            add_file(
                archive,
                Path(projectpath)
                / "build-win"
                / "installer"
                / f"{installer_name}.exe",
            )
            add_file(archive, Path(projectpath) / "installer" / "changelog.txt")
            add_file(archive, Path(projectpath) / "installer" / "known-issues.txt")
            add_file(
                archive,
                Path(projectpath) / "manual" / "NAM Division README.pdf",
            )

        add_directory(archive, example_collection)
        for template in thumbnail_templates:
            add_file(archive, template)

    print(f"wrote {distribution_archive.name}")

    pdb_files = sorted((Path(projectpath) / "build-win" / "pdbs").glob("*.pdb"))
    if not pdb_files:
        raise FileNotFoundError("No Windows PDB files were produced")

    pdb_archive = output_dir / f"{archive_name}-pdbs.zip"
    with zipfile.ZipFile(pdb_archive, mode="w") as archive:
        for pdb_file in pdb_files:
            add_file(archive, pdb_file)

    print(f"wrote {pdb_archive.name}")


if __name__ == "__main__":
    main()
