import os
import sys
import time
import multiprocessing as mp
import logging
from .Grammar import *
from .CodeGen import *
from .Runner import *
from .Listener import *

class Stage1Process(object):
    def __init__(self, context):
        self.context = context
        self.grammar = Grammar(context.grammar_path)
        self.parser = Parser(self.grammar)
        self.runner = None
    def parse(self, path):
        self.runner = Stage1Runner(path, self.parser, self.context.bank_size, self.context.bank_num)
        self.runner.start()
    def start(self):
        pass
    def stop(self):
        if self.runner:
            self.runner.wait()
            self.runner = None
    def attach(self, listener):
        pass

class Stage2Process(object):
    """Methods invoked from the master process"""
    def __init__(self, context):
        self.context = context
        self.master_pid = os.getpid()
        self.proc = mp.Process(target=self.worker)
        self.cmd_queue = mp.SimpleQueue()
        self.listener = None
    def start(self):
        self.proc.start()
    def stop(self):
        self.cmd_queue.put(('stop', ''))
        self.proc.join()
    def parse(self, path):
        self.cmd_queue.put(('parse', path))
    """Methods invoked from the worker process"""
    def worker(self):
        self.grammar = Grammar(self.context.grammar_path)
        self.chaser = Chaser(self.grammar)
        logging.basicConfig(filename="log%d.log" % os.getpid(), level=logging.DEBUG)
        while True:
            cmd = self.cmd_queue.get()
            if cmd[0] == 'stop':
                return
            elif cmd[0] == 'parse':
                logging.debug('Stage2 process started.')
                start_time = time.time()
                runner = Stage2Runner(cmd[1], self.chaser, self.context.bank_size, self.context.bank_num, self.master_pid)
                adapter = ListenerAdapter(self.grammar, self.listener, runner)
                runner.start()
                logging.debug('Stage2 thread started.')
                runner.wait()
                elapsed_time = time.time() - start_time
                logging.debug('Stage2 process finished.')
                logging.debug('Elapsed time = %.1f' % (elapsed_time * 1E+3,))
    def attach(self, listener):
        self.listener = listener

class Stage3Process(object):
    def __init__(self, context):
        self.context = context
        self.master_pid = os.getpid()
        self.proc = mp.Process(target=self.worker)
        self.cmd_queue = mp.SimpleQueue()
        self.listener = None
    def start(self):
        self.proc.start()
    def stop(self):
        self.cmd_queue.put(('stop', ''))
        self.proc.join()
    def parse(self, path):
        self.cmd_queue.put(('parse', path))
    def worker(self):
        self.grammar = Grammar(self.context.grammar_path)
        self.chaser = Chaser(self.grammar)
        while True:
            cmd = self.cmd_queue.get()
            if cmd[0] == 'stop':
                return
            elif cmd[0] == 'parse':
                runner = Stage3Runner(cmd[1], self.chaser, self.context.bank_size, self.context.bank_num, self.master_pid)
                #adapter = ListenerAdapter(self.grammar, self.listener, runner)
                runner.start()
                runner.wait()
    def attach(self, listener):
        self.listener = listener

class Context(object):
    def __init__(self, grammar_path, worker_num):
        self.grammar_path = grammar_path
        self.bank_size = 8 * 1024 * 1024
        self.bank_num = 8
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
    def stop(self):
        for worker in self.workers:
            worker.stop()
    def attach(self, listener):
        for worker in self.workers:
            worker.attach(listener)