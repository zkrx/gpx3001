#!/usr/bin/env python3

import argparse
import os

parser = argparse.ArgumentParser(description="Convert even & odd ROM files to a single binary")
# Positional arguments
parser.add_argument("even_file", nargs=1)
parser.add_argument("odd_file", nargs=1)
parser.add_argument("out_file", nargs=1)
args = parser.parse_args()

if __name__ == "__main__":
	with open(args.even_file[0], "rb") as even_file, \
	     open(args.odd_file[0], "rb") as odd_file, \
	     open(args.out_file[0], "wb+") as out_file:

		# Check sizes, both files should have the same size!
		even_size = os.fstat(even_file.fileno()).st_size
		odd_size = os.fstat(odd_file.fileno()).st_size
		if even_size != odd_size:
			raise Exception('ROM sizes mismatch!')

		while True:
			even_byte = even_file.read(1)
			odd_byte = odd_file.read(1)

			if not even_byte or not odd_byte:
				out_file.flush()
				break

			# According to GPX3001 documentation, even ROM = KD[00:07]
			# and odd ROM = KD[08:15]
			out_file.write(even_byte)
			out_file.write(odd_byte)

		# Output file size should be twice the input file size
		out_size = os.fstat(out_file.fileno()).st_size
		if out_size != 2*even_size:
			raise Exception('Output file size mismatch!')
