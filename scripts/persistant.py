import pickle
import new
import bnirc
from os.path import exists
from os import mkdir
from os import environ
import atexit

class _persistant_code:
    def __init__(self, fn):
        fnc = fn.func_code
        self.fn_tuple = (fnc.co_argcount, fnc.co_nlocals, fnc.co_stacksize, fnc.co_flags, fnc.co_code, \
                            fnc.co_consts, fnc.co_names, fnc.co_varnames, fnc.co_filename, fnc.co_name, \
                            fnc.co_firstlineno, fnc.co_lnotab, fnc.co_freevars, fnc.co_cellvars)
        self.doc_string = fn.__doc__

    def function(self):
        def fn():
            pass
        t = self.fn_tuple
        fn.func_code = new.code(t[0],t[1],t[2],t[3],t[4],t[5],t[6],t[7],t[8],t[9],t[10],t[11],t[12],t[13])
        fn.__doc__ = self.doc_string
        return fn

home = environ["HOME"]

if not exists(home + "/.bnIRC"):
    mkdir(home + "/.bnIRC")

_saved_commands = {}
_saved_loops = []
_saved_server_strings = {}
_stored_globals = {}

if exists(home + "/.bnIRC/_persistant"):
    f = open(home + "/.bnIRC/_persistant")
    _saved_commands = pickle.load(f)
    _saved_loops = pickle.load(f)
    _saved_server_strings = pickle.load(f)
    _stored_globals = pickle.load(f)
    f.close()

def save_state():
    if exists(home + "/.bnIRC/_persistant"):
        inf = open(home + "/.bnIRC/_persistant")
        outf = open(home + "/.bnIRC/_persistant.bak", 'w')
        outf.write(inf.read())
        inf.close()
        outf.close()

    f = open(home + "/.bnIRC/_persistant", "w")
    pickle.dump(_saved_commands, f)
    pickle.dump(_saved_loops, f)
    pickle.dump(_saved_server_strings, f)
    pickle.dump(_stored_globals, f)
    f.close()

atexit.register(save_state)

def _apply_stored(fn):
    for k in _stored_globals.keys():
        fn.func_globals[k] = _stored_globals[k]
        def pass_func():
            pass
        if type(fn.func_globals[k]) == type(_persistant_code(pass_func)):
            fn.func_globals[k] = fn.func_globals[k].function()

def store(name, val):
    def pass_func():
        pass
    if type(val) == type(pass_func):
        val = _persisant_code(val)
    _stored_globals[name] = val

def _store_cmd(args):
    name = args[1]
    if len(args) > 2:
        val = args[2]
    else:
        val = name
    exec "store('%s', %s)" % (name, val)
    return 0

bnirc.RegisterCommand('store', 'store <name> [real name]', 'store a python global', 2, 3, _store_cmd)

for i in _saved_commands.values():
    fn = i[5].function()
    _apply_stored(fn)
    bnirc.RegisterCommand(i[0], i[1], i[2], i[3], i[4], fn)

for i in _saved_loops:
    fn = i.function()
    _apply_stored(fn)
    bnirc.RegisterLoop(fn)

for i in _saved_server_strings.values():
    fn = i[3].function()
    _apply_stored(fn)
    bnirc.RegisterServerString(i[0], i[1], i[2], fn)

_RegisterCommand = bnirc.RegisterCommand
_RegisterServerString = bnirc.RegisterServerString
_RegisterLoop = bnirc.RegisterLoop

def _rc(name, usage, description, min_argcount, max_argcount, func):
    _saved_commands[name] = (name, usage, description, min_argcount, max_argcount, _persistant_code(func))
    _RegisterCommand(name, usage, description, min_argcount, max_argcount, func)

_rc.__doc__ = bnirc.RegisterCommand.__doc__
bnirc.RegisterCommand = _rc

def _rl(func):
    _saved_loops.append(_persistant_code(func))
    _RegisterLoop(func)

_rl.__doc__ = bnirc.RegisterLoop.__doc__
bnirc.RegisterLoop = _rl

def _rss(key, strip_colon, argc, func):
    _saved_server_strings[key] = (key, strip_colon, argc, _persistant_code(func))
    _RegisterServerString(key, strip_colon, argc, func)

_rss.__doc__ = bnirc.RegisterServerString.__doc__
bnirc.RegisterServerString = _rss
