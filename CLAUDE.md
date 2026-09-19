# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

This file is project memory for any Claude/LLM coding agent working in this
repo. It records verified facts and deliberate design decisions from past
sessions so you don't have to re-derive them or reintroduce bugs that were
already found and fixed. Where marked "unverified" or "not yet checked,"
confirm against actual source/actual test runs before relying on it.

Scope of this file: **Animula only** (the embedded VM runtime). laco (the
compiler that targets it) and Saruman (the JTAG/SWD debugger) have their
own CLAUDE.md files.

---

## Build, run, and formatting

**There is no build system in this repo** — no `Makefile`, CMake, `.github`
CI, or test harness. The README says it plainly: "The code can not be built
directly, you have to use building project for specific platform." Animula
is compiled *into* one of two host projects that supply the real build and
the entry `main()`:

- **animula-linux** — POSIX host used for CI testing; runs a `.lef`
  directly (`./animula-vm foo.lef`).
- **animula-zephyr** — the real target (ZephyrRTOS on Cortex-M, e.g. the
  "Alonzo" board).

**Platform selection is one compile-time macro**, checked in `inc/vos.h`
(`#if defined ANIMULA_ZEPHYR … #elif defined ANIMULA_LINUX … #else #error
"Please specify a platform!"`). The host project passes `-DANIMULA_LINUX`
or `-DANIMULA_ZEPHYR`; the core never chooses. `vos.h` dispatches to one
per-platform OAL header (`inc/vos/oal/linux/vos.h` or
`inc/vos/oal/zephyr/vos.h`) which maps the `os_*` names
(`os_malloc`/`os_printk`/`os_memcpy`/etc.) onto that platform's libc/RTOS.
`inc/os.h` is the OS-abstraction entry the Core actually includes: it pulls
in `vos.h` and adds VM build config (endianness, `GLOBAL_*`, etc.).

**The VM does not compile Scheme.** The [laco
compiler](https://github.com/hardenedlinux/laco) turns `.scm` into `.lef`
(lightweight executable format) bytecode; this repo only loads and runs
`.lef`.

**Formatting**: `.clang-format` (LLVM-based, GNU brace style, 2-space
indent, 80-col, `PointerAlignment: Right`). Format with `clang-format -i
<file>`; there is no enforced lint step in this repo.

**Tests**: none live in this repo. CI testing happens in the animula-linux
project, or manually on the Alonzo board (debuggable via BLE).

**Entry-point flow** (`animula.c`): `animula_init()` allocates a `vm_t`,
inits the RAM heap/primitives/stdio/GC and binds the GC VM singleton
(`gc_bind_vm`); `animula_start(lef_loader)` runs `LEF_LOAD`, checks the
file's `LEF` signature, then either `vm_load_lef()` + `vm_run()` (a `.lef`
present) or `run_shell()` (no LEF — drops into the kernel shell).
`animula_clean()` tears down GC first, then the VM.

---

## 1. What this project is

Animula is an embedded Scheme **bytecode VM**, derived from Marc Feeley's
picobit, targeting Zephyr/Cortex-M and Linux. It runs **atop an RTOS**
rather than managing hardware directly  no thread model of its own;
ISR/Scheme boundary is handled via polling through primitives, not critical
sections. It executes `.lef` binaries produced by the laco compiler.

**Target hardware profile**: Cortex-M class MCUs (~100MHz / 96KB RAM
today, expected to improve over time), with TF-card filesystems. Must also
support a minimal configuration down to ~10KB RAM for basic algorithms.

**`object.h` is the single authoritative design document** for Animula's
object encoding. When code and `object.h` disagree, fix the code to match
`object.h`, never the reverse. This is a hard rule from the project owner.

**Architecture layering (OS/VOS abstraction)**: Animula core → prim layer
(business semantics only) → OS abstraction (`os_*` names) → VOS (platform
selector) → OAL (per-RTOS implementation, e.g. FreeRTOS/Zephyr). Concrete
files:
- `inc/vos.h` — the single platform selector (`#if ANIMULA_LINUX → include
  "vos/oal/linux/vos.h" / #elif ANIMULA_ZEPHYR → "vos/oal/zephyr/vos.h" /
  #else #error`). No arch/board/linker selection, no weak symbols.
