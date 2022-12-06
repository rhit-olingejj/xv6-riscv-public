#!/usr/bin/env python

import re
from gradelib import *

r = Runner(save("xv6.out"))

@test(5, "rhmm, test basic case")
def test_basic_case():
    r.run_qemu(shell_script(['lab01basic']))
    r.match("^TestCase1:.*: Ok\.",
            no=[".*FAILED.*"])

@test(20, "rhmm, test basic alloc")
def test_basic_alloc():
    r.run_qemu(shell_script(['lab01test 1']))
    r.match("^test_basic_alloc\\(\\d+\\): OK\.",
            no=[".*Assertion FAILED.*"])


@test(20, "rhmm, test basic free")
def test_basic_free():
    r.run_qemu(shell_script(['lab01test 2']))
    r.match("^test_basic_free\\(\\d+\\): OK\.",
            no=[".*Assertion FAILED.*"])


@test(10, "rhmm, test coalesce basic 1")
def test_coalesce_basic1():
    r.run_qemu(shell_script(['lab01test 3']))
    r.match("^test_coalesce_basic1\\(\\d+\\): OK\.",
            no=[".*Assertion FAILED.*"])


@test(10, "rhmm, test coalesce basic 2")
def test_coalesce_basic1():
    r.run_qemu(shell_script(['lab01test 4']))
    r.match("^test_coalesce_basic2\\(\\d+\\): OK\.",
            no=[".*Assertion FAILED.*"])


@test(10, "rhmm, test coalesce 3")
def test_coalesce3():
    r.run_qemu(shell_script(['lab01test 5']))
    r.match("^test_coalesce3\\(\\d+\\): OK\.",
            no=[".*Assertion FAILED.*"])


@test(10, "rhmm, test force alignment")
def test_foce_alignment():
    r.run_qemu(shell_script(['lab01test 6']))
    r.match("^test_force_alignment\\(\\d+\\): OK\.",
            no=[".*Assertiona FAILED.*"])


@test(10, "rhmm, test too small blocks")
def test_too_small_blocks():
    r.run_qemu(shell_script(['lab01test 7']))
    r.match("^test_too_small_blocks\\(\\d+\\): OK\.",
            no=[".*Assertion FAILED.*"])


@test(10, "rhmm, test gracefully run out of memory")
def test_gracefully_run_out_of_memory():
    r.run_qemu(shell_script(['lab01test 8']))
    r.match("^test_gracefully_run_out_of_memory\\(\\d+\\): OK\.",
            no=[".*Assertion FAILED.*"])


@test(15, "rhmm, test stress big heap chunks")
def test_stress_big_heap_chunks():
    r.run_qemu(shell_script(['lab01test 9']))
    r.match("^test_stress_big_heap_chunks\\(\\d+\\): OK\.",
            no=[".*Assertion FAILED.*"])


@test(20, "rhmm, test stress overlapping")
def test_stress_overlapping():
    r.run_qemu(shell_script(['lab01test 10']))
    r.match(no=[".*Assertion FAILED.*"])


run_tests()

