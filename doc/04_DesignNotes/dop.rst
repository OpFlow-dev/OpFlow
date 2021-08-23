Data Oriented Programming
+++++++++++++++++++++++++

Data Oriented Programming(DOP), as opposed to the Object Oriented Programming(OOP), is getting known widely for the
last decade. Yehonathan Sharvit defines DOP in his book `Data-Oriented Programming: Unlearning objects
<https://www.manning.com/books/data-oriented-programming>`_ by 3 principles:

- Separate code from data
- Represent data entities with generic data structures
- Data is immutable

These rules somewhat describes a functional programming style. However, strictly obeying these rules could hit
the overall performance. For example, creating large data objects repeatedly to keep data immutable is usually not a
goo idea. Therefore, OpFlow tries to follow the idiom of DOP, but make some changes for better readability & performance.
In summary, 3 aspects are specially designed in DOP style:

- **Seperated expression & operator implementation**. Expressions are the first class citizens in OpFlow. They are
  data objects with only methods for state maintenance. All operations are extracted & implemented as various types of
  operators. By using CRTP, one type of operator can handle a class of expression types, resulting in more concise
  implementations.

- **Immutable intermediate expression**. Although concrete fields are designed to be changeable, all intermediate
  expressions are light-weighted proxies and immutable. All the computations are carried out at the final assignment.
  With the automatic evaluation engine which will generate temporary fields smartly to assure the const-ness of the rhs,
  it's totally safe to change the order of expression's parts without affecting the final result.

- **Stateless operator implementation**. Operators are designed to be static methods with all parameters and arguments
  passing from outside. This ensures that an operator will always give the same result with the same input. This is
  very important especially in multithreading environment.