- `inc/vos/oal/{linux,zephyr}/vos.h` — the OAL headers mapping the `os_*`
  names onto POSIX/libc (Linux) or Zephyr/newlib (Zephyr). Zephyr-only
  board content (e.g. `gpio.h`) lives under `inc/vos/oal/zephyr/`.
- `inc/os.h` — the OS-abstraction entry the Core includes: it includes
  `vos.h` and adds VM build config (endianness, `ANIMULA_BITS_*`,
  `ADDRESS_64`, `GLOBAL_*`, `MEMORY_TRACKER_*`, `PRE_ARN`/`PRE_OLN`).

**The `os_*` API is the OS abstraction, not `vos_*`.** Every capability the
old `os.h` provided belongs to the OS abstraction and must keep going
through `os_*`, even when on Linux it is just a thin alias onto libc/POSIX
(`os_malloc` → the VM heap manager, `os_printk` → `printf`, `os_memcpy` →
`memcpy`, …). Never bypass `os_*` for "portable C", and do not rename
`os_*` to `vos_*`. Current surface:
- memory: `os_malloc`/`os_calloc`/`os_free` (real functions in `memory.c`,
  backed by the raw allocator `__malloc`/`__calloc`/`__free` → libc).
- console: `os_printk`, `os_getchar`, `os_getline`; formatting:
  `os_snprintf`.
- string/memory/math: `os_memcpy`, `os_memset`, `os_strlen`, `os_strnlen`,
  `os_strncmp`, `os_strchr`, `os_strncpy`, `os_abs`, `os_fabs`, `os_usleep`.
- file: `os_open`/`os_read`/`os_close`/`os_file_exist` (implemented in
  `storage.c`, dispatching Linux vs Zephyr backend), plus the LEF-loading
  helpers `os_open_input_file`/`os_read_u32`/`os_read_u16`.
- time: `os_timestamp` (monotonic); platform: `get_platform_info()`;
  termination: `os_abort`.
- type layer: `inc/__types.h` is the single type header for both platforms
  (guarded by `ANIMULA_ZEPHYR`/`ANIMULA_LINUX`); the former
  `inc/vos/zephyr_types.h` is merged into it and deleted. VM type
  semantics/sizes/signedness/ABI are unchanged.

**Primitives must never call RTOS APIs directly** — everything RTOS-specific
goes through the OAL beneath the `os_*` contract. The contract is meant to
grow from real delivery history, not be designed top-down in advance.

---

## 2. Object / type model

Every value is an `Object` (`oattr attr` + `void *value`, packed tightly).
`oattr` packs `type` (6 bits) and `gc` (2 bits) into one byte. `otype_t` is
the tag enum (see `types.h`): pair/list/vector/closure/bytevector/
mut_bytevector/mut_string/string/symbol/primitive/procedure/imm_int/
rational_pos/rational_neg/complex_exact/complex_inexact/character/boolean/
null_obj/none/keyword/continuation/arbi_int, plus `unbooked = -1` as a
sentinel.

Rational numbers deliberately split sign into the type tag itself
(`rational_pos` vs `rational_neg`) specifically to avoid spending a bit on
sign inside the packed representation  this is a memory-footprint
optimization, not an oversight; don't "simplify" it into a single
`rational` type with a sign field.

### Two-layer object model (outer `Object` vs. inner heap value)

This distinction is the single most common source of confusion in past
sessions  internalize it before touching GC or allocation code:

| `otype_t` | inner struct | owns extra heap memory? |
|---|---|---|
| `pair` | `Pair` | no (just two `object_t`) |
| `list` | `List` | yes  internal `ListNode` chain |
| `vector` | `Vector` | yes  `object_t *vec` array |
| `bytevector`/`mut_bytevector` | `ByteVector`/`MutByteVector` | yes  `u8_t *vec` array |
| `closure_on_heap` | `Closure` | no separate alloc  `env[]` is a flexible array member in the same allocation |
| `mut_string` | *(none  raw `char*` in `.value`)* | yes, but **not pool-tracked at all** (see 6) |

