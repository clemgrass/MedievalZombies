[Español](README.md)

# MedievalZombies

A third-person action game about a medieval zombie apocalypse, on a custom engine. In
full plate armor and with a greatsword, you hack your way through a castle overrun by
the dead. **You aim and charge every swing yourself, and it really cuts the zombie's
mesh, exactly where you aimed.**

> **Early development:** there is no playable version yet.

## The game

On a day of celebration, almost everyone in the castle turned into zombies, and nobody
knows why. The princess is trapped in the highest tower, and the way to her runs through
the castle and its grounds.

### The swing

Most zombie games are about shooting, or hitting with fixed animations. Here **the
player decides the angle, the strength and the point of the cut:**

- **Aim.** The mouse or the stick guides the sword, and a reticle marks where it will
  land: the neck, the waist or a joint in the armor.
- **Charge.** Holding the attack charges its strength, and while it charges you rotate
  the line of the cut. Releasing it swings the sword. Precision decides *where* it cuts,
  and strength decides *how far* it goes: an arm, a whole zombie or several in a row.
- **Everything has weight.** Charging, correcting the angle, jogging and breaking free
  from a grab all spend the same energy. There is no health bar: zombies grab you, and
  if you run out of energy before breaking free, you die.

### The enemies

Slow peasants, soldiers in chainmail and soldiers in full plate, each with its own
resistance to cuts. Against armor, hitting hard is not enough: you have to find the
joints and the angle. At the end waits a zombie unlike the others, which calls the
horde and hides behind an armored bodyguard.

A zombie cut in two does not always die. A cut to the head kills it, but as long as it
keeps its head and something else, it keeps going: cut it at the waist and the upper
half crawls toward you.

### The look

Dark PSX style: low-poly geometry, unfiltered low-resolution textures, fog and a low
internal resolution. The palette is muted (stone, ochre, greenish gray), and **red is
the only saturated color**: in the middle of the horde, you can always see where you
cut.

## The engine

The engine is custom and written in **C++20**. It uses **Vulkan 1.3** as the graphics
API and **SDL3** for the window, input, timing and audio. Everything that defines the
game is custom:

| Part | What it does |
|---|---|
| Rendering | Draws with Vulkan at 640×360 and scales the image to the screen |
| Mesh slicing | Splits the zombie's animated mesh along the plane of the swing, on the CPU |
| Combat | Decides how far the blade goes from the strength, the material and the angle |
| Animation | Blends clips in layers and animates the swing procedurally with IK |
| Physics | Rigid bodies, ragdolls and characters, with a sub-stepped impulse solver |
| Architecture | A custom ECS with sparse sets and a fixed 60 Hz simulation step |

The target is **a stable 60 fps on an integrated GPU** (Intel UHD 620) with 50 zombies
on screen and about 100 pieces on the ground. The low-resolution, low-poly look is not
only aesthetic: it makes the GPU's work cheaper and leaves CPU time for slicing.

## How slicing works

The swing really cuts the geometry: it is not a shader effect or a cut at fixed points.
When the blade goes through a zombie, this happens inside the simulation step:

1. **The plane.** The plane of the cut passes through the character's chest and the
   aimed point, along the line the player drew.
2. **The pose.** The zombie is moving, so the CPU deforms its mesh into the exact pose
   at the moment of the hit, with the same math the GPU uses to draw it.
3. **Classify.** Each vertex ends up on one side of the plane according to its signed
   distance. Vertices the model duplicates along its seams are classified only once,
   and a fixed tie-break (a distance of exactly 0 counts as positive) keeps rounding
   from leaving holes.
4. **Split.** Triangles that cross the plane are split. Each cut point is computed only
   once per edge, so both sides stay exactly stitched together.
5. **The cap.** The edges of the cut form closed contours, which are triangulated by ear
   clipping to close each piece. The new face shows the inside: flesh and bone.
6. **Keep animating.** The new vertices are taken back to the bind pose (un-skinned),
   with the weights of their bones. That way the half that is still alive keeps
   animating with the same skeleton, and the wound does not come apart as it moves.
