# iterator — CLAUDE.md

## Purpose

Generic sequential collection with a stateful cursor for Karel programs. Fills the gap left by Karel's lack of native generics, iterators, or standard containers. Two backends — Array (static, fixed-size) and Path (dynamic, linked-list) — share an identical traversal API. Used independently; no other Ka-Boost module imports iterator as a dependency at present.

Layer 3 — depends on: `ktransw-macros`, `errors`, `Strings`, `systemlib`, `registers`, `pose`, `TPElib`.

---

## Repository Layout

```
lib/iterator/
├── package.json                        rossum manifest
├── readme.md                           developer docs
├── lib/
│   ├── array/
│   │   ├── iteratorarray.klc           Array iterator class body (GPP template)
│   │   ├── iteratorarray.klh           Header — all public routine declarations
│   │   └── iteratorarray.tpp           TP+ namespace (Iterarr::RESET…ISNULL)
│   └── path/
│       ├── iteratorpath.klc            Path iterator class body (GPP template)
│       ├── iteratorpath.klh            Header — all public routine declarations
│       └── iteratorpath.tpp           TP+ namespace (Iterpath::RESET…ISNULL)
└── test/
    ├── config/
    │   ├── array/
    │   │   ├── default_int_array.klt   Config: INTEGER in Array
    │   │   └── default_custom_array.klt Config: custom struct in Array
    │   └── path/
    │       ├── default_int_path.klt    Config: INTEGER wrapped in t_INTEGER for Path
    │       ├── default_path_path.klt   Config: XYZWPR wrapped in t_POSE for Path
    │       └── default_custom_path.klt Config: custom struct in Path
    ├── test_itr_arr.kl                 KUnit tests for array backend
    ├── test_itr_pth.kl                 KUnit tests for path backend
    └── test_iter_struct.tpp            TP+ integration tests for both backends
```

---

## Full API Reference

### Common Interface (Array and Path)

| Routine | Description |
|---------|-------------|
| `new` | Initialize iterator: set cursor to 1, clear storage, nullify `nde` |
| `delete` | Reset cursor to 0, clear storage, nullify `nde` |
| `push(n : ITR_TYPE)` | Append item to end; errors if array is full |
| `pop : ITR_TYPE` | Remove and return last item; returns null sentinel if empty |
| `next : ITR_TYPE` | Return item at current cursor, then increment; returns null sentinel past end |
| `prev : ITR_TYPE` | Return item at current cursor, then decrement; returns null sentinel before start |
| `get : ITR_TYPE` | Return item at current cursor without moving it |
| `get_index(i) : ITR_TYPE` | Return item at absolute index `i` without moving cursor |
| `set_index(i)` | Move cursor to `i`; negative indices supported (−1 = last, −2 = second-to-last) |
| `insert(index_, n : ITR_TYPE)` | Insert item at `index_`, shift remaining items right |
| `is_empty : BOOLEAN` | TRUE if collection is empty |
| `is_null : BOOLEAN` | TRUE if cursor is out of bounds OR current node is uninitialized (config-defined) |
| `len : INTEGER` | Number of items: `length` (array) or `PATH_LEN(this)` (path) |

### Path-Only Extensions

| Routine | Description |
|---------|-------------|
| `take(i, out_pth)` | `COPY_PATH` from current index for `i` nodes into `out_pth` |
| `skip(i) : ITR_TYPE` | Advance cursor `i` positions; return item at new position |

### Wrap / Unwrap (Path + config-dependent)

These are **defined in the `.klt` config file**, not in the `.klc`. They are injected via GPP macros.

| Routine | Description |
|---------|-------------|
| `wrap(value)` | Construct `ITR_TYPE` struct from native value and `push` |
| `wrap_insert(index_, value)` | Construct `ITR_TYPE` and `insert` at index |
| `unwrap : ITR_UNWRAP_TYPE` | Extract native value from `nde` (current node) |
| `unwrap_pop : ITR_UNWRAP_TYPE` | `pop` then extract native value |
| `unwrap_next : ITR_UNWRAP_TYPE` | `next` then extract native value |
| `unwrap_prev : ITR_UNWRAP_TYPE` | `prev` then extract native value |
| `unwrap_from(n : ITR_TYPE) : ITR_UNWRAP_TYPE` | Extract native value from any node |

