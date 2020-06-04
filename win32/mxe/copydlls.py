#!/usr/bin/env python2
# DLL dependency resolution and copying script.
# Copyright (C) 2010 John Stumpo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import shutil
import struct
import sys

if len(sys.argv) != 3:
    sys.stderr.write('''Usage: %s [source] [destination]
Copies DLLs in source needed by PE executables in destination to destination.
Both source and destination should be directories.
''' % sys.argv[0])
    sys.exit(1)

def is_pe_file(file):
    f = open(file, 'rb')
    if f.read(2) != 'MZ':
        return False  # DOS magic number not present
    f.seek(60)
    peoffset = struct.unpack('<L', f.read(4))[0]
    f.seek(peoffset)
    if f.read(4) != 'PE\0\0':
        return False  # PE magic number not present
    return True

def get_imports(file):
    f = open(file, 'rb')
    # We already know it's a PE, so don't bother checking again.
    f.seek(60)
    pe_header_offset = struct.unpack('<L', f.read(4))[0]

    # Get sizes of tables we need.
    f.seek(pe_header_offset + 6)
    number_of_sections = struct.unpack('<H', f.read(2))[0]
    f.seek(pe_header_offset + 116)
    number_of_data_directory_entries = struct.unpack('<L', f.read(4))[0]
    data_directory_offset = f.tell()  # it's right after the number of entries

    # Where is the import table?
    f.seek(data_directory_offset + 8)
    rva_of_import_table = struct.unpack('<L', f.read(4))[0]

    # Get the section ranges so we can convert RVAs to file offsets.
    f.seek(data_directory_offset + 8 * number_of_data_directory_entries)
    sections = []
    for i in range(number_of_sections):
        section_descriptor_data = f.read(40)
        name, size, va, rawsize, offset = struct.unpack('<8sLLLL', section_descriptor_data[:24])
        sections.append({'min': va, 'max': va+rawsize, 'offset': offset})

    def seek_to_rva(rva):
        for s in sections:
            if s['min'] <= rva and rva < s['max']:
                f.seek(rva - s['min'] + s['offset'])
                return
        raise ValueError, 'Could not find section for RVA.'

    # Walk the import table and get RVAs to the null-terminated names of DLLs this file uses.
    # The table is terminated by an all-zero entry.
    seek_to_rva(rva_of_import_table)
    dll_rvas = []
    while True:
        import_descriptor = f.read(20)
        if import_descriptor == '\0' * 20:
            break
        dll_rvas.append(struct.unpack('<L', import_descriptor[12:16])[0])

    # Read the DLL names from the RVAs we found in the import table.
    dll_names = []
    for rva in dll_rvas:
        seek_to_rva(rva)
        name = ''
        while True:
            c = f.read(1)
            if c == '\0':
                break
            name += c
        dll_names.append(name)

    return dll_names


src_contents = os.listdir(sys.argv[1])
dest_contents = os.listdir(sys.argv[2])
for dest_name in dest_contents:
    dest_fname = os.path.join(sys.argv[2], dest_name)
    if os.path.isfile(dest_fname) and is_pe_file(dest_fname):
        print dest_name
        for dll in get_imports(dest_fname):
            print '- %s' % dll,
            if dll.lower() in [n.lower() for n in dest_contents]:
                print '(already present)'
            else:
                for n in src_contents:
                    if n.lower() == dll.lower():
                        shutil.copyfile(os.path.join(sys.argv[1], n), os.path.join(sys.argv[2], n))
                        dest_contents.append(n)
                        print '(copied)'
                        break
                else:
                    print '(assumed to be provided by operating system)'
