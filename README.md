# tsig #

[![Code Size Shield][shield_code_size]][ref_floppy_disk]
[![License Shield][shield_license]][file_license_md]

Tsig is a **Work In Progress**.

Minimal signals in C++11. Inspired by more complex signals and slots systems
like Qt and Boost.Signals2, Tsig implements minimal signals in a single header
file, depending only on the standard library.

```cpp
void DoSomethingA(int x, int y) {
  std::cout << "[A] Hello " << x << ", " << y << "\n";
}

struct DoerOfSomethingB {
  void operator()(int x, int y) const {
    std::cout << "[B] Hello " << x << ", " << y << "\n";
  }
};

auto do_something_c = [](int x, int y) {
  std::cout << "[C] Hello " << x << ", " << y << "\n";
};

Signal<void(int, int)> signal;

// Connect handlers to make sigcons (signal connections)
auto sigcon_a = signal.Connect(DoSomethingA);
auto sigcon_b = signal.Connect(DoerOfSomethingB());
auto sigcon_c = std::make_shared<Sigcon>(signal.Connect(do_something_c));

// Emit to call all handlers
signal.Emit(1, 2);

// Output:
// [A] Hello 1, 2
// [B] Hello 1, 2
// [C] Hello 1, 2

// Reset or destruct sigcons to remove handlers
sigcon_b.Reset();
sigcon_c = nullptr;

// This will only call the 'A' handler
signal.Emit(3, 4);

// Output:
// [A] Hello 3, 4
```

## Building ##

The build uses Meson and Ninja. You will need to install those. On Ubuntu you
can probably run something like:

```text
$ sudo apt-get install python3-pip ninja-build
$ sudo pip3 install meson
```

Once the build tools are in place, you can run the build:

```text
$ meson build
$ ninja -C build
```

## Testing ##

Run the tests using the following commands:

```text
$ meson build
$ ninja -C build
$ ./build/signal_test
```

<!-- Links -->

[shield_code_size]: https://img.shields.io/github/languages/code-size/tprk77/tsig
[ref_floppy_disk]: https://en.wikipedia.org/wiki/History_of_the_floppy_disk
[shield_license]: https://img.shields.io/github/license/tprk77/tsig?color=informational
[file_license_md]: https://github.com/tprk77/tsig/blob/master/LICENSE.md

<!-- Local Variables: -->
<!-- fill-column: 79 -->
<!-- End: -->
