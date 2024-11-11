Let's implement part of the `std::shared_ptr` interface:

- custom deleters
- weak pointers
- aliasing
- `std::make_shared`
- no allocators
- no `std::atomic<std::shared_ptr>`
- no `std::enable_shared_from_this`
- no `std::make_shared_for_overwrite`

Everything is in `namspace ptr`:

- `class ptr::Shared`
- `class ptr::Weak`
- `ptr::make_shared`
- `ptr::swap`
