import os
import sys
import time
import multiprocessing as mp
import logging
import traceback
from .Grammar import *
from .CodeGen import *
from .Runner import *
from .Listener import *
from .Logger import *

class Stage1Process(object):
    def __init__(self, context, core_affinity=-1):
        self.context = context
        self.grammar = Grammar(context.grammar_path)
        self.parser = Parser(self.grammar)
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

class Stage2Process(object):
    """Methods invoked from the master process"""
    def __init__(self, context, core_affinity=-1):
        self.context = context
        self.master_pid = os.getpid()
        self.proc = mp.Process(target=self.worker)
        self.cmd_queue = mp.Queue()
        self.listener = None
        self.core_affinity = core_affinity
    def start(self):
        self.proc.start()
    def stop(self):
        self.cmd_queue.put(('stop', ''))
        self.proc.join()
    def parse(self, path):
        self.cmd_queue.put(('parse', path))
    """Methods invoked from the worker process"""
    def worker(self):
        if sys.platform.startswith('linux') and self.core_affinity >= 0:
            os.system("taskset -p -c %d %d" % (self.core_affinity, os.getpid()))

        LoggerManifold.init_process(self.context.log_queue)

        logger = logging.getLogger("Centaurus.Stage2Process[%d]" % os.getpid())
        logger.setLevel(logging.DEBUG)

        self.grammar = Grammar(self.context.grammar_path)
        while True:
            cmd = self.cmd_queue.get()
            if cmd[0] == 'stop':
                return
            elif cmd[0] == 'parse':
                runner = Stage2Runner(cmd[1], self.context.bank_size, self.context.bank_num, self.master_pid)
                adapter = Stage2ListenerAdapter(self.grammar, self.listener, self.context.channels, runner.get_window())
                runner.attach(adapter.reduction_callback, adapter.transfer_callback)
                runner.start()
                runner.wait()
    def attach(self, listener):
        self.listener = listener
    def set_core_affinity(self, core_affinity):
        self.core_affinity = core_affinity

class Stage3Process(object):
    def __init__(self, context, core_affinity=-1):
        self.context = context
        self.master_pid = os.getpid()
        self.proc = mp.Process(target=self.worker)
        self.cmd_queue = mp.Queue()
        self.listener = None
        self.core_affinity = core_affinity
    def start(self):
        self.proc.start()
    def stop(self):
        self.cmd_queue.put(('stop', ''))
        self.proc.join()
    def parse(self, path):
        self.cmd_queue.put(('parse', path))
    def worker(self):
        if sys.platform.startswith('linux') and self.core_affinity >= 0:
            os.system("taskset -p -c %d %d" % (self.core_affinity, os.getpid()))

        LoggerManifold.init_process(self.context.log_queue)

        logger = logging.getLogger("Centaurus.Stage3Process[%d]" % os.getpid())
        logger.setLevel(logging.DEBUG)

        self.grammar = Grammar(self.context.grammar_path)
        while True:
            cmd = self.cmd_queue.get()
            if cmd[0] == 'stop':
                return
            elif cmd[0] == 'parse':
                runner = Stage3Runner(cmd[1], self.context.bank_size, self.context.bank_num, self.master_pid)
                adapter = Stage3ListenerAdapter(self.grammar, self.listener, self.context.channels, runner.get_window())
                runner.attach(adapter.reduction_callback, adapter.transfer_callback)
                runner.start()
                runner.wait()
                self.context.drain.put(adapter.values[-1])
            
    def attach(self, listener):
        self.listener = listener
    def set_core_affinity(self, core_affinity):
        self.core_affinity = core_affinity

class Context(object):
    def __init__(self, grammar_path, worker_num):
        self.grammar_path = grammar_path
        self.bank_size = 8 * 1024 * 1024
        self.bank_num = worker_num * 2
        self.channels = []
        for i in range(self.bank_num):
            self.channels.append(mp.Queue())
        self.drain = mp.Queue()
        self.log_queue = LoggerManifold.get_queue()
        self.workers = []
        self.workers.append(Stage1Process(self))
        for i in range(worker_num):
            self.workers.append(Stage2Process(self))
        self.workers.append(Stage3Process(self))
    def start(self):
        for worker in self.workers:
            worker.start()
    def parse(self, path):
        for worker in self.workers:
            worker.parse(path)
        result = self.drain.get()
        return result
    def stop(self):
        for worker in self.workers:
            worker.stop()
    def attach(self, listener):
        for worker in self.workers:
            worker.attach(listener)
