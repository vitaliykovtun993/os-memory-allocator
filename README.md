# Memory Allocator

Мінімальний алокатор пам'яті на C: `os_malloc()`, `os_calloc()`, `os_realloc()`, `os_free()`,
реалізований поверх системних викликів `brk()` / `mmap()` / `munmap()`.

Умова завдання: [docs/ASSIGNMENT.md](docs/ASSIGNMENT.md)
(джерело: [open-education-hub/operating-systems](https://github.com/open-education-hub/operating-systems/tree/main/content/assignments/memory-allocator)).

## Структура

```
src/     реалізація алокатора (osmem.c) -> libosmem.so
utils/   інтерфейс (osmem.h), struct block_meta (block_meta.h), printf без heap
tests/   тести: snippets/ (програми), ref/ (еталонні трейси), run_tests.py
docs/    умова завдання
```

## Етапи розробки

- [x] Етап 0 — заготовка проєкту
- [ ] Етап 1 — великі блоки через `mmap`/`munmap`
- [ ] Етап 2 — преалокація heap (128 KB) та `brk`-алокації
- [ ] Етап 3 — best fit, split, coalesce, розширення останнього блоку
- [ ] Етап 4 — `os_calloc`
- [ ] Етап 5 — `os_realloc`
- [ ] Етап 6 — перевірка витоків пам'яті, стиль коду, скріншоти тестів

## Збірка та тестування (Linux / WSL)

Залежності (Ubuntu):

```sh
sudo apt install -y build-essential python-is-python3 ltrace valgrind
```

Збірка бібліотеки:

```sh
cd src && make
```

Запуск усіх тестів:

```sh
cd tests && make check
```

Запуск одного тесту:

```sh
cd tests && python3 run_tests.py test-malloc-preallocate
```
