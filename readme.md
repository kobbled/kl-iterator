[rossum]: https://github.com/kobbled/rossum
[TP-Plus]: https://github.com/kobbled/tp_plus
[Ka-Boost]: https://github.com/kobbled/Ka-Boost

# Iterator

Generic sequential collection with a stateful cursor for FANUC Karel programs — provides Array and Path backends with an identical traversal API, plus TP/TP+ callable interfaces.

## Overview

Karel has no built-in iterators, lists, or generics. `iterator` fills that gap with two GPP-template-based collection classes:

- **Array iterator** — backed by a fixed `ARRAY[N]`, no wrapping required for atomic types, compile-time capacity.
- **Path iterator** — backed by a Karel `PATH` (dynamic linked list), runtime-resizable, but atomic types must be wrapped in a struct because `PATH nodedata` requires a structure type.

Both share the same traversal API (`push`, `pop`, `next`, `prev`, `set_index`, `get_index`, `get`, `len`, `is_empty`, `is_null`). A `.klt` configuration file drives all generic expansion — you supply the element type, null-handling macros, and optional TP register mappings once; the class template does the rest.

## Files

| File | Purpose |
|------|---------|
| `lib/array/iteratorarray.klc` | Array iterator class body (GPP template) |
| `lib/array/iteratorarray.klh` | Public routine declarations for array backend |
| `lib/array/iteratorarray.tpp` | TP+ namespace `Iterarr` — enum constants for TP dispatch |
| `lib/path/iteratorpath.klc` | Path iterator class body (GPP template) |
| `lib/path/iteratorpath.klh` | Public routine declarations for path backend |
| `lib/path/iteratorpath.tpp` | TP+ namespace `Iterpath` — enum constants for TP dispatch |
| `test/config/array/default_int_array.klt` | Example config: INTEGER in Array |
| `test/config/array/default_custom_array.klt` | Example config: custom struct in Array |
| `test/config/path/default_int_path.klt` | Example config: INTEGER wrapped in `t_INTEGER` for Path |
| `test/config/path/default_path_path.klt` | Example config: XYZWPR wrapped in `t_POSE` for Path |
| `test/config/path/default_custom_path.klt` | Example config: custom struct in Path |
| `test/test_itr_arr.kl` | KUnit tests for array backend |
| `test/test_itr_pth.kl` | KUnit tests for path backend |
| `test/test_iter_struct.tpp` | TP+ integration tests for both backends |

## API Reference

### Common Methods (Array and Path)

```karel
ROUTINE new FROM class_name
-- Initialize: set cursor to 1, clear storage, nullify scratch variable

ROUTINE delete FROM class_name
-- Reset cursor to 0, clear storage

ROUTINE push(n : ITR_TYPE) FROM class_name
-- Append item to end. Error if array is full.

ROUTINE pop : ITR_TYPE FROM class_name
-- Remove and return last item. Returns null sentinel if empty.

ROUTINE next : ITR_TYPE FROM class_name
-- Return item at current cursor, then increment.
-- Returns null sentinel when cursor moves past end.

ROUTINE prev : ITR_TYPE FROM class_name
-- Return item at current cursor, then decrement.
-- Returns null sentinel when cursor moves before start.

ROUTINE get : ITR_TYPE FROM class_name
-- Return item at current cursor without moving it.

ROUTINE get_index(i : INTEGER) : ITR_TYPE FROM class_name
-- Return item at index i without moving cursor. Supports negative indices.

ROUTINE set_index(i : INTEGER) FROM class_name
-- Move cursor to i. Negative indices: -1 = last, -2 = second-to-last, etc.

ROUTINE insert(index_ : INTEGER; n : ITR_TYPE) FROM class_name
-- Insert item at index_, shift remaining items right.

ROUTINE is_empty : BOOLEAN FROM class_name
-- TRUE if collection has no items.

ROUTINE is_null : BOOLEAN FROM class_name
-- TRUE if cursor is out of bounds OR current item is uninitialized.
-- Exact check is defined per-config via define_is_null macro.

ROUTINE len : INTEGER FROM class_name
-- Number of items currently stored.
```

### Path-Only Methods

```karel
ROUTINE take(i : INTEGER; out_pth : PATH nodedata = ITR_TYPE) FROM class_name
-- COPY_PATH from current index for i nodes into out_pth.

ROUTINE skip(i : INTEGER) : ITR_TYPE FROM class_name
-- Advance cursor i positions and return item at new position.
```

### Wrap / Unwrap Methods (Path only, config-defined)

Path iterators for atomic types need a wrapper struct. The config generates these methods:

