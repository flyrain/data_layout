import logging

logger = logging.getLogger('fixed_function')
logger.setLevel(logging.DEBUG) # logging.DEBUG is the lowest level, if level is DEBUG, it will output all level logging information 
# create file handler which logs even debug messages
fh = logging.FileHandler('fixedFunction.log')
fh.setLevel(logging.INFO)
# create console handler with a higher log level
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
# create formatter and add it to the handlers
formatter = logging.Formatter('%(levelname)s - %(message)s - %(asctime)s')
ch.setFormatter(formatter)
fh.setFormatter(formatter)
# add the handlers to logger
logger.addHandler(ch)
logger.addHandler(fh)

# 'application' code
#logger.debug('debug message')
#logger.info('info message')
#logger.warn('warn message')
#logger.error('error message')
#logger.critical('critical message')