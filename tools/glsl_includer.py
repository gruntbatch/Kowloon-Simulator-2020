import os
import re
import sys


if __name__ == '__main__':
    in_filepath = os.path.join(os.getcwd(), sys.argv[1])
    search_directory = os.path.dirname(in_filepath)

    out_filepath = os.path.join(os.getcwd(), sys.argv[2])

    def include_file(match):
        with open(os.path.join(search_directory, match.group(1))) as f:
            return f.read()

    with open(in_filepath) as in_f, open(out_filepath, 'w') as out_f:
        out_f.write(re.sub('#include "(.*?)"\n', include_file, in_f.read()))