```karel
ROUTINE wrap(value : ITR_UNWRAP_TYPE) FROM class_name
-- Construct ITR_TYPE struct from native value and push.

ROUTINE wrap_insert(index_ : INTEGER; value : ITR_UNWRAP_TYPE) FROM class_name
-- Construct ITR_TYPE struct and insert at index_.

ROUTINE unwrap : ITR_UNWRAP_TYPE FROM class_name
-- Extract native value from current node (nde).

ROUTINE unwrap_pop : ITR_UNWRAP_TYPE FROM class_name
-- pop, then extract native value.

ROUTINE unwrap_next : ITR_UNWRAP_TYPE FROM class_name
-- next, then extract native value.

ROUTINE unwrap_prev : ITR_UNWRAP_TYPE FROM class_name
-- prev, then extract native value.

ROUTINE unwrap_from(n : ITR_TYPE) : ITR_UNWRAP_TYPE FROM class_name
-- Extract native value from any node (not just current).
```

### TP+ Dispatch Enums

Both `Iterarr` and `Iterpath` namespaces export the same constants:

| Constant | Value |
|----------|-------|
| `RESET` | 1 |
| `PUSH` | 2 |
| `POP` | 3 |
| `INSERT` | 4 |
| `GET` | 5 |
| `NEXT` | 6 |
| `PREV` | 7 |
| `ISNULL` | 8 |

---

## Common Patterns

### Pattern 1: Integer Array — Basic Use

The simplest case. Use when you know the maximum size at compile time and don't need dynamic resizing.

```karel
-- Instantiation (three-arg %class: implementation, header, config)
%class myarr('iteratorarray.klc','iteratorarray.klh','default_int_array.klt')

-- In a routine:
myarr__new
myarr__push(10)
myarr__push(20)
myarr__push(30)

-- Forward traversal (is_null checks bounds AND initialization)
WHILE NOT myarr__is_null DO
  val = myarr__next
  -- process val
ENDWHILE

myarr__delete
```

### Pattern 2: Integer Path — Wrap/Unwrap for Atomic Types

Karel `PATH nodedata` must be a struct. Use `wrap`/`unwrap` to hide this detail.
The config defines `t_INTEGER { v : INTEGER }` behind the scenes.

```karel
%class mypath('iteratorpath.klc','iteratorpath.klh','default_int_path.klt')

mypath__new
mypath__wrap(5)     -- wraps INTEGER 5 into t_INTEGER, appends to PATH
mypath__wrap(10)
mypath__wrap(15)

-- unwrap_next = next() then extract .v field
WHILE NOT mypath__is_null DO
  val = mypath__unwrap_next   -- returns INTEGER directly
ENDWHILE
```

### Pattern 3: XYZWPR Pose Path

`default_path_path.klt` wraps `XYZWPR` into `t_POSE { v : XYZWPR }`.

```karel
%class posepath('iteratorpath.klc','iteratorpath.klh','default_path_path.klt')

posepath__new
posepath__wrap(POS(0,80,0,0,0,0,(ZEROPOS(1).Config_data)))
posepath__wrap(POS(48,64,10,0,0,0,(ZEROPOS(1).Config_data)))
posepath__wrap(POS(77,21,20,0,0,0,(ZEROPOS(1).Config_data)))

WHILE NOT posepath__is_null DO
  current_pose = posepath__unwrap_next   -- returns XYZWPR
  -- use current_pose
ENDWHILE
```

### Pattern 4: Custom Struct in Path with TP Register Mapping

When you need to read iterator data from TP programs, map struct fields to registers in the config via `REGMAPPGET`. Calling `unwrap_next` then automatically populates those registers.

**In your config `.klt`:**
```c
%define REGMAPPGET `
  map_select_getter('class_name', 'nde', 'SR', 1, 'str' , 'STRING')
  map_select_getter('class_name', 'nde', 'R',  2, 'reg1', 'INTEGER')
  map_select_getter('class_name', 'nde', 'R',  3, 'reg2', 'REAL')
  map_select_getter('class_name', 'nde', 'PR', 4, 'pose', 'XYZWPR')
`
```

**In Karel:**
```karel
%class mydata('iteratorpath.klc','iteratorpath.klh','my_custom_config.klt')

mydata__new
mydata__wrap('hello', 1, 3.14, POS(0,80,0,0,0,0,(ZEROPOS(1).Config_data)))
mydata__wrap('world', 23, 6.28, POS(48,64,10,0,0,0,(ZEROPOS(1).Config_data)))

WHILE NOT mydata__is_null DO
  mydata__unwrap_next                          -- populates SR[1], R[2], R[3], PR[4]
  str_val  = registers__get_string(1)
  int_val  = registers__get_int(2)
  real_val = registers__get_real(3)
  pose_val = pose__get_posreg_xyz(4, 1)
ENDWHILE
```

### Pattern 5: Negative Index Access / Backward Traversal

Both backends support Python-style negative indexing.

```karel
-- After pushing 5 elements:
myarr__set_index(-1)    -- move cursor to last element (index 5)
val = myarr__get        -- returns element 5 without advancing

myarr__set_index(-2)    -- move cursor to second-to-last
val = myarr__get

-- Backward traversal from end:
myarr__set_index(-1)
WHILE NOT myarr__is_null DO
  val = myarr__prev     -- returns current, then decrements
ENDWHILE
```

### Pattern 6: TP+ / TP Dispatch

