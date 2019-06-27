import logging
import threading
import multiprocessing as mp

class CustomQueueHandler(logging.Handler):
    def __init__(self, queue):
        logging.Handler.__init__(self)
        self.queue = queue
    def prepare(self, record):
        msg = self.format(record)
        record.message = msg
        record.msg = msg
        record.args = None
        record.exc_info = None
        return record
    def emit(self, record):
        try:
            self.queue.put_nowait(self.prepare(record))
        except Exception:
            self.handleError(record)

class CustomQueueListener(object):
    def __init__(self, queue, name):
        self.queue = queue
        self.target = logging.getLogger(name)
    def start(self):
        self._thread = t = threading.Thread(target=self._monitor)
        t.daemon = True
        t.start()
    def _monitor(self):
        while True:
            record = self.queue.get()
            if record is None:
                break
            self.handle(record)
    def handle(self, record):
        if record.levelno >= self.target.getEffectiveLevel():
            self.target.handle(record)
    def stop(self):
        self.queue.put_nowait(None)
        self._thread.join()
        self._thread = None

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
        root_logger = logging.getLogger()
        root_logger.setLevel(logging.DEBUG)
        root_logger.addHandler(CustomQueueHandler(queue))