### TP+ Enum Constants

Defined in both `iteratorarray.tpp` and `iteratorpath.tpp`:

| Constant | Value | Operation |
|----------|-------|-----------|
| `RESET` | 1 | `new` |
| `PUSH` | 2 | push / wrap |
| `POP` | 3 | pop / unwrap_pop |
| `INSERT` | 4 | insert / wrap_insert |
| `GET` | 5 | set_index + get / unwrap |
| `NEXT` | 6 | next / unwrap_next |
| `PREV` | 7 | prev / unwrap_prev |
| `ISNULL` | 8 | is_null → flag |

---

## Configuration File Structure (`.klt`)

The `.klt` config drives all generic expansion. Required macros:

### Array Config Required Macros

```
ITR_TYPE = INTEGER              -- stored element type (any atomic or struct)
ARRAY_SIZE = 10                 -- fixed capacity

set_null_array                  -- macro to clear all array elements
set_null_nde                    -- macro to clear the nde scratch variable

define_is_null(parent)          -- macro defining is_null routine body
ITER_TP_VARS                    -- extra variables for TP dispatch (func_, int_, etc.)
REGMAPPGET                      -- maps nde to registers for TP retrieval
ITER_TP_INTERFACE               -- select-case dispatch body for TP callers
```

### Path Config Required Macros

```
ITR_TYPE = t_INTEGER            -- stored type (MUST be a struct for PATH nodedata)
ITR_UNWRAP_TYPE = INTEGER       -- exposed native type

set_null                        -- macro to return an uninitialized ITR_TYPE
define_is_null(parent)          -- macro defining is_null routine body
define_itr_headers(parent)      -- declares wrap/unwrap routines in header
ITER_TP_VARS                    -- extra variables for TP dispatch
REGMAPPGET                      -- maps nde to registers
ITER_TP_INTERFACE               -- select-case dispatch body
```

---

## Core Patterns

### Pattern 1: Integer Array Iterator

```karel
-- Instantiate
%class myarr('iteratorarray.klc','iteratorarray.klh','default_int_array.klt')

-- Use
myarr__new
myarr__push(10)
myarr__push(20)
myarr__push(30)

-- Forward traversal
WHILE NOT myarr__is_null DO
  val = myarr__next
ENDWHILE

-- Cleanup
myarr__delete
```

### Pattern 2: Integer Path Iterator (with wrap/unwrap)

Karel PATHs require struct node types — atomic types must be wrapped.

```karel
-- Instantiate with wrapping config
%class mypath('iteratorpath.klc','iteratorpath.klh','default_int_path.klt')

-- Push atomics via wrap (config generates t_INTEGER { v : INTEGER })
mypath__new
mypath__wrap(5)
mypath__wrap(10)
mypath__wrap(15)

-- Traverse and unwrap
WHILE NOT mypath__is_null DO
  val = mypath__unwrap_next   -- returns ITR_UNWRAP_TYPE (INTEGER)
ENDWHILE
```

### Pattern 3: XYZWPR Path (pose storage)

```karel
%class posepath('iteratorpath.klc','iteratorpath.klh','default_path_path.klt')

posepath__new
posepath__wrap(POS(0,80,0,0,0,0,(ZEROPOS(1).Config_data)))
posepath__wrap(POS(48,64,10,0,0,0,(ZEROPOS(1).Config_data)))

WHILE NOT posepath__is_null DO
  current_pose = posepath__unwrap_next   -- returns XYZWPR
ENDWHILE
```

### Pattern 4: Custom Struct in Path with TP Register Mapping

```karel
-- Config defines t_TSTCUSTOM { str, reg1, reg2, pose } and maps to SR[1], R[2], R[3], PR[4]
%class mydata('iteratorpath.klc','iteratorpath.klh','default_custom_path.klt')

mydata__new
mydata__wrap('hello', 1, 3.14, POS(0,80,0,0,0,0,(ZEROPOS(1).Config_data)))
mydata__wrap('world', 23, 6.28, POS(48,64,10,0,0,0,(ZEROPOS(1).Config_data)))

-- unwrap_next populates registers automatically via REGMAPPGET
WHILE NOT mydata__is_null DO
  mydata__unwrap_next
  str_val  = registers__get_string(1)
  int_val  = registers__get_int(2)
  real_val = registers__get_real(3)
  pose_val = pose__get_posreg_xyz(4, 1)
ENDWHILE
```

