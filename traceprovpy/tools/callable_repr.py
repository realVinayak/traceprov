from typing import Any, Callable


class CallableRepr(object):
    func: Callable
    name: str
    
    def __init__(self, func: Callable, name: str) -> None:
        self.func = func
        self.name = name
    
    def __call__(self, *args: Any, **kwds: Any) -> Any:
        return self.func(*args, **kwds)
    