Import("env")

from pathlib import Path
import re

project_dir = Path(env["PROJECT_DIR"])
web_dir = project_dir / "web"
output_dir = project_dir / "include" / "generated"

output_dir.mkdir(parents=True, exist_ok=True)


def make_name(filename):
    return re.sub(r"[^A-Za-z0-9]", "_", filename).upper()


for source_file in web_dir.iterdir():
    if not source_file.is_file():
        continue

    name = make_name(source_file.name)

    output_file = output_dir / f"{source_file.name.replace('.', '_')}.h"

    contents = source_file.read_text(encoding="utf-8")

    header = f"""\
#pragma once

const char {name}[] PROGMEM = R"WEB(
{contents}
)WEB";
"""

    output_file.write_text(header, encoding="utf-8")

    print(f"Generated: {source_file.name} -> {output_file.name}")