The **outer** `Object` (stack-resident container, copied by value) is not
the same thing as the **inner** value registered in a pool. Only the inner
value goes through `gc_inner_obj_book` and gets the `.attr.gc` generation
tag that pool-based collection cares about. A standalone heap-boxed
"simple" value (via `animula_new_object` with no inner type) lives in
`obj_free_pool`. A value sitting directly on the VM stack, never separately
heap-boxed, isn't pool-tracked at all  its liveness is whatever the
enclosing frame's liveness is.

---

## 3. Calling conventions

Three call *modes*, tracked in `vm->attr.mode` / `vm->attr.shadow`:

- **`NORMAL_CALL`**: pushes a full 5-field prelude (`pc` placeholder,
  `local`, `fp`, `attr`, `closure`) via `SAVE_ENV()`'s default branch,
  clears `vm->attr.shadow`, sets a new `vm->fp`. Unwound later by
  `RESTORE()`.
- **`TAIL_CALL`**: does **nothing at all**  no prelude pushed, `vm->fp`
  untouched. By design: a genuine tail call needs no new frame, since
  control never returns to the current one. A `tail-call`-mode `prelude`
  bytecode asserts "the caller already arranged everything the callee
  needs, and nothing here needs to survive past this call."
- **`TAIL_REC`** (self-recursive tail call, reusing the same frame): sets
  `vm->attr.shadow = arity`, enabling `IS_SHADOW_FRAME()`/
  `COPY_SHADOW_FRAME()` to shift new argument values down into the
  *existing* frame position on the next `PRELUDE`, avoiding stack growth
  across recursive iterations. **Not the same mechanism as `TAIL_CALL`** 
  conflating the two was a real debugging dead-end in a past session.

**Confirmed hazard**: `SAVE_ENV()`'s `TAIL_CALL` branch **never touches
`vm->attr.shadow`** (only `NORMAL_CALL` clears it, only `TAIL_REC` sets
it). A stale shadow value from an earlier, unrelated `TAIL_REC` can persist
across an intervening `TAIL_CALL` and misfire `IS_SHADOW_FRAME()` later.
This is real and demonstrated, not theoretical  be careful with any code
path that mixes tail-call and tail-rec modes.

**Reference implementations, not equivalent**:
- `PROC_CALL` (used by `CALL_PROCEDURE`, the normal bytecode-level way to
  call a `procedure` object) is the canonical "how to correctly enter a
  callee": check `IS_SHADOW_FRAME`, clear `vm->closure`, set
  `vm->local = vm->fp + FPS`, jump.
