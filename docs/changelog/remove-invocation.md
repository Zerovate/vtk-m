# Invocation object removed

When the dispatch mechanism for VTK-m worklets was originally designed,
VTK-m still had to support compilers that did not implement C++11. As such,
the dispatch invoke had to deal with an unknown number of arguments without
support for variadic template arguments.

To help with the design, an `Invocation` class was made. This class is a
simple `struct` with several template arguments containing information
about the particular invocation. This includes the arguments passed to
invoke (or transferred to the device), both the control and execution
signatures, the domain index, the device, and multiple array types for
mapping. A lot of this information is repetitious and got added to a deep
stack of calls required for the dispatch. The upshot was that a lot of
space was needed to represent the mangled symbols to the functions.

Now that variadic templates are supported, we can simplify the invoke quite
a bit by just passing arguments directly. Consequently, the `Invocation`
class is removed, which both simplifies the code and reduces symbol tables
in compiled files.

