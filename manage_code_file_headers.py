"""
---- Copyright Start ----

MIT License

Copyright (c) 2022 Digital-Production-Aachen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---- Copyright End ----
"""

import os
import sys
import re

###### parameter section ######

# path where a file with the license header to use is found
header_path = r"code_file_header.txt"

# dictionary with file types to process and in and out markers for block comments
file_types = {
    ".cc":              (r"/*",  r"*/" ),
    ".h":               (r"/*",  r"*/" ),
    ".cs":              (r"/*",  r"*/" ),
    ".py":              (r'"""', r'"""'),
    ".proto":           (r"/*",  r"*/" ),
    "CMakeLists.txt":   (r"#[[", r"]]" ),
}

# list with files / directories to ignore. please use '/' as directory seperator, will be replaced with system standard later.
# matching is done via regex against the relative file path.
exclude_list = [
    ".*/OpenVectorFormat/.*",
    ".*/build/.*"
    ".*/bin/.*",
    ".*/obj/.*",
    ".*/submodules/.*",
    ".*/.git/.*",
    ".*/.vs/.*",
]

###### end parameter section ######

with open(header_path, "r") as header_handle:
    license_header = header_handle.read()

license_header = license_header.strip("\n\r")

header_start = license_header.split("\n",1)[0]
header_end = license_header.rsplit("\n",1)[-1]

# check if run in CI - no replacing, just break at bad header
ci_mode = (len(sys.argv) >= 2 and sys.argv[1].lower() == "ci")
success = True

reg_patterns = []
for i in range(len(exclude_list)):
    exclude_list[i] = exclude_list[i].replace("/", os.sep.replace("\\", "\\\\"))
    reg_patterns.append(re.compile(exclude_list[i]))

for subdir, dirs, files in os.walk(r'.'):
    for filename in files:
        filepath = subdir + os.sep + filename

        # check if file should be excluded
        if any(pattern.match(filepath) for pattern in reg_patterns):
            continue

        # check if file is in the list of supported file types
        valid_types = [ext for ext in file_types.keys() if filepath.endswith(ext)]
        file_type = valid_types[-1] if len(valid_types) > 0 else None
        if file_type is None:
            continue

        with open(filepath, "r") as source_handle:
            source_file = source_handle.read()

        # check if correct license header is already present
        if source_file.find(license_header) >= 0:
            continue

        # if run inside of the CI - record bad header and continue, no replacing in CI
        elif ci_mode:
            print("[" + filepath + "]: no or invalid copyright header.")
            success = False
            continue

        comment_marker_in, comment_marker_out = file_types[file_type]

        # check if header start / end mark is present 
        # if yes, replace header
        # if not, insert new header at begining of file
        start = source_file.find(header_start)
        end = source_file.find(header_end)
        if (start > end):
            sys.exit(-1)
        elif (start >= 0 and end > 0):
            print("[" + filepath + "]: replace header")
            source_start = source_file.split(header_start)[0]
            source_end = source_file.split(header_end, 1)[-1]
            new_source = source_start + license_header + source_end
        else:
            if (source_file.lower().find("copyright") >= 0):
                print("[" + filepath + "]: found different copyright header - please remove")
                sys.exit(1)
            print("[" + filepath + "]: insert new header")
            new_source = comment_marker_in + '\n' + license_header + '\n' + comment_marker_out +'\n\n' + source_file

        with open(filepath, "w") as source_handle:
            source_handle.write(new_source)
        continue

if ci_mode and not success:
    print("Invalid copyright headers found.  Please run the " + sys.argv[0].rsplit(os.sep, 1)[-1] + " script locally to fix and commit again.")
    sys.exit(-1)
