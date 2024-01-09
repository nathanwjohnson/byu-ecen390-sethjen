#!/usr/bin/python3

"""
This module is used to verify that your lab solution will build with the lab
submission system.
"""

import pathlib
import argparse
import zipfile
import getpass
import sys
import os

repo_path = pathlib.Path(__file__).absolute().parent.resolve()


class TermColors:
    """ Terminal codes for printing in color """

    # pylint: disable=too-few-public-methods

    PURPLE = "\033[95m"
    BLUE = "\033[94m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    END = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


def print_color(color, *msg):
    """ Print a message in color """
    print(color + " ".join(str(item) for item in msg), TermColors.END)


def error(*msg, returncode=-1):
    """ Print an error message and exit program """

    print_color(TermColors.RED, "ERROR:", " ".join(str(item) for item in msg))
    sys.exit(returncode)


def get_files_to_copy_and_zip(lab):
    """ Build a list of (src,dest,in_zip) files to copy into the temp repo given the lab """

    print_color(TermColors.BLUE, "Enumerating files to copy/zip")

    lasertag_path = repo_path / "lasertag"

    # Build a list of files
    # Each entry in this list is a tuple in format (src - pathlib.Path, dest - pathlib.Path, include_in_zip? - boolean)
    files = []

    # if lab == "390m1":
        # files.append((lasertag_path / "queue.c", None, True))
    # elif lab == "390m3-1":
        # files.append((lasertag_path / "filter.c", None, True))
    # elif lab == "390m3-2":
        # files.append((lasertag_path / "hitLedTimer.c", None, True))
        # files.append((lasertag_path / "lockoutTimer.c", None, True))
        # files.append((lasertag_path / "transmitter.c", None, True))
        # files.append((lasertag_path / "trigger.c", None, True))
    # elif lab == "390m3-3":
        # files.append((lasertag_path / "buffer.c", None, True))
        # files.append((lasertag_path / "detector.c", None, True))
        # files.append((lasertag_path / "isr.c", None, True))
    # elif lab == "390m5":
        # files.append((lasertag_path / "game.c", None, True))

    # Add all files in path
    with os.scandir(lasertag_path) as it:
        for entry in it:
            if entry.is_file():
                files.append((lasertag_path / entry.name, None, True))

    print(
        len([f for f in files if f[2]]), "files to be included in the submission zip archive."
    )
    return files


def zip(lab, files):
    """ Zip the lab files """

    zip_path = repo_path / (getpass.getuser() + "_" + lab + ".zip")
    print_color(TermColors.BLUE, "Creating zip file", zip_path.relative_to(repo_path))
    if zip_path.is_file():
        print("Deleting existing file.")
        zip_path.unlink()
    with zipfile.ZipFile(zip_path, "w") as zf:
        print("Created new zip file")
        # Loop through files that are marked for zip (f[2] == True)
        for f in (f for f in files if f[2]):
            if not f[0].is_file():
                error(f[0].relative_to(repo_path), "does not exist")
            print("Adding", f[0].relative_to(repo_path))
            zf.write(f[0], arcname=f[0].name)

    return zip_path.relative_to(repo_path)


def main():
    """ Zip up designated files """

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "lab",
        choices=[
            "390m1",
            "390m3-1",
            "390m3-2",
            "390m3-3",
            "390m5",
        ],
    )
    args = parser.parse_args()

    # Get a list of files need to build and zip
    files = get_files_to_copy_and_zip(args.lab)

    # Zip it
    zip_relpath = zip(args.lab, files)

    print_color(TermColors.BLUE, "Created", zip_relpath, "\nDone.")


if __name__ == "__main__":
    main()
