import logging
from logging.handlers import QueueHandler, QueueListener
import multiprocessing as mp

class CustomQueueListener(QueueListener):
    def __init__(self, queue, name):
        QueueListener.__init__(self, queue)
        self.target = logging.getLogger(name)
    def handle(self, record):
        record = self.prepare(record)
        self.target.handle(record)

class LoggerManifold(object):
    def __init__(self, name):
        self.name = name
        self.queue = mp.Queue()
        self.listener = CustomQueueListener(self.queue, name)
        LoggerManifold.queue = self.queue
    def start(self):
        self.listener.start()
    def stop(self):
        self.listener.stop()
    @classmethod
    def get_queue(klass):
        return klass.queue
    @classmethod
    def init_process(klass, queue):
        root_logger = logger.getLogger()
        root_logger.setLevel(logging.DEBUG)
        root_logger.addHandler(QueueHandler(queue))
