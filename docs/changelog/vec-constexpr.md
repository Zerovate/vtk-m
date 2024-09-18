# Improve constexpr behavior of `Vec` classes

The implementation of `Vec` made it difficult to use within a `constexpr`
function. For example, the following would not compile.

``` cpp
constexpr vtkm::UInt8 OctFlip(vtkm::UInt8 value)
{
  vtkm::Vec<vtkm::UInt8, 8> flips = { 0, 6, 2, 6, 1, 5, 3, 7 };
  return flips[value];
}
```

The problem was that `flips` could not be initialized in a `constexpr`
function because the constructer was not `constexpr`.

This change fixes this problem by _removing_ the constructor that takes a
`std::initializer_list`. The problem with `std::initializer_list` is that
it cannot be used to directly initialize an array and the loop to do the
initialization cannot be used in a `constexpr`.

With the removal of this constructor, construction with a braced list
should go to the constructor that takes a variadic number of parameters.

Note that it is possible that some initialization of `vtkm::Vec` could
break. You can no longer construct an `std::initializer_list` and pass it
to a `vtkm::Vec` constructor. This seems like a less likely case and can be
gotten around by just copying the data yourself. It is also possible that
putting a braced list in parentheses will stop working. This can be fixed
by just deleting the parentheses.