- `apply_proc` (the C-level "synchronously call a procedure object and get
  its return value" path, used by `map` and `with-exception-handler`) is a
  **separate, less complete implementation of the same idea** and does not
  currently fully match `PROC_CALL`'s convention. (It was investigated and
  cleared of blame for the `lambda-lifting-2` bug below  see 9  but the
  incompleteness itself is still real; don't assume it's fully correct just
  because that particular bug wasn't its fault.)

**`vm->local` vs `vm->fp`**: `local` exists because "the prelude would
pre-execute before the actual call, so the local frame was hidden by
prelude" (verbatim comment in `types.h`'s `LambdaVM` struct). `fp` marks
where the *prelude* lives; `local` marks where the callee's own
arguments/locals actually start. They're related but **not
interchangeable**, and `local == fp + FPS` only holds when a real
`NORMAL_CALL`-style prelude was actually pushed immediately before.

---

## 4. Allocation path

- `animula_new_object(type)` / `NEW_OBJ` / `NEW_INNER_OBJ` macros
  (`object.h`) are the entry points. `CREATE_NEW_OBJ` tries
  `gc_pool_malloc` (reuse a `FREE_OBJ` slot) first, falls back to
  `GC_MALLOC` (raw allocation) if the pool has nothing free.
- Every allocation attempt checks `gc_alloc_budget_exceeded()` in addition
  to `object_list_node_available() == 0` (both trigger a `GC()` retry loop)
   this is what makes GC proactive instead of purely reactive (9 below).
- `CREATE_RET_OBJ()` (`vm.h`) is a **different thing**  a stack-resident,
  not-pool-tracked `Object` used as a scratch "return value container"
  that primitives fill and the caller copies out of. Anything built
  directly into one of these whose `.value` needs its own heap cleanup has
  **no natural pool-based lifecycle at all** (this is exactly the
  `mut_string` problem, 6).
- **`CREATE_NEW_OBJ`-style constructors allocate raw memory and initialize
  nothing.** `animula_new_pair`, `animula_new_vector`, `animula_new_list`,
  `animula_new_bytevector`, `animula_new_mut_bytevector` all follow this
  pattern. `animula_new_list` needed a bespoke, non-macro constructor
  (like `make_closure` already has) because `List` has an invariant
  (`slh_first == NULL` for "empty") the generic macro can't express  see
  the postmortem in 9. **Worth auditing** whether `pair`/`vector`/
  `bytevector`/`mut_bytevector` have any similar hidden "must start as X,
  not garbage" invariants  none are currently known, but none have been
  specifically checked for either.

---

## 5. GC backends and active-root construction

Two backends, switched via `USE_TINY_GC` / `USE_OBG_GC`:

- **tiny gc**: wraps a Boehm-style (BDW) conservative mark-and-sweep
  collector. Simple, **believed correct**, kept out of scope for changes.
  `GC_MALLOC` under this backend **zeroes the memory it returns** (a
  well-known BDW property)  this matters, see 9.
- **obg gc** ("object-based generational"): the custom backend aimed at
  eventual real-time behavior. Objects live in fixed-size pools per type
  (`pair_free_pool`, `list_free_pool`, `vector_free_pool`,
  `closure_free_pool`, `bytevector_free_pool`, `mut_bytevector_free_pool`,
  `obj_free_pool`). Liveness is decided by walking the VM's own call-frame
  chain and globals, not a full heap scan. No reference counting anywhere
  (rejected early in the project's history  per-copy runtime overhead on
  an embedded target, plus it doesn't handle cycles).

**Everything obg-specific is compiled out entirely under `USE_TINY_GC`** 
`gc.c` is wrapped in `#ifdef USE_OBG_GC`, and every call site in
`object.h`/`vm.c` goes through `gc.h`'s abstraction layer, which supplies
no-ops / constant-folded `false`s for tiny gc. **Hard requirement**: tiny gc
must see zero behavioral or binary-size difference from any obg-related
work.

### `build_active_root` (the most bug-prone part of the whole backend)

Current, fixed-up state:
1. Walk the call-frame chain via `fp`/`NEXT_FP()`, scanning each frame's
   locals, **and** separately any currently-executing closure's *live
   invocation* locals via `closure->local`.
2. Also scan `[0, sp)`  the top-level region  after the frame-walk loop
   exits. `fp == 0` is the base case ("no enclosing call"), not "nothing to
   scan." Top-level `define`s live directly on the stack with no call
   prelude; the old code skipped this unconditionally. **Fixed.**
3. Also scan `vm->globals` (size from `GLOBAL_REF(VM_GLOBALSEG_SIZE)`, set
   by `vm_load_lef`/`vm_init_globals`). Before this fix, anything reachable
   *only* through a global binding was invisible to a real `gc()` cycle.
   **Fixed.**
4. `active_root_inner_insert`'s `closure_on_heap`/`closure_on_stack` case
   now walks the closure's own `env[]` (permanent captured-variable
   storage) regardless of how the closure was reached (global, frame, or
   nested in a pair/list)  it used to wrongly assume closure frames are
   only walked when mid-call. **Fixed.**
5. Marking dispatch unified: `active_root_insert` (outer `object_t`) now
   delegates to `active_root_inner_insert` (inner value + `otype_t`)
   instead of two independently hand-maintained per-type switches.

**Still open**: `vector` has real struct support in mark/free paths (7
below), but nobody has re-audited every corner of the active-root walk
against it end to end beyond what that pass specifically covered.

### Generations and forced teardown

- `FREE_OBJ` (0) / `GEN_1_OBJ` (1) / `GEN_2_OBJ` (2) / `PERMANENT_OBJ` (3).
- Aging is mostly about **eviction priority** for `hurt` (out-of-memory)
  collections, not scan-work reduction  every `gc()` still scans every
  pool every time. GEN_2 is protected except during a `hurt` collect.
- `hurt` used to be compile-time-fixed (`ANIMULA_GC_HURT`), defeating the
  whole point. **Fixed**: `ODB_GC_MALLOC`'s retry loop now escalates  the
  first retry after a failed `os_malloc` is a normal collect; a *second*
  failure triggers a real hurt collect.
- `free_object`/`free_inner_object` each used to have their **own**
  `if (PERMANENT_OBJ == gc) return;` guard, deaf to any caller's force
  intent  a forced sweep could free the outer struct without tearing down
  what it owned internally (orphaning e.g. a `list_t`'s `ListNode` chain).
  **Fixed** with a single `static bool g_gc_force_teardown` flag that both
  guards now respect.

### Three "clean everything" entry points  do not confuse them

| Function | When it runs | Respects reachability? | Overrides `PERMANENT_OBJ`? |
|---|---|---|---|
| `gc()` (normal cycle) | reactively (alloc failure / OLN exhaustion) or proactively (`gc_alloc_budget_exceeded`) | yes  real `build_active_root` | no |
| `gc_try_to_recycle()` | every time `vm->sp == 0` in `vm_run`'s loop (rarely actually reached for short scripts  confirmed by instrumentation) | **no**  blind sweep, no active-root check | no (but still skips `PERMANENT_OBJ`) |
| `gc_clean_cache()` | on `HALT`, when not `VM_INIT_GLOBALS`  the one that fires for most short test scripts | no (same blind approach) | **yes**, after fix  sets `g_gc_force_teardown` |
| `gc_teardown()` | called exactly once, right before process exit (`animula_clean`, before `vm_clean`) | no  `clean_active_root()` first | yes  sets `g_gc_force_teardown` |

**Known design tension, explicitly flagged, not a bug**: `gc_clean_cache`
overriding `PERMANENT_OBJ` means every `HALT` treats the ending
script/session as "nothing needs to survive." Correct for the current
one-shot-script model (`animula_start` loads one LEF, runs it, exits) and
for `shell.c`'s `sload run`/`run_prog` (which reset the VM or reload
globals anyway). **Would be wrong** if `run_shell`'s interactive mode ever
needs "permanent" bindings to survive across multiple `HALT`s in one live
process  that scenario doesn't exist today, but watch for it.

**Also open**: `gc_try_to_recycle`'s total lack of reachability checking is
a real correctness gap by itself, mostly moot today because the function
rarely fires  but if `vm->sp == 0` starts happening more often mid-script,
this needs the same real fix `gc_teardown`/`gc_clean_cache` got.

---

## 6. `mut_string`  the one open, accepted-as-a-leak gap

`.value` is a bare `char*`, never wrapped in a struct with its own `.attr`,
and **never registered in any pool**. `free_object`'s `mut_string` case
correctly frees it  the problem is whether anything ever calls
`free_object` on the Object holding it.

- `recycle_object` and `active_root_inner_insert` were missing a
  `mut_string` case entirely (would `PANIC` if reached)  **fixed**, closes
  a real crash risk for `mut_string` locals going out of scope via
  `RESTORE()` or scanned during a real `gc()`.
- **Still an accepted, un-fixed leak**: a `mut_string` built as a
  throwaway argument directly consumed by a primitive (e.g.
  `(display (list->string ...))`)  `display`'s implementation pops the
  argument by value and never registers it anywhere pool-based.
- **Deliberately reverted attempt**: freeing it right at the `display`
  call site. Reasoning: `mut_string` is heap-allocated and passed **by
  reference**  copying the `Object` struct only copies the pointer, so if
  the same string were bound to a variable and displayed twice, freeing on
  the first `display` would use-after-free the second. Only sound with
  real compiler-side proof the freed reference is the *last* one, which
  doesn't exist yet. **A leaked buffer is an acceptable interim cost; a
  correctness regression is not.** Do not re-attempt this fix without that
  proof mechanism in place.
- **Real fix, not started**: either (a) give `mut_string` the same
  two-layer treatment `bytevector` has (wrap in a struct with its own
  `.attr`, register in a proper pool  needs auditing every place in
  `str.c`/`print.c` that assumes `.value` is directly a `char*`), or (b)
  wait for compiler-side liveness/escape analysis (see 8) to make freeing
  at a known-last-use point safe.

---

## 7. `vector`  completed

`Vector` is `{ oattr attr; u16_t size; object_t *vec; }`. All five places
needing vector handling now mirror `pair` (recurse into `object_t`
elements) + `bytevector` (owns a separately-allocated array needing its own
`os_free`):
- `free_object`: recurse over elements, mark dead.
- `free_inner_object`: `os_free(v->vec)`, mark dead.
- `recycle_object`: recurse, mark via `free_object_from_pool`.
- `active_root_inner_insert`: recurse `active_root_insert` over elements.
- `gc_recycle_current_frame`: already delegated to `recycle_object`, no
  change needed.
- `print.c`'s `vector_printer` was fixed (used to infinitely recurse,
  printing the whole vector instead of element `i`).

