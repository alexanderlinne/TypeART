from ntpath import realpath
import subprocess
import os
import re
from dataclasses import dataclass
import sys
from jinja2 import Environment, FileSystemLoader, select_autoescape

self_path = os.path.dirname(os.path.abspath(__file__))

env = Environment(
    loader=FileSystemLoader(searchpath=self_path),
    autoescape=select_autoescape(default=False),
)

stack_st = env.get_template("templates/stack_st.cpp.jinja")
stack_st_loop = env.get_template("templates/stack_st_loop.cpp.jinja")
stack_mt = env.get_template("templates/stack_mt.cpp.jinja")
heap_st = env.get_template("templates/heap_st.cpp.jinja")
heap_mt = env.get_template("templates/heap_mt.cpp.jinja")
heap_mt_loop = env.get_template("templates/heap_mt_loop.cpp.jinja")


@dataclass
class ExecutionTime:
    user: float
    sys: float
    real: float
    rss: int


time_regex = re.compile(
    "([0-9]*\.[0-9]*) user, ([0-9]*\.[0-9]*) sys, ([0-9]*\.[0-9]*) real, ([0-9]*)k RSS"
)


def find_and_parse_time(str):
    match = time_regex.match(str)
    return ExecutionTime(
        float(match.group(1)),
        float(match.group(2)),
        float(match.group(3)),
        int(match.group(4)),
    )


def compile_bench(template, **args):
    with open("{}/gen/main.cpp".format(self_path), "w") as f:
        f.write(template.render(**args))
    command = "{0}/../build/scripts/run.sh {0}/gen/main.cpp --only_compile".format(self_path)
    subprocess.run(
        command,
        shell=True,
        # stdout=subprocess.PIPE,
        # stderr=subprocess.PIPE,
    )


def run_bench():
    command = "{0}/../build/scripts/run.sh {0}/gen/main.cpp --only_execute --time 2>&1 | grep RSS".format(
        self_path
    )
    result = subprocess.run(
        command,
        shell=True,
        stdout=subprocess.PIPE,
    )
    return find_and_parse_time(result.stdout.decode("utf-8"))


for i in range(10000, 100000, 10000):
    for threads in range(4, 17, 4):
        compile_bench(heap_mt_loop, num_threads=threads, num_vars=i)
        for _ in range(1, 6):
            print(i, threads, run_bench())
