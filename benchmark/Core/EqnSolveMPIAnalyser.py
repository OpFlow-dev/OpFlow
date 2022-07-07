import getopt
import os
import subprocess
import shutil
import json
import re
import sys

import xlwt


def run_config(i, j):
    print("Run {}x{}".format(i, j))
    os.system(
        "mpirun -n {} --cpus-per-rank {} BENCH_EqnSolveMPI {} --benchmark_out=bench{}x{}.json "
        "--benchmark_out_format=json".format(
            i, j, j, i, j))


def run_case(total_core):
    for i in range(1, total_core + 1):
        for j in range(1, total_core + 1):
            if i * j <= total_core:
                run_config(i, j)


class Record:
    def __init__(self, name, count, proc, thread, time):
        self.name = name
        self.proc = proc
        self.thread = thread
        self.time = time
        self.count = count


all_case_name = set()
all_case_size = set()


def parse_case(total_core):
    result = []
    for i in range(1, total_core + 1):
        for j in range(1, total_core + 1):
            if i * j <= total_core:
                with open("bench{}x{}.json".format(i, j)) as f:
                    data = json.load(f)
                    for item in data["benchmarks"]:
                        match = re.match(r"(\S+)/(\S+)/(\d+)/.*", item["name"])
                        all_case_name.add(match.group(2))
                        all_case_size.add(match.group(3))
                        result.append(Record(match.group(2), match.group(3), i, j, item["real_time"]))
    return result


def write_excel(result, total_core):
    wb = xlwt.Workbook(encoding="utf-8", style_compression=0)
    for name in all_case_name:
        for size in all_case_size:
            st = wb.add_sheet(name + '-' + size, cell_overwrite_ok=True)
            for i in range(1, total_core + 1):
                st.write(0, i, i)
                st.write(i, 0, i)
            for r in result:
                if r.name == name and r.count == size:
                    st.write(r.proc, r.thread, r.time)
    wb.save("benchmark.xls")


def main(argv):
    total_core = 1
    try:
        opts, args = getopt.getopt(argv, "n:", ["ncore="])
    except getopt.GetoptError:
        print('EqnSolveMPIAnalyser.py -n <total-core>')
        sys.exit(2)
    for opt, arg in opts:
        if opt in ('-n', '--ncore'):
            total_core = int(arg)
    run_case(total_core)
    result = parse_case(total_core)
    write_excel(result, total_core)


if __name__ == "__main__":
    main(sys.argv[1:])