---

## 8. Long-term direction (discussed, not implemented)

Compiler-side liveness/escape analysis doing something RAII-like: the
compiler proves a value's last use and emits a hint the runtime can act on
without runtime tracing. Raised specifically re: `mut_string` (6) but is
the general direction for anything currently relying on GC tracing. Not
started as of the last session.

---

## 9. Postmortem: `lambda-lifting-2` looked like a GC bug and wasn't

Keeping this in full because the failure mode looked exactly like a GC
problem for a long time and wasted real effort before the actual cause
surfaced. **Read this before assuming any obg-only failure is a GC bug.**

**Symptom**: `(display (map (lambda (x) (* x 2)) '(1 2 3)))` segfaulted
only under obg, never tiny gc, with an ASan `SEGV on unknown address
0xbebebec2` inside `list_printer`'s `SLIST_NEXT` walk, reading a garbage
`node` pointer. `lambda-lifting.scm` (which lifts the argument lambda to a
top-level `procedure`, routing the call through `map`'s `apply_proc` path)
was involved only incidentally  a total red herring.

**Wrong turn #1  blamed `apply_proc`'s calling convention.** Theory:
`apply_proc` never sets up `vm->local`/`vm->closure`/`vm->attr.shadow`
before jumping into the lifted procedure's `tail-call`-mode entry. Several
variations tried; each left the crash unchanged or broke `raise-cont`.
**Fully reverted**  `apply_proc` needed zero changes for this bug.

**Wrong turn #2  blamed GC-rooting in `map`.** Theory: `map` builds its
input/result lists in pure C locals, invisible to `build_active_root`
(which only walks VM stack frames, top-level region, and globals  never C
locals), so a mid-loop `gc()` could sweep them; `0xbebebe...` assumed to be
obg's pool-poison-on-free value. Fix tried: park both lists as real Objects
on the VM stack. **Zero effect  byte-identical crash address, same frame,
before and after.** That non-result was itself the important clue: a real
GC-timing bug would not reproduce at the *exact* same address run after
run with no randomization involved. **Fully reverted.**

**Actual root cause**: `object.c`'s `animula_new_list()` is a bare
`CREATE_NEW_OBJ`-style allocation  it never zeroes or sets any field, so
`List.list.slh_first` (the `SLIST_HEAD`'s head pointer) starts as whatever
garbage was already there, not `NULL`. `map`'s construction loop
(`SLIST_INSERT_HEAD`/`SLIST_INSERT_AFTER`, standard BSD queue-macro
semantics) makes each newly-inserted node inherit whatever "next" value was
already at the insertion point and never overwrites it  so the
uninitialized garbage is carried forward node-by-node until it ends up in
the actual final tail node's "next" field. The list looks fully linked
right up until the last node, whose end-of-list marker is silent garbage
instead of `NULL`. Iterating/printing past that dereferences it as a live
`ListNode*`.

**Why obg-only**: tiny gc's `GC_MALLOC` is Boehm's `GC_malloc()`, which
**zeroes memory it returns**  made `slh_first` come out `NULL` by
accident every time under tiny gc, so the chain always terminated
correctly. obg's `GC_MALLOC` fallback is a raw, non-zeroing `os_malloc()`;
under ASan, that fresh-but-never-written memory reads back as ASan's
`malloc_fill_byte` default (`0xbe`)  exactly the observed crash pattern.
**Not a free-then-read / pool-poisoning pattern  an
allocate-then-read-before-write pattern.**

**Fix**: `animula_new_list()` now explicitly sets
`o->list.slh_first = NULL;` and `o->non_shared = 0;` right after
allocation. Entire fix contained in `object.c`; `vm.c`/`vm.h` needed no
changes and were reverted to baseline.

**Lessons, stated as rules for this codebase:**
- An "only obg fails" symptom is not by itself evidence of a GC bug  it's
  evidence that *something* differs between the allocators, and "zeroes
  memory vs. doesn't" is just as plausible as anything in
  collection/rooting logic. **Check the allocator's raw-memory contract
  (zeroing, alignment, poisoning) before assuming the bug is in
  collection/rooting.**
- An **identical crash address across independent runs**, with no
  intervening randomization, argues *against* a GC-timing/reachability
  theory (which would predict some variation) and *for* a deterministic
  logic bug.
- Never attribute a poison-byte pattern like `0xbebebe...` to "the GC wrote
  this on free" without actually grepping `gc.c` for that byte pattern 
  here it was ASan's own `malloc_fill_byte`, not anything Animula wrote.

---

## 10. Other structural notes

- **Bytecode ISA** (`bytecode.h`): encodings from 8 to 32 bits
  (`SINGLE`/`DOUBLE`/`TRIPLE`/`QUADRUPLE`/`SPECIAL`, matching `encode_type`
  in `types.h`). `CLOSURE_ON_HEAP`/`CLOSURE_ON_STACK` exist as encodings,
  but **only the heap variant is implemented at runtime** 
  `call_closure_on_stack` is a `PANIC` stub, confirmed not needed since
  Animula only uses heap closures; any stack-closure IR gets lowered to a
  heap closure somewhere in the compiler before reaching the runtime.
- **Primitive dispatch gotcha**: some primitives are registered in
  `primitives.c` with a `NULL` function pointer
  (`with-exception-handler`, `raise-continuable`, and presumably
  `raise`/`return`/`restore`)  their real implementation is a
  special-cased `switch` arm directly inside `call_prim` in `vm.c`, not the
  generic function-pointer dispatch. **`primitives.c` alone is not a
  complete behavior list  always check `vm.c`'s `call_prim` before
  assuming a primitive number is unimplemented.** Primitive numbers of
  note: `0` = `return`, `14` = `restore` (triggers real `RESTORE()`),
  `19` = `map`, `45` = `with-exception-handler`, `47` =
  `raise-continuable`.
- **`vm_t` singleton binding**: `obg_gc.h`'s macros need to reach the
  current VM's `fp`/`sp`/`stack` from call sites with no `vm_t vm`
  parameter of their own, but the codebase otherwise threads `vm_t`
  explicitly with no global singleton. Solution: `g_current_vm` +
  `gc_bind_vm(vm)`, owned entirely by `gc.c`/`obg_gc.h`, **deliberately not
  wired into `vm.c`** (reaching a global VM pointer into a
  backend-agnostic file to serve one GC backend was explicitly rejected).
  Called once, right after `vm_init(vm)`, currently wired into
  `animula.c`'s `animula_init`. No-op under tiny gc.
- **RB-tree active root** (performance, not correctness): the active-root
  membership check used to be a hand-rolled O(n) linear walk despite
  `RB_GENERATE_STATIC` already being called  the tree was never usable
  because `struct ActiveRoot` was only forward-declared. Fixed by adding
  `RB_HEAD (ActiveRoot, ActiveRootNode);` in `obg_gc.h` and switching to
  real `RB_INSERT`/`RB_FIND`/`RB_INIT`. `exist()` is now O(log n).

---

## 11. Non-goals / explicitly out of scope (don't "helpfully" start these)

- No change to the reactive-then-proactive GC trigger model into anything
  more sophisticated (e.g. byte-count-based instead of allocation-count).
- No incremental/interruptible collection  every `gc()` is still a single
  uninterruptible pass. Real-time GC is a long-term goal, not started.
- No dedicated pool for `mut_string` (6)  deliberately deferred pending a
  struct-layout audit or compiler-side liveness analysis.
- `closure_on_stack` is not implemented at the VM level  confirmed not
  needed (10).

---

## 12. On the horizon (planned, not yet done)

- **Record type support**: new `otype_t` tag, inner struct mirroring
  `Vector`'s layout (`object_t *fields` + `u16_t size`), new
  `record-ref`/`record-set!` primitives, GC trace via additional
  `case record:` entries in the relevant switches. No runtime bounds
  checking in release builds. Must stay consistent with laco's
  compile-time-only record design (see laco's CLAUDE.md 3.5)  the VM
  side only needs indexed slot storage, no type metadata.
- **Bignum/arbitrary-precision integers**: deferred. GMP was evaluated and
  rejected for this target  conflicts with the pool-based GC model, no
  Cortex-M assembly kernels. A pluggable architecture is recommended
  whenever this is picked up, rather than hardcoding one bignum backend.
- **OAL implementations per RTOS** (FreeRTOS and others)  the
  commercial/consulting deliverable; VOS contract stability is the margin
  driver here, so changes to the VOS contract should be made conservatively
  and grown from real delivery history rather than speculative design.

---

## 13. Working conventions for an AI coding agent in this repo

1. **`object.h` is the single source of truth for object encoding.** If
   code disagrees with it, the code is wrong.
2. Before touching anything allocation- or GC-related, know which layer
   you're in: outer `Object` vs. inner heap value (2), and which of the
   `CREATE_NEW_OBJ`-style constructors zero nothing by default (4).
3. An "only fails under obg, not tiny gc" symptom is a *clue about an
   allocator difference*, not proof of a GC-collection bug. Check the raw
   allocation contract first (9).
4. Identical crash addresses across independent, non-randomized runs point
   at deterministic logic bugs, not GC-timing/reachability issues.
5. Don't conflate `TAIL_CALL` and `TAIL_REC`  they are different
   mechanisms with different register side effects (3).
6. Any change to `gc()`/`gc_try_to_recycle()`/`gc_clean_cache()`/
   `gc_teardown()` must state explicitly which of the three properties in
   the table in 5 it affects (reachability-respecting? overrides
   `PERMANENT_OBJ`?)  don't blur these together.
7. All obg-specific code must compile out to true no-ops under
   `USE_TINY_GC`  verify this, don't assume the `#ifdef` boundaries are
   already correct after your change.
8. Prefer reading actual source files (`object.h`, `types.h`, `vm.c`,
   `gc.c`) over inferring struct layouts or calling conventions from
   memory/documentation  this file summarizes past findings but is not a
   substitute for checking current source.
9. Ground diagnosis in actual uploaded/read source and, where possible,
   actual instrumented runs (ASan output, debug probes) rather than
   speculating from symptoms alone  this is how the `lambda-lifting-2` bug
   (9) was eventually solved after two wrong turns.

---

*This file is derived from accumulated project memory across sessions. If
it conflicts with actual source code or current test results, trust the
code and tests, and update this file.*
