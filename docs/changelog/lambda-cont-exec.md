# Support execution environment lambdas from control environment

Lamda functions are a convenient syntax for defining functions that get
passed to other functions to be executed later. Typically, a lambda
function is compiled for the environment it is being used. Lambdas defined
in a control-environment function can run in the control environment
whereas lambdas defined in an execution-environment function can run in the
execution environment.

Occasionally, it is useful to define a lambda expression in the control
environment that will later be executed in the execution environment. For
example, it could be useful to use a lamba function to construct an
`ArrayHandleImplicit`. The lambda created will need to run in the execution
environment when that array is used with a worklet.

To handle these situations, VTK-m now provides the `VTKM_LAMBDA` macro.
This macro is used in place of the captures in a lambda expression to
specify that the lambda can be used in either control or execution
environment. Here is an example of its use.

``` cpp
VTKM_LAMBDA(vtkm::Id index) { return foo + bar + index; }
```

Note that `VTKM_LAMBDA` sets the captures to `[=]`, which means that any
referenced value will be copied by value. In the example above, the
variables `foo` and `bar` are captured from the scope the lambda is
declared in, and copies of their values will be contained in the object the
lambda expression creates. Capturing by value is important in this context
as references cannot carry from control to execution environment. The above
example is similar to the following (except it is built for both
environments).

``` cpp
[=](vtkm::Id index) { return foo + bar + index; }
```

If a custom capture is needed, the alternate `VTKM_LAMBDA_CAPTURES` macro,
which takes the captures as an argument, can be used. This can be used to
specify exactly which variables to capture rather than automatically
capture them. The following example can specify exactly which variables are
allowed to be captures.

``` cpp
VTKM_LAMBDA_CAPTURES(foo, bar)(vtkm::Id index) { return foo + bar + index; }
```

If you want to enforce no captures, you can specify a blank capture.

``` cpp
VTKM_LAMBDA_CAPTURES()(vtkm::Id index) { return 2 * index; }
```

It should be noted that a perhaps more complete name for this macro would
be something like `VTKM_LAMBDA_EXEC_CONT`. However, there is no real use
case for using this macro for any purpose other than defining a lambda for
both environments, so the briefer `VTKM_LAMBDA` is used.
