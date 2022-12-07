[rossum]: https://github.com/kobbled/rossum
[TP-Plus]: https://github.com/kobbled/tp_plus
[Ka-Boost]: https://github.com/kobbled/Ka-Boost
# Iterator

FANUC KAREL interface for dealing with iterators. Also provides an interface for using PATHS, and ARRAYS in TP/TP+.

## Install

* This package is a part of the larger library package [Ka-Boost]. Please install from there.
* The package manager [rossum] is used to build all [Ka-Boost] packages. Read through the [rossum]( guide, and make sure [Ka-Boost], and this package are in the `ROSSUM_PKG_PATH` environment variable.

## Building

<details>
  <summary>Click to Reveal</summary>
  
* Open the _Iteraror_ root folder in a terminal.
* Set your roboguide configuration with:
```bat
del /f robot.ini
setrobot
```
* Create a build directory
```bat
mkdir build
cd build
```
* Build all tests, and send to robot
```bat
rossum .. -w -o -t
ninja
kpush
```
</details>

## Usage

Iterators can be specified as either `Arrays`, or `Paths`. Declaring an array can be done with:

```
%class <obj_name>('iteratorarray.klc','iteratorarray.klh','<array_config_filename>.klt')
```
where `<obj_name>` should be replaced with the object name determined by the user. And `<array_config_filename>` should be replaced with the correct configuration file.

> [!**NOTE**]
> An `Array` type size is statically allocated, and cannot be made larger during runtime. A `Path` type size is dynamically allocated and can be made larger or smaller during runtime. An index is used to keep track of how many items have been pushed onto an `Array`; however, if you run out of room in the arry you will ahve to resize it.

Declaring a path can be done with:

```
%class <obj_name>('iteratorpath.klc','iteratorpath.klh','<path_config_filename>.klt')
```

The defined item type can be added and removed from the iterator with `<obj_name>__push` / `<obj_name>__pop` commands. The iterator can be traversed with `<obj_name>__next` / `<obj_name>__prev` commands. The index pointer can be changed with `<obj_name>__set_index(<new_index>)`. And the current index can be returned with `<obj_name>__get`.

>[!**IMPORTANT**]
> `Path` items cannot be directly set to atomic types (i.e. INTEGER, REAL, BOOLEAN, STRING, etc ...). A support file `systemlib.type.klt` from [kl-system](https://github.com/kobbled/kl-system) can be used to obfuscate the usage of atomic types in paths. In the configuration file you will notice a `ITR_TYPE` and `ITR_UNWRAP_TYPE` variable to expose the underlying type to the interface.

Custom structs can be be declared either directly within the iterator object using the `ITER_STRUCT` argument, or can be included from an external file with the `ITER_STRUCT_IMPORT` argument.

Iterator objects can also be used in TP/TP+ programs. See **test/test_iter_struct.tpp** for usage. To retrieve the data in the iterator the item struct will be mapped to TP registers (see **test/test_regmap.kl** [kl-registers](https://github.com/kobbled/kl-registers) for using register mapping outside of an iterator). These mappings are defined in each configuration file using the `REGMAPPGET` macro. For example a custom struct can be mapped to various registers like:

```c
%define REGMAPPGET `
  map_select_getter('class_name', 'nde', 'SR', 1, 'str' , 'STRING')
  map_select_getter('class_name', 'nde', 'R',  2, 'reg1', 'INTEGER')
  map_select_getter('class_name', 'nde', 'R',  3, 'reg2', 'REAL')
  map_select_getter('class_name', 'nde', 'PR', 4, 'pose', 'XYZWPR')
`
```

Each line item refers to each member of the struct. The last 4 arguments need to be modified. Argument 3 is the register type to map the struct member to('R', 'PR', 'SR', 'F', 'DO', etc..). Argument 4 is the register number. argument 5 is the name of the member in the struct, and Argument 6 is the type of the struct member. 

>[!NOTE]
> TP functionality can be disabled and excluded from the configuration file by removing the `ENABLE_REGMAPPING` macro.

For atomic type items `REGMAPPGET` must be defined with the appropriate getter function from `registerstp.klh` in [kl-registers](https://github.com/kobbled/kl-registers). For example setting the `int_` variable in the iterator object to `R[2]` can be done as so: 

```
%define REGMAPPGET `
registerstp__get_karel_int('classname', 'int_', 2)
`
```

### Creating Configuration files

Example configuration files are found in **test/config/array**, or **test/config/path**.

First define the user struct, or import file, and set `ITR_TYPE` (and `ITR_UNWRAP_TYPE` when using a path). 

#### Paths

When defining defining the user interface functions for path types, you will notice a bunch of `unwrap` functions in `define_itr_headers`:

```c
%define define_itr_headers(parent) `
declare_member(parent,is_null,parent,isnll)
ROUTINE is_null : BOOLEAN FROM parent
declare_member(parent,wrap,parent,wrap)
ROUTINE wrap(str : STRING; reg1 : INTEGER; reg2 : REAL; pose : XYZWPR) FROM parent
declare_member(parent,wrap_insert,parent,wrpin)
ROUTINE wrap_insert(index_ : INTEGER; str : STRING; reg1 : INTEGER; reg2 : REAL; pose : XYZWPR) FROM parent
declare_member(parent,unwrap,parent,unwrp)
ROUTINE unwrap FROM parent
declare_member(parent,unwrap_pop,parent,uwpop)
ROUTINE unwrap_pop FROM parent
declare_member(parent,unwrap_next,parent,uwnxt)
ROUTINE unwrap_next FROM parent
declare_member(parent,unwrap_prev,parent,uwprv)
ROUTINE unwrap_prev FROM parent
`
```

These functions transform output of the base class functions (push,pop,next,prev) to the unwrap type `ITR_UNWRAP_TYPE`, or for the user struct example map the struct to a register set on the teach pendant.

`ITER_TP_INTERFACE` macro is what needs to be filled out to use iterator object in TP programs. The contents gets placed in the entry point of the class, where different function members will be called through a select block. In **test/test_iter_struct.tpp** function call can be seen as:

**TP+**
```ruby
tstist(Iterpath::PUSH, 'hello', 1, 3.14, &poseset)
```

where the first arugment is the function to run. This enum is defined in **lib/path/iteratorpath.tpp**, and is defined in the karel class as:

**KAREL**
```c
CONST
  --tp functions
  ITER_RESET  = 1
  ITER_PUSH   = 2
  ITER_POP    = 3
  ITER_INSERT = 4
  ITER_GET    = 5
  ITER_NEXT   = 6
  ITER_PREV   = 7
  ITER_NULL   = 8
```

The next arguments vary based on the function that is being called. For the `t_INTEGER` path type defined in **default_int_path.klt**, `ITER_TP_INTERFACE` is defined as:

```c
%define ITER_TP_INTERFACE `
  --tpe class function
  func_ = tpe__get_int_arg(1)

  SELECT func_ OF
    CASE(ITER_RESET):
      new
    CASE(ITER_PUSH):
      wrap(tpe__get_int_arg(2))
    CASE(ITER_POP):
      int_ = unwrap_pop
      REGMAPPGET
    CASE(ITER_INSERT):
      wrap_insert(tpe__get_int_arg(3), tpe__get_int_arg(2))
    CASE(ITER_GET):
      set_index(tpe__get_int_arg(2))
      int_ = unwrap
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

where `tpe__get_int_arg(2)` is passing the integer argument from the TP call into the object, and `REGMAPPGET` is handling the return by pop, get, next, and prev, mapping it to a number register.

For a custom struct arugments are handled like so:

```c
CASE(ITER_PUSH):
    tp_nde.str = tpe__get_string_arg(2)
    tp_nde.reg1 = tpe__get_int_arg(3)
    tp_nde.reg2 = tpe__get_real_arg(4)
    tp_nde.pose = pose__get_posreg_xyz(tpe__get_int_arg(5), 1)

    wrap(tp_nde.str, tp_nde.reg1, tp_nde.reg2, tp_nde.pose)
```

where each TP argument is mapped into the user struct, and then passed through into the member function.

checking if the iterator is null (i.e. `ITER_NULL`), the result must be passed to a io flag. This is done in TP+ by:

```ruby
check := F[1]
check = tstist(Iterpath::ISNULL)
```

#### Arrays

The configuration files for array will be simpiler than for paths. No `unwrap` methods are needed, as atomic types can be set as the array type. The array is sized with the `ARRAY_SIZE` macro.  

`set_null` is split into two functions `set_null_array`, and `set_null_nde`. make sure to set both for it to function properly. 

Set an extra return variable in `ITER_TP_VARS` for storing the result of push, next, prev etc.. for example **default_int_array.klt** defines `int_` to store the result, which is then used in `REGMAPPGET` to map to a register:

```c
%define ITER_TP_VARS `
  func_ : INTEGER
  int_ : ITR_TYPE
`
```