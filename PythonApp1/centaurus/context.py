import os
import sys
import time
import multiprocessing as mp
import logging
import traceback
import itertools
from .corelib import *
from .listener import *

class Stage1Worker(object):
    def __init__(self, context, core_affinity=-1):
        self.context = context
        self.parser = Parser(self.context.grammar)
        self.runner = None
        self.core_affinity=core_affinity
    def parse(self, path):
        self.runner = Stage1Runner(path, self.parser, self.context.bank_size, self.context.bank_num)
        self.runner.start()
    def start(self):
        if sys.platform.startswith('linux') and self.core_affinity >= 0:
            os.system("taskset -p -c %d %d" % (self.core_affinity, os.getpid()))
    def stop(self):
        if self.runner:
            self.runner.wait()
            self.runner = None
    def attach(self, listener):
        pass
    def set_core_affinity(self, core_affinity):
        self.core_affinity = core_affinity

class SubprocessWorker:
    """Methods invoked from the master process"""
    def __init__(self, context, core_affinity=-1):
        self.context = context
        self.master_pid = os.getpid()
        self.proc = mp.Process(target=self.workloop)
        self.cmd_queue = mp.Queue()
        self.listener = None
        self.core_affinity = core_affinity
    def start(self):
        self.proc.start()
    def stop(self):
        self.cmd_queue.put(('stop', None))
        self.proc.join()
    def parse(self, path):
        self.cmd_queue.put(('parse', path))
    def attach(self, listener):
        self.listener = listener
    def set_core_affinity(self, core_affinity):
        self.core_affinity = core_affinity

    """Methods invoked from the worker process"""
    def workloop(self):
        if sys.platform.startswith('linux') and self.core_affinity >= 0:
            os.system('taskset -p -c {} {}'.format(self.core_affinity, os.getpid()))
        self.logger = logging.getLogger('Centaurus.{}[{}]'.format(type(self).__name__, os.getpid()))
        self.logger.setLevel(logging.DEBUG)
        while True:
            cmd, arg =  self.cmd_queue.get()
            if cmd == 'stop':
                break
            elif cmd == 'parse':
                self.parse_impl(arg)

class Stage2Worker(SubprocessWorker):
    def parse_impl(self, path):
        runner = Stage2Runner(path, self.context.bank_size, self.context.bank_num, self.master_pid)
        adapter = Stage2ListenerAdapter(self.context.grammar, self.listener, self.context.channels, runner.get_window())
        runner.attach(adapter.reduction_callback, adapter.transfer_callback)
        runner.start()
        runner.wait()

class Stage3Worker(SubprocessWorker):
    def parse_impl(self, path):
        runner = Stage3Runner(path, self.context.bank_size, self.context.bank_num, self.master_pid)
        adapter = Stage3ListenerAdapter(self.context.grammar, self.listener, self.context.channels, runner.get_window())
        runner.attach(adapter.reduction_callback, adapter.transfer_callback)
        runner.start()
        runner.wait()
        assert len(adapter.values) <= 1
        self.context.drain.put(adapter.values[0] if adapter.values else None)

class Context(object):
    bank_size = 8 * 1024 * 1024
    def __init__(self, grammar_path):
        self.grammar = Grammar(grammar_path)
        self.drain = mp.Queue()
        self.serial_workers = (Stage1Worker(self), Stage3Worker(self))
    def start(self, num_workers=1):
        self.bank_num = num_workers * 2
        self.channels = tuple(mp.Queue() for _ in range(self.bank_num))
        self.parallel_workers = tuple(Stage2Worker(self) for _ in range(num_workers))
        for w in itertools.chain(self.serial_workers, self.parallel_workers):
            w.attach(self.listener)
            w.start()
    def parse(self, path):
        for w in itertools.chain(self.serial_workers, self.parallel_workers):
            w.parse(path)
        return self.drain.get()
    def stop(self):
        for w in itertools.chain(self.serial_workers, self.parallel_workers):
            w.stop()
        del self.bank_num, self.channels, self.parallel_workers
    def attach(self, listener):
        self.listener = listener
