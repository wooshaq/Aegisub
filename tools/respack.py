#!/usr/bin/env python3

import sys, os
from collections import defaultdict

manifestfile, cppfile, hfile, sourcepath = sys.argv[1:]

with open(manifestfile, 'r') as manifest:
    files = dict(x.strip().split('|') if '|' in x else (x.strip(), None) for x in manifest.readlines() if x.strip() != '')    # type: ignore

buildpath = os.path.split(cppfile)[0]

for k in files:
    if files[k] == '':
        files[k] = None

    if files[k] != None:
        if not os.path.isfile(files[k]):
            print("{}: Failed to open '{}'".format(manifestfile, k))
            sys.exit(1)
        continue

    sf = os.path.join(sourcepath, k)
    bf = os.path.join(buildpath, k)

    if os.path.isfile(sf):
        files[k] = sf
    elif os.path.isfile(bf):
        files[k] = bf
    else:
        print("{}: Failed to open '{}'".format(manifestfile, k))
        sys.exit(1)

with open(cppfile, 'w') as cpp:
    cpp.write('#include "libresrc.h"\n')
    with open(hfile, 'w') as h:
        groups = defaultdict(list)

        for k in files:
            with open(files[k], 'rb') as f:
                data = [str(int(x)) for x in f.read()]

            datastr = ','.join(data)
            name = os.path.splitext(os.path.basename(k))[0]
            cpp.write('const unsigned char {}[] = {{{}}};\n'.format(name, datastr))
            h.write('extern const unsigned char {}[{}];\n'.format(name, len(data)))

            if name.split('_')[-1].isnumeric():
                groups['_'.join(name.split('_')[:-1])].append(name)

        if groups:
            h.write('\n')

        for group in sorted(groups):
            h.write('const LibresrcBlob {}[] = {{{}}};\n'.format(group, ', '.join('{{{}, sizeof({}), {}}}'.format(file, file, file.split('_')[-1]) for file in groups[group])))

        # Themed lookup: <name>_themed(dark) gives the <name>_dark group when
        # there is one and the dark theme is on, else the normal group.
        light = [g for g in groups if not g.endswith('_dark')]
        if light:
            h.write('\n')
        for group in sorted(light):
            dark = group + '_dark'
            if dark in groups:
                h.write('inline LibresrcBlobs {0}_themed(bool dark) {{ return dark ? LibresrcBlobs{{{1}, {2}}} : LibresrcBlobs{{{0}, {3}}}; }}\n'
                        .format(group, dark, len(groups[dark]), len(groups[group])))
            else:
                h.write('inline LibresrcBlobs {0}_themed(bool) {{ return LibresrcBlobs{{{0}, {1}}}; }}\n'.format(group, len(groups[group])))
