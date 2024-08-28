# Properties

Properties are values that can notify observers of changes.
They can therefore be used as values in [data bindings](data-binding.md).

Unlike [Qts Properties](https://doc.qt.io/qt-5/properties.html), KDBindings Properties don't
require a Meta Object Compiler and are available in pure C++17.  They can even be used outside
of classes as free values.

## Declaring Properties

Properties can be declared for most types by creating a `KDBindings::Property<T>` instance.

The property instance will then emit [signals](signals-slots.md) every time the properties value
changes, the property is moved or destructed.

### Minimal example: declaring properties

``` cpp
#include <kdbindings/property.h>

using namespace KDBindings;

Property<std::string> myProperty;
myProperty.valueChanged().connect([](const std::string& value) {
  std::cout << value << std::endl;
});

myProperty = "Hello World!";
```

Expected output:

```text
Hello World!
```

For more information and examples see the KDBindings::Property documentation.