7. **The pieces.** A real model is several overlapping shells (torso, arms, legs), so
   the fragments are grouped by the bone that moves them most. One cut can give more
   than two pieces: cutting a crouching zombie at the waist drops the legs and also the
   hands.
8. **The stuck blade.** If the strength is not enough to go through, the blade stops
   inside the body. Whatever it fully crossed comes off (a forearm, for example), and
   where it stopped there is a real gash, opened as a wedge: wider at the mouth of the
   wound and closed at the bottom.

The pieces share the original model's vertex buffer and only upload their indices and
their new vertices to the GPU. Pieces can be cut again, even on the ground. In a
prototype, cutting an 8,500-vertex mesh, skinning included, took 0.5 to 0.8 ms without
optimization, on the same laptop as the 60 fps target (the best of 300 runs, measured
outside the protocol). A zombie in the game has a third of those vertices: that would be
about 0.2 to 0.3 ms.

## How combat works

- **Strength is spent crossing the body.** The more area the blade goes through, the
  more strength it uses, and whatever is left carries on to the next zombie.
- **Flesh does not stop the blade from entering; metal does.** Each piece of armor has
  an entry threshold. Below it, the swing bounces off and the character is stunned for
  a moment.
- **Angle and joints matter.** A swing that crosses the bone sideways gets in more
  easily than one that runs along it, and the threshold drops near a joint.
- **The blade can get stuck.** If the strength is enough to get in but not to go
  through, a quick-time sequence starts: the zombie keeps coming, skewered on the sword,
  the rest of the horde almost stops, and the player has to finish the cut before being
  grabbed or running out of energy.
- **The impact.** On every swing, the character and the zombie that was hit freeze for
  50 to 80 ms while everything else keeps moving (*hit-stop*). On special cuts, a slow
  motion camera follows the blade through the body.

## How the swing works

- **There are no recorded attacks.** The tip of the sword follows an arc in the plane of
  the cut, both arms reach the grip with analytic two-bone IK, the torso turns to follow
  the tip, and a clip adds the body's weight: the step forward and the charge.
- **Hits are detected in the plane.** Each zombie's body is a set of capsules, one per
  bone. Between two simulation steps the blade sweeps an annular sector within the
  plane, and every capsule it touches is a hit, in order.
- **The pieces have physics.** When cutting, the capsules are clipped toward the side
  of each piece. Small pieces become rigid bodies, large ones turn into ragdolls, and
  characters push them aside as they walk without getting stuck.

## Building

On Linux, with CMake 3.29 or newer, Ninja, Clang and `lld`:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/game/game
```

There are five presets: `debug` (with AddressSanitizer and UndefinedBehaviorSanitizer),
`debug-gcc` (the same with GCC, the secondary compiler), `relwithdebinfo` (optimized,
with symbols and frame pointers, for profiling), `release` and `coverage`. Each one
builds into `build/<preset>`. To configure, build and test in a single command:

```bash
cmake --workflow --preset debug      # or debug-gcc; relwithdebinfo and release don't run tests
```

The tests use GoogleTest and CTest, with labels per module (`core`…) and per kind
(`unit`, `property`, `regression`, `sanitizer`):

```bash
ctest --preset debug -L core         # only the engine_core tests
ctest --preset debug -LE sanitizer   # without the ones that depend on a sanitizer
```

A failing property test prints its seed and the command that reproduces it
(`--seed=N`). Line and branch coverage, with `llvm-cov`:

```bash
cmake --workflow --preset coverage
# build/coverage/coverage/html/index.html and summary.txt (summary per module)
```

The folders listed in `tests/CMakeLists.txt` have a 100% line and branch coverage
target: if anything is left uncovered, the command fails and `summary.txt` lists every
missing line and branch.

Warnings in the project's own code are errors. To experiment without them stopping the
build: `cmake --preset debug --compile-no-warning-as-error`.

## License

- **Code:** [MIT](LICENSES/MIT.txt).
- **Documentation and original assets:** [CC BY 4.0](LICENSES/CC-BY-4.0.txt).
- **Third-party dependencies and assets:** each with its own license and author, in
  [`CREDITS.md`](CREDITS.md).

More technical detail in [`docs/`](docs/README.md) (in Spanish).

itch.io page: https://cgrassdev.itch.io/medieval-zombies