### Pattern 5: Negative Index Access

Both backends support Python-style negative indexing. −1 is last, −2 is second-to-last.

```karel
-- After pushing 5 elements:
myarr__set_index(-1)    -- cursor → last element
val = myarr__get        -- returns element 5

myarr__set_index(-2)    -- cursor → second-to-last
val = myarr__get        -- returns element 4

-- Backward traversal from end
myarr__set_index(-1)
WHILE NOT myarr__is_null DO
  val = myarr__prev
ENDWHILE
```

### Pattern 6: TP+ Dispatch

```ruby
-- In .tpp file
.require iteratorpath
.require iteratorarray

-- Reset iterator
tstist(Iterpath::RESET)

-- Push custom struct (args: func, str, reg1, reg2, pose_reg)
tstist(Iterpath::PUSH, 'hello', 1, 3.14, &poseset)

-- Is null? Result → F[20]
check = tstist(Iterpath::ISNULL)

-- Get next item (result goes to registers via REGMAPPGET)
tstist(Iterpath::NEXT)
str  = SR[1]
num  = R[2]
```

---

## Common Mistakes

| Mistake | Symptom | Fix |
|---------|---------|-----|
| Using an atomic type (INTEGER, REAL) directly as `ITR_TYPE` for a Path iterator | Karel compile error: PATH nodedata must be a structure | Wrap atomic in a struct (`t_INTEGER { v : INTEGER }`) and use `ITR_UNWRAP_TYPE` for the exposed type |
| Not calling `new` before first use | Uninitialized cursor; `is_null` returns TRUE immediately; `push` may corrupt state | Always call `__new` before any other operation |
| Forgetting `ARRAY_SIZE` in array config | Default or zero capacity; `push` immediately errors with array full | Set `ARRAY_SIZE` to the maximum expected element count |
| Omitting `set_null_array` or `set_null_nde` in array config (defining only one) | is_null check misbehaves; stale data from previous `new` cycle | Both must be defined — they initialize the scratch variable and the backing array separately |
| Calling `next` without checking `is_null` in the WHILE condition | Loop reads one past end; last value is the null sentinel (uninitialized) | Use `WHILE NOT myobj__is_null DO ... val = myobj__next` pattern |
| Setting `REGMAPPGET` for atomic types using `map_select_getter` instead of `registerstp__get_karel_*` | Compile error or wrong register read at runtime | Atomic types need `registerstp__get_karel_int/real/string/xyz` calls directly |
| Calling `take` or `skip` on Array iterator | Compile error — those routines only exist in Path variant | Use `get_index` for random access in Array; switch to Path if you need `take`/`skip` |

---

## Dependencies

### Depends On
- `ktransw-macros` — `%class`, `declare_member`, `funcname`, `classfunc` macros
- `errors` — `karelError`, `CHK_STAT`
- `Strings` — string operations for error messages
- `systemlib` — `SET_UNINIT_*` macros, type wrappers (`t_INTEGER`, `t_POSE`)
- `registers` — `registers__set_boolean` for TP null flag output
- `pose` — `pose__get_posreg_xyz` in XYZWPR path configs
- `TPElib` — `tpe__get_int/real/string_arg` for reading TP AR[] arguments

### Dependents
No other Ka-Boost module currently imports iterator as a dependency. It is a standalone utility consumed directly by user programs.

---

## Build / Integration Notes

- Instantiate with a three-argument `%class` call; the third argument is the `.klt` config
- The `.klt` config must be on the `ROSSUM_PKG_PATH` or referenced by relative path
- `%class` expansion inlines all GPP macros from the config; ITR_TYPE is substituted everywhere
- Array capacity is compile-time fixed by `ARRAY_SIZE`; Path capacity is runtime-dynamic
- TP+ `.tpp` integration requires `.require iteratorpath` or `.require iteratorarray` to import the enum namespace
- No `.pc` binary is produced for iterator itself — it expands inline into the consuming program
- To build with tests: `rossum .. -w -o -t` from the module root