Iterator objects can be called from TP programs via a select-case dispatch. The config's `ITER_TP_INTERFACE` macro defines the dispatch body.

**In your `.tpp` file:**
```ruby
.require iteratorpath

-- Reset iterator
tstist(Iterpath::RESET)

-- Push custom struct fields as TP arguments
tstist(Iterpath::PUSH, 'hello', 1, 3.14, &poseset)

-- Get next item (result written to registers via REGMAPPGET)
tstist(Iterpath::NEXT)
str_val = SR[1]
num_val = R[2]

-- Check null → result written to a flag register
check := F[20]
check = tstist(Iterpath::ISNULL)
```

**Corresponding `ITER_TP_INTERFACE` in config (for INTEGER path):**
```c
%define ITER_TP_INTERFACE `
  func_ = tpe__get_int_arg(1)
  SELECT func_ OF
    CASE(ITER_RESET):
      new
    CASE(ITER_PUSH):
      wrap(tpe__get_int_arg(2))
    CASE(ITER_POP):
      int_ = unwrap_pop
      REGMAPPGET
    CASE(ITER_NEXT):
      int_ = unwrap_next
      REGMAPPGET
    CASE(ITER_PREV):
      int_ = unwrap_prev
      REGMAPPGET
    CASE(ITER_NULL):
      registers__set_boolean(tpe__get_int_arg(2), is_null)
    ELSE:
  ENDSELECT
`
```

---

## Creating a Configuration File

Copy the closest example from `test/config/` and modify:

### Array Config Checklist

1. Define your element type: `%define ITR_TYPE INTEGER` (or a struct)
2. Set capacity: `%define ARRAY_SIZE 10`
3. Define `set_null_array` — clears backing array elements
4. Define `set_null_nde` — clears the scratch variable `nde`
5. Define `define_is_null(parent)` — body of `is_null` routine
6. Define `ITER_TP_VARS` — extra variables for TP dispatch (`func_`, plus one per return type)
7. Define `REGMAPPGET` — how to write `nde` to registers (for TP retrieval)
8. Define `ITER_TP_INTERFACE` — select-case dispatch for TP callers
9. Omit `ENABLE_REGMAPPING` if TP integration is not needed

### Path Config Checklist

1. Define struct for PATH nodedata: `%define ITR_TYPE t_MYTYPE`
2. Define `ITR_UNWRAP_TYPE` — native type exposed by `unwrap_*` routines
3. Define `set_null` — returns an uninitialized `ITR_TYPE` (used as sentinel)
4. Define `define_is_null(parent)` — checks `UNINIT(nde.<key_field>)` or index bounds
5. Define `define_itr_headers(parent)` — header declarations for wrap/unwrap routines
6. Define wrap/unwrap implementation macros
7. Define `ITER_TP_VARS`, `REGMAPPGET`, `ITER_TP_INTERFACE` (if TP needed)

---

## Common Mistakes

| Mistake | Symptom | Fix |
|---------|---------|-----|
| Using an atomic type (INTEGER, REAL) as `ITR_TYPE` for a Path iterator | Karel compile error — PATH nodedata requires a structure | Wrap the atomic in a struct (`t_INTEGER { v : INTEGER }`) and set `ITR_UNWRAP_TYPE` to the atomic type |
| Not calling `new` before first use | `is_null` returns TRUE immediately; `push` may corrupt storage | Always call `__new` before any other method |
| Forgetting `ARRAY_SIZE` in array config | Zero or undefined capacity; `push` errors immediately | Set `ARRAY_SIZE` to the maximum expected count |
| Defining only `set_null_array` or only `set_null_nde` (not both) in array config | Stale data appears after `new` is called; `is_null` misbehaves | Both macros are required — they reinitialize the array and the scratch variable independently |
| Reading one element past end (no `is_null` guard) | Last iteration returns the uninitialized null sentinel | Use `WHILE NOT myobj__is_null DO ... val = myobj__next` pattern |
| Using `map_select_getter` in `REGMAPPGET` for an atomic `ITR_TYPE` | Compile error or wrong register at runtime | For atomic types, use `registerstp__get_karel_int/real/string/xyz` directly |
| Calling `take` or `skip` on an Array iterator | Compile error — those methods only exist in the Path variant | Use `get_index` for random access in Array; switch to Path backend if you need `take`/`skip` |

---

## Build Flow

The iterator module produces no `.pc` binary of its own. The `%class` directive expands the template inline into your program at compile time.

```shell
# From your module (which depends on iterator):
cd lib/<your_module>
del /f robot.ini && setrobot
mkdir build && cd build
rossum .. -w -o          # resolves iterator as a dependency
ninja                    # compiles — iterator code is inlined
kpush                    # deploy .pc to controller
```

To build the iterator tests directly:

```shell
cd lib/iterator
del /f robot.ini && setrobot
mkdir build && cd build
rossum .. -w -o -t       # include test programs
ninja
kpush
```

See the top-level [Ka-Boost] readme for full build and environment setup instructions.
