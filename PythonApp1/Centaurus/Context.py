import os
import sys
import time
import multiprocessing as mp
import logging
from logging.handlers import QueueHandler, QueueListener
import traceback
from .Grammar import *
from .CodeGen import *
from .Runner import *
from .Listener import *
from .Logger import *

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
        LoggerManifold.init_process(self.context.log_queue)

        logger = logging.getLogger("Centaurus.Stage2Process[%d]" % os.getpid())
        logger.setLevel(logging.DEBUG)

        self.grammar = Grammar(self.context.grammar_path)
        self.chaser = Chaser(self.grammar)
        while True:
            cmd = self.cmd_queue.get()
            if cmd[0] == 'stop':
                return
            elif cmd[0] == 'parse':
                logger.debug('Stage2 process started.')
                start_time = time.time()
                runner = Stage2Runner(cmd[1], self.chaser, self.context.bank_size, self.context.bank_num, self.master_pid)
                adapter = Stage2ListenerAdapter(self.grammar, self.listener, self.context.channels, runner)
                runner.start()
                logger.debug('Stage2 thread started.')
                runner.wait()
                elapsed_time = time.time() - start_time
                logger.debug('Stage2 process finished.')
                logger.debug('Elapsed time = %.1f' % (elapsed_time * 1E+3,))
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
        LoggerManifold.init_process(self.context.log_queue)

        logger = logging.getLogger("Centaurus.Stage3Process[%d]" % os.getpid())
        logger.setLevel(logging.DEBUG)

        self.grammar = Grammar(self.context.grammar_path)
        self.chaser = Chaser(self.grammar)
        while True:
            cmd = self.cmd_queue.get()
            if cmd[0] == 'stop':
                return
            elif cmd[0] == 'parse':
                logger.debug('Stage3 process started.')
                start_time = time.time()
                runner = Stage3Runner(cmd[1], self.chaser, self.context.bank_size, self.context.bank_num, self.master_pid)
                adapter = Stage3ListenerAdapter(self.grammar, self.listener, self.context.channels, runner)
                runner.start()
                logger.debug('Stage3 thread started.')
                runner.wait()
                elapsed_time = time.time() - start_time
                logger.debug('Stage3 process finished.')
                logger.debug('Elapsed time = %.1f' % (elapsed_time * 1E+3,))
                logger.debug('Reporting the parse result to Stage1...')
                self.context.drain.put(adapter.values[-1])
                logger.debug('Reporting complete.')
            
    def attach(self, listener):
        self.listener = listener

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
